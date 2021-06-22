#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include "wrapcan.h"

static int verbose = 0;
static int cansoc;
char device_name[1024] = "can0";

void block_can_send(uint32_t id, int dlc, void* data)
{
    assert(dlc >= 0 && dlc <= 8);

    if(verbose >= 3)
    {
        printf("block_can_send() id=%x dlc=%d data=", id, dlc);
        for(int i=0; i<dlc; i++)
            printf("%02x ", ((uint8_t*)data)[i]);
        printf("\n");
    }

    struct can_frame frame;
    frame.can_id = id;
    frame.can_dlc = dlc;
    if(dlc > 0) memcpy(frame.data, data, dlc);

    if(write(cansoc, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame))
    {
        fprintf(stderr, "write() to CAN device failed\n");
        exit(1);
    }

    usleep(1000);
}

int block_can_expect(uint32_t id, int dlc, void* data)
{
    if(verbose >=3)
        printf("Trying to read, expecting id=0x%x, dlc=%d\n", id, dlc);

    size_t nbytes;
    struct can_frame frame;
    nbytes = read(cansoc, &frame, sizeof(struct can_frame));

    if(verbose >=3)
    {
        printf("Read %d bytes, id=0x%x, dlc=%d, data=", (int)nbytes, frame.can_id, frame.can_dlc);
        for(int i=0; i<frame.can_dlc; i++)
            printf("%02x ", frame.data[i]);
        printf("\n");
    }

    if(nbytes < sizeof(struct can_frame))
    {
        fprintf(stderr, "Error: read() returned partial can_frame. This failure mode is not supported. nbytes=%d, expected %d\n", (int)nbytes, (int)sizeof(struct can_frame));
        exit(1);
    }

    if(frame.can_dlc != dlc)
    {
        if(verbose >= 1)
            printf("Error: block_can_expect(): received unexpected data length code\n");
        return -1;
    }

    if(frame.can_id != id)
    {
        if(verbose >= 1)
            printf("Error: block_can_expect(): received unexpected id\n");
        return -2;
    }

    assert(frame.can_dlc <= 8);

    if(frame.can_dlc > 0)
        memcpy(data, frame.data, frame.can_dlc);

    return 0;
}

int block_can_read(uint32_t *id, int *dlc, void* data)
{
    if(verbose >=3)
        printf("Trying to read\n");

    size_t nbytes;
    struct can_frame frame;
    nbytes = read(cansoc, &frame, sizeof(struct can_frame));

    if(verbose >=3)
    {
        printf("Read %d bytes, id=0x%x, dlc=%d, data=", (int)nbytes, frame.can_id, frame.can_dlc);
        for(int i=0; i<frame.can_dlc; i++)
            printf("%02x ", frame.data[i]);
        printf("\n");
    }

    if(nbytes < sizeof(struct can_frame))
    {
        fprintf(stderr, "Error: read() returned partial can_frame. This failure mode is not supported. nbytes=%d, expected %d\n", (int)nbytes, (int)sizeof(struct can_frame));
        exit(1);
    }

    *id = frame.can_id;
    *dlc = frame.can_dlc;

    assert(frame.can_dlc <= 8);

    if(frame.can_dlc > 0)
        memcpy(data, frame.data, frame.can_dlc);

    return 0;
}

int timeout_can_read(uint32_t *id, int *dlc, void* data, int millisec)
{
    if(verbose >=3)
        printf("Trying to read with timeout\n");


    int usec = millisec*1000;
    int sec = usec/1000000;
    usec -= sec*1000000;

    struct timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = usec;
    setsockopt(cansoc, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    int nbytes;
    struct can_frame frame;
    nbytes = read(cansoc, &frame, sizeof(struct can_frame));

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    setsockopt(cansoc, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    if(nbytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
        if(verbose >=3)
            printf("Read timed out, no packets\n");

        return 1;
    }
    else if(nbytes < 0)
    {
        printf("ERROR: read() returned error %d (%s)\n", errno, strerror(errno));
        exit(1);
    }
    else if(nbytes == 0)
    {
        printf("ERROR: timeout_can_read read() returned 0 indicate end of stream\n");
        exit(1);
    }

    if(nbytes != sizeof(struct can_frame))
    {
        fprintf(stderr, "Error: read() returned partial can_frame. This failure mode is not supported. nbytes=%d, expected %d\n", (int)nbytes, (int)sizeof(struct can_frame));
        exit(1);
    }
    
    if(verbose >=3)
    {
        printf("Read %d bytes, id=0x%x, dlc=%d, data=", (int)nbytes, frame.can_id, frame.can_dlc);
        for(int i=0; i<frame.can_dlc; i++)
            printf("%02x ", frame.data[i]);
        printf("\n");
    }


    *id = frame.can_id;
    *dlc = frame.can_dlc;

    assert(frame.can_dlc <= 8);

    if(frame.can_dlc > 0)
        memcpy(data, frame.data, frame.can_dlc);

    return 0;
}


void block_can_init(uint32_t flt_id, uint32_t flt_mask, int verbosity)
{
    verbose = verbosity;
    
    if(verbose >= 3)
        printf("block_can_init(): create socket\n");
    cansoc = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(cansoc < 0)
    {
        fprintf(stderr, "error opening CAN device: socket() failed. Seems like a problem with your operating system.\n");
        exit(1);
    }

    struct sockaddr_can addr;
    struct ifreq ifr;

    strcpy(ifr.ifr_name, device_name);
    if(ioctl(cansoc, SIOCGIFINDEX, &ifr) < 0)
    {
        fprintf(stderr, "Error opening CAN device %s. Check connections and verify correct device name.\n", device_name);
        exit(1);
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    //fcntl(cansoc, F_SETFL, O_NONBLOCK);

    if(verbose >= 3)
        printf("block_can_init(): bind socket\n");

    if(bind(cansoc, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        fprintf(stderr, "error opening CAN device: bind() failed. Seems like a problem with your operating system.\n");
        exit(1);
    }

    struct can_filter filters[1];
    filters[0].can_id   = flt_id;
    filters[0].can_mask = flt_mask;

    if(verbose >= 3)
        printf("block_can_init(): apply filter\n");

    setsockopt(cansoc, SOL_CAN_RAW, CAN_RAW_FILTER, &filters, sizeof filters);
}

void block_can_add_filter(uint32_t id, uint32_t mask)
{

}


void block_can_deinit()
{
    close(cansoc);
}

