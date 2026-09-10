#include "sha256.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t state[8];
    uint64_t bit_count;
    uint8_t block[64];
    size_t block_size;
} sha256_context_t;

static const uint32_t SHA256_CONSTANTS[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

static uint32_t RotateRight(uint32_t value, uint32_t amount) {
    return (value >> amount) | (value << (32U - amount));
}

static uint32_t Choose(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

static uint32_t Majority(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

static uint32_t Sigma0(uint32_t value) {
    return RotateRight(value, 2U) ^ RotateRight(value, 13U) ^ RotateRight(value, 22U);
}

static uint32_t Sigma1(uint32_t value) {
    return RotateRight(value, 6U) ^ RotateRight(value, 11U) ^ RotateRight(value, 25U);
}

static uint32_t Gamma0(uint32_t value) {
    return RotateRight(value, 7U) ^ RotateRight(value, 18U) ^ (value >> 3U);
}

static uint32_t Gamma1(uint32_t value) {
    return RotateRight(value, 17U) ^ RotateRight(value, 19U) ^ (value >> 10U);
}

static void Transform(sha256_context_t *context, const uint8_t block[64]) {
    uint32_t words[64];
    uint32_t a = context->state[0];
    uint32_t b = context->state[1];
    uint32_t c = context->state[2];
    uint32_t d = context->state[3];
    uint32_t e = context->state[4];
    uint32_t f = context->state[5];
    uint32_t g = context->state[6];
    uint32_t h = context->state[7];
    for (size_t index = 0U; index < 16U; ++index) {
        words[index] = ((uint32_t)block[index * 4U] << 24U) | ((uint32_t)block[index * 4U + 1U] << 16U) |
                       ((uint32_t)block[index * 4U + 2U] << 8U) | block[index * 4U + 3U];
    }
    for (size_t index = 16U; index < 64U; ++index) {
        words[index] = Gamma1(words[index - 2U]) + words[index - 7U] + Gamma0(words[index - 15U]) + words[index - 16U];
    }
    for (size_t index = 0U; index < 64U; ++index) {
        const uint32_t t1 = h + Sigma1(e) + Choose(e, f, g) + SHA256_CONSTANTS[index] + words[index];
        const uint32_t t2 = Sigma0(a) + Majority(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

static void Init(sha256_context_t *context) {
    *context = (sha256_context_t){.state = {0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU, 0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U}};
}

static void Update(sha256_context_t *context, const uint8_t *data, size_t size) {
    context->bit_count += (uint64_t)size * 8U;
    while (size > 0U) {
        const size_t available = sizeof(context->block) - context->block_size;
        const size_t copied = size < available ? size : available;
        memcpy(context->block + context->block_size, data, copied);
        context->block_size += copied;
        data += copied;
        size -= copied;
        if (context->block_size == sizeof(context->block)) {
            Transform(context, context->block);
            context->block_size = 0U;
        }
    }
}

static void Finish(sha256_context_t *context, uint8_t digest[SHA256_DIGEST_SIZE]) {
    context->block[context->block_size++] = 0x80U;
    if (context->block_size > 56U) {
        while (context->block_size < 64U) context->block[context->block_size++] = 0U;
        Transform(context, context->block);
        context->block_size = 0U;
    }
    while (context->block_size < 56U) context->block[context->block_size++] = 0U;
    for (size_t index = 0U; index < 8U; ++index) context->block[63U - index] = (uint8_t)(context->bit_count >> (index * 8U));
    Transform(context, context->block);
    for (size_t index = 0U; index < 8U; ++index) {
        for (size_t byte = 0U; byte < 4U; ++byte) digest[index * 4U + byte] = (uint8_t)(context->state[index] >> (24U - byte * 8U));
    }
}

sha256_status_t Sha256File(const char *path, uint8_t digest[SHA256_DIGEST_SIZE]) {
    FILE *file;
    sha256_context_t context;
    uint8_t buffer[4096];
    if (!path || !digest) return SHA256_STATUS_INVALID_ARGUMENT;
    file = fopen(path, "rb");
    if (!file) return SHA256_STATUS_OPEN;
    Init(&context);
    for (;;) {
        const size_t count = fread(buffer, 1U, sizeof(buffer), file);
        if (count > 0U) Update(&context, buffer, count);
        if (count < sizeof(buffer)) {
            if (ferror(file)) {
                (void)fclose(file);
                return SHA256_STATUS_READ;
            }
            break;
        }
    }
    if (fclose(file) != 0) return SHA256_STATUS_READ;
    Finish(&context, digest);
    return SHA256_STATUS_OK;
}

const char *Sha256StatusString(sha256_status_t status) {
    switch (status) {
        case SHA256_STATUS_OK: return "ok";
        case SHA256_STATUS_INVALID_ARGUMENT: return "invalid argument";
        case SHA256_STATUS_OPEN: return "could not open file";
        case SHA256_STATUS_READ: return "could not read file";
        default: return "unknown SHA-256 error";
    }
}
