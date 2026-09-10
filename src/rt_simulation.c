#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "rt_simulation.h"
#include "release_schedule.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static double TimespecToSeconds(const struct timespec *ts) {
    return (double)ts->tv_sec + (double)ts->tv_nsec / 1e9;
}

static double ElapsedSeconds(const struct timespec *start, const struct timespec *end) {
    return TimespecToSeconds(end) - TimespecToSeconds(start);
}

static uint64_t ElapsedNanoseconds(const struct timespec *start, const struct timespec *end) {
    time_t seconds = end->tv_sec - start->tv_sec;
    long nanoseconds = end->tv_nsec - start->tv_nsec;
    if (nanoseconds < 0L) {
        --seconds;
        nanoseconds += 1000000000L;
    }
    if (seconds < 0) return 0U;
    return (uint64_t)seconds * UINT64_C(1000000000) + (uint64_t)nanoseconds;
}

static struct timespec AddNanoseconds(const struct timespec *origin, uint64_t nanoseconds) {
    struct timespec result = *origin;
    result.tv_sec += (time_t)(nanoseconds / UINT64_C(1000000000));
    result.tv_nsec += (long)(nanoseconds % UINT64_C(1000000000));
    if (result.tv_nsec >= 1000000000L) {
        ++result.tv_sec;
        result.tv_nsec -= 1000000000L;
    }
    return result;
}

static int ConfigureRealtimeThread(RtSimulationContext *context) {
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

static void SetInputProtectionFailure(RtSimulationContext *context, const input_step_t *step) {
    const size_t channel_index = step->invalid_limit_channels[0];
    const input_channel_state_t *channel = &context->input_state.channels[channel_index];
    context->run_result.state = SIMULATION_RUN_ERROR;
    context->run_result.code = -1;
    snprintf(context->run_result.stage, sizeof(context->run_result.stage), "%s", "input-protection");
    snprintf(context->run_result.message, sizeof(context->run_result.message), "invalid input limit: %.70s", channel->descriptor.input_name);
}

static void ReleasePreparedSimulation(RtSimulationContext *context) {
    if (context->input_state_ready) {
        InputStateDestroy(&context->input_state);
        context->input_state_ready = false;
    }
    if (context->prepared) {
        fmu_model_terminate(context->model);
        context->prepared = false;
    }
}

static void *SimulationThread(void *arg) {
    RtSimulationContext *context = arg;
    memset(&context->stats, 0, sizeof(context->stats));
    context->result = -1;
    context->run_result = (simulation_run_result_t){.state = SIMULATION_RUN_ERROR, .code = -1};

    if (ConfigureRealtimeThread(context) != 0) {
        SetRunFailure(context, "realtime", "could not configure the simulation thread");
        goto finish;
    }

    struct timespec wall_start;
    clock_gettime(CLOCK_MONOTONIC, &wall_start);
    double sim_time = 0.0;
    uint64_t sequence = 0;
    release_schedule_t release_schedule;
    if (!ReleaseScheduleInit(&release_schedule, context->config->step_size_s)) {
        SetRunFailure(context, "schedule", "could not initialize the fixed release schedule");
        goto finish;
    }
    bool first_step = true;

    while (sim_time + 1e-12 < context->config->stop_time_s &&
           !atomic_load_explicit(context->stop_requested, memory_order_relaxed)) {
        if (!first_step) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (!ReleaseScheduleAdvanceToEarliestRelease(&release_schedule, ElapsedNanoseconds(&wall_start, &now))) {
                SetRunFailure(context, "schedule", "could not advance the fixed release schedule");
                goto finish;
            }
            const struct timespec release_time = AddNanoseconds(&wall_start, ReleaseScheduleReleaseNs(&release_schedule));
            int sleep_rc;
            do {
                sleep_rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &release_time, NULL);
            } while (sleep_rc == EINTR && !atomic_load_explicit(context->stop_requested, memory_order_relaxed));
            if (sleep_rc != 0 && sleep_rc != EINTR) {
                SetRunFailure(context, "schedule", "could not wait for the fixed release schedule");
                goto finish;
            }
            if (atomic_load_explicit(context->stop_requested, memory_order_relaxed)) break;
        }
        first_step = false;
        double h = context->config->step_size_s;
        if (sim_time + h > context->config->stop_time_s) h = context->config->stop_time_s - sim_time;

        struct timespec compute_start, compute_end;
        clock_gettime(CLOCK_MONOTONIC, &compute_start);

        input_step_t input_step;
        const input_state_status_t input_status = InputStatePrepareStep(&context->input_state, &input_step);
        if (input_status == INPUT_STATE_STATUS_PROTECTION_TRIPPED) {
            SetInputProtectionFailure(context, &input_step);
            goto finish;
        }
        if (input_status != INPUT_STATE_STATUS_OK ||
            fmu_model_set_inputs(context->model, context->input_descriptors, input_step.values, input_step.value_count) != 0) {
            SetRunFailure(context, "inputs", "could not apply FMU inputs");
            goto finish;
        }

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
        double computation_s = ElapsedSeconds(&compute_start, &compute_end);
        if (computation_s > context->stats.max_computation_s) context->stats.max_computation_s = computation_s;

        if (!ReleaseScheduleCompleteStep(&release_schedule, ElapsedNanoseconds(&wall_start, &compute_end))) {
            SetRunFailure(context, "schedule", "could not complete the fixed release schedule");
            goto finish;
        }
        context->stats.deadline_misses = release_schedule.late_steps;
        context->stats.unused_releases = release_schedule.unused_releases;
        context->stats.max_lateness_s = (double)release_schedule.max_completion_lateness_ns / 1e9;

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        plot_sample.wall_time_s = ElapsedSeconds(&wall_start, &now);

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
    ReleasePreparedSimulation(context);
    context->run_result.stats = context->stats;
    if (context->logging) RunLoggingProducerDone(context->logging);
    if (context->plot_producer_done) atomic_store_explicit(context->plot_producer_done, true, memory_order_release);
    return NULL;
}

int RtSimulationPrepare(RtSimulationContext *context, FmuModel *model, const AppConfig *config) {
    if (!context || !model || !config || !model->fmu || config->input_count > INPUT_STATE_MAX_CHANNELS) return -1;
    memset(context, 0, sizeof(*context));
    context->model = model;
    context->config = config;
    context->result = -1;
    memcpy(context->input_descriptors, config->inputs, config->input_count * sizeof(context->input_descriptors[0]));
    if (fmu_model_initialize_cosimulation(model, 0.0, config->stop_time_s) != 0 ||
        fmu_model_resolve_input_initial_values(model, context->input_descriptors, config->input_count) != 0 ||
        InputStateInit(&context->input_state, context->input_descriptors, config->input_count, config->stop_on_invalid_input_limit) != INPUT_STATE_STATUS_OK) {
        fmu_model_terminate(model);
        return -1;
    }
    context->input_state_ready = true;
    context->prepared = true;
    return 0;
}

void RtSimulationAbort(RtSimulationContext *context) {
    if (!context || context->thread_started) return;
    ReleasePreparedSimulation(context);
}

int RtSimulationStart(RtSimulationContext *context, run_logging_t *logging, SampleQueue *plot_queue,
                      _Atomic bool *stop_requested, _Atomic bool *plot_producer_done) {
    if (!context || !context->prepared || context->thread_started || !stop_requested) return -1;
    context->logging = logging;
    context->plot_queue = plot_queue;
    context->stop_requested = stop_requested;
    context->plot_producer_done = plot_producer_done;
    context->result = -1;
    if (pthread_create(&context->thread, NULL, SimulationThread, context) != 0) return -1;
    context->thread_started = true;
    return 0;
}

int RtSimulationJoin(RtSimulationContext *context) {
    if (!context || !context->thread_started) return -1;
    if (pthread_join(context->thread, NULL) != 0) return -1;
    context->thread_started = false;
    return context->result;
}
