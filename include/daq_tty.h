#ifndef DAQ_TTY_H
#define DAQ_TTY_H

/* Contrato público do módulo daq tty. */

#include <stddef.h>
#include <stdint.h>

typedef enum {
    DAQ_TTY_STATUS_OK = 0,
    DAQ_TTY_STATUS_INVALID_ARGUMENT,
    DAQ_TTY_STATUS_UNSUPPORTED_BAUD,
    DAQ_TTY_STATUS_OPEN_FAILED,
    DAQ_TTY_STATUS_CONFIGURE_FAILED,
    DAQ_TTY_STATUS_TIMEOUT,
    DAQ_TTY_STATUS_IO_FAILED
} daq_tty_status_t;

typedef struct {
    int file_descriptor;
    unsigned int baud_rate;
} daq_tty_t;

daq_tty_status_t DaqTtyOpen(daq_tty_t *tty, const char *path, unsigned int baud_rate);
void DaqTtyClose(daq_tty_t *tty);
daq_tty_status_t DaqTtyRead(daq_tty_t *tty, uint8_t *buffer, size_t capacity, uint32_t timeout_ms, size_t *received);
daq_tty_status_t DaqTtyWrite(daq_tty_t *tty, const uint8_t *buffer, size_t size, uint32_t timeout_ms, size_t *written);

#endif
