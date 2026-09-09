#ifndef RT_SIMULATION_H
#define RT_SIMULATION_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include "app_config.h"
#include "fmu_model.h"
#include "run_logging.h"
#include "run_result.h"
#include "sample_queue.h"

typedef struct {
    pthread_t thread;
    FmuModel *model;
    const AppConfig *config;
    SampleQueue *plot_queue;
    _Atomic bool *stop_requested;
    _Atomic bool *plot_producer_done;
    run_logging_t *logging;
    input_state_t input_state;
    bool input_state_ready;
    simulation_stats_t stats;
    simulation_run_result_t run_result;
    int result;
} RtSimulationContext;

int rt_simulation_start(RtSimulationContext *context, FmuModel *model, const AppConfig *config,
                        run_logging_t *logging, SampleQueue *plot_queue,
                        _Atomic bool *stop_requested, _Atomic bool *plot_producer_done);
int rt_simulation_join(RtSimulationContext *context);

#endif
