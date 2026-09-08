#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>

#define SHA256_DIGEST_SIZE 32U

typedef enum {
    SHA256_STATUS_OK = 0,
    SHA256_STATUS_INVALID_ARGUMENT,
    SHA256_STATUS_OPEN,
    SHA256_STATUS_READ
} sha256_status_t;

sha256_status_t Sha256File(const char *path, uint8_t digest[SHA256_DIGEST_SIZE]);
const char *Sha256StatusString(sha256_status_t status);

#endif
