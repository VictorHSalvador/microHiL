#define _POSIX_C_SOURCE 200809L
#include "app_config.h"
#include "csv_logger.h"
#include "fmu_model.h"
#include "plotter.h"
#include "rt_simulation.h"
#include "sample_queue.h"
#include <ctype.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define MAX_CANDIDATE_OUTPUTS 512

static _Atomic bool g_stop_requested = false;

static void signal_handler(int signal_number) {
    (void)signal_number;
    atomic_store_explicit(&g_stop_requested, true, memory_order_relaxed);
}

static void read_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buffer, (int)size, stdin)) {
        buffer[0] = '\0';
        return;
    }
    buffer[strcspn(buffer, "\r\n")] = '\0';
}

static int read_int(const char *prompt, int current) {
    char line[64];
    read_line(prompt, line, sizeof(line));
    if (!line[0]) return current;
    char *end = NULL;
    long value = strtol(line, &end, 10);
    if (!end || *end != '\0') return current;
    return (int)value;
}

static double read_double(const char *prompt, double current) {
    char line[64];
    read_line(prompt, line, sizeof(line));
    if (!line[0]) return current;
    char *end = NULL;
    double value = strtod(line, &end);
    if (!end || *end != '\0') return current;
    return value;
}

static bool read_bool(const char *prompt, bool current) {
    char line[32];
    read_line(prompt, line, sizeof(line));
    if (!line[0]) return current;
    char c = (char)tolower((unsigned char)line[0]);
    if (c == 'y' || c == 's' || c == '1') return true;
    if (c == 'n' || c == '0') return false;
    return current;
}

static const char *numeric_type_name(NumericType type) {
    switch (type) {
        case NUMERIC_REAL: return "Real";
        case NUMERIC_INTEGER: return "Integer";
        case NUMERIC_BOOLEAN: return "Boolean";
        case NUMERIC_ENUMERATION: return "Enumeration";
        default: return "Unknown";
    }
}

static size_t get_candidates(FmuModel *model, OutputVariable *candidates, size_t capacity) {
    size_t total = fmu_model_list_numeric_outputs(model, candidates, capacity);
    if (total > capacity) {
        fprintf(stderr, "FMU has %zu numeric outputs; only the first %zu can be displayed in this menu.\n", total, capacity);
        total = capacity;
    }
    return total;
}

static void list_outputs(FmuModel *model) {
    OutputVariable candidates[MAX_CANDIDATE_OUTPUTS];
    size_t count = get_candidates(model, candidates, MAX_CANDIDATE_OUTPUTS);
    printf("\nNumeric FMU outputs (%zu):\n", count);
    for (size_t i = 0; i < count; ++i) {
        printf("%4zu) %-12s VR=%-8u %s\n", i + 1, numeric_type_name(candidates[i].type), candidates[i].value_reference, candidates[i].name);
    }
    printf("\n");
}

static void select_outputs(FmuModel *model, AppConfig *config) {
    OutputVariable candidates[MAX_CANDIDATE_OUTPUTS];
    size_t count = get_candidates(model, candidates, MAX_CANDIDATE_OUTPUTS);
    if (!count) {
        printf("No numeric output variables found.\n");
        return;
    }

    list_outputs(model);
    char line[2048];
    read_line("Select output numbers separated by commas, or 'all': ", line, sizeof(line));
    config->output_count = 0;

    if (strcasecmp(line, "all") == 0) {
        size_t n = count < MAX_OUTPUTS ? count : MAX_OUTPUTS;
        for (size_t i = 0; i < n; ++i) config->outputs[config->output_count++] = candidates[i];
        if (count > MAX_OUTPUTS) printf("Only the first %d outputs were selected.\n", MAX_OUTPUTS);
        return;
    }

    char *saveptr = NULL;
    for (char *token = strtok_r(line, ",", &saveptr); token && config->output_count < MAX_OUTPUTS; token = strtok_r(NULL, ",", &saveptr)) {
        while (isspace((unsigned char)*token)) ++token;
        char *end = NULL;
        long index = strtol(token, &end, 10);
        while (end && isspace((unsigned char)*end)) ++end;
        if (!end || *end != '\0' || index < 1 || (size_t)index > count) {
            printf("Ignoring invalid selection: '%s'\n", token);
            continue;
        }

        bool duplicate = false;
        for (size_t i = 0; i < config->output_count; ++i) {
            if (config->outputs[i].value_reference == candidates[index - 1].value_reference &&
                config->outputs[i].type == candidates[index - 1].type) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) config->outputs[config->output_count++] = candidates[index - 1];
    }

    printf("Selected %zu output(s).\n", config->output_count);
}

static int load_fmu_menu(FmuModel *model, AppConfig *config) {
    char path[PATH_LEN];
    read_line("FMU file path: ", path, sizeof(path));
    if (!path[0]) return -1;
    if (access(path, R_OK) != 0) {
        perror("FMU path");
        return -1;
    }
    if (fmu_model_load(model, path) != 0) return -1;
    snprintf(config->fmu_path, sizeof(config->fmu_path), "%s", path);
    config->output_count = 0;
    printf("Loaded FMI 2.0 Co-Simulation model: %s\n", fmu_model_name(model));
    list_outputs(model);
    return 0;
}

static void configure_timing(AppConfig *config) {
    config->step_size_s = read_double("Step size in seconds [Enter keeps current]: ", config->step_size_s);
    config->stop_time_s = read_double("Simulation duration in seconds [Enter keeps current]: ", config->stop_time_s);
    if (config->step_size_s <= 0.0) config->step_size_s = 0.02;
    if (config->stop_time_s <= 0.0) config->stop_time_s = 60.0;
}

static void configure_realtime(AppConfig *config) {
    config->rt_priority = read_int("SCHED_FIFO priority (1..99): ", config->rt_priority);
    if (config->rt_priority < 1) config->rt_priority = 1;
    if (config->rt_priority > 99) config->rt_priority = 99;
    config->cpu_core = read_int("CPU core (-1 disables affinity): ", config->cpu_core);
    config->strict_realtime = read_bool("Abort if SCHED_FIFO cannot be enabled? [y/n]: ", config->strict_realtime);
}

static void configure_csv(AppConfig *config) {
    config->csv_enabled = read_bool("Enable CSV logging? [y/n]: ", config->csv_enabled);
    if (config->csv_enabled) {
        char path[PATH_LEN];
        read_line("CSV output path [Enter keeps current]: ", path, sizeof(path));
        if (path[0]) snprintf(config->csv_path, sizeof(config->csv_path), "%s", path);
    }
}

static void configure_plot(AppConfig *config) {
    config->plot_enabled = read_bool("Enable real-time gnuplot? [y/n]: ", config->plot_enabled);
    if (config->plot_enabled) {
        config->plot_window_s = read_double("Visible plot window in seconds: ", config->plot_window_s);
        config->plot_refresh_s = read_double("Plot refresh period in seconds: ", config->plot_refresh_s);
        if (config->plot_window_s <= 0.0) config->plot_window_s = 10.0;
        if (config->plot_refresh_s < 0.02) config->plot_refresh_s = 0.02;
    }
}

static int run_simulation(FmuModel *model, AppConfig *config) {
    if (!model->fmu) {
        printf("Load an FMU first.\n");
        return -1;
    }
    if (config->output_count == 0) {
        printf("Select at least one output first.\n");
        return -1;
    }

    SampleQueue log_queue, plot_queue;
    sample_queue_init(&log_queue);
    sample_queue_init(&plot_queue);

    _Atomic bool producer_done = false;
    atomic_store_explicit(&g_stop_requested, false, memory_order_relaxed);

    CsvLoggerContext csv_context;
    PlotterContext plot_context;
    RtSimulationContext sim_context;
    bool csv_started = false;
    bool plot_started = false;

    if (config->csv_enabled) {
        if (csv_logger_start(&csv_context, &log_queue, config, &producer_done) == 0) csv_started = true;
        else fprintf(stderr, "Could not start CSV logger thread.\n");
    }
    if (config->plot_enabled) {
        if (plotter_start(&plot_context, &plot_queue, config, &producer_done) == 0) plot_started = true;
        else fprintf(stderr, "Could not start plotter thread.\n");
    }

    printf("\nStarting simulation. Press Ctrl+C to request a clean stop.\n");
    int rc = rt_simulation_start(&sim_context, model, config,
                                 csv_started ? &log_queue : NULL,
                                 plot_started ? &plot_queue : NULL,
                                 &g_stop_requested, &producer_done);
    if (rc != 0) {
        fprintf(stderr, "Could not create simulation thread.\n");
        atomic_store_explicit(&producer_done, true, memory_order_release);
        if (csv_started) csv_logger_join(&csv_context);
        if (plot_started) plotter_join(&plot_context);
        return -1;
    }

    rc = rt_simulation_join(&sim_context);
    if (csv_started) (void)csv_logger_join(&csv_context);
    if (plot_started) (void)plotter_join(&plot_context);

    printf("\n=== Simulation statistics ===\n");
    printf("Result:                    %s\n", rc == 0 ? "OK" : "ERROR");
    printf("SCHED_FIFO active:         %s\n", sim_context.stats.sched_fifo_active ? "yes" : "no");
    printf("Completed steps:           %llu\n", (unsigned long long)sim_context.stats.completed_steps);
    printf("Deadline misses:           %llu\n", (unsigned long long)sim_context.stats.deadline_misses);
    printf("Max FMU computation time:  %.6f ms\n", sim_context.stats.max_computation_s * 1000.0);
    printf("Max deadline lateness:     %.6f ms\n", sim_context.stats.max_lateness_s * 1000.0);
    if (csv_started) printf("Dropped CSV samples:       %llu\n", (unsigned long long)sample_queue_dropped(&log_queue));
    if (plot_started) printf("Dropped plot samples:      %llu\n", (unsigned long long)sample_queue_dropped(&plot_queue));
    printf("=============================\n\n");
    return rc;
}

static void print_menu(void) {
    printf("1) Load/import FMU\n");
    printf("2) List numeric FMU outputs\n");
    printf("3) Select outputs for CSV/plot\n");
    printf("4) Configure simulation timing\n");
    printf("5) Configure real-time thread\n");
    printf("6) Configure CSV logging\n");
    printf("7) Configure real-time plot\n");
    printf("8) Show current configuration\n");
    printf("9) Start simulation\n");
    printf("0) Exit\n");
}

int main(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    AppConfig config;
    FmuModel model;
    app_config_set_defaults(&config);
    fmu_model_init(&model);

    printf("FMU 2.0 Co-Simulation Real-Time Runner\n");
    printf("Linux / POSIX threads / SCHED_FIFO / gnuplot\n\n");

    for (;;) {
        print_menu();
        int option = read_int("Option: ", -1);
        switch (option) {
            case 1: (void)load_fmu_menu(&model, &config); break;
            case 2: if (model.fmu) list_outputs(&model); else printf("Load an FMU first.\n"); break;
            case 3: if (model.fmu) select_outputs(&model, &config); else printf("Load an FMU first.\n"); break;
            case 4: configure_timing(&config); break;
            case 5: configure_realtime(&config); break;
            case 6: configure_csv(&config); break;
            case 7: configure_plot(&config); break;
            case 8: app_config_print(&config); break;
            case 9: (void)run_simulation(&model, &config); break;
            case 0: fmu_model_unload(&model); return 0;
            default: printf("Invalid option.\n"); break;
        }
    }
}
