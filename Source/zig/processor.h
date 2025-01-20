#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "stdint.h"
#ifdef __cplusplus
extern "C" {
#endif

enum GainChannel {
    LowGainChannel,
    HiGainChannel,
};

typedef struct Processor Processor;

Processor* processor_init(uint32_t num_channels);
void processor_deinit(Processor*);
void processor_prepare(Processor*, double sample_rate, uint32_t num_samples, uint32_t num_channels);
void processor_reset(Processor*);
void processor_process(Processor*, float* const* buffer, uint32_t num_samples, uint32_t num_channels);
void processor_process64(Processor*, double** buffer, uint32_t num_samples, uint32_t num_channels);
void processor_param_change(Processor*, const char *id, float value);

// TODO Other API ideas
// save processor state
// void processor_save_state(Processor *)
// load processor state
// void processor_load_state(Processor *)

#ifdef __cplusplus
}
#endif

#endif
