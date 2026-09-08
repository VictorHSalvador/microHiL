#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "rt_simulation.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static double timespec_to_seconds(const struct timespec *ts) {
    return (double)ts->tv_sec + (double)ts->tv_nsec / 1e9;
}

static struct timespec seconds_to_timespec(double seconds) {
    struct timespec ts;
    ts.tv_sec = (time_t)floor(seconds);
    ts.tv_nsec = (long)((seconds - (double)ts.tv_sec) * 1e9);
    if (ts.tv_nsec >= 1000000000L) {
        ++ts.tv_sec;
        ts.tv_nsec -= 1000000000L;
    }
    return ts;
}

static double elapsed_seconds(const struct timespec *start, const struct timespec *end) {
    return timespec_to_seconds(end) - timespec_to_seconds(start);
}

static int configure_realtime_thread(RtSimulationContext *context) {
    if (context->config->cpu_core >= 0) {
        if (context->config->cpu_core >= CPU_SETSIZE) {
            fprintf(stderr, "Warning: CPU core %d is outside CPU_SETSIZE=%d; affinity disabled.\n",
                    context->config->cpu_core, CPU_SETSIZE);
        } else {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET((unsigned int)context->config->cpu_core, &cpuset);
            int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
            if (rc != 0) fprintf(stderr, "Warning: CPU affinity could not be set: %s\n", strerror(rc));
        }
    }

    struct sched_param param;
    memset(&param, 0, sizeof(param));
    param.sched_priority = context->config->rt_priority;
    int rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if (rc != 0) {
        fprintf(stderr, "Warning: SCHED_FIFO priority %d could not be enabled: %s\n", context->config->rt_priority, strerror(rc));
        fprintf(stderr, "Grant CAP_SYS_NICE to the executable or configure RLIMIT_RTPRIO.\n");
        context->stats.sched_fifo_active = false;
        return context->config->strict_realtime ? -1 : 0;
    }

    context->stats.sched_fifo_active = true;
    return 0;
}

static void MarkInvalid(log_sample_t *sample, size_t index) {
    sample->quality_bitmap[index / 8U] |= (uint8_t)(UINT32_C(1) << (index % 8U));
}

static double PlotValue(const log_value_t *value, NumericType type) {
    switch (type) {
        case NUMERIC_REAL: return value->real_value;
        case NUMERIC_INTEGER:
        case NUMERIC_ENUMERATION: return (double)value->discrete_value;
        case NUMERIC_BOOLEAN: return value->boolean_value ? 1.0 : 0.0;
        default: return NAN;
    }
}

static void NormalizeLogSample(const AppConfig *config, log_sample_t *sample) {
    for (size_t index = 0U; index < config->output_count; ++index) {
        switch (config->outputs[index].type) {
            case NUMERIC_REAL:
                if (!isfinite(sample->values[index].real_value)) {
                    sample->values[index].real_value = NAN;
                    MarkInvalid(sample, index);
                }
                break;
            case NUMERIC_INTEGER:
            case NUMERIC_ENUMERATION:
                if (sample->values[index].discrete_value < INT32_MIN || sample->values[index].discrete_value > INT32_MAX) {
                    sample->values[index].discrete_value = 0;
                    MarkInvalid(sample, index);
                }
                break;
            case NUMERIC_BOOLEAN:
                if (sample->values[index].boolean_value > 1U) {
                    sample->values[index].boolean_value = 0U;
                    MarkInvalid(sample, index);
                }
                break;
            default:
                MarkInvalid(sample, index);
                break;
        }
    }
}

static void SetRunFailure(RtSimulationContext *context, const char *stage, const char *message) {
    context->run_result.state = SIMULATION_RUN_ERROR;
    context->run_result.code = -1;
    snprintf(context->run_result.stage, sizeof(context->run_result.stage), "%s", stage);
    snprintf(context->run_result.message, sizeof(context->run_result.message), "%s", message);
}

static void *simulation_thread(void *arg) {
    RtSimulationContext *context = arg;
    memset(&context->stats, 0, sizeof(context->stats));
    context->result = -1;
    context->run_result = (simulation_run_result_t){.state = SIMULATION_RUN_ERROR, .code = -1};

    if (configure_realtime_thread(context) != 0) {
        SetRunFailure(context, "realtime", "could not configure the simulation thread");
        goto finish;
    }

    if (fmu_model_initialize_cosimulation(context->model, 0.0, context->config->stop_time_s) != 0) {
        SetRunFailure(context, "initialize", "could not initialize the FMU");
        goto finish;
    }

    struct timespec wall_start;
    clock_gettime(CLOCK_MONOTONIC, &wall_start);
    double wall_start_s = timespec_to_seconds(&wall_start);
    double sim_time = 0.0;
    uint64_t sequence = 0;

    while (sim_time + 1e-12 < context->config->stop_time_s &&
           !atomic_load_explicit(context->stop_requested, memory_order_relaxed)) {
        double h = context->config->step_size_s;
        if (sim_time + h > context->config->stop_time_s) h = context->config->stop_time_s - sim_time;

        struct timespec compute_start, compute_end, release_time;
        clock_gettime(CLOCK_MONOTONIC, &compute_start);

        if (fmu_model_do_step(context->model, sim_time, h) != 0) {
            SetRunFailure(context, "step", "FMU step failed");
            goto finish;
        }

        log_sample_t log_sample = {.sequence = sequence, .sim_time_s = sim_time + h};
        SimulationSample plot_sample = {.sequence = sequence, .sim_time_s = sim_time + h, .value_count = context->config->output_count};
        if (fmu_model_read_outputs(context->model, context->config->outputs, context->config->output_count, log_sample.values) != 0) {
            SetRunFailure(context, "outputs", "could not read final FMU outputs");
            goto finish;
        }
        NormalizeLogSample(context->config, &log_sample);
        for (size_t index = 0U; index < context->config->output_count; ++index) plot_sample.values[index] = PlotValue(&log_sample.values[index], context->config->outputs[index].type);

        clock_gettime(CLOCK_MONOTONIC, &compute_end);
        double computation_s = elapsed_seconds(&compute_start, &compute_end);
        if (computation_s > context->stats.max_computation_s) context->stats.max_computation_s = computation_s;

        double deadline_s = wall_start_s + log_sample.sim_time_s;
        release_time = seconds_to_timespec(deadline_s);

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double now_s = timespec_to_seconds(&now);
        if (now_s < deadline_s) {
            int sleep_rc;
            do {
                sleep_rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &release_time, NULL);
            } while (sleep_rc == EINTR && !atomic_load_explicit(context->stop_requested, memory_order_relaxed));
        } else {
            double lateness = now_s - deadline_s;
            ++context->stats.deadline_misses;
            if (lateness > context->stats.max_lateness_s) context->stats.max_lateness_s = lateness;
        }

        clock_gettime(CLOCK_MONOTONIC, &now);
        plot_sample.wall_time_s = elapsed_seconds(&wall_start, &now);

        if (context->logging) (void)RunLoggingPublish(context->logging, &log_sample);
        if (context->config->plot_enabled && context->plot_queue) (void)sample_queue_push(context->plot_queue, &plot_sample);

        sim_time += h;
        ++sequence;
        ++context->stats.completed_steps;
    }

    context->result = 0;
    const bool stopped = atomic_load_explicit(context->stop_requested, memory_order_relaxed);
    context->run_result.state = stopped ? SIMULATION_RUN_STOPPED : SIMULATION_RUN_FINISHED;
    context->run_result.code = 0;
    snprintf(context->run_result.stage, sizeof(context->run_result.stage), "%s", stopped ? "stop" : "complete");
    snprintf(context->run_result.message, sizeof(context->run_result.message), "%s", stopped ? "simulation stopped by request" : "simulation completed");

finish:
    fmu_model_terminate(context->model);
    context->run_result.stats = context->stats;
    if (context->logging) RunLoggingProducerDone(context->logging);
    if (context->plot_producer_done) atomic_store_explicit(context->plot_producer_done, true, memory_order_release);
    return NULL;
}

int rt_simulation_start(RtSimulationContext *context, FmuModel *model, const AppConfig *config,
                        run_logging_t *logging, SampleQueue *plot_queue,
                        _Atomic bool *stop_requested, _Atomic bool *plot_producer_done) {
    memset(context, 0, sizeof(*context));
    context->model = model;
    context->config = config;
    context->logging = logging;
    context->plot_queue = plot_queue;
    context->stop_requested = stop_requested;
    context->plot_producer_done = plot_producer_done;
    context->result = -1;
    return pthread_create(&context->thread, NULL, simulation_thread, context);
}

int rt_simulation_join(RtSimulationContext *context) {
    if (pthread_join(context->thread, NULL) != 0) return -1;
    return context->result;
}
