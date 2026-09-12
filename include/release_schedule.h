#ifndef RELEASE_SCHEDULE_H
#define RELEASE_SCHEDULE_H

/* Contrato público do módulo release schedule. */

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint64_t release_index;
    uint64_t unused_releases;
    uint64_t late_steps;
    uint64_t max_completion_lateness_ns;
    uint64_t step_ns;
} release_schedule_t;

bool ReleaseScheduleInit(release_schedule_t *schedule, double step_size_s);
bool ReleaseScheduleAdvanceToEarliestRelease(release_schedule_t *schedule, uint64_t now_since_origin_ns);
bool ReleaseScheduleCompleteStep(release_schedule_t *schedule, uint64_t completion_since_origin_ns);
uint64_t ReleaseScheduleReleaseNs(const release_schedule_t *schedule);

#endif
