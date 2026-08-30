#define _POSIX_C_SOURCE 200809L
#include "plotter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PLOT_HISTORY_MAX 5000

typedef struct {
    SimulationSample samples[PLOT_HISTORY_MAX];
    size_t count;
    size_t start;
} PlotHistory;

static double monotonic_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void sleep_briefly(void) {
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 5000000L};
    nanosleep(&ts, NULL);
}

static void history_push(PlotHistory *history, const SimulationSample *sample, double window_s) {
    if (history->count < PLOT_HISTORY_MAX) {
        size_t index = (history->start + history->count) % PLOT_HISTORY_MAX;
        history->samples[index] = *sample;
        ++history->count;
    } else {
        history->samples[history->start] = *sample;
        history->start = (history->start + 1U) % PLOT_HISTORY_MAX;
    }

    while (history->count > 2) {
        const SimulationSample *oldest = &history->samples[history->start];
        if (sample->sim_time_s - oldest->sim_time_s <= window_s) break;
        history->start = (history->start + 1U) % PLOT_HISTORY_MAX;
        --history->count;
    }
}

static void plot_history(FILE *gp, const AppConfig *config, const PlotHistory *history) {
    if (history->count == 0) return;

    const SimulationSample *latest = &history->samples[(history->start + history->count - 1U) % PLOT_HISTORY_MAX];
    double xmin = latest->sim_time_s - config->plot_window_s;
    if (xmin < 0.0) xmin = 0.0;

    fprintf(gp, "set xrange [%.12g:%.12g]\n", xmin, latest->sim_time_s > 0.0 ? latest->sim_time_s : config->plot_window_s);
    fprintf(gp, "plot ");
    for (size_t v = 0; v < config->output_count; ++v) {
        if (v) fprintf(gp, ", ");
        fprintf(gp, "'-' using 1:2 with lines title '%s'", config->outputs[v].name);
    }
    fputc('\n', gp);

    for (size_t v = 0; v < config->output_count; ++v) {
        for (size_t i = 0; i < history->count; ++i) {
            const SimulationSample *sample = &history->samples[(history->start + i) % PLOT_HISTORY_MAX];
            fprintf(gp, "%.12g %.17g\n", sample->sim_time_s, sample->values[v]);
        }
        fprintf(gp, "e\n");
    }
    fflush(gp);
}

static void *plotter_thread(void *arg) {
    PlotterContext *context = arg;

    if (system("command -v gnuplot >/dev/null 2>&1") != 0) {
        fprintf(stderr, "gnuplot was not found. Real-time plot disabled.\n");
        context->result = -1;
        return NULL;
    }

    FILE *gp = popen("gnuplot -persist", "w");
    if (!gp) {
        perror("popen gnuplot");
        context->result = -1;
        return NULL;
    }

    fprintf(gp, "set title 'FMU 2.0 Co-Simulation - Real-Time Outputs'\n");
    fprintf(gp, "set xlabel 'Simulation time [s]'\n");
    fprintf(gp, "set grid\n");
    fprintf(gp, "set key outside\n");
    fflush(gp);

    PlotHistory history;
    memset(&history, 0, sizeof(history));
    double last_refresh = 0.0;
    SimulationSample sample;

    for (;;) {
        bool consumed = false;
        while (sample_queue_pop(context->queue, &sample)) {
            history_push(&history, &sample, context->config->plot_window_s);
            consumed = true;
        }

        double now = monotonic_seconds();
        if (history.count > 0 && (last_refresh == 0.0 || now - last_refresh >= context->config->plot_refresh_s)) {
            plot_history(gp, context->config, &history);
            last_refresh = now;
        }

        if (atomic_load_explicit(context->producer_done, memory_order_acquire) && !consumed) {
            SimulationSample check;
            if (!sample_queue_pop(context->queue, &check)) break;
            history_push(&history, &check, context->config->plot_window_s);
        }
        sleep_briefly();
    }

    if (history.count > 0) plot_history(gp, context->config, &history);
    pclose(gp);
    context->result = 0;
    return NULL;
}

int plotter_start(PlotterContext *context, SampleQueue *queue, const AppConfig *config, _Atomic bool *producer_done) {
    context->queue = queue;
    context->config = config;
    context->producer_done = producer_done;
    context->result = -1;
    return pthread_create(&context->thread, NULL, plotter_thread, context);
}

int plotter_join(PlotterContext *context) {
    if (pthread_join(context->thread, NULL) != 0) return -1;
    return context->result;
}
