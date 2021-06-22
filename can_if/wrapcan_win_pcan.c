#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <windows.h>

#include "PCANBasic.h"

#include "wrapcan.h"

static int verbose = 0;
TPCANHandle channel = 0x51;

int LoadDLL();
int UnloadDLL();
int GetFunctionAdress(HINSTANCE h_module);
typedef TPCANStatus (__stdcall *PCAN_Initialize)(TPCANHandle Channel, TPCANBaudrate Btr0Btr1, TPCANType HwType , DWORD IOPort , WORD Interrupt);
typedef TPCANStatus (__stdcall *PCAN_Uninitialize)( TPCANHandle Channel);
typedef TPCANStatus (__stdcall *PCAN_Reset)(TPCANHandle Channel);
typedef TPCANStatus (__stdcall *PCAN_GetStatus)(TPCANHandle Channel);
typedef TPCANStatus (__stdcall *PCAN_Read)(TPCANHandle Channel, TPCANMsg* MessageBuffer, TPCANTimestamp* TimestampBuffer);
typedef TPCANStatus (__stdcall *PCAN_Write)(TPCANHandle Channel, TPCANMsg* MessageBuffer);
typedef TPCANStatus (__stdcall *PCAN_FilterMessages)(TPCANHandle Channel, DWORD FromID, DWORD ToID, TPCANMode Mode);
typedef TPCANStatus (__stdcall *PCAN_GetValue)(TPCANHandle Channel, TPCANParameter Parameter, void* Buffer, DWORD BufferLength);
typedef TPCANStatus (__stdcall *PCAN_SetValue)(TPCANHandle Channel, TPCANParameter Parameter, void* Buffer, DWORD BufferLength);
typedef TPCANStatus (__stdcall *PCAN_GetErrorText)(TPCANStatus Error, WORD Language, LPSTR Buffer);

PCAN_Initialize g_CAN_Initialize;
PCAN_Uninitialize g_CAN_Uninitialize;
PCAN_Reset g_CAN_Reset;
PCAN_GetStatus  g_CAN_GetStatus;
PCAN_Read g_CAN_Read;
PCAN_Write  g_CAN_Write;
PCAN_FilterMessages  g_CAN_FilterMessages;
PCAN_GetValue  g_CAN_GetValue;
PCAN_SetValue  g_CAN_SetValue;
PCAN_GetErrorText  g_CAN_GetErrorText;

// name of DLL
static char g_LibFileName[] = "PCANBasic";
//DLL Instance Handle
static HINSTANCE g_i_DLL;
// TPCANHandle
static TPCANHandle g_hChannel;
static TPCANBaudrate g_Baudrate;
// nur für non PNP
static TPCANType g_CANType;
static DWORD g_IOPort;
static WORD g_Int;


int LoadDLL()
{
    if(g_i_DLL==NULL)
    {
        g_i_DLL = LoadLibrary("PCANBasic");
        if(g_i_DLL == NULL)
        {
            printf("ERROR: can not load pcanbasic.dll\n");
            return -1;
        }	
        else
        {
            if(verbose >= 3)
            {
                printf("DLL Handle: 0x%p\n",g_i_DLL);
            }

            if(GetFunctionAdress( g_i_DLL ))
            {
                if(verbose >= 3)
                {
                    printf("Load function adress for pcan_basic.dll\n");
                }
            }
            else
            {
                printf("ERROR: can not load Function Adress\n");
                return -2;
            }
        }
    }
    return 0;
}


//
// Function: GetFunctionAdress
// Parameter: instance of DLL
// ret value: true if OK false if pointer not vallid
//
// load the function pointer from the DLL spec. by handle
//



int GetFunctionAdress(HINSTANCE h_module)
{
  //Lade alle Funktionen
  if(h_module == NULL)
   return 0;

  g_CAN_Initialize = (PCAN_Initialize) GetProcAddress(h_module, "CAN_Initialize");
  if(g_CAN_Initialize == NULL)
   return 0;

  g_CAN_Uninitialize = (PCAN_Uninitialize) GetProcAddress(h_module, "CAN_Uninitialize");
  if(g_CAN_Uninitialize == NULL)
   return 0;

  g_CAN_Reset = (PCAN_Reset) GetProcAddress(h_module, "CAN_Reset");
  if(g_CAN_Reset == NULL)
   return 0;

  g_CAN_GetStatus = (PCAN_GetStatus) GetProcAddress(h_module, "CAN_GetStatus");
  if(g_CAN_GetStatus == NULL)
   return 0;

  g_CAN_Read = (PCAN_Read) GetProcAddress(h_module, "CAN_Read");
  if(g_CAN_Read == NULL)
   return 0;

  g_CAN_Write = (PCAN_Write) GetProcAddress(h_module, "CAN_Write");
  if(g_CAN_Write == NULL)
   return 0;

  g_CAN_FilterMessages = (PCAN_FilterMessages) GetProcAddress(h_module, "CAN_FilterMessages");
  if(g_CAN_FilterMessages == NULL)
   return 0;

  g_CAN_GetValue = (PCAN_GetValue) GetProcAddress(h_module, "CAN_GetValue");
  if(g_CAN_GetValue == NULL)
   return 0;

  g_CAN_SetValue = (PCAN_SetValue) GetProcAddress(h_module, "CAN_SetValue");
  if(g_CAN_SetValue == NULL)
   return 0;

  g_CAN_GetErrorText = (PCAN_GetErrorText) GetProcAddress(h_module, "CAN_GetErrorText");
  if(g_CAN_GetErrorText == NULL)
   return 0;

  return 1;
}
//
// Function: Unload DLL
// Parameter: none
// ret value: 0 if OK 
//
// unload the DLL and free all pointers
//

int UnloadDLL()
{
 if(g_i_DLL)
 {
  FreeLibrary(g_i_DLL);
  g_CAN_Initialize = NULL;
  g_CAN_Uninitialize = NULL;
  g_CAN_Reset = NULL;
  g_CAN_GetStatus = NULL;
  g_CAN_Read = NULL;
  g_CAN_Write = NULL;
  g_CAN_FilterMessages = NULL;
  g_CAN_GetValue = NULL;
  g_CAN_SetValue = NULL;
  g_CAN_GetErrorText = NULL;
  return 0;
 }
 return -1;
}


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

    TPCANMsg frame;
    frame.ID = id;
    frame.MSGTYPE = PCAN_MESSAGE_STANDARD;
    frame.LEN = dlc;

    if(dlc > 0) memcpy(frame.DATA, data, dlc);


    int rc;
    if( (rc = g_CAN_Write(channel, &frame)) != PCAN_ERROR_OK)
    {
        fprintf(stderr, "CAN_Write() failed, rc=%d\n", rc);
        exit(1);
    }

    usleep(1000);
}

int block_can_expect(uint32_t id, int dlc, void* data)
{
    if(verbose >=3)
        printf("Trying to read, expecting id=0x%x, dlc=%d\n", id, dlc);


    TPCANMsg frame;

    // blocking mode not available; poll until there is data (or error other than EMPTY)

    TPCANStatus rc;
    int32_t empty_attempts = 5000;
    int32_t attempts = 300;

    do
    {
        while( (rc = g_CAN_Read(channel, &frame, NULL)) == PCAN_ERROR_QRCVEMPTY && empty_attempts-- > 0)
        {
            usleep(1000);
        }

        if(rc == PCAN_ERROR_BUSHEAVY)
        {
            fprintf(stderr, "ERROR: Can bus is not working properly. Check for connectivity, are you using a terminator?\n");
            exit(1);
        }

        if(rc == PCAN_ERROR_QRCVEMPTY)
        {
            fprintf(stderr, "ERROR: The unit does not respond. Try to power-cycle the unit.\n");
            exit(1);
        }

        if(rc != PCAN_ERROR_OK)
        {
            fprintf(stderr, "CAN_Read() failed, rc=%lu\n", rc);
            exit(1);
        }

        if(verbose >=3)
        {
            printf("Read id=0x%lx, dlc=%d, data=", frame.ID, frame.LEN);
            for(int i=0; i<frame.LEN; i++)
                printf("%02x ", frame.DATA[i]);
            printf("\n");

            if (frame.LEN != dlc || frame.ID != id)
            {
                printf("Error: can_expect(): retrying. received unexpected frame (was id %04lx DLC %d, expected id %04x dlc %d)\n", frame.ID, frame.LEN, id, dlc);
            }
        }

    } while ((frame.LEN != dlc || frame.ID != id) && attempts-- > 0);

    if(frame.LEN != dlc)
    {
        if(verbose >= 1)
            printf("Error: can_expect(): received unexpected data length code %d, expected %d\n", frame.LEN, dlc);
        return -1;
    }

    if(frame.ID != id)
    {
        if(verbose >= 1)
            printf("Error: can_expect(): received unexpected id %04lx, expected %04x\n", frame.ID, id);
        return -2;
    }

    assert(frame.LEN <= 8);
    
    if(frame.LEN > 0)
        memcpy(data, frame.DATA, frame.LEN);

    return 0;
}

int block_can_read(uint32_t* id, int* dlc, void* data)
{
    if(verbose >=3)
        printf("Trying to read\n");

    TPCANMsg frame;

    // blocking mode not available; poll until there is data (or error other than EMPTY)

    TPCANStatus rc;

    while( (rc = g_CAN_Read(channel, &frame, NULL)) == PCAN_ERROR_QRCVEMPTY )
    {
        usleep(1000);
    }

    if(rc != PCAN_ERROR_OK)
    {
        fprintf(stderr, "CAN_Read() failed, rc=%lu\n", rc);
        exit(1);
    }

    if(verbose >=3)
    {
        printf("Read id=0x%lx, dlc=%d, data=", frame.ID, frame.LEN);
        for(int i=0; i<frame.LEN; i++)
            printf("%02x ", frame.DATA[i]);
        printf("\n");
    }

    assert(frame.LEN <= 8);

    *id = frame.ID;
    *dlc = frame.LEN;
    
    if(frame.LEN > 0)
        memcpy(data, frame.DATA, frame.LEN);

    return 0;
}

int timeout_can_read(uint32_t* id, int* dlc, void* data, int millisec)
{
    if(verbose >=3)
        printf("Trying to read\n");

    TPCANMsg frame;

    // blocking mode not available; poll until there is data (or error other than EMPTY), or our timeout counter reaches the argument

    TPCANStatus rc;

    int ms_cnt = 0;
    while( (rc = g_CAN_Read(channel, &frame, NULL)) == PCAN_ERROR_QRCVEMPTY )
    {
        usleep(1000);
        ms_cnt++;
        if(ms_cnt > millisec)
        {
            if(verbose >= 3)
                printf("Read timed out, no packets\n");

            return 1;
        }
    }

    if(rc != PCAN_ERROR_OK)
    {
        fprintf(stderr, "CAN_Read() failed, rc=%lu\n", rc);
        exit(1);
    }

    if(verbose >=3)
    {
        printf("Read id=0x%lx, dlc=%d, data=", frame.ID, frame.LEN);
        for(int i=0; i<frame.LEN; i++)
            printf("%02x ", frame.DATA[i]);
        printf("\n");
    }

    assert(frame.LEN <= 8);

    *id = frame.ID;
    *dlc = frame.LEN;
    
    if(frame.LEN > 0)
        memcpy(data, frame.DATA, frame.LEN);

    return 0;
}


// WARNING: PCAN API does not implement mask filters, only range filters
// flt_id is abused as start id, flt_mask as end id, range is inclusive of both ends.
void block_can_init(uint32_t flt_id, uint32_t flt_mask, int verbosity)
{
    verbose = verbosity;

    int ret = LoadDLL();
    if(ret!=0)
    {
     fprintf(stderr, "LoadDLL() failed with error code %d", ret);
     exit(1);
    }

    TPCANStatus rc;

    if( (rc = g_CAN_Initialize(channel, PCAN_BAUD_1M, 0, 0, 0)) != PCAN_ERROR_OK)
    {
        fprintf(stderr, "CAN_Initialize() failed with error code 0x%lx\n", rc);
        exit(1);
    }

    if( (rc = g_CAN_FilterMessages(channel, flt_id, flt_mask, PCAN_MESSAGE_STANDARD)) != PCAN_ERROR_OK)
    {
        fprintf(stderr, "CAN_FilterMessages() failed with error code 0x%lx\n", rc);
        exit(1);

    }
}

void block_can_deinit()
{

    UnloadDLL();

}

