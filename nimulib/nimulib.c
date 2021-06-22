#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "nimulib.h"


#define MAX_SUBSCRIPTIONS 64

typedef nimu_message_t (*parseFunc)(void* payload);
typedef struct {
    uint32_t msg_id;
    parseFunc parser;
    nimu_message_type_e type;
} message_parser_t;

static nimu_message_t fixpoint_parser(void* payload);
static nimu_message_t fixpoint_with_combined_valid_flag_parser(void* payload);
static nimu_message_t imu_status_parser(void* payload);

uint64_t msgs[MAX_SUBSCRIPTIONS];
uint8_t boxids[MAX_SUBSCRIPTIONS];
uint8_t subs[MAX_SUBSCRIPTIONS];
uint8_t valids[MAX_SUBSCRIPTIONS];
nimu_message_t nimu_messages[MAX_SUBSCRIPTIONS];

on_block_reception_complete_callback on_block_rx_complete = NULL;
int n_subs;
double cur_ts = 0.0;
double ts_incr = 0.0;

const message_parser_t parsers[] = 
{
    {MSG_IMU0_A, fixpoint_parser, MessageTypeAccelerometer},
    {MSG_IMU0_G, fixpoint_with_combined_valid_flag_parser, MessageTypeGyroscope},
    {MSG_IMU0_STATUS, imu_status_parser, MessageTypeStatus},
    {MSG_IMU1_A, fixpoint_parser, MessageTypeAccelerometer},
    {MSG_IMU1_G, fixpoint_with_combined_valid_flag_parser, MessageTypeGyroscope},
    {MSG_IMU1_STATUS, imu_status_parser, MessageTypeStatus},
};

static const message_parser_t* find_message_parser(uint32_t msg_id)
{
    for (int i = 0; i < sizeof(parsers) / sizeof(parsers[0]); i++)
    {
        if (parsers[i].msg_id == msg_id)
        {
            return &parsers[i];
        }
    }

    return NULL;
}

static nimu_message_t fixpoint_parser(void* payload)
{
    nimu_message_t message;
    fixpoint_rate_t fpr = *((fixpoint_rate_t*)payload);
    message.data.vector = fixpoint_rate_to_vec3f(fpr);
    message.data.valid = true;
    return message;
}

static nimu_message_t fixpoint_with_combined_valid_flag_parser(void* payload)
{
    nimu_message_t message;
    fixpoint_rate_t fpr = *((fixpoint_rate_t*)payload);
    message.data.vector = fixpoint_rate_to_vec3f(fpr);
    message.data.valid = fpr.flag_x && fpr.flag_y && fpr.flag_z;
    return message;
}

static nimu_message_t imu_status_parser(void* payload)
{    
    nimu_message_t message;
    message.status = *((msg_imu_status_t*)payload);
    return message;
}

void nimulib_init(double data_rate_hz, on_block_reception_complete_callback callback)
{
    cur_ts = 0.0;
    ts_incr = 1.0 / data_rate_hz;
    on_block_rx_complete = callback;
}

bool nimulib_subscribe(uint32_t box_id, uint32_t msg_id)
{
    if (n_subs >= MAX_SUBSCRIPTIONS || find_message_parser(msg_id) == NULL)
    {
        return false;
    }
    boxids[n_subs] = box_id;
    subs[n_subs] = msg_id;
    n_subs++;
    return true;
}

void nimulib_feed(uint32_t can_id, uint64_t data)
{
    int msgid = MSGID(can_id);
    int boxid = BOXID(can_id);

    assert(msgid >= 0 && msgid <= 255);

    for(int i=0; i<n_subs; i++)
    {
        if(subs[i] == msgid && (boxids[i] == ANYBOX || boxids[i] == boxid))
        {
            msgs[i] = data;
            valids[i] = 1;
        }
    }

    int all_valid = 1;
    for(int i=0; i<n_subs; i++)
    {
        if(valids[i] == 0)
            all_valid = 0;
    }

    if(all_valid)
    {
        if(msgid == subs[n_subs-1])
        {
            for(int i=0; i<n_subs; i++)
            {
                const message_parser_t* parser = find_message_parser(subs[i]);
                nimu_messages[i] = parser->parser((void*)&msgs[i]);
                nimu_messages[i].box_id = subs[i];
                nimu_messages[i].type = parser->type;
            }

            for(int i=0; i<n_subs; i++)
                valids[i] = 0;

            if (on_block_rx_complete)
                on_block_rx_complete(cur_ts, nimu_messages, n_subs);

            cur_ts += ts_incr;
        }
    }
}