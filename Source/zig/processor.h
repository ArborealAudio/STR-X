#ifndef STR_X_H
#define STR_X_H

#include "stdint.h"
#include "stdatomic.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct Processor Processor;

Processor* processor_init(uint32_t num_channels);
void processor_deinit(Processor*);
void processor_prepare(Processor*, double sample_rate, uint32_t num_samples, uint32_t num_channels);
void processor_reset(Processor*);
void processor_process(Processor*, float* const* buffer, uint32_t num_samples, uint32_t num_channels);
void processor_process64(Processor*, double** buffer, uint32_t num_samples, uint32_t num_channels);
void processor_param_change(Processor*, const char *id, float value);

#ifdef __cplusplus
}
#endif

#endif
