#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log_format.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void RequireStatus(log_format_status_t actual, log_format_status_t expected, const char *message) {
    if (actual != expected) {
        fprintf(stderr, "%s: got %s\n", message, LogFormatStatusString(actual));
        exit(EXIT_FAILURE);
    }
}

static log_descriptor_t MakeDescriptor(void) {
    log_descriptor_t descriptor = {0};

    for (size_t index = 0U; index < LOG_FORMAT_SHA256_SIZE; ++index) descriptor.fmu_sha256[index] = (uint8_t)index;
    descriptor.t_start_s = -1.25;
    descriptor.step_size_s = 0.01;
    descriptor.output_count = 4U;
    descriptor.outputs[0] = (log_output_t){.xml_index = 2U, .value_reference = 19U, .type = LOG_VALUE_REAL, .name = "temperatura \xC2\xB0" "C"};
    descriptor.outputs[1] = (log_output_t){.xml_index = 9U, .value_reference = 19U, .type = LOG_VALUE_INTEGER, .name = "contagem"};
    descriptor.outputs[2] = (log_output_t){.xml_index = 12U, .value_reference = 21U, .type = LOG_VALUE_BOOLEAN, .name = "ativo"};
    descriptor.outputs[3] = (log_output_t){.xml_index = 15U, .value_reference = 22U, .type = LOG_VALUE_ENUMERATION, .name = "modo"};
    return descriptor;
}

static log_sample_t MakeSample(void) {
    log_sample_t sample = {0};

    sample.sequence = UINT64_C(0x0102030405060708);
    sample.sim_time_s = 2.5;
    sample.values[0].real_value = 3.125;
    sample.values[1].discrete_value = -42;
    sample.values[2].boolean_value = 1U;
    sample.values[3].discrete_value = 7;
    return sample;
}

/* @spec:AC-015 */
static void TestHeader(void) {
    const log_descriptor_t descriptor = MakeDescriptor();
    log_descriptor_t decoded;
    uint32_t header_size;
    uint8_t prefix[14];
    FILE *file = tmpfile();

    Require(file != NULL, "could not create temporary file");
    RequireStatus(LogFormatHeaderSize(&descriptor, &header_size), LOG_FORMAT_STATUS_OK, "valid header size was rejected");
    Require(header_size == 66U + (11U + 15U) + (11U + 8U) + (11U + 5U) + (11U + 4U), "unexpected header size");
    RequireStatus(LogFormatWriteHeader(file, &descriptor), LOG_FORMAT_STATUS_OK, "header write failed");
    Require(fflush(file) == 0, "header flush failed");
    Require(fseek(file, 0L, SEEK_SET) == 0, "header rewind failed");
    Require(fread(prefix, 1U, sizeof(prefix), file) == sizeof(prefix), "could not inspect header prefix");
    Require(memcmp(prefix, "MHILLOG1", 8U) == 0, "header magic is not serialized field by field");
    Require(prefix[8] == 1U && prefix[9] == 0U, "version is not little-endian");
    Require(prefix[10] == (uint8_t)header_size && prefix[11] == (uint8_t)(header_size >> 8U), "header size is not little-endian");
    Require(fseek(file, 0L, SEEK_SET) == 0, "header rewind for decode failed");
    RequireStatus(LogFormatReadHeader(file, &decoded), LOG_FORMAT_STATUS_OK, "header read failed");
    Require(decoded.output_count == descriptor.output_count, "decoded output count differs");
    Require(decoded.outputs[0].xml_index == 2U && decoded.outputs[3].xml_index == 15U, "XML order was not preserved");
    Require(decoded.outputs[0].value_reference == decoded.outputs[1].value_reference, "duplicate value references were changed");
    Require(decoded.outputs[0].type == LOG_VALUE_REAL && decoded.outputs[1].type == LOG_VALUE_INTEGER &&
            decoded.outputs[2].type == LOG_VALUE_BOOLEAN && decoded.outputs[3].type == LOG_VALUE_ENUMERATION,
            "header value types were not preserved");
    Require(strcmp(decoded.outputs[0].name, "temperatura \xC2\xB0" "C") == 0 && strcmp(decoded.outputs[2].name, "ativo") == 0, "UTF-8 names were not preserved");
    Require(memcmp(decoded.fmu_sha256, descriptor.fmu_sha256, LOG_FORMAT_SHA256_SIZE) == 0, "FMU hash differs");
    fclose(file);
}

/* @spec:AC-016 */
static void TestRecords(void) {
    const log_descriptor_t descriptor = MakeDescriptor();
    log_sample_t expected = MakeSample();
    log_sample_t decoded;
    uint8_t raw_record[34];
    FILE *file = tmpfile();

    Require(file != NULL, "could not create temporary file");
    RequireStatus(LogFormatWriteRecord(file, &descriptor, &expected), LOG_FORMAT_STATUS_OK, "typed record write failed");
    Require(fflush(file) == 0, "typed record flush failed");
    Require(fseek(file, 0L, SEEK_SET) == 0, "typed record raw rewind failed");
    Require(fread(raw_record, 1U, sizeof(raw_record), file) == sizeof(raw_record), "could not inspect typed record");
    Require(raw_record[0] == 8U && raw_record[7] == 1U && raw_record[8] == 0U && raw_record[14] == 4U && raw_record[15] == UINT8_C(0x40),
            "sequence or simulation time is not little-endian");
    Require(raw_record[16] == 0U && raw_record[25] == UINT8_C(0xd6) && raw_record[26] == UINT8_MAX && raw_record[28] == UINT8_MAX && raw_record[29] == 1U && raw_record[30] == 7U,
            "typed record values are not little-endian");
    Require(fseek(file, 0L, SEEK_END) == 0, "typed record seek to end failed");
    expected.quality_bitmap[0] = UINT8_C(0x0b);
    expected.values[0].real_value = INFINITY;
    expected.values[1].discrete_value = INT64_MAX;
    expected.values[3].discrete_value = 99;
    RequireStatus(LogFormatWriteRecord(file, &descriptor, &expected), LOG_FORMAT_STATUS_OK, "normalizing record write failed");
    Require(fseek(file, 0L, SEEK_SET) == 0, "record rewind failed");
    RequireStatus(LogFormatReadRecord(file, &descriptor, &decoded), LOG_FORMAT_STATUS_OK, "typed record read failed");
    Require(decoded.sequence == UINT64_C(0x0102030405060708) && decoded.sim_time_s == 2.5, "record scalar fields differ");
    Require(decoded.quality_bitmap[0] == 0U, "valid quality bitmap differs");
    Require(decoded.values[0].real_value == 3.125 && decoded.values[1].discrete_value == -42 && decoded.values[2].boolean_value == 1U && decoded.values[3].discrete_value == 7,
            "typed values differ");
    RequireStatus(LogFormatReadRecord(file, &descriptor, &decoded), LOG_FORMAT_STATUS_OK, "normalized record read failed");
    Require((decoded.quality_bitmap[0] & UINT8_C(0x0b)) == UINT8_C(0x0b), "invalid qualities were not retained");
    Require(decoded.values[1].discrete_value == 0 && decoded.values[3].discrete_value == 0 && isnan(decoded.values[0].real_value), "invalid values were not normalized");
    RequireStatus(LogFormatReadRecord(file, &descriptor, &decoded), LOG_FORMAT_STATUS_END_OF_FILE, "complete stream did not end cleanly");
    fclose(file);
}

static void WriteU16(FILE *file, uint16_t value) {
    const uint8_t bytes[2] = {(uint8_t)value, (uint8_t)(value >> 8U)};
    Require(fwrite(bytes, 1U, sizeof(bytes), file) == sizeof(bytes), "could not write malformed input");
}

static void WriteU32(FILE *file, uint32_t value) {
    const uint8_t bytes[4] = {(uint8_t)value, (uint8_t)(value >> 8U), (uint8_t)(value >> 16U), (uint8_t)(value >> 24U)};
    Require(fwrite(bytes, 1U, sizeof(bytes), file) == sizeof(bytes), "could not write malformed input");
}

static void WriteMinimalHeader(FILE *file, uint32_t header_size, uint8_t type, const uint8_t *name, uint16_t name_size) {
    const uint8_t magic[8] = {'M', 'H', 'I', 'L', 'L', 'O', 'G', '1'};
    const uint8_t zero_hash[32] = {0};
    const double one = 1.0;
    uint64_t bits;

    memcpy(&bits, &one, sizeof(bits));
    Require(fwrite(magic, 1U, sizeof(magic), file) == sizeof(magic), "could not write magic");
    WriteU16(file, 1U);
    WriteU32(file, header_size);
    Require(fwrite(zero_hash, 1U, sizeof(zero_hash), file) == sizeof(zero_hash), "could not write hash");
    for (size_t index = 0U; index < 8U; ++index) fputc((int)(uint8_t)(bits >> (index * 8U)), file);
    for (size_t index = 0U; index < 8U; ++index) fputc((int)(uint8_t)(bits >> (index * 8U)), file);
    WriteU32(file, 1U);
    WriteU32(file, 1U);
    WriteU32(file, 2U);
    fputc(type, file);
    WriteU16(file, name_size);
    Require(fwrite(name, 1U, name_size, file) == name_size, "could not write name");
}

/* @spec:AC-017 */
static void TestRejection(void) {
    log_descriptor_t descriptor = MakeDescriptor();
    log_descriptor_t decoded;
    log_sample_t sample = MakeSample();
    const uint8_t valid_name[] = "x";
    const uint8_t invalid_utf8[] = {UINT8_C(0xc0), UINT8_C(0x80)};
    FILE *file = tmpfile();

    Require(file != NULL, "could not create temporary file");
    descriptor.outputs[1].xml_index = descriptor.outputs[0].xml_index;
    RequireStatus(LogFormatWriteHeader(file, &descriptor), LOG_FORMAT_STATUS_ORDER, "unordered XML indices were accepted");
    descriptor = MakeDescriptor();
    descriptor.outputs[0].type = (log_value_type_t)9;
    RequireStatus(LogFormatWriteHeader(file, &descriptor), LOG_FORMAT_STATUS_TYPE, "unknown writer type was accepted");
    descriptor = MakeDescriptor();
    descriptor.output_count = LOG_FORMAT_MAX_OUTPUTS + 1U;
    RequireStatus(LogFormatWriteHeader(file, &descriptor), LOG_FORMAT_STATUS_LIMIT, "excessive output count was accepted");
    descriptor = MakeDescriptor();
    memset(descriptor.outputs[0].name, 'a', sizeof(descriptor.outputs[0].name));
    RequireStatus(LogFormatWriteHeader(file, &descriptor), LOG_FORMAT_STATUS_LIMIT, "unterminated excessive name was accepted");

    WriteMinimalHeader(file, 78U, 9U, valid_name, 1U);
    Require(fseek(file, 0L, SEEK_SET) == 0, "invalid type rewind failed");
    RequireStatus(LogFormatReadHeader(file, &decoded), LOG_FORMAT_STATUS_TYPE, "unknown reader type was accepted");
    Require(freopen(NULL, "w+b", file) != NULL, "could not reset malformed file");
    WriteMinimalHeader(file, UINT32_MAX, 0U, valid_name, 1U);
    Require(fseek(file, 0L, SEEK_SET) == 0, "overflow rewind failed");
    RequireStatus(LogFormatReadHeader(file, &decoded), LOG_FORMAT_STATUS_HEADER_SIZE, "overflowing header size was accepted");
    Require(freopen(NULL, "w+b", file) != NULL, "could not reset malformed file");
    WriteMinimalHeader(file, 79U, 0U, invalid_utf8, sizeof(invalid_utf8));
    Require(fseek(file, 0L, SEEK_SET) == 0, "UTF-8 rewind failed");
    RequireStatus(LogFormatReadHeader(file, &decoded), LOG_FORMAT_STATUS_UTF8, "invalid UTF-8 was accepted");
    Require(freopen(NULL, "w+b", file) != NULL, "could not reset malformed file");
    Require(fwrite("MHIL", 1U, 4U, file) == 4U, "could not write truncated header");
    Require(fseek(file, 0L, SEEK_SET) == 0, "truncated header rewind failed");
    RequireStatus(LogFormatReadHeader(file, &decoded), LOG_FORMAT_STATUS_TRUNCATED, "truncated header was accepted");
    Require(freopen(NULL, "w+b", file) != NULL, "could not reset malformed file");
    descriptor = MakeDescriptor();
    RequireStatus(LogFormatWriteRecord(file, &descriptor, &sample), LOG_FORMAT_STATUS_OK, "record setup failed");
    Require(fflush(file) == 0, "record flush before truncation failed");
    Require(ftruncate(fileno(file), 12) == 0, "could not truncate record");
    Require(fseek(file, 0L, SEEK_SET) == 0, "truncated record rewind failed");
    RequireStatus(LogFormatReadRecord(file, &descriptor, &sample), LOG_FORMAT_STATUS_TRUNCATED, "truncated record was accepted");
    Require(freopen(NULL, "w+b", file) != NULL, "could not reset malformed file");
    RequireStatus(LogFormatWriteRecord(file, &descriptor, &sample), LOG_FORMAT_STATUS_OK, "bitmap record setup failed");
    Require(fflush(file) == 0 && fseek(file, 16L, SEEK_SET) == 0 && fputc(UINT8_C(0xf0), file) != EOF && fflush(file) == 0, "could not corrupt bitmap");
    Require(fseek(file, 0L, SEEK_SET) == 0, "bitmap rewind failed");
    RequireStatus(LogFormatReadRecord(file, &descriptor, &sample), LOG_FORMAT_STATUS_INCONSISTENT, "bitmap excess bits were accepted");
    Require(freopen(NULL, "w+b", file) != NULL, "could not reset malformed file");
    RequireStatus(LogFormatWriteRecord(file, &descriptor, &sample), LOG_FORMAT_STATUS_OK, "Boolean record setup failed");
    Require(fflush(file) == 0 && fseek(file, 29L, SEEK_SET) == 0 && fputc(2, file) != EOF && fflush(file) == 0, "could not corrupt Boolean value");
    Require(fseek(file, 0L, SEEK_SET) == 0, "Boolean rewind failed");
    RequireStatus(LogFormatReadRecord(file, &descriptor, &sample), LOG_FORMAT_STATUS_INCONSISTENT, "Boolean outside {0,1} was accepted");
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) return EXIT_FAILURE;
    if (strcmp(argv[1], "header") == 0) TestHeader();
    else if (strcmp(argv[1], "records") == 0) TestRecords();
    else if (strcmp(argv[1], "rejection") == 0) TestRejection();
    else return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
