#ifndef RUN_RESULT_H
#define RUN_RESULT_H

/* Contrato público do módulo run result. */

#include <stdbool.h>
#include <stdint.h>

#include "run_logging.h"

#define SIMULATION_RESULT_MESSAGE_SIZE 96U

typedef enum {
    SIMULATION_RUN_FINISHED = 0,
    SIMULATION_RUN_STOPPED,
    SIMULATION_RUN_ERROR
} simulation_run_state_t;

typedef struct {
    uint64_t completed_steps;
    uint64_t deadline_misses;
    uint64_t unused_releases;
    double max_computation_s;
    double max_lateness_s;
    bool sched_fifo_active;
} simulation_stats_t;

typedef struct {
    simulation_run_state_t state;
    int code;
    char stage[32];
    char message[SIMULATION_RESULT_MESSAGE_SIZE];
    simulation_stats_t stats;
} simulation_run_result_t;

typedef struct {
    simulation_run_result_t simulation;
    run_logging_result_t logging;
} run_result_t;

void RunResultAggregate(run_result_t *result, const simulation_run_result_t *simulation, const run_logging_result_t *logging);

#endif
