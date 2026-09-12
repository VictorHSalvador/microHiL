#ifndef RT_SIMULATION_H
#define RT_SIMULATION_H

/* Contrato público do módulo rt simulation. */

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include "app_config.h"
#include "daq_output_bridge.h"
#include "fmu_model.h"
#include "run_logging.h"
#include "run_result.h"
#include "sample_queue.h"

typedef struct {
    pthread_t thread;
    FmuModel *model;
    const AppConfig *config;
    input_channel_descriptor_t input_descriptors[INPUT_STATE_MAX_CHANNELS];
    SampleQueue *plot_queue;
    _Atomic bool *stop_requested;
    _Atomic bool *plot_producer_done;
    run_logging_t *logging;
    daq_output_bridge_t *output_bridge;
    input_state_t input_state;
    bool input_state_ready;
    bool prepared;
    bool thread_started;
    bool require_realtime;
    simulation_stats_t stats;
    simulation_run_result_t run_result;
    int result;
} RtSimulationContext;

int RtSimulationPrepare(RtSimulationContext *context, FmuModel *model, const AppConfig *config);
int RtSimulationCheckHilRealtime(const AppConfig *config, char *message, size_t message_size);
void RtSimulationAbort(RtSimulationContext *context);
int RtSimulationStart(RtSimulationContext *context, run_logging_t *logging, SampleQueue *plot_queue, daq_output_bridge_t *output_bridge,
                      _Atomic bool *stop_requested, _Atomic bool *plot_producer_done);
int RtSimulationJoin(RtSimulationContext *context);

#endif
