#include <stdio.h>
#include <stdlib.h>

#include "daq_tty.h"

static void Require(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    daq_tty_t tty = {.file_descriptor = -1};
    Require(DaqTtyOpen(&tty, "/dev/null", 12345U) == DAQ_TTY_STATUS_UNSUPPORTED_BAUD, "unsupported baud rate was accepted");
    Require(DaqTtyOpen(&tty, "", 115200U) == DAQ_TTY_STATUS_INVALID_ARGUMENT, "empty device path was accepted");
    Require(DaqTtyRead(&tty, NULL, 0U, 5U, NULL) == DAQ_TTY_STATUS_INVALID_ARGUMENT, "invalid read arguments were accepted");
    Require(DaqTtyWrite(&tty, NULL, 1U, 6U, NULL) == DAQ_TTY_STATUS_INVALID_ARGUMENT, "invalid write arguments were accepted");
    return EXIT_SUCCESS;
}
