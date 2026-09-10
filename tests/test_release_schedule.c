#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "release_schedule.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void TestFixedGridAfterOverrun(void) {
    release_schedule_t schedule;
    Require(ReleaseScheduleInit(&schedule, 0.01), "could not initialize a 10 ms release schedule");
    Require(ReleaseScheduleReleaseNs(&schedule) == 0U, "first release must start at the origin");
    Require(ReleaseScheduleCompleteStep(&schedule, UINT64_C(12000000)), "could not complete the first step");
    Require(schedule.release_index == 2U && ReleaseScheduleReleaseNs(&schedule) == UINT64_C(20000000),
            "a 12 ms first step did not wait for the 20 ms release");
    Require(schedule.late_steps == 1U && schedule.unused_releases == 1U && schedule.max_completion_lateness_ns == UINT64_C(2000000),
            "overrun metrics did not separate the late step from the unused release");
}

static void TestExactBoundaryAndInvalidStep(void) {
    release_schedule_t schedule;
    Require(!ReleaseScheduleInit(&schedule, 0.0) && !ReleaseScheduleInit(&schedule, -0.01), "invalid steps must be rejected");
    Require(ReleaseScheduleInit(&schedule, 0.01), "could not initialize a release schedule");
    Require(ReleaseScheduleCompleteStep(&schedule, UINT64_C(10000000)), "could not complete on the boundary");
    Require(schedule.release_index == 1U && schedule.unused_releases == 0U && schedule.late_steps == 0U,
            "completion exactly on the boundary must not count as a missed release");
}

static void TestLateWakeupWaitsForTheNextFixedRelease(void) {
    release_schedule_t schedule;
    Require(ReleaseScheduleInit(&schedule, 0.01), "could not initialize a release schedule");
    Require(ReleaseScheduleCompleteStep(&schedule, UINT64_C(12000000)), "could not complete the first step");
    Require(ReleaseScheduleAdvanceToEarliestRelease(&schedule, UINT64_C(22000000)), "could not advance after a late wakeup");
    Require(schedule.release_index == 3U && ReleaseScheduleReleaseNs(&schedule) == UINT64_C(30000000) && schedule.unused_releases == 2U,
            "late wakeup did not skip to the next fixed release");
}

int main(int argc, char **argv) {
    if (argc != 2) return EXIT_FAILURE;
    if (argv[1][0] == 'o') TestFixedGridAfterOverrun();
    else if (argv[1][0] == 'b') TestExactBoundaryAndInvalidStep();
    else if (argv[1][0] == 'w') TestLateWakeupWaitsForTheNextFixedRelease();
    else return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
