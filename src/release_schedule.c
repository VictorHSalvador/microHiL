#include "release_schedule.h"

#include <math.h>
#include <stddef.h>

static bool SecondsToNanoseconds(double seconds, uint64_t *nanoseconds) {
    if (!isfinite(seconds) || seconds <= 0.0 || seconds > (double)UINT64_MAX / 1e9) return false;
    const double converted = round(seconds * 1e9);
    if (converted < 1.0 || converted > (double)UINT64_MAX) return false;
    *nanoseconds = (uint64_t)converted;
    return true;
}

bool ReleaseScheduleInit(release_schedule_t *schedule, double step_size_s) {
    if (!schedule || !SecondsToNanoseconds(step_size_s, &schedule->step_ns)) return false;
    schedule->release_index = 0U;
    schedule->unused_releases = 0U;
    schedule->late_steps = 0U;
    schedule->max_completion_lateness_ns = 0U;
    return true;
}

uint64_t ReleaseScheduleReleaseNs(const release_schedule_t *schedule) {
    if (!schedule || schedule->step_ns == 0U || schedule->release_index > UINT64_MAX / schedule->step_ns) return UINT64_MAX;
    return schedule->release_index * schedule->step_ns;
}

bool ReleaseScheduleAdvanceToEarliestRelease(release_schedule_t *schedule, uint64_t now_since_origin_ns) {
    if (!schedule || schedule->step_ns == 0U) return false;
    const uint64_t current_release_ns = ReleaseScheduleReleaseNs(schedule);
    if (now_since_origin_ns <= current_release_ns) return current_release_ns != UINT64_MAX;

    uint64_t earliest_release = now_since_origin_ns / schedule->step_ns;
    if (now_since_origin_ns % schedule->step_ns != 0U) ++earliest_release;
    if (earliest_release < schedule->release_index || earliest_release == UINT64_MAX) return false;
    schedule->unused_releases += earliest_release - schedule->release_index;
    schedule->release_index = earliest_release;
    return true;
}

bool ReleaseScheduleCompleteStep(release_schedule_t *schedule, uint64_t completion_since_origin_ns) {
    if (!schedule || schedule->step_ns == 0U || schedule->release_index == UINT64_MAX) return false;
    if (schedule->release_index > (UINT64_MAX / schedule->step_ns) - 1U) return false;

    const uint64_t deadline_ns = (schedule->release_index + 1U) * schedule->step_ns;
    if (completion_since_origin_ns > deadline_ns) {
        const uint64_t lateness_ns = completion_since_origin_ns - deadline_ns;
        ++schedule->late_steps;
        if (lateness_ns > schedule->max_completion_lateness_ns) schedule->max_completion_lateness_ns = lateness_ns;
    }

    uint64_t next_release = completion_since_origin_ns / schedule->step_ns;
    if (completion_since_origin_ns % schedule->step_ns != 0U) ++next_release;
    if (next_release <= schedule->release_index) next_release = schedule->release_index + 1U;
    if (next_release > schedule->release_index + 1U) schedule->unused_releases += next_release - schedule->release_index - 1U;
    schedule->release_index = next_release;
    return true;
}
