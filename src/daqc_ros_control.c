/* Implementação do módulo daqc ros control. */
#define _POSIX_C_SOURCE 200809L
#include "daqc_ros_control.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "microhil_interfaces/msg/daqc_errors.h"
#include "microhil_interfaces/msg/daqc_setup.h"
#include "microhil_interfaces/msg/daqc_state.h"
#include "rcl/rcl.h"
#include "rosidl_typesupport_interface/macros.h"

#define DAQC_ROS_SPIN_TIMEOUT_NS UINT64_C(5000000)

typedef struct {
    rcl_context_t context;
    rcl_node_t node;
    rcl_publisher_t setup_publisher;
    rcl_subscription_t state_subscription;
    rcl_subscription_t errors_subscription;
    rcl_wait_set_t wait_set;
    microhil_interfaces__msg__DaqcState state_message;
    microhil_interfaces__msg__DaqcErrors errors_message;
    bool context_initialized;
    bool node_initialized;
    bool setup_publisher_initialized;
    bool state_subscription_initialized;
    bool errors_subscription_initialized;
    bool wait_set_initialized;
} daqc_ros_implementation_t;

static void IgnoreRclResult(rcl_ret_t result) {
    (void)result;
}

static bool HasRemoteError(const daqc_ros_errors_t *errors) {
    return errors->ros_error || errors->communication_error || errors->fmu_error || errors->profile_error ||
           errors->adc_configuration_error || errors->pwm_configuration_error || errors->timeout_error || errors->invalid_data_error;
}

static struct timespec AddMilliseconds(const struct timespec *origin, uint32_t timeout_ms) {
    struct timespec deadline = *origin;
    deadline.tv_sec += (time_t)(timeout_ms / 1000U);
    deadline.tv_nsec += (long)(timeout_ms % 1000U) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        ++deadline.tv_sec;
        deadline.tv_nsec -= 1000000000L;
    }
    return deadline;
}

static void FinalizeImplementation(daqc_ros_implementation_t *implementation) {
    if (!implementation) return;
    if (implementation->wait_set_initialized) IgnoreRclResult(rcl_wait_set_fini(&implementation->wait_set));
    if (implementation->errors_subscription_initialized) IgnoreRclResult(rcl_subscription_fini(&implementation->errors_subscription, &implementation->node));
    if (implementation->state_subscription_initialized) IgnoreRclResult(rcl_subscription_fini(&implementation->state_subscription, &implementation->node));
    if (implementation->setup_publisher_initialized) IgnoreRclResult(rcl_publisher_fini(&implementation->setup_publisher, &implementation->node));
    if (implementation->node_initialized) IgnoreRclResult(rcl_node_fini(&implementation->node));
    if (implementation->context_initialized) IgnoreRclResult(rcl_shutdown(&implementation->context));
    if (implementation->context_initialized) IgnoreRclResult(rcl_context_fini(&implementation->context));
    free(implementation);
}

void DaqcRosControlObserveState(daqc_ros_control_t *control, const daqc_ros_state_t *state) {
    if (!control || !state || !control->initialized) return;
    if (pthread_mutex_lock(&control->mutex) != 0) return;
    control->last_state = *state;
    control->state_received = true;
    (void)pthread_cond_broadcast(&control->condition);
    (void)pthread_mutex_unlock(&control->mutex);
}

void DaqcRosControlObserveErrors(daqc_ros_control_t *control, const daqc_ros_errors_t *errors) {
    if (!control || !errors || !control->initialized) return;
    if (pthread_mutex_lock(&control->mutex) != 0) return;
    control->last_errors = *errors;
    control->errors_received = true;
    (void)pthread_cond_broadcast(&control->condition);
    (void)pthread_mutex_unlock(&control->mutex);
}

static void TakeState(daqc_ros_control_t *control, daqc_ros_implementation_t *implementation) {
    if (rcl_take(&implementation->state_subscription, &implementation->state_message, NULL, NULL) != RCL_RET_OK) return;
    const daqc_ros_state_t state = {
        .state = implementation->state_message.state,
        .profile_id = implementation->state_message.profile_id,
        .profile_applied = implementation->state_message.profile_applied != 0U,
        .configuration_applied = implementation->state_message.configuration_applied != 0U,
    };
    DaqcRosControlObserveState(control, &state);
}

static void TakeErrors(daqc_ros_control_t *control, daqc_ros_implementation_t *implementation) {
    if (rcl_take(&implementation->errors_subscription, &implementation->errors_message, NULL, NULL) != RCL_RET_OK) return;
    const daqc_ros_errors_t errors = {
        .source_is_daqc = implementation->errors_message.source_is_daqc != 0U,
        .ros_error = implementation->errors_message.ros_error != 0U,
        .communication_error = implementation->errors_message.communication_error != 0U,
        .fmu_error = implementation->errors_message.fmu_error != 0U,
        .profile_error = implementation->errors_message.profile_error != 0U,
        .adc_configuration_error = implementation->errors_message.adc_configuration_error != 0U,
        .pwm_configuration_error = implementation->errors_message.pwm_configuration_error != 0U,
        .timeout_error = implementation->errors_message.timeout_error != 0U,
        .invalid_data_error = implementation->errors_message.invalid_data_error != 0U,
    };
    DaqcRosControlObserveErrors(control, &errors);
}

static void *DaqcRosControlThread(void *argument) {
    daqc_ros_control_t *control = argument;
    daqc_ros_implementation_t *implementation = control->implementation;
    while (true) {
        if (pthread_mutex_lock(&control->mutex) != 0) break;
        const bool stop_requested = control->stop_requested;
        if (pthread_mutex_unlock(&control->mutex) != 0 || stop_requested) break;

        if (rcl_wait_set_clear(&implementation->wait_set) != RCL_RET_OK ||
            rcl_wait_set_add_subscription(&implementation->wait_set, &implementation->state_subscription, NULL) < 0 ||
            rcl_wait_set_add_subscription(&implementation->wait_set, &implementation->errors_subscription, NULL) < 0) {
            continue;
        }
        const rcl_ret_t wait_status = rcl_wait(&implementation->wait_set, DAQC_ROS_SPIN_TIMEOUT_NS);
        if (wait_status != RCL_RET_OK && wait_status != RCL_RET_TIMEOUT) continue;
        if (implementation->wait_set.subscriptions[0]) TakeState(control, implementation);
        if (implementation->wait_set.subscriptions[1]) TakeErrors(control, implementation);
    }
    return NULL;
}

daqc_ros_control_status_t DaqcRosControlInit(daqc_ros_control_t *control) {
    if (!control) return DAQC_ROS_CONTROL_INVALID_ARGUMENT;
    *control = (daqc_ros_control_t){0};
    if (pthread_mutex_init(&control->mutex, NULL) != 0) return DAQC_ROS_CONTROL_MUTEX;
    if (pthread_cond_init(&control->condition, NULL) != 0) {
        (void)pthread_mutex_destroy(&control->mutex);
        return DAQC_ROS_CONTROL_MUTEX;
    }
    control->initialized = true;
    return DAQC_ROS_CONTROL_OK;
}

daqc_ros_control_status_t DaqcRosControlStart(daqc_ros_control_t *control) {
    if (!control || !control->initialized || control->started) return DAQC_ROS_CONTROL_INVALID_ARGUMENT;
    daqc_ros_implementation_t *implementation = calloc(1U, sizeof(*implementation));
    if (!implementation) return DAQC_ROS_CONTROL_INITIALIZATION;
    implementation->context = rcl_get_zero_initialized_context();
    implementation->node = rcl_get_zero_initialized_node();
    implementation->setup_publisher = rcl_get_zero_initialized_publisher();
    implementation->state_subscription = rcl_get_zero_initialized_subscription();
    implementation->errors_subscription = rcl_get_zero_initialized_subscription();
    implementation->wait_set = rcl_get_zero_initialized_wait_set();

    rcl_init_options_t options = rcl_get_zero_initialized_init_options();
    rcl_allocator_t allocator = rcl_get_default_allocator();
    if (rcl_init_options_init(&options, allocator) != RCL_RET_OK || rcl_init(0, NULL, &options, &implementation->context) != RCL_RET_OK) {
        IgnoreRclResult(rcl_init_options_fini(&options));
        FinalizeImplementation(implementation);
        return DAQC_ROS_CONTROL_INITIALIZATION;
    }
    IgnoreRclResult(rcl_init_options_fini(&options));
    implementation->context_initialized = true;
    const rcl_node_options_t node_options = rcl_node_get_default_options();
    if (rcl_node_init(&implementation->node, "microhil_runner", "", &implementation->context, &node_options) != RCL_RET_OK) {
        FinalizeImplementation(implementation);
        return DAQC_ROS_CONTROL_INITIALIZATION;
    }
    implementation->node_initialized = true;
    const rcl_publisher_options_t publisher_options = rcl_publisher_get_default_options();
    const rcl_subscription_options_t subscription_options = rcl_subscription_get_default_options();
    if (rcl_publisher_init(&implementation->setup_publisher, &implementation->node,
                           ROSIDL_GET_MSG_TYPE_SUPPORT(microhil_interfaces, msg, DaqcSetup), "/daqc_setup", &publisher_options) != RCL_RET_OK ||
        rcl_subscription_init(&implementation->state_subscription, &implementation->node,
                              ROSIDL_GET_MSG_TYPE_SUPPORT(microhil_interfaces, msg, DaqcState), "/daqc_state", &subscription_options) != RCL_RET_OK ||
        rcl_subscription_init(&implementation->errors_subscription, &implementation->node,
                              ROSIDL_GET_MSG_TYPE_SUPPORT(microhil_interfaces, msg, DaqcErrors), "/daqc_errors", &subscription_options) != RCL_RET_OK) {
        FinalizeImplementation(implementation);
        return DAQC_ROS_CONTROL_INITIALIZATION;
    }
    implementation->setup_publisher_initialized = true;
    implementation->state_subscription_initialized = true;
    implementation->errors_subscription_initialized = true;
    if (rcl_wait_set_init(&implementation->wait_set, 2U, 0U, 0U, 0U, 0U, 0U, &implementation->context, allocator) != RCL_RET_OK) {
        FinalizeImplementation(implementation);
        return DAQC_ROS_CONTROL_INITIALIZATION;
    }
    implementation->wait_set_initialized = true;
    control->implementation = implementation;
    if (pthread_create(&control->thread, NULL, DaqcRosControlThread, control) != 0) {
        control->implementation = NULL;
        FinalizeImplementation(implementation);
        return DAQC_ROS_CONTROL_INITIALIZATION;
    }
    control->started = true;
    return DAQC_ROS_CONTROL_OK;
}

void DaqcRosControlStop(daqc_ros_control_t *control) {
    if (!control || !control->initialized) return;
    if (control->started) {
        if (pthread_mutex_lock(&control->mutex) == 0) {
            control->stop_requested = true;
            (void)pthread_cond_broadcast(&control->condition);
            (void)pthread_mutex_unlock(&control->mutex);
        }
        (void)pthread_join(control->thread, NULL);
        control->started = false;
    }
    FinalizeImplementation(control->implementation);
    control->implementation = NULL;
    (void)pthread_cond_destroy(&control->condition);
    (void)pthread_mutex_destroy(&control->mutex);
    *control = (daqc_ros_control_t){0};
}

daqc_ros_control_status_t DaqcRosControlPublishSetup(daqc_ros_control_t *control, uint8_t command,
                                                      const profile_config_t *profile, bool apply_configuration) {
    if (!control || !control->started || !control->implementation || !profile || profile->profile_id == 0U) return DAQC_ROS_CONTROL_INVALID_ARGUMENT;
    const profile_daqc_configuration_t *configuration = &profile->daqc_configuration;
    microhil_interfaces__msg__DaqcSetup message = {
        .command = command,
        .profile_id = profile->profile_id,
        .apply_configuration = apply_configuration ? 1U : 0U,
        .adc_resolution_bits = configuration->adc_resolution_bits,
    };
    memcpy(message.adc_attenuation, configuration->adc_attenuation, sizeof(message.adc_attenuation));
    memcpy(message.pwm_frequency_hz, configuration->pwm_frequency_hz, sizeof(message.pwm_frequency_hz));
    memcpy(message.pwm_resolution_bits, configuration->pwm_resolution_bits, sizeof(message.pwm_resolution_bits));
    daqc_ros_implementation_t *implementation = control->implementation;
    return rcl_publish(&implementation->setup_publisher, &message, NULL) == RCL_RET_OK ? DAQC_ROS_CONTROL_OK : DAQC_ROS_CONTROL_PUBLISH;
}

daqc_ros_control_status_t DaqcRosControlWaitConfiguration(daqc_ros_control_t *control, uint8_t expected_state,
                                                           uint32_t profile_id, uint32_t timeout_ms) {
    if (!control || !control->initialized || profile_id == 0U || timeout_ms == 0U) return DAQC_ROS_CONTROL_INVALID_ARGUMENT;
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) != 0) return DAQC_ROS_CONTROL_TIMEOUT;
    const struct timespec deadline = AddMilliseconds(&now, timeout_ms);
    if (pthread_mutex_lock(&control->mutex) != 0) return DAQC_ROS_CONTROL_MUTEX;
    while (true) {
        if (control->errors_received && HasRemoteError(&control->last_errors)) {
            (void)pthread_mutex_unlock(&control->mutex);
            return DAQC_ROS_CONTROL_REMOTE_ERROR;
        }
        if (control->state_received && control->last_state.state == expected_state && control->last_state.profile_id == profile_id &&
            control->last_state.profile_applied && control->last_state.configuration_applied) {
            (void)pthread_mutex_unlock(&control->mutex);
            return DAQC_ROS_CONTROL_OK;
        }
        const int wait_status = pthread_cond_timedwait(&control->condition, &control->mutex, &deadline);
        if (wait_status == ETIMEDOUT) {
            (void)pthread_mutex_unlock(&control->mutex);
            return DAQC_ROS_CONTROL_TIMEOUT;
        }
        if (wait_status != 0) {
            (void)pthread_mutex_unlock(&control->mutex);
            return DAQC_ROS_CONTROL_MUTEX;
        }
    }
}

const char *DaqcRosControlStatusString(daqc_ros_control_status_t status) {
    switch (status) {
        case DAQC_ROS_CONTROL_OK: return "ok";
        case DAQC_ROS_CONTROL_INVALID_ARGUMENT: return "invalid argument";
        case DAQC_ROS_CONTROL_MUTEX: return "ROS control synchronization failed";
        case DAQC_ROS_CONTROL_INITIALIZATION: return "could not initialize ROS control";
        case DAQC_ROS_CONTROL_PUBLISH: return "could not publish DAQC setup";
        case DAQC_ROS_CONTROL_TIMEOUT: return "DAQC ROS confirmation timed out";
        case DAQC_ROS_CONTROL_REMOTE_ERROR: return "DAQC reported a ROS control error";
        default: return "unknown ROS control status";
    }
}
