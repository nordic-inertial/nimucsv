#pragma once

#include <stdbool.h>
#include "types.h"
#include "can_ids.h"

#define ANYBOX 255

typedef enum {
    MessageTypeAccelerometer,
    MessageTypeGyroscope,
    MessageTypeStatus,
} nimu_message_type_e;

typedef struct {
    vec3f_t vector;
    bool valid;
} imu_data_t;

typedef struct {
    uint32_t box_id;
    nimu_message_type_e type;
    union {
        imu_data_t data;
        msg_imu_status_t status;
    };
} nimu_message_t;

typedef void (*on_block_reception_complete_callback)(double timestamp, const nimu_message_t message[], uint32_t len);

void nimulib_init(double data_rate_hz, on_block_reception_complete_callback callback);
bool nimulib_subscribe(uint32_t box_id, uint32_t can_id);
void nimulib_feed(uint32_t can_id, uint64_t data);