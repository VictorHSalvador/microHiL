#ifndef DAQ_SERIAL_SERVICE_H
#define DAQ_SERIAL_SERVICE_H

#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <pthread.h>

#include "daq_coordinator.h"
#include "daq_tty.h"

typedef struct {
    daq_coordinator_t *coordinator;
    const char *device_path;
    unsigned int baud_rate;
} daq_serial_service_config_t;

typedef struct {
    daq_tty_t tty;
    daq_coordinator_t *coordinator;
    pthread_t worker;
    atomic_bool running;
    atomic_uint_fast64_t received_bytes;
    atomic_uint_fast64_t transmitted_frames;
    atomic_uint_fast64_t read_timeouts;
    atomic_uint_fast64_t io_failures;
    bool worker_started;
    bool initialized;
} daq_serial_service_t;

typedef enum {
    DAQ_SERIAL_SERVICE_OK = 0,
    DAQ_SERIAL_SERVICE_INVALID_ARGUMENT,
    DAQ_SERIAL_SERVICE_TTY,
    DAQ_SERIAL_SERVICE_THREAD
} daq_serial_service_status_t;

daq_serial_service_status_t DaqSerialServiceStart(daq_serial_service_t *service, const daq_serial_service_config_t *config);
void DaqSerialServiceStop(daq_serial_service_t *service);
bool DaqSerialServiceIsRunning(const daq_serial_service_t *service);
const char *DaqSerialServiceStatusString(daq_serial_service_status_t status);

#endif
