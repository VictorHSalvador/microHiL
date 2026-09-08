#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log_converter.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void RequireStatus(log_converter_status_t actual, log_converter_status_t expected, const char *message) {
    if (actual != expected) {
        fprintf(stderr, "%s: got %s\n", message, LogConverterStatusString(actual));
        exit(EXIT_FAILURE);
    }
}

static void MakeTemporaryPath(char path[static 32]) {
    memcpy(path, "/tmp/mhil-log-XXXXXX", sizeof("/tmp/mhil-log-XXXXXX"));
    const int file_descriptor = mkstemp(path);

    Require(file_descriptor >= 0, "could not create temporary file");
    Require(close(file_descriptor) == 0, "could not close temporary file");
}

static void WriteTextFile(const char *path, const char *text) {
    FILE *file = fopen(path, "wb");

    Require(file != NULL, "could not open text fixture");
    Require(fputs(text, file) != EOF, "could not write text fixture");
    Require(fclose(file) == 0, "could not close text fixture");
}

static char *ReadTextFile(const char *path) {
    FILE *file = fopen(path, "rb");
    long size;
    char *text;

    Require(file != NULL, "could not open text result");
    Require(fseek(file, 0L, SEEK_END) == 0, "could not seek text result");
    size = ftell(file);
    Require(size >= 0L, "could not size text result");
    Require(fseek(file, 0L, SEEK_SET) == 0, "could not rewind text result");
    text = malloc((size_t)size + 1U);
    Require(text != NULL, "could not allocate text result");
    Require(fread(text, 1U, (size_t)size, file) == (size_t)size, "could not read text result");
    text[size] = '\0';
    Require(fclose(file) == 0, "could not close text result");
    return text;
}

static log_descriptor_t MakeDescriptor(void) {
    log_descriptor_t descriptor = {0};

    for (uint32_t index = 0U; index < LOG_FORMAT_SHA256_SIZE; ++index) descriptor.fmu_sha256[index] = (uint8_t)(index + 1U);
    descriptor.t_start_s = 0.0;
    descriptor.step_size_s = 0.01;
    descriptor.output_count = 4U;
    descriptor.outputs[0] = (log_output_t){.xml_index = 1U, .value_reference = 11U, .type = LOG_VALUE_REAL, .name = "real"};
    descriptor.outputs[1] = (log_output_t){.xml_index = 10U, .value_reference = 22U, .type = LOG_VALUE_INTEGER, .name = "integer"};
    descriptor.outputs[2] = (log_output_t){.xml_index = 20U, .value_reference = 33U, .type = LOG_VALUE_ENUMERATION, .name = "enumeration"};
    descriptor.outputs[3] = (log_output_t){.xml_index = 30U, .value_reference = 44U, .type = LOG_VALUE_BOOLEAN, .name = "boolean"};
    return descriptor;
}

static log_sample_t MakeSample(uint64_t sequence, double sim_time_s) {
    log_sample_t sample = {0};

    sample.sequence = sequence;
    sample.sim_time_s = sim_time_s;
    sample.values[0].real_value = 1.25;
    sample.values[1].discrete_value = -2;
    sample.values[2].discrete_value = 7;
    sample.values[3].boolean_value = 1U;
    return sample;
}

static void WriteLog(const char *path, const log_descriptor_t *descriptor, const log_sample_t *samples, size_t sample_count, bool append_truncated_tail) {
    FILE *file = fopen(path, "wb");

    Require(file != NULL, "could not open binary fixture");
    Require(LogFormatWriteHeader(file, descriptor) == LOG_FORMAT_STATUS_OK, "could not write binary header");
    for (size_t index = 0U; index < sample_count; ++index) {
        Require(LogFormatWriteRecord(file, descriptor, &samples[index]) == LOG_FORMAT_STATUS_OK, "could not write binary sample");
    }
    if (append_truncated_tail) Require(fputc(0, file) != EOF, "could not append truncated tail");
    Require(fclose(file) == 0, "could not close binary fixture");
}

static void RequireSentinel(const char *path) {
    char *text = ReadTextFile(path);

    Require(strcmp(text, "sentinel") == 0, "existing destination was changed");
    free(text);
}

/* @spec:AC-022 */
static void TestRunningIsRejectedBeforeDestination(void) {
    char binary_path[32];
    char csv_path[32];
    log_converter_result_t result;

    MakeTemporaryPath(binary_path);
    MakeTemporaryPath(csv_path);
    WriteTextFile(csv_path, "sentinel");
    RequireStatus(LogConverterConvert(LOG_CONVERTER_EXECUTION_RUNNING, binary_path, csv_path, NULL, &result), LOG_CONVERTER_STATUS_RUNNING,
                  "running conversion was accepted");
    RequireSentinel(csv_path);
    Require(result.message[0] != '\0', "running rejection has no message");
    Require(unlink(binary_path) == 0 && unlink(csv_path) == 0, "could not remove running fixtures");
}

static void RequireSchemaMismatch(const log_descriptor_t *actual, const log_descriptor_t *expected, log_converter_field_t field, uint32_t index) {
    char binary_path[32];
    char csv_path[32];
    log_converter_result_t result;

    MakeTemporaryPath(binary_path);
    MakeTemporaryPath(csv_path);
    WriteLog(binary_path, actual, NULL, 0U, false);
    WriteTextFile(csv_path, "sentinel");
    RequireStatus(LogConverterConvert(LOG_CONVERTER_EXECUTION_CLOSED, binary_path, csv_path, expected, &result),
                  LOG_CONVERTER_STATUS_SCHEMA_MISMATCH, "incompatible descriptor was accepted");
    Require(result.field == field && result.output_index == index, "schema mismatch did not identify its first field and index");
    Require(result.message[0] != '\0' && result.message[LOG_CONVERTER_MESSAGE_SIZE - 1U] == '\0', "schema mismatch message is not bounded");
    RequireSentinel(csv_path);
    Require(unlink(binary_path) == 0 && unlink(csv_path) == 0, "could not remove schema fixtures");
}

/* @spec:AC-023 */
static void TestSchemaMismatchReportsFirstField(void) {
    const log_descriptor_t actual = MakeDescriptor();
    log_descriptor_t expected;

    expected = actual;
    expected.fmu_sha256[0] ^= UINT8_C(0xff);
    RequireSchemaMismatch(&actual, &expected, LOG_CONVERTER_FIELD_SHA256, UINT32_MAX);

    expected = actual;
    expected.output_count = 3U;
    RequireSchemaMismatch(&actual, &expected, LOG_CONVERTER_FIELD_OUTPUT_COUNT, UINT32_MAX);

    expected = actual;
    expected.outputs[1].xml_index = 11U;
    RequireSchemaMismatch(&actual, &expected, LOG_CONVERTER_FIELD_XML_INDEX, 1U);

    expected = actual;
    expected.outputs[1].value_reference = 99U;
    RequireSchemaMismatch(&actual, &expected, LOG_CONVERTER_FIELD_VALUE_REFERENCE, 1U);

    expected = actual;
    expected.outputs[1].type = LOG_VALUE_ENUMERATION;
    RequireSchemaMismatch(&actual, &expected, LOG_CONVERTER_FIELD_TYPE, 1U);

    expected = actual;
    memcpy(expected.outputs[1].name, "renamed", sizeof("renamed"));
    RequireSchemaMismatch(&actual, &expected, LOG_CONVERTER_FIELD_NAME, 1U);
}

/* @spec:AC-024 */
static void TestCsvPreservesTypesQualityAndEscaping(void) {
    log_descriptor_t descriptor = MakeDescriptor();
    log_sample_t samples[2];
    char binary_path[32];
    char csv_path[32];
    char *csv;
    log_converter_result_t result;

    memcpy(descriptor.outputs[0].name, "real, \"quoted\"", sizeof("real, \"quoted\""));
    samples[0] = MakeSample(7U, 0.5);
    samples[1] = MakeSample(8U, 0.75);
    samples[1].quality_bitmap[0] = UINT8_C(0x0b);
    samples[1].values[0].real_value = NAN;
    samples[1].values[1].discrete_value = 999;
    samples[1].values[2].discrete_value = 9;
    samples[1].values[3].boolean_value = 1U;
    MakeTemporaryPath(binary_path);
    MakeTemporaryPath(csv_path);
    WriteLog(binary_path, &descriptor, samples, 2U, false);
    RequireStatus(LogConverterConvert(LOG_CONVERTER_EXECUTION_CLOSED, binary_path, csv_path, &descriptor, &result), LOG_CONVERTER_STATUS_OK,
                  "complete conversion failed");
    Require(result.exported_records == 2U && result.missing_sequences == 0U && !result.truncated, "complete result metrics are incorrect");
    csv = ReadTextFile(csv_path);
    Require(strstr(csv, "\"real, \"\"quoted\"\"_value\",\"real, \"\"quoted\"\"_valid\"") != NULL, "CSV header did not escape output name");
    Require(strstr(csv, "7,0.5,1.25,1,-2,1,7,1,1,1") != NULL, "CSV did not preserve valid output types");
    Require(strstr(csv, "8,0.75,NaN,0,0,0,9,1,0,0") != NULL, "CSV did not preserve invalid quality representation");
    free(csv);
    Require(unlink(binary_path) == 0 && unlink(csv_path) == 0, "could not remove CSV fixtures");
}

/* @spec:AC-024 */
static void TestSequenceGapsAndStructuralErrors(void) {
    log_descriptor_t descriptor = MakeDescriptor();
    log_sample_t samples[2];
    char binary_path[32];
    char csv_path[32];
    log_converter_result_t result;

    descriptor.output_count = 1U;
    samples[0] = MakeSample(2U, 0.02);
    samples[1] = MakeSample(5U, 0.05);
    MakeTemporaryPath(binary_path);
    MakeTemporaryPath(csv_path);
    WriteLog(binary_path, &descriptor, samples, 2U, false);
    RequireStatus(LogConverterConvert(LOG_CONVERTER_EXECUTION_CLOSED, binary_path, csv_path, &descriptor, &result), LOG_CONVERTER_STATUS_OK,
                  "conversion with sequence gap failed");
    Require(result.missing_sequences == 2U, "sequence gap count is incorrect");
    Require(unlink(binary_path) == 0 && unlink(csv_path) == 0, "could not remove gap fixtures");

    samples[0] = MakeSample(5U, 0.05);
    samples[1] = MakeSample(5U, 0.06);
    MakeTemporaryPath(binary_path);
    MakeTemporaryPath(csv_path);
    WriteLog(binary_path, &descriptor, samples, 2U, false);
    WriteTextFile(csv_path, "sentinel");
    RequireStatus(LogConverterConvert(LOG_CONVERTER_EXECUTION_CLOSED, binary_path, csv_path, &descriptor, &result),
                  LOG_CONVERTER_STATUS_STRUCTURAL, "non-increasing sequence was accepted");
    Require(result.field == LOG_CONVERTER_FIELD_SEQUENCE && result.record_index == 1U, "sequence error did not identify the offending record");
    RequireSentinel(csv_path);
    Require(unlink(binary_path) == 0 && unlink(csv_path) == 0, "could not remove sequence fixtures");
}

/* @spec:AC-024 */
static void TestTruncatedTailPublishesPartialCsv(void) {
    log_descriptor_t descriptor = MakeDescriptor();
    log_sample_t sample = MakeSample(3U, 0.03);
    char binary_path[32];
    char csv_path[32];
    char *csv;
    log_converter_result_t result;

    descriptor.output_count = 1U;
    MakeTemporaryPath(binary_path);
    MakeTemporaryPath(csv_path);
    WriteLog(binary_path, &descriptor, &sample, 1U, true);
    RequireStatus(LogConverterConvert(LOG_CONVERTER_EXECUTION_CLOSED, binary_path, csv_path, &descriptor, &result), LOG_CONVERTER_STATUS_PARTIAL,
                  "truncated tail was reported as integral");
    Require(result.truncated && result.exported_records == 1U, "partial result is incorrect");
    csv = ReadTextFile(csv_path);
    Require(strstr(csv, "3,0.029999999999999999,1.25,1") != NULL, "partial CSV omitted complete record");
    Require(strstr(csv, "\n0,") == NULL, "partial CSV interpreted the truncated tail as a record");
    free(csv);
    Require(unlink(binary_path) == 0 && unlink(csv_path) == 0, "could not remove truncated fixtures");
}

/* @spec:AC-023 */
static void TestHeaderErrorPreservesDestination(void) {
    const log_descriptor_t descriptor = MakeDescriptor();
    char binary_path[32];
    char csv_path[32];
    log_converter_result_t result;

    MakeTemporaryPath(binary_path);
    MakeTemporaryPath(csv_path);
    WriteTextFile(binary_path, "not a binary log");
    WriteTextFile(csv_path, "sentinel");
    RequireStatus(LogConverterConvert(LOG_CONVERTER_EXECUTION_CLOSED, binary_path, csv_path, &descriptor, &result), LOG_CONVERTER_STATUS_HEADER,
                  "malformed log header was accepted");
    RequireSentinel(csv_path);
    Require(unlink(binary_path) == 0 && unlink(csv_path) == 0, "could not remove header fixtures");
}

int main(int argc, char **argv) {
    if (argc != 2) return EXIT_FAILURE;
    if (strcmp(argv[1], "running") == 0) TestRunningIsRejectedBeforeDestination();
    else if (strcmp(argv[1], "schema") == 0) {
        TestSchemaMismatchReportsFirstField();
        TestHeaderErrorPreservesDestination();
    } else if (strcmp(argv[1], "csv") == 0) TestCsvPreservesTypesQualityAndEscaping();
    else if (strcmp(argv[1], "sequences") == 0) TestSequenceGapsAndStructuralErrors();
    else if (strcmp(argv[1], "truncated") == 0) TestTruncatedTailPublishesPartialCsv();
    else return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
