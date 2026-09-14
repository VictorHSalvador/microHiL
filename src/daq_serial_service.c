/* Implementação do módulo daq serial service. */
#include "daq_serial_service.h"

#include <string.h>

#define DAQ_SERIAL_TRANSFER_TIMEOUT_MS 5U
#define DAQ_SERIAL_BUFFER_SIZE (DAQ_PROTOCOL_DATA_PREFIX_SIZE + DAQ_PROTOCOL_MAX_DATA_PAYLOAD)

static void *RunService(void *context) {
    daq_serial_service_t *service = context;
    uint8_t receive_buffer[DAQ_SERIAL_BUFFER_SIZE];
    uint8_t transmit_buffer[DAQ_SERIAL_BUFFER_SIZE];

    while (atomic_load_explicit(&service->running, memory_order_acquire)) {
        size_t received = 0U;
        const daq_tty_status_t read_status = DaqTtyRead(&service->tty, receive_buffer, sizeof(receive_buffer), DAQ_SERIAL_TRANSFER_TIMEOUT_MS, &received);
        if (read_status == DAQ_TTY_STATUS_OK) {
            atomic_fetch_add_explicit(&service->received_bytes, received, memory_order_relaxed);
            if (DaqCoordinatorReceive(service->coordinator, receive_buffer, received) != DAQ_COORDINATOR_OK) {
                atomic_fetch_add_explicit(&service->io_failures, 1U, memory_order_relaxed);
                break;
            }
        } else if (read_status == DAQ_TTY_STATUS_TIMEOUT) {
            atomic_fetch_add_explicit(&service->read_timeouts, 1U, memory_order_relaxed);
        } else {
            atomic_fetch_add_explicit(&service->io_failures, 1U, memory_order_relaxed);
            break;
        }

        daq_transmit_frame_t frame;
        const daq_coordinator_status_t coordinator_status = DaqCoordinatorTakeTransmit(service->coordinator, transmit_buffer, sizeof(transmit_buffer), &frame);
        if (coordinator_status != DAQ_COORDINATOR_OK) {
            atomic_fetch_add_explicit(&service->io_failures, 1U, memory_order_relaxed);
            break;
        }
        if (frame.kind == DAQ_TRANSMIT_NONE) continue;

        size_t written = 0U;
        if (DaqTtyWrite(&service->tty, transmit_buffer, frame.size, DAQ_SERIAL_TRANSFER_TIMEOUT_MS, &written) != DAQ_TTY_STATUS_OK || written != frame.size) {
            atomic_fetch_add_explicit(&service->io_failures, 1U, memory_order_relaxed);
            break;
        }
        atomic_fetch_add_explicit(&service->transmitted_frames, 1U, memory_order_relaxed);
    }

    atomic_store_explicit(&service->running, false, memory_order_release);
    return NULL;
}

daq_serial_service_status_t DaqSerialServiceStart(daq_serial_service_t *service, const daq_serial_service_config_t *config) {
    if (!service || !config || !config->coordinator || !config->device_path || !config->device_path[0] || !config->coordinator->initialized) {
        return DAQ_SERIAL_SERVICE_INVALID_ARGUMENT;
    }
    *service = (daq_serial_service_t){.tty = {.file_descriptor = -1}};
    if (DaqTtyOpen(&service->tty, config->device_path, config->baud_rate) != DAQ_TTY_STATUS_OK) {
        return DAQ_SERIAL_SERVICE_TTY;
    }
    if (config->reset_daqc_before_start && DaqTtyResetDaqc(&service->tty) != DAQ_TTY_STATUS_OK) {
        DaqTtyClose(&service->tty);
        return DAQ_SERIAL_SERVICE_TTY;
    }
    /* Discard UART bytes produced before this service became the sole owner. */
    if (DaqTtyFlushInput(&service->tty) != DAQ_TTY_STATUS_OK) {
        DaqTtyClose(&service->tty);
        return DAQ_SERIAL_SERVICE_TTY;
    }
    service->coordinator = config->coordinator;
    atomic_init(&service->running, true);
    atomic_init(&service->received_bytes, 0U);
    atomic_init(&service->transmitted_frames, 0U);
    atomic_init(&service->read_timeouts, 0U);
    atomic_init(&service->io_failures, 0U);
    if (pthread_create(&service->worker, NULL, RunService, service) != 0) {
        DaqTtyClose(&service->tty);
        *service = (daq_serial_service_t){.tty = {.file_descriptor = -1}};
        return DAQ_SERIAL_SERVICE_THREAD;
    }
    service->worker_started = true;
    service->initialized = true;
    return DAQ_SERIAL_SERVICE_OK;
}

void DaqSerialServiceStop(daq_serial_service_t *service) {
    if (!service || !service->initialized) return;
    atomic_store_explicit(&service->running, false, memory_order_release);
    if (service->worker_started) (void)pthread_join(service->worker, NULL);
    DaqTtyClose(&service->tty);
    *service = (daq_serial_service_t){.tty = {.file_descriptor = -1}};
}

bool DaqSerialServiceIsRunning(const daq_serial_service_t *service) {
    return service && service->initialized && atomic_load_explicit(&service->running, memory_order_acquire);
}

bool DaqSerialServiceGetStats(const daq_serial_service_t *service, daq_serial_service_stats_t *stats) {
    if (!service || !service->initialized || !stats) return false;
    *stats = (daq_serial_service_stats_t){
        .received_bytes = atomic_load_explicit(&service->received_bytes, memory_order_relaxed),
        .transmitted_frames = atomic_load_explicit(&service->transmitted_frames, memory_order_relaxed),
        .read_timeouts = atomic_load_explicit(&service->read_timeouts, memory_order_relaxed),
        .io_failures = atomic_load_explicit(&service->io_failures, memory_order_relaxed),
    };
    return true;
}

const char *DaqSerialServiceStatusString(daq_serial_service_status_t status) {
    switch (status) {
        case DAQ_SERIAL_SERVICE_OK: return "ok";
        case DAQ_SERIAL_SERVICE_INVALID_ARGUMENT: return "invalid argument";
        case DAQ_SERIAL_SERVICE_TTY: return "could not open or configure TTY";
        case DAQ_SERIAL_SERVICE_THREAD: return "could not start serial worker";
        default: return "unknown serial service status";
    }
}
