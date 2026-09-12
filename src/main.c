/* Implementação do módulo main. */
#define _POSIX_C_SOURCE 200809L
#include "app_config.h"
#include "daq_coordinator.h"
#include "daq_output_bridge.h"
#include "daq_serial_service.h"
#include "daq_state_control.h"
#include "daq_xrce_udp_bridge.h"
#ifdef MICROHIL_WITH_ROS2_CONTROL
#include "daqc_ros_control.h"
#endif
#include "execution_session.h"
#include "log_converter.h"
#include "plotter.h"
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
#define MAX_CANDIDATE_INPUTS 512

static _Atomic bool *g_stop_requested = NULL;

#ifdef MICROHIL_WITH_ROS2_CONTROL
typedef struct {
    daq_coordinator_t coordinator;
    daq_xrce_udp_bridge_t xrce_bridge;
    daq_serial_service_t serial_service;
    daqc_ros_control_t ros_control;
    daq_output_bridge_t output_bridge;
    bool coordinator_initialized;
    bool xrce_bridge_started;
    bool serial_service_started;
    bool ros_control_initialized;
} daqc_runtime_t;

static void StopDaqcRuntime(daqc_runtime_t *runtime, const AppConfig *config) {
    if (!runtime) return;
    if (runtime->coordinator_initialized && runtime->serial_service_started) {
        daq_link_mode_t mode;
        if (DaqCoordinatorGetMode(&runtime->coordinator, &mode) == DAQ_COORDINATOR_OK && mode != DAQ_LINK_DISABLED) {
            (void)DaqStateControlTransition(&runtime->coordinator, mode, DAQ_PROTOCOL_COMMAND_DISABLE, config->daqc_config_timeout_ms);
        }
    }
    if (runtime->serial_service_started) DaqSerialServiceStop(&runtime->serial_service);
    if (runtime->ros_control_initialized) DaqcRosControlStop(&runtime->ros_control);
    if (runtime->xrce_bridge_started) DaqXrceUdpBridgeStop(&runtime->xrce_bridge);
    if (runtime->coordinator_initialized) DaqCoordinatorDestroy(&runtime->coordinator);
    *runtime = (daqc_runtime_t){0};
}

static int StartDaqcRuntime(daqc_runtime_t *runtime, const AppConfig *config, RtSimulationContext *simulation) {
    if (!runtime || !config || !simulation || !simulation->input_state_ready || !config->profile_loaded || !config->daqc_device_path[0]) return -1;
    *runtime = (daqc_runtime_t){0};
    const daq_coordinator_config_t coordinator_config = {
        .acquisition_schema = &config->acquisition_schema,
        .input_state = &simulation->input_state,
        .xrce_mtu = DAQ_PROTOCOL_XRCE_MTU,
        .xrce_receive = DaqXrceUdpBridgeReceiveFromDaqc,
        .xrce_context = &runtime->xrce_bridge,
    };
    if (DaqCoordinatorInit(&runtime->coordinator, &coordinator_config) != DAQ_COORDINATOR_OK) goto fail;
    runtime->coordinator_initialized = true;
    const daq_xrce_udp_bridge_config_t bridge_config = {.coordinator = &runtime->coordinator, .agent_port = config->daqc_agent_port};
    if (DaqXrceUdpBridgeStart(&runtime->xrce_bridge, &bridge_config) != DAQ_XRCE_UDP_BRIDGE_OK) goto fail;
    runtime->xrce_bridge_started = true;
    const daq_serial_service_config_t serial_config = {
        .coordinator = &runtime->coordinator,
        .device_path = config->daqc_device_path,
        .baud_rate = config->daqc_baud_rate,
    };
    if (DaqSerialServiceStart(&runtime->serial_service, &serial_config) != DAQ_SERIAL_SERVICE_OK) goto fail;
    runtime->serial_service_started = true;
    if (DaqcRosControlInit(&runtime->ros_control) != DAQC_ROS_CONTROL_OK) goto fail;
    runtime->ros_control_initialized = true;
    if (DaqcRosControlStart(&runtime->ros_control) != DAQC_ROS_CONTROL_OK) goto fail;
    if (DaqStateControlTransition(&runtime->coordinator, DAQ_LINK_DISABLED, DAQ_PROTOCOL_COMMAND_ENABLE, config->daqc_config_timeout_ms) != DAQ_STATE_CONTROL_OK ||
        DaqcRosControlPublishSetup(&runtime->ros_control, DAQ_PROTOCOL_COMMAND_ENABLE, &config->profile, true) != DAQC_ROS_CONTROL_OK ||
        DaqcRosControlWaitConfiguration(&runtime->ros_control, DAQ_PROTOCOL_COMMAND_ENABLE, config->profile.profile_id, config->daqc_ros_timeout_ms) != DAQC_ROS_CONTROL_OK ||
        DaqOutputBridgeInit(&runtime->output_bridge, &config->actuation_schema, &runtime->coordinator) != DAQ_OUTPUT_BRIDGE_OK ||
        DaqStateControlPlay(&runtime->coordinator, config->daqc_config_timeout_ms) != DAQ_STATE_CONTROL_OK) {
        goto fail;
    }
    return 0;

fail:
    StopDaqcRuntime(runtime, config);
    return -1;
}
#endif

static void signal_handler(int signal_number) {
    (void)signal_number;
    if (g_stop_requested) atomic_store_explicit(g_stop_requested, true, memory_order_relaxed);
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
        printf("%4zu) XML=%-6u %-12s VR=%-8u %s\n", i + 1, candidates[i].xml_index,
               numeric_type_name(candidates[i].type), candidates[i].value_reference, candidates[i].name);
    }
    printf("\n");
}

static void list_inputs(FmuModel *model) {
    input_channel_descriptor_t candidates[MAX_CANDIDATE_INPUTS];
    size_t count = fmu_model_list_numeric_inputs(model, candidates, MAX_CANDIDATE_INPUTS);
    if (count > MAX_CANDIDATE_INPUTS) count = MAX_CANDIDATE_INPUTS;
    printf("\nNumeric FMU inputs (%zu):\n", count);
    for (size_t index = 0U; index < count; ++index) {
        printf("%4zu) %-12s VR=%-8u %s%s\n", index + 1U, candidates[index].input_name, candidates[index].value_reference,
               numeric_type_name(candidates[index].type), candidates[index].initial_value_valid ? " (initial value available)" : "");
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
            if (config->outputs[i].xml_index == candidates[index - 1].xml_index) {
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
    config->profile_loaded = false;
    config->profile_path[0] = '\0';
    config->input_count = fmu_model_list_numeric_inputs(model, config->inputs, INPUT_STATE_MAX_CHANNELS);
    if (config->input_count > INPUT_STATE_MAX_CHANNELS) {
        fprintf(stderr, "FMU has too many numeric inputs for the current HOST limit.\n");
        config->input_count = 0U;
        return -1;
    }
    printf("Loaded FMI 2.0 Co-Simulation model: %s\n", fmu_model_name(model));
    list_outputs(model);
    list_inputs(model);
    return 0;
}

static bool IsMappedOutputSelected(const AppConfig *config, const profile_mapping_t *mapping) {
    for (size_t index = 0U; index < config->output_count; ++index) {
        const OutputVariable *output = &config->outputs[index];
        if (output->fmu_index == mapping->fmu_index && output->value_reference == mapping->value_reference && output->type == mapping->type) {
            return true;
        }
    }
    return false;
}

static bool ProfileOutputsAreSelected(const AppConfig *config) {
    for (size_t index = 0U; index < config->profile.mapping_count; ++index) {
        const profile_mapping_t *mapping = &config->profile.mappings[index];
        if (!mapping->is_input && !IsMappedOutputSelected(config, mapping)) {
            printf("Mapped DAQC output is not selected: %s (%s).\n", mapping->variable, mapping->channel);
            return false;
        }
    }
    return true;
}

static int load_profile_menu(FmuModel *model, AppConfig *config) {
    if (!model->fmu) {
        printf("Load an FMU first.\n");
        return -1;
    }
    char path[PATH_LEN];
    profile_config_t candidate;
    daq_schema_t acquisition_schema;
    daq_schema_t actuation_schema;
    read_line("YAML profile path: ", path, sizeof(path));
    if (!path[0]) return -1;
    if (ProfileConfigLoadYaml(path, &candidate) != PROFILE_CONFIG_OK) {
        printf("Could not load the YAML profile.\n");
        return -1;
    }
    if (fmu_model_resolve_profile_mappings(model, &candidate) != 0) {
        printf("YAML mappings are incompatible with the loaded FMU.\n");
        return -1;
    }
    if (ProfileConfigBuildAcquisitionSchema(&candidate, &acquisition_schema) != PROFILE_CONFIG_OK ||
        ProfileConfigBuildActuationSchema(&candidate, &actuation_schema) != PROFILE_CONFIG_OK) {
        printf("YAML mappings are incompatible with the ESP32 profile.\n");
        return -1;
    }
    config->profile = candidate;
    config->acquisition_schema = acquisition_schema;
    config->actuation_schema = actuation_schema;
    config->step_size_s = candidate.step_size_s;
    config->stop_time_s = candidate.stop_time_s;
    config->profile_loaded = true;
    snprintf(config->profile_path, sizeof(config->profile_path), "%s", path);
    printf("Loaded YAML profile %u with %zu mapping(s). Timing was updated from the profile.\n", candidate.profile_id, candidate.mapping_count);
    return 0;
}

static void configure_timing(AppConfig *config) {
    const double step_size_s = read_double("Step size in seconds [Enter keeps current]: ", config->step_size_s);
    const double stop_time_s = read_double("Simulation duration in seconds [Enter keeps current]: ", config->stop_time_s);
    const AppConfig candidate = {.step_size_s = step_size_s, .stop_time_s = stop_time_s};
    if (!app_config_timing_is_valid(&candidate)) {
        printf("Step size and duration must be finite positive values. Configuration unchanged.\n");
        return;
    }
    config->step_size_s = step_size_s;
    config->stop_time_s = stop_time_s;
}

static void configure_daqc_timeout(AppConfig *config) {
    const int timeout_ms = read_int("DAQC CONFIG confirmation timeout in milliseconds: ", (int)config->daqc_config_timeout_ms);
    if (timeout_ms <= 0) {
        printf("DAQC CONFIG confirmation timeout must be positive.\n");
        return;
    }
    config->daqc_config_timeout_ms = (uint32_t)timeout_ms;
    const int ros_timeout_ms = read_int("DAQC ROS setup confirmation timeout in milliseconds: ", (int)config->daqc_ros_timeout_ms);
    if (ros_timeout_ms <= 0) {
        printf("DAQC ROS setup confirmation timeout must be positive.\n");
        return;
    }
    config->daqc_ros_timeout_ms = (uint32_t)ros_timeout_ms;
}

static void ConfigureDaqcLink(AppConfig *config) {
    config->daqc_enabled = read_bool("Enable DAQC integration? [y/n]: ", config->daqc_enabled);
    if (!config->daqc_enabled) return;
    char device_path[PATH_LEN];
    read_line("DAQC TTY path [Enter keeps current]: ", device_path, sizeof(device_path));
    if (device_path[0]) snprintf(config->daqc_device_path, sizeof(config->daqc_device_path), "%s", device_path);
    const int agent_port = read_int("Local micro-ROS Agent UDP port [Enter keeps current]: ", (int)config->daqc_agent_port);
    if (agent_port < 1 || agent_port > 65535) {
        printf("Agent UDP port must be between 1 and 65535.\n");
        return;
    }
    config->daqc_agent_port = (uint16_t)agent_port;
}

static void configure_realtime(AppConfig *config) {
    config->rt_priority = read_int("SCHED_FIFO priority (1..99): ", config->rt_priority);
    if (config->rt_priority < 1) config->rt_priority = 1;
    if (config->rt_priority > 99) config->rt_priority = 99;
    config->cpu_core = read_int("CPU core (-1 disables affinity): ", config->cpu_core);
    config->strict_realtime = read_bool("Abort if SCHED_FIFO cannot be enabled? [y/n]: ", config->strict_realtime);
}

static void ConfigureBinaryLogging(AppConfig *config) {
    config->binary_log_enabled = read_bool("Enable binary logging? [y/n]: ", config->binary_log_enabled);
    if (config->binary_log_enabled) {
        char path[PATH_LEN];
        read_line("Binary log path [Enter keeps current]: ", path, sizeof(path));
        if (path[0]) snprintf(config->binary_log_path, sizeof(config->binary_log_path), "%s", path);
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

static void PrintLoggingResult(const run_logging_result_t *result) {
    if (!result) return;
    printf("\n=== Logging result ===\n");
    printf("Enabled:                   %s\n", result->enabled ? "yes" : "no");
    printf("Coordinator status:        %s\n", RunLoggingStatusString(result->status));
    printf("Incomplete:                %s\n", result->logger.incomplete ? "yes" : "no");
    printf("Accepted samples:          %llu\n", (unsigned long long)result->logger.accepted);
    printf("Persisted samples:         %llu\n", (unsigned long long)result->logger.persisted);
    printf("Discarded samples:         %llu\n", (unsigned long long)result->logger.discarded);
    if (result->logger.has_last_persisted_sequence) printf("Last persisted sequence:   %llu\n", (unsigned long long)result->logger.last_persisted_sequence);
    if (result->logger.first_error_stage != BINARY_LOGGER_STAGE_NONE) {
        printf("First logging error:       %s / %s: %s\n", BinaryLoggerStageString(result->logger.first_error_stage),
               BinaryLoggerStatusString(result->logger.first_error), result->logger.error_message);
    }
    printf("======================\n\n");
}

static const char *SimulationStateName(simulation_run_state_t state) {
    switch (state) {
        case SIMULATION_RUN_FINISHED: return "FINISHED";
        case SIMULATION_RUN_STOPPED: return "STOPPED";
        case SIMULATION_RUN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

static int run_simulation(execution_session_t *session, char last_log_path[PATH_LEN], bool *last_log_closed) {
    FmuModel *model = &session->model;
    AppConfig *config = &session->config;
    if (!model->fmu) {
        printf("Load an FMU first.\n");
        return -1;
    }
    if (!app_config_timing_is_valid(config)) {
        printf("Simulation timing must contain finite positive step and duration values.\n");
        return -1;
    }
    if (config->output_count == 0) {
        printf("Select at least one output first.\n");
        return -1;
    }
    if (config->profile_loaded && !ProfileOutputsAreSelected(config)) {
        printf("Select every FMU output mapped to the DAQC before starting the simulation.\n");
        return -1;
    }

#ifdef MICROHIL_WITH_ROS2_CONTROL
    if (config->daqc_enabled) {
        char realtime_message[160];
        if (RtSimulationCheckHilRealtime(config, realtime_message, sizeof(realtime_message)) != 0) {
            printf("DAQC Play requires SCHED_FIFO before streaming: %s.\n", realtime_message);
            return -1;
        }
    }
#endif

    if (app_config_normalize_outputs(config) != 0) {
        printf("Output selection has duplicate XML indices.\n");
        return -1;
    }
    printf("Output selection uses ascending XML index order for FMU reads, log samples, and the descriptor.\n");

    SampleQueue plot_queue;
    sample_queue_init(&plot_queue);

    PlotterContext plot_context;
#ifdef MICROHIL_WITH_ROS2_CONTROL
    daqc_runtime_t daqc_runtime;
    daq_output_bridge_t *output_bridge = NULL;
#endif
    bool plot_started = false;
    *last_log_closed = false;
    if (ExecutionSessionPrepare(session) != EXECUTION_SESSION_OK) {
        printf("Could not initialize the FMU and establish valid input references.\n");
        return -1;
    }
#ifdef MICROHIL_WITH_ROS2_CONTROL
    if (config->daqc_enabled) {
        if (!config->profile_loaded || !config->daqc_device_path[0]) {
            printf("DAQC integration requires a loaded YAML profile and a configured TTY path.\n");
            ExecutionSessionAbort(session);
            return -1;
        }
        if (StartDaqcRuntime(&daqc_runtime, config, &session->simulation) != 0) {
            printf("Could not prepare the DAQC, Agent bridge, or ROS confirmation.\n");
            ExecutionSessionAbort(session);
            return -1;
        }
        output_bridge = &daqc_runtime.output_bridge;
    }
#else
    if (config->daqc_enabled) {
        printf("This runner was built without ROS 2 DAQC support.\n");
        ExecutionSessionAbort(session);
        return -1;
    }
#endif
    (void)ExecutionSessionStartLogging(session);
    if (session->logging.result.started) snprintf(last_log_path, PATH_LEN, "%s", config->binary_log_path);
    if (config->plot_enabled) {
        if (plotter_start(&plot_context, &plot_queue, config, &session->producer_done) == 0) plot_started = true;
        else fprintf(stderr, "Could not start plotter thread.\n");
    }

    printf("\nStarting simulation. Press Ctrl+C to request a clean stop.\n");
    atomic_store_explicit(&session->stop_requested, false, memory_order_relaxed);
    g_stop_requested = &session->stop_requested;
    int rc = ExecutionSessionStart(session, plot_started ? &plot_queue : NULL,
#ifdef MICROHIL_WITH_ROS2_CONTROL
                                   output_bridge
#else
                                   NULL
#endif
    );
    if (rc != EXECUTION_SESSION_OK) {
        g_stop_requested = NULL;
        fprintf(stderr, "Could not create simulation thread.\n");
        ExecutionSessionAbort(session);
        if (plot_started) plotter_join(&plot_context);
#ifdef MICROHIL_WITH_ROS2_CONTROL
        if (config->daqc_enabled) StopDaqcRuntime(&daqc_runtime, config);
#endif
        PrintLoggingResult(RunLoggingResult(&session->logging));
        return -1;
    }

    rc = ExecutionSessionJoin(session);
    g_stop_requested = NULL;
#ifdef MICROHIL_WITH_ROS2_CONTROL
    if (config->daqc_enabled) StopDaqcRuntime(&daqc_runtime, config);
#endif
    if (plot_started) (void)plotter_join(&plot_context);
    const run_result_t *run_result = ExecutionSessionResult(session);
    if (!run_result) return -1;
    *last_log_closed = config->binary_log_enabled && run_result->logging.status == RUN_LOGGING_STATUS_OK && !run_result->logging.logger.incomplete;

    printf("\n=== Simulation statistics ===\n");
    printf("State:                     %s\n", SimulationStateName(run_result->simulation.state));
    printf("Code:                      %d\n", run_result->simulation.code);
    printf("Stage:                     %s\n", run_result->simulation.stage);
    printf("Message:                   %s\n", run_result->simulation.message);
    printf("SCHED_FIFO active:         %s\n", run_result->simulation.stats.sched_fifo_active ? "yes" : "no");
    printf("Completed steps:           %llu\n", (unsigned long long)run_result->simulation.stats.completed_steps);
    printf("Deadline misses:           %llu\n", (unsigned long long)run_result->simulation.stats.deadline_misses);
    printf("Unused releases:           %llu\n", (unsigned long long)run_result->simulation.stats.unused_releases);
    printf("Max FMU computation time:  %.6f ms\n", run_result->simulation.stats.max_computation_s * 1000.0);
    printf("Max deadline lateness:     %.6f ms\n", run_result->simulation.stats.max_lateness_s * 1000.0);
    if (plot_started) printf("Dropped plot samples:      %llu\n", (unsigned long long)sample_queue_dropped(&plot_queue));
    printf("=============================\n\n");
    PrintLoggingResult(&run_result->logging);
    return rc == EXECUTION_SESSION_OK ? 0 : -1;
}

static void ConvertClosedLog(AppConfig *config, const char *binary_path, bool log_closed) {
    char csv_path[PATH_LEN];
    log_descriptor_t descriptor;
    log_converter_result_t result;

    if (!log_closed || !binary_path || !binary_path[0]) {
        printf("No closed binary log is available for conversion.\n");
        return;
    }
    if (app_config_normalize_outputs(config) != 0 ||
        RunLoggingBuildDescriptor(config->fmu_path, config->outputs, config->output_count, config->step_size_s, &descriptor) != RUN_LOGGING_STATUS_OK) {
        printf("The selected FMU and outputs cannot provide a valid descriptor for conversion.\n");
        return;
    }
    read_line("CSV destination path: ", csv_path, sizeof(csv_path));
    if (!csv_path[0]) return;
    if (LogConverterConvert(LOG_CONVERTER_EXECUTION_CLOSED, binary_path, csv_path, &descriptor, &result) == LOG_CONVERTER_STATUS_OK ||
        result.status == LOG_CONVERTER_STATUS_PARTIAL) {
        printf("CSV conversion: %s (%llu record(s)).\n", LogConverterStatusString(result.status), (unsigned long long)result.exported_records);
    } else {
        printf("CSV conversion failed: %s (%s).\n", LogConverterStatusString(result.status), result.message);
    }
}

static void print_menu(void) {
    printf("1) Load/import FMU\n");
    printf("12) Load YAML DAQC profile\n");
    printf("13) Configure DAQC CONFIG confirmation timeout\n");
    printf("14) Configure DAQC link\n");
    printf("2) List numeric FMU outputs\n");
    printf("11) List numeric FMU inputs\n");
    printf("3) Select outputs for binary log/plot\n");
    printf("4) Configure simulation timing\n");
    printf("5) Configure real-time thread\n");
    printf("6) Configure binary logging\n");
    printf("7) Configure real-time plot\n");
    printf("8) Show current configuration\n");
    printf("9) Start simulation\n");
    printf("10) Convert last closed binary log to CSV\n");
    printf("0) Exit\n");
}

int main(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    execution_session_t session;
    ExecutionSessionInit(&session);
    AppConfig *config = &session.config;
    FmuModel *model = &session.model;
    char last_log_path[PATH_LEN] = {0};
    bool last_log_closed = false;

    printf("FMU 2.0 Co-Simulation Real-Time Runner\n");
    printf("Linux / POSIX threads / SCHED_FIFO / gnuplot\n\n");

    for (;;) {
        print_menu();
        int option = read_int("Option: ", -1);
        switch (option) {
            case 1: (void)load_fmu_menu(model, config); break;
            case 2: if (model->fmu) list_outputs(model); else printf("Load an FMU first.\n"); break;
            case 3: if (model->fmu) select_outputs(model, config); else printf("Load an FMU first.\n"); break;
            case 4: configure_timing(config); break;
            case 5: configure_realtime(config); break;
            case 6: ConfigureBinaryLogging(config); break;
            case 7: configure_plot(config); break;
            case 8: app_config_print(config); break;
            case 9: (void)run_simulation(&session, last_log_path, &last_log_closed); break;
            case 10: ConvertClosedLog(config, last_log_path, last_log_closed); break;
            case 11: if (model->fmu) list_inputs(model); else printf("Load an FMU first.\n"); break;
            case 12: (void)load_profile_menu(model, config); break;
            case 13: configure_daqc_timeout(config); break;
            case 14: ConfigureDaqcLink(config); break;
            case 0: ExecutionSessionDestroy(&session); return 0;
            default: printf("Invalid option.\n"); break;
        }
    }
}
