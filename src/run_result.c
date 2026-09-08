#include "run_result.h"

void RunResultAggregate(run_result_t *result, const simulation_run_result_t *simulation, const run_logging_result_t *logging) {
    if (!result) return;
    *result = (run_result_t){0};
    if (simulation) result->simulation = *simulation;
    if (logging) result->logging = *logging;
}
