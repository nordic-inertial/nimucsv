#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include <inttypes.h>

// Write to the send queue. May block.
void block_can_send(uint32_t id, int dlc, void* data);

// Blocking read from receive queue. Expects the exact given id and dlc. If they do not match,
// fails with negative return value.
int block_can_expect(uint32_t id, int dlc, void* data);

// Blocking read from receive queue. Writes id, dlc and data.
int block_can_read(uint32_t *id, int *dlc, void* data);

// Like block_can_read. Blocks, but stops blocking after timeout
// Returns 1 if timed out due to no packets. id, dlc, data are NOT written to.
// Returns 0 if packet was available. id, dlc, data are written to.
int timeout_can_read(uint32_t *id, int *dlc, void* data, int millisec);



// Filter id and mask works normally for socketcan and kvaser.
// WARNING: PCAN API does not implement mask filters, only range filters
// flt_id is abused as start id, flt_mask as end id, range is inclusive of both ends.
// verbosity controls amount of debug traces. 0 = no prints except errors. 3 = full level of status prints
void block_can_init(uint32_t flt_id, uint32_t flt_mask, int verbosity);


void block_can_deinit();



// Access one of these in your application to set the interface (if necessary; they have defaults which may work)
#ifdef SOCKETCAN
    extern char device_name[1024];
#endif

#ifdef PCAN
    #include <windows.h>
    #include "PCANBasic.h"
    extern TPCANHandle channel;
#endif

#ifdef KVASER
    extern int channel;
#endif

#ifdef __cplusplus
}
#endif
