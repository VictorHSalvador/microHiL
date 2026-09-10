#define _XOPEN_SOURCE 700

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <asm/ioctls.h>
#include <asm/termbits.h>

#include "daq_tty.h"

static void Require(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static int OpenPseudoTerminal(char *path, size_t path_capacity) {
    const int master = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);
    Require(master >= 0, "could not open pseudo-terminal master");
    Require(grantpt(master) == 0, "could not grant pseudo-terminal");
    Require(unlockpt(master) == 0, "could not unlock pseudo-terminal");
    const char *slave_path = ptsname(master);
    Require(slave_path != NULL, "could not resolve pseudo-terminal slave");
    Require(strlen(slave_path) + 1U <= path_capacity, "pseudo-terminal path is too long");
    memcpy(path, slave_path, strlen(slave_path) + 1U);
    return master;
}

int main(void) {
    daq_tty_t tty = {.file_descriptor = -1};
    Require(DaqTtyOpen(&tty, "/dev/null", 12345U) == DAQ_TTY_STATUS_UNSUPPORTED_BAUD, "unsupported baud rate was accepted");
    Require(DaqTtyOpen(&tty, "", 115200U) == DAQ_TTY_STATUS_INVALID_ARGUMENT, "empty device path was accepted");
    Require(DaqTtyRead(&tty, NULL, 0U, 5U, NULL) == DAQ_TTY_STATUS_INVALID_ARGUMENT, "invalid read arguments were accepted");
    Require(DaqTtyWrite(&tty, NULL, 1U, 6U, NULL) == DAQ_TTY_STATUS_INVALID_ARGUMENT, "invalid write arguments were accepted");

    char path[128];
    const int master = OpenPseudoTerminal(path, sizeof(path));
    Require(DaqTtyOpen(&tty, path, 152000U) == DAQ_TTY_STATUS_OK, "152000 baud rate was not configured");

    struct termios2 settings;
    Require(ioctl(tty.file_descriptor, TCGETS2, &settings) == 0, "could not inspect pseudo-terminal settings");
    Require(settings.c_ispeed == 152000U && settings.c_ospeed == 152000U, "pseudo-terminal did not retain 152000 baud rate");
    Require((settings.c_cflag & CRTSCTS) == 0U, "hardware flow control was enabled");

    DaqTtyClose(&tty);
    Require(close(master) == 0, "could not close pseudo-terminal master");
    return EXIT_SUCCESS;
}
