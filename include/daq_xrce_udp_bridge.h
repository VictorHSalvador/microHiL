#ifndef DAQ_XRCE_UDP_BRIDGE_H
#define DAQ_XRCE_UDP_BRIDGE_H

#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <pthread.h>

#include "daq_coordinator.h"

typedef struct {
    daq_coordinator_t *coordinator;
    uint16_t agent_port;
} daq_xrce_udp_bridge_config_t;

typedef struct {
    daq_coordinator_t *coordinator;
    int socket_fd;
    pthread_t receiver;
    atomic_bool running;
    atomic_uint_fast64_t forwarded_to_agent;
    atomic_uint_fast64_t forwarded_to_daqc;
    atomic_uint_fast64_t rejected_datagrams;
    bool receiver_started;
    bool initialized;
} daq_xrce_udp_bridge_t;

typedef enum {
    DAQ_XRCE_UDP_BRIDGE_OK = 0,
    DAQ_XRCE_UDP_BRIDGE_INVALID_ARGUMENT,
    DAQ_XRCE_UDP_BRIDGE_SOCKET,
    DAQ_XRCE_UDP_BRIDGE_THREAD
} daq_xrce_udp_bridge_status_t;

daq_xrce_udp_bridge_status_t DaqXrceUdpBridgeStart(daq_xrce_udp_bridge_t *bridge, const daq_xrce_udp_bridge_config_t *config);
void DaqXrceUdpBridgeStop(daq_xrce_udp_bridge_t *bridge);
bool DaqXrceUdpBridgeReceiveFromDaqc(const uint8_t *payload, size_t payload_size, void *context);
uint16_t DaqXrceUdpBridgeLocalPort(const daq_xrce_udp_bridge_t *bridge);
const char *DaqXrceUdpBridgeStatusString(daq_xrce_udp_bridge_status_t status);

#endif
