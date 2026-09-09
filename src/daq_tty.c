#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "daq_tty.h"

#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

static bool BaudToSpeed(unsigned int baud_rate, speed_t *speed) {
    switch (baud_rate) {
        case 9600U: *speed = B9600; return true;
        case 19200U: *speed = B19200; return true;
        case 38400U: *speed = B38400; return true;
        case 57600U: *speed = B57600; return true;
        case 115200U: *speed = B115200; return true;
        default: return false;
    }
}

static daq_tty_status_t WaitForEvent(int file_descriptor, short events, uint32_t timeout_ms) {
    if (timeout_ms > 5U) return DAQ_TTY_STATUS_INVALID_ARGUMENT;
    struct pollfd descriptor = {.fd = file_descriptor, .events = events};
    int result;
    do result = poll(&descriptor, 1U, (int)timeout_ms); while (result < 0 && errno == EINTR);
    if (result == 0) return DAQ_TTY_STATUS_TIMEOUT;
    if (result < 0 || (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))) return DAQ_TTY_STATUS_IO_FAILED;
    return DAQ_TTY_STATUS_OK;
}

daq_tty_status_t DaqTtyOpen(daq_tty_t *tty, const char *path, unsigned int baud_rate) {
    speed_t speed;
    if (!tty || !path || !path[0]) return DAQ_TTY_STATUS_INVALID_ARGUMENT;
    if (!BaudToSpeed(baud_rate, &speed)) return DAQ_TTY_STATUS_UNSUPPORTED_BAUD;
    *tty = (daq_tty_t){.file_descriptor = -1};
    const int file_descriptor = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (file_descriptor < 0) return DAQ_TTY_STATUS_OPEN_FAILED;

    struct termios settings;
    if (tcgetattr(file_descriptor, &settings) != 0) {
        (void)close(file_descriptor);
        return DAQ_TTY_STATUS_CONFIGURE_FAILED;
    }
    cfmakeraw(&settings);
    settings.c_cflag |= (tcflag_t)(CLOCAL | CREAD | CS8);
    settings.c_cflag &= (tcflag_t)~(PARENB | CSTOPB);
#ifdef CRTSCTS
    settings.c_cflag &= (tcflag_t)~CRTSCTS;
#endif
    settings.c_iflag &= (tcflag_t)~(IXON | IXOFF | IXANY);
    settings.c_cc[VMIN] = 0;
    settings.c_cc[VTIME] = 0;
    if (cfsetispeed(&settings, speed) != 0 || cfsetospeed(&settings, speed) != 0 || tcsetattr(file_descriptor, TCSANOW, &settings) != 0) {
        (void)close(file_descriptor);
        return DAQ_TTY_STATUS_CONFIGURE_FAILED;
    }
    *tty = (daq_tty_t){.file_descriptor = file_descriptor, .baud_rate = baud_rate};
    return DAQ_TTY_STATUS_OK;
}

void DaqTtyClose(daq_tty_t *tty) {
    if (!tty || tty->file_descriptor < 0) return;
    (void)close(tty->file_descriptor);
    *tty = (daq_tty_t){.file_descriptor = -1};
}

daq_tty_status_t DaqTtyRead(daq_tty_t *tty, uint8_t *buffer, size_t capacity, uint32_t timeout_ms, size_t *received) {
    if (!tty || tty->file_descriptor < 0 || !buffer || capacity == 0U || !received) return DAQ_TTY_STATUS_INVALID_ARGUMENT;
    *received = 0U;
    const daq_tty_status_t wait_status = WaitForEvent(tty->file_descriptor, POLLIN, timeout_ms);
    if (wait_status != DAQ_TTY_STATUS_OK) return wait_status;
    const ssize_t result = read(tty->file_descriptor, buffer, capacity);
    if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return DAQ_TTY_STATUS_TIMEOUT;
    if (result <= 0) return DAQ_TTY_STATUS_IO_FAILED;
    *received = (size_t)result;
    return DAQ_TTY_STATUS_OK;
}

daq_tty_status_t DaqTtyWrite(daq_tty_t *tty, const uint8_t *buffer, size_t size, uint32_t timeout_ms, size_t *written) {
    if (!tty || tty->file_descriptor < 0 || (size > 0U && !buffer) || !written) return DAQ_TTY_STATUS_INVALID_ARGUMENT;
    *written = 0U;
    while (*written < size) {
        const daq_tty_status_t wait_status = WaitForEvent(tty->file_descriptor, POLLOUT, timeout_ms);
        if (wait_status != DAQ_TTY_STATUS_OK) return wait_status;
        const ssize_t result = write(tty->file_descriptor, buffer + *written, size - *written);
        if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        if (result <= 0) return DAQ_TTY_STATUS_IO_FAILED;
        *written += (size_t)result;
    }
    return DAQ_TTY_STATUS_OK;
}
