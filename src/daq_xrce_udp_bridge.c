#include "daq_xrce_udp_bridge.h"

#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "daq_protocol.h"

#define DAQ_XRCE_UDP_POLL_TIMEOUT_MS 5

static void *ReceiveFromAgent(void *context) {
    daq_xrce_udp_bridge_t *bridge = context;
    uint8_t payload[DAQ_PROTOCOL_XRCE_MTU + 1U];
    while (atomic_load_explicit(&bridge->running, memory_order_acquire)) {
        struct pollfd descriptor = {.fd = bridge->socket_fd, .events = POLLIN};
        const int poll_status = poll(&descriptor, 1U, DAQ_XRCE_UDP_POLL_TIMEOUT_MS);
        if (poll_status == 0 || (poll_status < 0 && errno == EINTR)) continue;
        if (poll_status < 0 || (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) break;
        if ((descriptor.revents & POLLIN) == 0) continue;
        const ssize_t received = recv(bridge->socket_fd, payload, sizeof(payload), MSG_TRUNC);
        if (received <= 0) {
            if (received < 0 && errno == EINTR) continue;
            break;
        }
        if ((size_t)received > DAQ_PROTOCOL_XRCE_MTU ||
            DaqCoordinatorQueueXrce(bridge->coordinator, payload, (size_t)received) != DAQ_COORDINATOR_OK) {
            atomic_fetch_add_explicit(&bridge->rejected_datagrams, 1U, memory_order_relaxed);
            continue;
        }
        atomic_fetch_add_explicit(&bridge->forwarded_to_daqc, 1U, memory_order_relaxed);
    }
    atomic_store_explicit(&bridge->running, false, memory_order_release);
    return NULL;
}

daq_xrce_udp_bridge_status_t DaqXrceUdpBridgeStart(daq_xrce_udp_bridge_t *bridge, const daq_xrce_udp_bridge_config_t *config) {
    if (!bridge || !config || !config->coordinator || !config->coordinator->initialized || config->agent_port == 0U) {
        return DAQ_XRCE_UDP_BRIDGE_INVALID_ARGUMENT;
    }
    *bridge = (daq_xrce_udp_bridge_t){.socket_fd = -1};
    const int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) return DAQ_XRCE_UDP_BRIDGE_SOCKET;

    const struct sockaddr_in local_address = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK), .sin_port = 0U};
    const struct sockaddr_in agent_address = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK), .sin_port = htons(config->agent_port)};
    if (bind(socket_fd, (const struct sockaddr *)&local_address, sizeof(local_address)) != 0 ||
        connect(socket_fd, (const struct sockaddr *)&agent_address, sizeof(agent_address)) != 0) {
        (void)close(socket_fd);
        return DAQ_XRCE_UDP_BRIDGE_SOCKET;
    }
    bridge->coordinator = config->coordinator;
    bridge->socket_fd = socket_fd;
    atomic_init(&bridge->running, true);
    atomic_init(&bridge->forwarded_to_agent, 0U);
    atomic_init(&bridge->forwarded_to_daqc, 0U);
    atomic_init(&bridge->rejected_datagrams, 0U);
    bridge->initialized = true;
    if (pthread_create(&bridge->receiver, NULL, ReceiveFromAgent, bridge) != 0) {
        (void)close(socket_fd);
        *bridge = (daq_xrce_udp_bridge_t){.socket_fd = -1};
        return DAQ_XRCE_UDP_BRIDGE_THREAD;
    }
    bridge->receiver_started = true;
    return DAQ_XRCE_UDP_BRIDGE_OK;
}

void DaqXrceUdpBridgeStop(daq_xrce_udp_bridge_t *bridge) {
    if (!bridge || !bridge->initialized) return;
    atomic_store_explicit(&bridge->running, false, memory_order_release);
    if (bridge->socket_fd >= 0) (void)shutdown(bridge->socket_fd, SHUT_RDWR);
    if (bridge->receiver_started) (void)pthread_join(bridge->receiver, NULL);
    if (bridge->socket_fd >= 0) (void)close(bridge->socket_fd);
    *bridge = (daq_xrce_udp_bridge_t){.socket_fd = -1};
}

bool DaqXrceUdpBridgeReceiveFromDaqc(const uint8_t *payload, size_t payload_size, void *context) {
    daq_xrce_udp_bridge_t *bridge = context;
    if (!bridge || !bridge->initialized || !payload || payload_size == 0U || payload_size > DAQ_PROTOCOL_XRCE_MTU ||
        !atomic_load_explicit(&bridge->running, memory_order_acquire)) {
        return false;
    }
    const ssize_t sent = send(bridge->socket_fd, payload, payload_size, MSG_DONTWAIT);
    if (sent != (ssize_t)payload_size) {
        atomic_fetch_add_explicit(&bridge->rejected_datagrams, 1U, memory_order_relaxed);
        return false;
    }
    atomic_fetch_add_explicit(&bridge->forwarded_to_agent, 1U, memory_order_relaxed);
    return true;
}

uint16_t DaqXrceUdpBridgeLocalPort(const daq_xrce_udp_bridge_t *bridge) {
    if (!bridge || !bridge->initialized || bridge->socket_fd < 0) return 0U;
    struct sockaddr_in address = {0};
    socklen_t length = sizeof(address);
    if (getsockname(bridge->socket_fd, (struct sockaddr *)&address, &length) != 0 || address.sin_family != AF_INET) return 0U;
    return ntohs(address.sin_port);
}

const char *DaqXrceUdpBridgeStatusString(daq_xrce_udp_bridge_status_t status) {
    switch (status) {
        case DAQ_XRCE_UDP_BRIDGE_OK: return "ok";
        case DAQ_XRCE_UDP_BRIDGE_INVALID_ARGUMENT: return "invalid argument";
        case DAQ_XRCE_UDP_BRIDGE_SOCKET: return "could not create the loopback UDP bridge";
        case DAQ_XRCE_UDP_BRIDGE_THREAD: return "could not start the UDP receiver";
        default: return "unknown UDP bridge status";
    }
}
