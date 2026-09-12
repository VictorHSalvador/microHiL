#define _XOPEN_SOURCE 700

#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "daq_serial_service.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static int OpenPseudoTerminal(char *path, size_t path_capacity) {
    const int master = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC | O_NONBLOCK);
    Require(master >= 0, "could not open pseudo-terminal master");
    Require(grantpt(master) == 0, "could not grant pseudo-terminal");
    Require(unlockpt(master) == 0, "could not unlock pseudo-terminal");
    const char *slave_path = ptsname(master);
    Require(slave_path != NULL && strlen(slave_path) + 1U <= path_capacity, "could not resolve pseudo-terminal slave");
    memcpy(path, slave_path, strlen(slave_path) + 1U);
    return master;
}

static bool WriteAll(int descriptor, const uint8_t *bytes, size_t size) {
    size_t written = 0U;
    while (written < size) {
        const ssize_t result = write(descriptor, bytes + written, size - written);
        if (result <= 0) return false;
        written += (size_t)result;
    }
    return true;
}

static bool ReadFrame(int descriptor, uint8_t *bytes, size_t capacity, size_t *received) {
    struct pollfd descriptor_state = {.fd = descriptor, .events = POLLIN};
    if (poll(&descriptor_state, 1U, 250) <= 0) return false;
    const ssize_t result = read(descriptor, bytes, capacity);
    if (result <= 0) return false;
    *received = (size_t)result;
    return true;
}

static void SleepMillisecond(void) {
    const struct timespec interval = {.tv_nsec = 1000000L};
    (void)nanosleep(&interval, NULL);
}

int main(void) {
    const input_channel_descriptor_t inputs[] = {
        {.input_name = "input", .value_reference = 1U, .type = NUMERIC_REAL, .initial_value = {.real_value = 0.0}, .initial_value_valid = true}
    };
    const daq_field_t fields[] = {
        {.gpio = 34U, .function = DAQ_CHANNEL_AI, .wire_type = DAQ_WIRE_FLOAT32, .fmu_type = NUMERIC_REAL,
         .offset = 0U, .width = 4U, .scale = 1.0, .offset_value = 0.0, .fmu_index = 0U}
    };
    daq_schema_t schema;
    input_state_t input_state;
    input_step_t input_step;
    daq_coordinator_t coordinator;
    daq_serial_service_t service;
    char path[128];
    const int master = OpenPseudoTerminal(path, sizeof(path));
    const daq_coordinator_config_t coordinator_config = {
        .acquisition_schema = &schema,
        .input_state = &input_state,
        .xrce_mtu = DAQ_PROTOCOL_XRCE_MTU
    };
    const daq_serial_service_config_t service_config = {
        .coordinator = &coordinator,
        .device_path = path,
        .baud_rate = 152000U
    };
    uint8_t incoming[10];
    uint8_t outgoing[32];
    size_t incoming_size = 0U;
    size_t outgoing_size = 0U;

    Require(DaqSchemaBuild(&schema, fields, 1U) == DAQ_SCHEMA_OK, "could not build schema");
    Require(InputStateInit(&input_state, inputs, 1U, true) == INPUT_STATE_STATUS_OK, "could not initialize input state");
    Require(DaqCoordinatorInit(&coordinator, &coordinator_config) == DAQ_COORDINATOR_OK, "could not initialize coordinator");
    Require(DaqSerialServiceStart(&service, &service_config) == DAQ_SERIAL_SERVICE_OK, "could not start serial service");

    incoming_size += DaqProtocolEncodeConfig(incoming + incoming_size, sizeof(incoming) - incoming_size, DAQ_PROTOCOL_COMMAND_ENABLE, true,
                                             DAQ_PROTOCOL_COMMAND_ENABLE);
    incoming_size += DaqProtocolEncodeConfig(incoming + incoming_size, sizeof(incoming) - incoming_size, DAQ_PROTOCOL_COMMAND_STREAMING, true,
                                             DAQ_PROTOCOL_COMMAND_STREAMING);
    Require(incoming_size == sizeof(incoming) && WriteAll(master, incoming, incoming_size), "could not send DAQC state confirmations");

    bool streaming = false;
    for (unsigned int attempt = 0U; attempt < 50U; ++attempt) {
        const uint8_t payload[] = {0x5aU};
        if (DaqCoordinatorPublishOutput(&coordinator, 7U, payload, sizeof(payload)) == DAQ_COORDINATOR_OK) {
            streaming = true;
            break;
        }
        SleepMillisecond();
    }
    Require(streaming, "serial service did not apply incoming STREAMING state");
    Require(ReadFrame(master, outgoing, sizeof(outgoing), &outgoing_size), "serial service did not emit pending DATA");
    Require(outgoing_size == 6U && outgoing[0] == DAQ_PROTOCOL_SYNC_FIRST && outgoing[1] == DAQ_PROTOCOL_SYNC_SECOND &&
            outgoing[2] == DAQ_PROTOCOL_MID_DATA && outgoing[3] == 7U && outgoing[4] == 0U && outgoing[5] == 0x5aU,
            "serial service emitted an unexpected DATA frame");
    const uint8_t acquisition_payload[] = {0U, 0U, 0x20U, 0x40U};
    Require(DaqProtocolEncodeData(incoming, sizeof(incoming), 9U, acquisition_payload, sizeof(acquisition_payload)) == 9U &&
                WriteAll(master, incoming, 9U),
            "could not send acquisition DATA");
    Require(ReadFrame(master, outgoing, sizeof(outgoing), &outgoing_size) && outgoing_size == 5U &&
                outgoing[2] == DAQ_PROTOCOL_MID_READ_ACK && outgoing[3] == 9U && outgoing[4] == 0U,
            "serial service did not prioritize READ_ACK after acquisition DATA");
    Require(InputStatePrepareStep(&input_state, &input_step) == INPUT_STATE_STATUS_OK && fabs(input_step.values[0].real_value - 2.5) < 1e-12,
            "serial service did not publish acquisition DATA to the FMU snapshot");
    Require(atomic_load_explicit(&service.received_bytes, memory_order_relaxed) == incoming_size + 9U,
            "serial service did not account for received bytes");
    daq_serial_service_stats_t stats;
    Require(DaqSerialServiceGetStats(&service, &stats) && stats.received_bytes == incoming_size + 9U &&
                stats.transmitted_frames >= 2U,
            "serial service did not expose the communication statistics");
    Require(atomic_load_explicit(&service.transmitted_frames, memory_order_relaxed) == 2U, "serial service did not account for transmitted frames");

    DaqSerialServiceStop(&service);
    DaqCoordinatorDestroy(&coordinator);
    InputStateDestroy(&input_state);
    Require(close(master) == 0, "could not close pseudo-terminal master");
    return EXIT_SUCCESS;
}
