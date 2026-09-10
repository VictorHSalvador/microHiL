#ifndef DAQC_ROS_CONTROL_H
#define DAQC_ROS_CONTROL_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "profile_config.h"

typedef struct {
    uint8_t state;
    uint32_t profile_id;
    bool profile_applied;
    bool configuration_applied;
} daqc_ros_state_t;

typedef struct {
    bool source_is_daqc;
    bool ros_error;
    bool communication_error;
    bool fmu_error;
    bool profile_error;
    bool adc_configuration_error;
    bool pwm_configuration_error;
    bool timeout_error;
    bool invalid_data_error;
} daqc_ros_errors_t;

typedef enum {
    DAQC_ROS_CONTROL_OK = 0,
    DAQC_ROS_CONTROL_INVALID_ARGUMENT,
    DAQC_ROS_CONTROL_MUTEX,
    DAQC_ROS_CONTROL_INITIALIZATION,
    DAQC_ROS_CONTROL_PUBLISH,
    DAQC_ROS_CONTROL_TIMEOUT,
    DAQC_ROS_CONTROL_REMOTE_ERROR
} daqc_ros_control_status_t;

typedef struct {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    void *implementation;
    daqc_ros_state_t last_state;
    daqc_ros_errors_t last_errors;
    bool state_received;
    bool errors_received;
    bool stop_requested;
    bool initialized;
    bool started;
} daqc_ros_control_t;

daqc_ros_control_status_t DaqcRosControlInit(daqc_ros_control_t *control);
daqc_ros_control_status_t DaqcRosControlStart(daqc_ros_control_t *control);
void DaqcRosControlStop(daqc_ros_control_t *control);
daqc_ros_control_status_t DaqcRosControlPublishSetup(daqc_ros_control_t *control, uint8_t command,
                                                      const profile_config_t *profile, bool apply_configuration);
daqc_ros_control_status_t DaqcRosControlWaitConfiguration(daqc_ros_control_t *control, uint8_t expected_state,
                                                           uint32_t profile_id, uint32_t timeout_ms);
void DaqcRosControlObserveState(daqc_ros_control_t *control, const daqc_ros_state_t *state);
void DaqcRosControlObserveErrors(daqc_ros_control_t *control, const daqc_ros_errors_t *errors);
const char *DaqcRosControlStatusString(daqc_ros_control_status_t status);

#endif
