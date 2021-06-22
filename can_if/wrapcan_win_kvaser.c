#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <windows.h>

#include "canlib.h"

#include "wrapcan.h"

static int verbose = 0;
int channel = 0;
static canHandle hnd;

void block_can_send(uint32_t id, int dlc, void* data)
{
	assert(dlc >= 0 && dlc <= 8);

	if(verbose >= 3)
	{
		printf("can_send id=%x dlc=%d data=", id, dlc);
		for(int i=0; i<dlc; i++)
			printf("%02x ", ((uint8_t*)data)[i]);
		printf("\n");
	}

	canStatus rc;
	if( (rc = canWrite(hnd, id, data, dlc, 0)) < 0)
	{
		fprintf(stderr, "canWrite() failed, rc=%d\n", rc);
		exit(1);
	}

	usleep(1000);
}

int block_can_expect(uint32_t id, int dlc, void* data)
{
	if(verbose >=3)
		printf("Trying to read, expecting id=0x%x, dlc=%d\n", id, dlc);


	canStatus rc;

	long got_id;
	unsigned int got_dlc;

	if( (rc = canReadWait(hnd, &got_id, data, &got_dlc, NULL, NULL, 60000 /*60 sec timeout*/)) < 0)
	{
		fprintf(stderr, "canReadWait() failed, rc=%d\n", rc);
		exit(1);
	}

	assert(got_dlc <= 8);

	if(verbose >=3)
	{
		printf("Read id=0x%lx, dlc=%d, data=", got_id, got_dlc);
		for(int i=0; i<got_dlc; i++)
			printf("%02x ", ((uint8_t*)data)[i]);
		printf("\n");
	}

	if(got_dlc != dlc)
	{
		if(verbose >= 1)
			printf("Error: can_expect(): received unexpected data length code\n");
		return -1;
	}

	if(got_id != id)
	{
		if(verbose >= 1)
			printf("Error: can_expect(): received unexpected id\n");
		return -2;
	}

	return 0;
}

int block_can_read(uint32_t* id, int* dlc, void* data)
{
	if(verbose >=3)
		printf("Trying to read\n");


	canStatus rc;

	long got_id;
	unsigned int got_dlc;

	if( (rc = canReadWait(hnd, &got_id, data, &got_dlc, NULL, NULL, 60000 /*60 sec timeout*/)) < 0)
	{
		fprintf(stderr, "canReadWait() failed, rc=%d\n", rc);
		exit(1);
	}

	assert(got_dlc <= 8);

	if(verbose >=3)
	{
		printf("Read id=0x%lx, dlc=%d, data=", got_id, got_dlc);
		for(int i=0; i<got_dlc; i++)
			printf("%02x ", ((uint8_t*)data)[i]);
		printf("\n");
	}

	*id = got_id;
	*dlc = got_dlc;

	return 0;
}

int timeout_can_read(uint32_t* id, int* dlc, void* data, int millisec)
{
	if(verbose >=3)
		printf("Trying to read\n");


	canStatus rc;

	long got_id;
	unsigned int got_dlc;

	if( (rc = canReadWait(hnd, &got_id, data, &got_dlc, NULL, NULL, millisec)) < 0)
	{
		if(rc == canERR_NOMSG)
		{
			if(verbose >=3)
				printf("Read timed out, no packets\n");

			return 1;
		}
		else
		{
			fprintf(stderr, "canReadWait() failed, rc=%d\n", rc);
			exit(1);
		}
	}

	assert(got_dlc <= 8);

	if(verbose >=3)
	{
		printf("Read id=0x%lx, dlc=%d, data=", got_id, got_dlc);
		for(int i=0; i<got_dlc; i++)
			printf("%02x ", ((uint8_t*)data)[i]);
		printf("\n");
	}

	*id = got_id;
	*dlc = got_dlc;

	return 0;
}


void block_can_init(uint32_t flt_id, uint32_t flt_mask, int verbosity)
{
	verbose = verbosity;

	canInitializeLibrary();

	canStatus rc;

	if( (hnd = canOpenChannel(channel, 0)) < 0)
	{
		fprintf(stderr, "canOpenChannel() failed with error code %d\n", hnd);
		exit(1);
	}


	if( (rc = canSetBusParams(hnd, canBITRATE_1M, 0,0,0,0,0)) < 0)
	{
		fprintf(stderr, "canSetBusParams() failed with error code %d\n", rc);
		exit(1);
	}

	if( (rc = canSetAcceptanceFilter(hnd, flt_id, flt_mask, 0)) < 0)
	{
		fprintf(stderr, "canSetAcceptanceFilter() failed with error code %d\n", rc);
		exit(1);

	}

	if( (rc = canBusOn(hnd)) < 0)
	{
		fprintf(stderr, "canBusOn() failed with error code %d\n", rc);
		exit(1);

	}

}

void block_can_deinit()
{
	canUnloadLibrary();
}

