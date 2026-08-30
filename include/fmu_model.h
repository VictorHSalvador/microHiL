#ifndef FMU_MODEL_H
#define FMU_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <fmilib.h>
#include "common.h"

typedef struct {
    jm_callbacks callbacks;
    fmi_import_context_t *context;
    fmi2_import_t *fmu;
    const char *tmp_path;
    bool dll_created;
    bool instantiated;
    bool initialized;
    char model_name[OUTPUT_NAME_LEN];
} FmuModel;

void fmu_model_init(FmuModel *model);
int fmu_model_load(FmuModel *model, const char *fmu_path);
void fmu_model_unload(FmuModel *model);
size_t fmu_model_list_numeric_outputs(FmuModel *model, OutputVariable *outputs, size_t capacity);
int fmu_model_initialize_cosimulation(FmuModel *model, double start_time_s, double stop_time_s);
int fmu_model_do_step(FmuModel *model, double current_time_s, double step_size_s);
int fmu_model_read_outputs(FmuModel *model, const OutputVariable *outputs, size_t count, double *values);
void fmu_model_terminate(FmuModel *model);
const char *fmu_model_name(const FmuModel *model);

#endif
