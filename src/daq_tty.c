#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "daq_tty.h"

#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <asm/ioctls.h>
#include <asm/termbits.h>

static bool IsSupportedBaud(unsigned int baud_rate) {
    switch (baud_rate) {
        case 9600U:
        case 19200U:
        case 38400U:
        case 57600U:
        case 115200U:
        case 152000U: return true;
        default: return false;
    }
}

static daq_tty_status_t ConfigurePort(int file_descriptor, unsigned int baud_rate) {
    struct termios2 settings;
    if (ioctl(file_descriptor, TCGETS2, &settings) != 0) return DAQ_TTY_STATUS_CONFIGURE_FAILED;

    settings.c_iflag &= (tcflag_t)~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);
    settings.c_oflag &= (tcflag_t)~OPOST;
    settings.c_lflag &= (tcflag_t)~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    settings.c_cflag &= (tcflag_t)~(CBAUD | CSIZE | PARENB | CSTOPB | CRTSCTS);
    // BOTHER preserves the selected non-standard UART rate in the Linux TTY driver.
    settings.c_cflag |= (tcflag_t)(BOTHER | CLOCAL | CREAD | CS8);
    settings.c_cc[VMIN] = 0;
    settings.c_cc[VTIME] = 0;
    settings.c_ispeed = baud_rate;
    settings.c_ospeed = baud_rate;

    if (ioctl(file_descriptor, TCSETS2, &settings) != 0) return DAQ_TTY_STATUS_CONFIGURE_FAILED;
    return DAQ_TTY_STATUS_OK;
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
    if (!tty || !path || !path[0]) return DAQ_TTY_STATUS_INVALID_ARGUMENT;
    if (!IsSupportedBaud(baud_rate)) return DAQ_TTY_STATUS_UNSUPPORTED_BAUD;
    *tty = (daq_tty_t){.file_descriptor = -1};
    const int file_descriptor = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (file_descriptor < 0) return DAQ_TTY_STATUS_OPEN_FAILED;

    if (ConfigurePort(file_descriptor, baud_rate) != DAQ_TTY_STATUS_OK) {
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
