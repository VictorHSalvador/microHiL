#include <stdio.h>
#include <stdlib.h>

#include "daqc_ros_control.h"

static void Require(bool condition, const char *message) {
    if (condition) return;
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}

int main(void) {
    daqc_ros_control_t control;
    Require(DaqcRosControlInit(&control) == DAQC_ROS_CONTROL_OK, "could not initialize ROS control state");
    Require(DaqcRosControlStart(&control) == DAQC_ROS_CONTROL_OK, "could not start the runner ROS control thread");

    profile_config_t profile = {
        .profile_id = 1U,
        .daqc_configuration = {
            .adc_resolution_bits = 12U,
            .adc_attenuation = {3U, 3U, 3U, 3U, 3U, 3U},
            .pwm_frequency_hz = {20000U, 20000U},
            .pwm_resolution_bits = {10U, 10U},
            .acquisition_frequency_hz = 100U,
        },
    };
    Require(DaqcRosControlPublishSetup(&control, 2U, &profile, true) == DAQC_ROS_CONTROL_OK,
            "could not publish the typed DAQC setup message");

    const daqc_ros_state_t enabled = {.state = 2U, .profile_id = 1U, .profile_applied = true, .configuration_applied = true};
    DaqcRosControlObserveState(&control, &enabled);
    Require(DaqcRosControlWaitConfiguration(&control, 2U, 1U, 1U) == DAQC_ROS_CONTROL_OK,
            "matching DAQC state did not confirm the prepared profile");

    const daqc_ros_errors_t errors = {.source_is_daqc = true, .profile_error = true};
    DaqcRosControlObserveErrors(&control, &errors);
    Require(DaqcRosControlWaitConfiguration(&control, 2U, 1U, 1U) == DAQC_ROS_CONTROL_REMOTE_ERROR,
            "DAQC diagnostic did not prevent configuration approval");

    DaqcRosControlStop(&control);
    return EXIT_SUCCESS;
}
