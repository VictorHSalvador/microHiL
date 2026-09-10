#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "daq_xrce_udp_bridge.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static int BindAgentSocket(uint16_t *port) {
    const int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    Require(socket_fd >= 0, "could not create the fake Agent socket");
    const struct sockaddr_in address = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK), .sin_port = 0U};
    Require(bind(socket_fd, (const struct sockaddr *)&address, sizeof(address)) == 0, "could not bind the fake Agent socket");
    struct sockaddr_in bound_address = {0};
    socklen_t length = sizeof(bound_address);
    Require(getsockname(socket_fd, (struct sockaddr *)&bound_address, &length) == 0, "could not discover the fake Agent port");
    *port = ntohs(bound_address.sin_port);
    return socket_fd;
}

static void ReceiveExactly(int socket_fd, uint8_t *payload, size_t payload_size) {
    struct pollfd descriptor = {.fd = socket_fd, .events = POLLIN};
    Require(poll(&descriptor, 1U, 1000) == 1 && (descriptor.revents & POLLIN) != 0, "UDP datagram was not received");
    Require(recv(socket_fd, payload, payload_size, 0) == (ssize_t)payload_size, "UDP datagram has an unexpected size");
}

int main(void) {
    const daq_field_t fields[] = {{.gpio = 32U, .function = DAQ_CHANNEL_AI, .wire_type = DAQ_WIRE_FLOAT32,
                                   .fmu_type = NUMERIC_REAL, .offset = 4U, .width = 4U, .scale = 1.0}};
    const input_channel_descriptor_t inputs[] = {{.input_name = "input", .type = NUMERIC_REAL, .initial_value = {.real_value = 0.0},
                                                  .initial_value_valid = true}};
    daq_schema_t acquisition_schema;
    input_state_t input_state;
    daq_coordinator_t coordinator;
    Require(DaqSchemaBuildFixedPayload(&acquisition_schema, fields, 1U, 28U) == DAQ_SCHEMA_OK, "could not build acquisition schema");
    Require(InputStateInit(&input_state, inputs, 1U, false) == INPUT_STATE_STATUS_OK, "could not initialize input state");
    const daq_coordinator_config_t coordinator_config = {.acquisition_schema = &acquisition_schema, .input_state = &input_state,
                                                          .xrce_mtu = DAQ_PROTOCOL_XRCE_MTU};
    Require(DaqCoordinatorInit(&coordinator, &coordinator_config) == DAQ_COORDINATOR_OK, "could not initialize coordinator");

    uint16_t agent_port = 0U;
    const int agent_socket = BindAgentSocket(&agent_port);
    daq_xrce_udp_bridge_t bridge;
    const daq_xrce_udp_bridge_config_t bridge_config = {.coordinator = &coordinator, .agent_port = agent_port};
    Require(DaqXrceUdpBridgeStart(&bridge, &bridge_config) == DAQ_XRCE_UDP_BRIDGE_OK, "could not start UDP bridge");

    const uint8_t to_agent[] = {0x11U, 0x22U, 0x33U};
    uint8_t received[3] = {0};
    Require(DaqXrceUdpBridgeReceiveFromDaqc(to_agent, sizeof(to_agent), &bridge), "could not forward DAQC XRCE to Agent");
    ReceiveExactly(agent_socket, received, sizeof(received));
    Require(memcmp(received, to_agent, sizeof(to_agent)) == 0, "Agent received an altered XRCE datagram");

    const uint8_t to_daqc[] = {0x44U, 0x55U};
    const struct sockaddr_in bridge_address = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
                                               .sin_port = htons(DaqXrceUdpBridgeLocalPort(&bridge))};
    Require(bridge_address.sin_port != 0U && sendto(agent_socket, to_daqc, sizeof(to_daqc), 0,
            (const struct sockaddr *)&bridge_address, sizeof(bridge_address)) == (ssize_t)sizeof(to_daqc), "could not reply to bridge");

    daq_transmit_frame_t frame = {0};
    uint8_t encoded[DAQ_PROTOCOL_DATA_PREFIX_SIZE + DAQ_PROTOCOL_MAX_DATA_PAYLOAD] = {0};
    bool received_by_coordinator = false;
    for (unsigned int retry = 0U; retry < 100U; ++retry) {
        Require(DaqCoordinatorTakeTransmit(&coordinator, encoded, sizeof(encoded), &frame) == DAQ_COORDINATOR_OK,
                "coordinator could not inspect outbound mailbox");
        if (frame.kind == DAQ_TRANSMIT_XRCE) {
            received_by_coordinator = true;
            break;
        }
        struct pollfd wait = {.fd = agent_socket, .events = 0};
        (void)poll(&wait, 1U, 1);
    }
    Require(received_by_coordinator && frame.size == 7U && encoded[3] == 2U && encoded[5] == to_daqc[0] && encoded[6] == to_daqc[1],
            "Agent response was not re-encapsulated as MID 04");

    DaqXrceUdpBridgeStop(&bridge);
    (void)close(agent_socket);
    DaqCoordinatorDestroy(&coordinator);
    InputStateDestroy(&input_state);
    return EXIT_SUCCESS;
}
