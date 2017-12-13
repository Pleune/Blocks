#ifndef PCANUTIL_H_INCLUDED
#define PCANUTIL_H_INCLUDED

#include <windows.h> // DLL Functions
#include <windef.h>  // Needed for PCANBasic (needs BYTE defined)
#include "PCANBasic.h"

typedef TPCANStatus (__stdcall *PeakFP_InitializeFD)    (TPCANHandle, TPCANBitrateFD);
typedef TPCANStatus (__stdcall *PeakFP_Initialize)      (TPCANHandle, TPCANBaudrate, TPCANType, DWORD, WORD);
typedef TPCANStatus (__stdcall *PeakFP_Uninitialize)    (TPCANHandle);
typedef TPCANStatus (__stdcall *PeakFP_Reset)           (TPCANHandle);
typedef TPCANStatus (__stdcall *PeakFP_GetStatus)       (TPCANHandle);
typedef TPCANStatus (__stdcall *PeakFP_Read)            (TPCANHandle, TPCANMsg*, TPCANTimestamp*);
typedef TPCANStatus (__stdcall *PeakFP_ReadFD)          (TPCANHandle, TPCANMsgFD*, TPCANTimestampFD*);
typedef TPCANStatus (__stdcall *PeakFP_Write)           (TPCANHandle, TPCANMsg*);
typedef TPCANStatus (__stdcall *PeakFP_WriteFD)         (TPCANHandle, TPCANMsgFD*);
typedef TPCANStatus (__stdcall *PeakFP_FilterMessages)  (TPCANHandle, DWORD, DWORD, TPCANMode);
typedef TPCANStatus (__stdcall *PeakFP_GetValue)        (TPCANHandle, TPCANParameter, void*, DWORD);
typedef TPCANStatus (__stdcall *PeakFP_SetValue)        (TPCANHandle, TPCANParameter, void*, DWORD);
typedef TPCANStatus (__stdcall *PeakFP_GetErrorText)    (TPCANStatus, WORD, LPSTR);

extern const TPCANHandle PCAN_handles[];
extern const unsigned PCAN_handles_len;

extern PeakFP_Initialize       PCAN_FP_Initialize;
extern PeakFP_InitializeFD     PCAN_FP_InitializeFD;
extern PeakFP_Uninitialize     PCAN_FP_UnInitialize;
extern PeakFP_Reset            PCAN_FP_Reset;
extern PeakFP_GetStatus        PCAN_FP_GetStatus;
extern PeakFP_Read             PCAN_FP_Read;
extern PeakFP_ReadFD           PCAN_FP_ReadFD;
extern PeakFP_Write            PCAN_FP_Write;
extern PeakFP_WriteFD          PCAN_FP_WriteFD;
extern PeakFP_FilterMessages   PCAN_FP_FilterMessages;
extern PeakFP_GetValue         PCAN_FP_GetValue;
extern PeakFP_SetValue         PCAN_FP_SetValue;
extern PeakFP_GetErrorText     PCAN_FP_GetTextError;

struct PCAN_channel_info {
    unsigned availaible ;
    unsigned fd         ;
    unsigned init       ;
};

extern struct PCAN_channel_info channel_info[];

int PCAN_load();
void PCAN_unload();

const char *PCAN_channel_name(const TPCANHandle handle);
int PCAN_channel_num(const TPCANHandle handle);

void PCAN_scan();
void PCAN_uninit_all();

#endif // PCANUTIL_H_INCLUDED
