#include <stdio.h>
#include <assert.h>

#include "wrapcan.h"
#include "nimulib.h"

#define IMU_DATARATE_HZ 250  // change depending on your settings, affects timestamps
#define CAN_INTERFACE_VERBOSITY 0  // 0-3, larger values print out more debug messages

void on_block_reception_complete(double timestamp, const nimu_message_t message[], uint32_t len)
{
    printf("%.5f;", timestamp);

    // the order of messages is the same as subscriptions
    for(int i = 0; i < len; i++)
    {
        nimu_message_t msg = message[i];

        switch (msg.type)
        {
            case MessageTypeAccelerometer:
                printf("%10.6f;%10.6f;%10.6f;  ", msg.data.vector.x, msg.data.vector.y, msg.data.vector.z);
                break;
            case MessageTypeGyroscope:
                printf("%1d;%10.6f;%10.6f;%10.6f;   ", msg.data.valid, msg.data.vector.x, msg.data.vector.y, msg.data.vector.z);
                break;
            case MessageTypeStatus:
                printf("%5.1f;   ", (double)msg.status.temp/10.0);
                break;
        }
    }

    printf("\n");
}

int main(int argc, char** argv)
{
    #ifdef PCAN
        // PCAN has range filter
        block_can_init(0x000, 0x3ff, CAN_INTERFACE_VERBOSITY);
    #else
        // Others have id&bitmask filter
        block_can_init(0x000, 0x400, CAN_INTERFACE_VERBOSITY);
    #endif

    // block reception callback will be called once all subscribed CAN frames have been received
    nimulib_init(IMU_DATARATE_HZ, on_block_reception_complete);

    // the order of subscriptions will be preserved on the block reception callback parameters
    nimulib_subscribe(ANYBOX, MSG_IMU0_A);
    nimulib_subscribe(ANYBOX, MSG_IMU0_G);
    nimulib_subscribe(ANYBOX, MSG_IMU0_STATUS);

    // add more if more than one imu is connected to can bus
    printf("timestamp;hax;hay;haz;hvalid;hgx;hgy;hgz;ht;\n");

    while(1)
    {
        uint32_t can_id;
        int dlc;
        uint64_t data;
        int rc = block_can_read(&can_id, &dlc, &data);
        assert(rc == 0);

        // will call the block reception callback once all frames have been received
        nimulib_feed(can_id, data);
    }
}
