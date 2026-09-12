/* Implementação do módulo daqc ros. */
#include "daqc_ros.h"

#include <stdatomic.h>
#include <string.h>

#include "daqc_transport.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "microhil_interfaces/msg/daqc_errors.h"
#include "microhil_interfaces/msg/daqc_setup.h"
#include "microhil_interfaces/msg/daqc_state.h"
#include "rcl/rcl.h"
#include "rcl/init_options.h"
#include "rclc/executor.h"
#include "rclc/rclc.h"
#include "rmw_microros/init_options.h"
#include "rmw_microros/rmw_microros.h"

#define DAQC_ROS_TASK_STACK_SIZE 6144U
#define DAQC_ROS_TASK_PRIORITY 4U
#define DAQC_XRCE_CLIENT_KEY 0x4D48494CUL

typedef struct {
    daqc_control_t *control;
    rclc_support_t support;
    rcl_node_t node;
    rcl_subscription_t setup_subscription;
    rcl_publisher_t state_publisher;
    rcl_publisher_t errors_publisher;
    rclc_executor_t executor;
    microhil_interfaces__msg__DaqcSetup setup_message;
    atomic_bool state_publication_pending;
    bool support_initialized;
    bool node_initialized;
    bool setup_subscription_initialized;
    bool state_publisher_initialized;
    bool errors_publisher_initialized;
    bool executor_initialized;
    bool started;
} daqc_ros_t;

static daqc_ros_t g_ros;

static void IgnoreRclStatus(rcl_ret_t status) { (void)status; }

static void ResetRos(void) {
    if (g_ros.executor_initialized) IgnoreRclStatus(rclc_executor_fini(&g_ros.executor));
    if (g_ros.errors_publisher_initialized) IgnoreRclStatus(rcl_publisher_fini(&g_ros.errors_publisher, &g_ros.node));
    if (g_ros.state_publisher_initialized) IgnoreRclStatus(rcl_publisher_fini(&g_ros.state_publisher, &g_ros.node));
    if (g_ros.setup_subscription_initialized) IgnoreRclStatus(rcl_subscription_fini(&g_ros.setup_subscription, &g_ros.node));
    if (g_ros.node_initialized) IgnoreRclStatus(rcl_node_fini(&g_ros.node));
    if (g_ros.support_initialized) IgnoreRclStatus(rclc_support_fini(&g_ros.support));
    g_ros = (daqc_ros_t){0};
}

static void PublishErrors(uint8_t ros_error, uint8_t communication_error, uint8_t profile_error,
                          uint8_t adc_configuration_error, uint8_t pwm_configuration_error) {
    if (!g_ros.started) return;
    const microhil_interfaces__msg__DaqcErrors message = {
        .source_is_daqc = 1U,
        .ros_error = ros_error,
        .communication_error = communication_error,
        .fmu_error = 0U,
        .profile_error = profile_error,
        .adc_configuration_error = adc_configuration_error,
        .pwm_configuration_error = pwm_configuration_error,
        .timeout_error = 0U,
        .invalid_data_error = 0U,
    };
    const rcl_ret_t publish_status = rcl_publish(&g_ros.errors_publisher, &message, NULL);
    (void)publish_status;
}

static void PublishState(void) {
    if (!g_ros.started) return;
    const microhil_interfaces__msg__DaqcState message = {
        .state = DaqcControlState(g_ros.control),
        .profile_id = DAQC_PROFILE_ESP32,
        .profile_applied = DaqcControlProfileApplied(g_ros.control) ? 1U : 0U,
        .configuration_applied = DaqcControlConfigurationApplied(g_ros.control) ? 1U : 0U,
    };
    if (rcl_publish(&g_ros.state_publisher, &message, NULL) != RCL_RET_OK) PublishErrors(1U, 0U, 0U, 0U, 0U);
}

void DaqcRosRequestStatePublication(void) {
    if (g_ros.started) atomic_store_explicit(&g_ros.state_publication_pending, true, memory_order_release);
}

void DaqcRosPublishCommunicationError(void) {
    PublishErrors(0U, 1U, 0U, 0U, 0U);
}

static bool IsAdcConfigurationValid(const microhil_interfaces__msg__DaqcSetup *setup) {
    if (setup->adc_resolution_bits < 9U || setup->adc_resolution_bits > 12U) return false;
    for (size_t index = 0U; index < 6U; ++index) if (setup->adc_attenuation[index] > 3U) return false;
    return true;
}

static bool IsPwmConfigurationValid(const microhil_interfaces__msg__DaqcSetup *setup) {
    for (size_t index = 0U; index < 2U; ++index) {
        if (setup->pwm_frequency_hz[index] == 0U || setup->pwm_resolution_bits[index] == 0U || setup->pwm_resolution_bits[index] > 20U) return false;
    }
    return true;
}

static void SetupCallback(const void *message) {
    const microhil_interfaces__msg__DaqcSetup *setup = message;
    if (!setup || setup->apply_configuration > 1U) {
        PublishErrors(0U, 1U, 0U, 0U, 0U);
        return;
    }
    const bool adc_valid = IsAdcConfigurationValid(setup);
    const bool pwm_valid = IsPwmConfigurationValid(setup);
    if (setup->apply_configuration && (!adc_valid || !pwm_valid)) {
        PublishErrors(0U, 0U, 0U, adc_valid ? 0U : 1U, pwm_valid ? 0U : 1U);
        return;
    }
    const daqc_configuration_t configuration = {
        .adc_resolution_bits = setup->adc_resolution_bits,
        .adc_attenuation = {setup->adc_attenuation[0], setup->adc_attenuation[1], setup->adc_attenuation[2],
                            setup->adc_attenuation[3], setup->adc_attenuation[4], setup->adc_attenuation[5]},
        .pwm_frequency_hz = {setup->pwm_frequency_hz[0], setup->pwm_frequency_hz[1]},
        .pwm_resolution_bits = {setup->pwm_resolution_bits[0], setup->pwm_resolution_bits[1]},
    };
    const daqc_control_status_t status = DaqcControlApplySetup(g_ros.control, setup->command, setup->profile_id,
                                                                setup->apply_configuration != 0U, &configuration);
    if (status == DAQC_CONTROL_OK) {
        PublishState();
    } else if (status == DAQC_CONTROL_PROFILE) {
        PublishErrors(0U, 0U, 1U, 0U, 0U);
    } else if (status == DAQC_CONTROL_CONFIGURATION) {
        PublishErrors(0U, 0U, 1U, 0U, 0U);
    } else {
        PublishErrors(0U, 1U, 0U, 0U, 0U);
    }
}

static void RosTask(void *argument) {
    (void)argument;
    while (true) {
        if (rclc_executor_spin_some(&g_ros.executor, RCL_MS_TO_NS(10U)) != RCL_RET_OK) PublishErrors(1U, 0U, 0U, 0U, 0U);
        if (atomic_exchange_explicit(&g_ros.state_publication_pending, false, memory_order_acq_rel)) PublishState();
        vTaskDelay(pdMS_TO_TICKS(1U));
    }
}

bool DaqcRosStart(daqc_control_t *control) {
    if (!control || g_ros.started) return false;
#if !defined(RMW_UXRCE_TRANSPORT_CUSTOM)
    return false;
#endif
    if (rmw_uros_set_custom_transport(false, NULL, DaqcTransportOpen, DaqcTransportClose,
                                      DaqcTransportWrite, DaqcTransportRead) != RMW_RET_OK) return false;
    rcl_allocator_t allocator = rcl_get_default_allocator();
    rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
    if (rcl_init_options_init(&init_options, allocator) != RCL_RET_OK) return false;
    rmw_init_options_t *rmw_options = rcl_init_options_get_rmw_init_options(&init_options);
    if (!rmw_options || rmw_uros_options_set_client_key(DAQC_XRCE_CLIENT_KEY, rmw_options) != RMW_RET_OK) {
        if (rcl_init_options_fini(&init_options) != RCL_RET_OK) return false;
        return false;
    }
    ResetRos();
    g_ros = (daqc_ros_t){.control = control, .node = rcl_get_zero_initialized_node(), .setup_subscription = rcl_get_zero_initialized_subscription(),
                          .state_publisher = rcl_get_zero_initialized_publisher(), .errors_publisher = rcl_get_zero_initialized_publisher(),
                          .executor = rclc_executor_get_zero_initialized_executor()};
    atomic_init(&g_ros.state_publication_pending, false);
    const rcl_ret_t support_status = rclc_support_init_with_options(&g_ros.support, 0, NULL, &init_options, &allocator);
    const rcl_ret_t finalize_status = rcl_init_options_fini(&init_options);
    if (support_status != RCL_RET_OK || finalize_status != RCL_RET_OK) return false;
    g_ros.support_initialized = true;
    if (rclc_node_init_default(&g_ros.node, "microhil_daqc", "", &g_ros.support) != RCL_RET_OK) goto error;
    g_ros.node_initialized = true;
    if (rclc_subscription_init_default(&g_ros.setup_subscription, &g_ros.node,
                                       ROSIDL_GET_MSG_TYPE_SUPPORT(microhil_interfaces, msg, DaqcSetup), "/daqc_setup") != RCL_RET_OK) goto error;
    g_ros.setup_subscription_initialized = true;
    if (rclc_publisher_init_default(&g_ros.state_publisher, &g_ros.node,
                                    ROSIDL_GET_MSG_TYPE_SUPPORT(microhil_interfaces, msg, DaqcState), "/daqc_state") != RCL_RET_OK) goto error;
    g_ros.state_publisher_initialized = true;
    if (rclc_publisher_init_default(&g_ros.errors_publisher, &g_ros.node,
                                    ROSIDL_GET_MSG_TYPE_SUPPORT(microhil_interfaces, msg, DaqcErrors), "/daqc_errors") != RCL_RET_OK) goto error;
    g_ros.errors_publisher_initialized = true;
    if (rclc_executor_init(&g_ros.executor, &g_ros.support.context, 1U, &allocator) != RCL_RET_OK) goto error;
    g_ros.executor_initialized = true;
    if (rclc_executor_add_subscription(&g_ros.executor, &g_ros.setup_subscription, &g_ros.setup_message, SetupCallback, ON_NEW_DATA) != RCL_RET_OK) goto error;
    g_ros.started = true;
    atomic_store_explicit(&g_ros.state_publication_pending, true, memory_order_release);
    if (xTaskCreatePinnedToCore(RosTask, "daqc_ros", DAQC_ROS_TASK_STACK_SIZE, NULL,
                                DAQC_ROS_TASK_PRIORITY, NULL, 0) == pdPASS) return true;
error:
    ResetRos();
    return false;
}
