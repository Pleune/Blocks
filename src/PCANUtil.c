#include "PCANUtil.h"

const TPCANHandle PCAN_handles[] = {
    /* 00-03 */ PCAN_ISABUS1,  PCAN_ISABUS2,  PCAN_ISABUS3,  PCAN_ISABUS4,
    /* 04-07 */ PCAN_ISABUS5,  PCAN_ISABUS6,  PCAN_ISABUS7,  PCAN_ISABUS8,
    /* 08-11 */ PCAN_DNGBUS1,  PCAN_PCIBUS1,  PCAN_PCIBUS2,  PCAN_PCIBUS3,
    /* 12-15 */ PCAN_PCIBUS4,  PCAN_PCIBUS5,  PCAN_PCIBUS6,  PCAN_PCIBUS7,
    /* 16-19 */ PCAN_PCIBUS8,  PCAN_PCIBUS9,  PCAN_PCIBUS10, PCAN_PCIBUS11,
    /* 20-23 */ PCAN_PCIBUS12, PCAN_PCIBUS13, PCAN_PCIBUS14, PCAN_PCIBUS15,
    /* 24-27 */ PCAN_PCIBUS16, PCAN_USBBUS1,  PCAN_USBBUS2,  PCAN_USBBUS3,
    /* 28-31 */ PCAN_USBBUS4,  PCAN_USBBUS5,  PCAN_USBBUS6,  PCAN_USBBUS7,
    /* 32-35 */ PCAN_USBBUS8,  PCAN_USBBUS9,  PCAN_USBBUS10, PCAN_USBBUS11,
    /* 36-39 */ PCAN_USBBUS12, PCAN_USBBUS13, PCAN_USBBUS14, PCAN_USBBUS15,
    /* 40-43 */ PCAN_USBBUS16, PCAN_PCCBUS1,  PCAN_PCCBUS2,  PCAN_LANBUS1,
    /* 44-47 */ PCAN_LANBUS2,  PCAN_LANBUS3,  PCAN_LANBUS4,  PCAN_LANBUS5,
    /* 48-51 */ PCAN_LANBUS6,  PCAN_LANBUS7,  PCAN_LANBUS8,  PCAN_LANBUS9,
    /* 52-55 */ PCAN_LANBUS10, PCAN_LANBUS11, PCAN_LANBUS12, PCAN_LANBUS13,
    /* 56-58 */ PCAN_LANBUS14, PCAN_LANBUS15, PCAN_LANBUS16
};

const unsigned PCAN_handles_len = sizeof(PCAN_handles)/sizeof(*PCAN_handles);

static HINSTANCE peak_dll;

PeakFP_Initialize       PCAN_FP_Initialize;
PeakFP_InitializeFD     PCAN_FP_InitializeFD;
PeakFP_Uninitialize     PCAN_FP_UnInitialize;
PeakFP_Reset            PCAN_FP_Reset;
PeakFP_GetStatus        PCAN_FP_GetStatus;
PeakFP_Read             PCAN_FP_Read;
PeakFP_ReadFD           PCAN_FP_ReadFD;
PeakFP_Write            PCAN_FP_Write;
PeakFP_WriteFD          PCAN_FP_WriteFD;
PeakFP_FilterMessages   PCAN_FP_FilterMessages;
PeakFP_GetValue         PCAN_FP_GetValue;
PeakFP_SetValue         PCAN_FP_SetValue;
PeakFP_GetErrorText     PCAN_FP_GetTextError;

struct PCAN_channel_info channel_info[sizeof(PCAN_handles)/sizeof(*PCAN_handles)] = {{0}};

int
PCAN_load()
{
	peak_dll = LoadLibrary("PCANBasic");

	if(!peak_dll)
        return 0;

	PCAN_FP_Initialize =      (PeakFP_Initialize)     GetProcAddress(peak_dll, "CAN_Initialize");
	PCAN_FP_InitializeFD =    (PeakFP_InitializeFD)   GetProcAddress(peak_dll, "CAN_InitializeFD");
	PCAN_FP_UnInitialize =    (PeakFP_Uninitialize)   GetProcAddress(peak_dll, "CAN_Uninitialize");
	PCAN_FP_Reset =           (PeakFP_Reset)          GetProcAddress(peak_dll, "CAN_Reset");
	PCAN_FP_GetStatus =       (PeakFP_GetStatus)      GetProcAddress(peak_dll, "CAN_GetStatus");
	PCAN_FP_Read =            (PeakFP_Read)           GetProcAddress(peak_dll, "CAN_Read");
	PCAN_FP_ReadFD =          (PeakFP_ReadFD)         GetProcAddress(peak_dll, "CAN_ReadFD");
	PCAN_FP_Write =           (PeakFP_Write)          GetProcAddress(peak_dll, "CAN_Write");
	PCAN_FP_WriteFD =         (PeakFP_WriteFD)        GetProcAddress(peak_dll, "CAN_WriteFD");
	PCAN_FP_FilterMessages =  (PeakFP_FilterMessages) GetProcAddress(peak_dll, "CAN_FilterMessages");
	PCAN_FP_GetValue =        (PeakFP_SetValue)       GetProcAddress(peak_dll, "CAN_GetValue");
	PCAN_FP_SetValue =        (PeakFP_GetValue)       GetProcAddress(peak_dll, "CAN_SetValue");
	PCAN_FP_GetTextError =    (PeakFP_GetErrorText)   GetProcAddress(peak_dll, "CAN_GetErrorText");

	int status = PCAN_FP_Initialize && PCAN_FP_InitializeFD && PCAN_FP_UnInitialize && PCAN_FP_Reset &&
                 PCAN_FP_GetStatus && PCAN_FP_Read && PCAN_FP_ReadFD && PCAN_FP_Write && PCAN_FP_WriteFD &&
                 PCAN_FP_FilterMessages && PCAN_FP_GetValue && PCAN_FP_SetValue && PCAN_FP_GetTextError;

    if(status)
    {
        return 1;
    } else {
        PCAN_FP_Initialize = 0;
        PCAN_FP_InitializeFD = 0;
        PCAN_FP_UnInitialize = 0;
        PCAN_FP_Reset = 0;
        PCAN_FP_GetStatus = 0;
        PCAN_FP_Read = 0;
        PCAN_FP_ReadFD = 0;
        PCAN_FP_Write = 0;
        PCAN_FP_WriteFD = 0;
        PCAN_FP_FilterMessages = 0;
        PCAN_FP_GetValue = 0;
        PCAN_FP_SetValue = 0;
        PCAN_FP_GetTextError = 0;

        return 0;
    }
}

void
PCAN_unload()
{
    PCAN_uninit_all();

    PCAN_FP_Initialize = 0;
    PCAN_FP_InitializeFD = 0;
    PCAN_FP_UnInitialize = 0;
    PCAN_FP_Reset = 0;
    PCAN_FP_GetStatus = 0;
    PCAN_FP_Read = 0;
    PCAN_FP_ReadFD = 0;
    PCAN_FP_Write = 0;
    PCAN_FP_WriteFD = 0;
    PCAN_FP_FilterMessages = 0;
    PCAN_FP_GetValue = 0;
    PCAN_FP_SetValue = 0;
    PCAN_FP_GetTextError = 0;

    FreeLibrary(peak_dll);
}

const char *
PCAN_channel_name(const TPCANHandle handle)
{
    const char *result = "PCAN_NONE";
	switch(handle)
	{
	case PCAN_ISABUS1:
	case PCAN_ISABUS2:
	case PCAN_ISABUS3:
	case PCAN_ISABUS4:
	case PCAN_ISABUS5:
	case PCAN_ISABUS6:
	case PCAN_ISABUS7:
	case PCAN_ISABUS8:
		result = "PCAN_ISA";
		break;

	case PCAN_DNGBUS1:
		result = "PCAN_DNG";
		break;

	case PCAN_PCIBUS1:
	case PCAN_PCIBUS2:
	case PCAN_PCIBUS3:
	case PCAN_PCIBUS4:
	case PCAN_PCIBUS5:
	case PCAN_PCIBUS6:
	case PCAN_PCIBUS7:
	case PCAN_PCIBUS8:
	case PCAN_PCIBUS9:
	case PCAN_PCIBUS10:
	case PCAN_PCIBUS11:
	case PCAN_PCIBUS12:
	case PCAN_PCIBUS13:
	case PCAN_PCIBUS14:
	case PCAN_PCIBUS15:
	case PCAN_PCIBUS16:
		result = "PCAN_PCI";
		break;

	case PCAN_USBBUS1:
	case PCAN_USBBUS2:
	case PCAN_USBBUS3:
	case PCAN_USBBUS4:
	case PCAN_USBBUS5:
	case PCAN_USBBUS6:
	case PCAN_USBBUS7:
	case PCAN_USBBUS8:
	case PCAN_USBBUS9:
	case PCAN_USBBUS10:
	case PCAN_USBBUS11:
	case PCAN_USBBUS12:
	case PCAN_USBBUS13:
	case PCAN_USBBUS14:
	case PCAN_USBBUS15:
	case PCAN_USBBUS16:
		result = "PCAN_USB";
		break;

	case PCAN_PCCBUS1:
	case PCAN_PCCBUS2:
		result = "PCAN_PCC";
		break;

	case PCAN_LANBUS1:
	case PCAN_LANBUS2:
	case PCAN_LANBUS3:
	case PCAN_LANBUS4:
	case PCAN_LANBUS5:
	case PCAN_LANBUS6:
	case PCAN_LANBUS7:
	case PCAN_LANBUS8:
	case PCAN_LANBUS9:
	case PCAN_LANBUS10:
	case PCAN_LANBUS11:
	case PCAN_LANBUS12:
	case PCAN_LANBUS13:
	case PCAN_LANBUS14:
	case PCAN_LANBUS15:
	case PCAN_LANBUS16:
		result = "PCAN_LAN";
		break;
	}
	return result;
}

int
PCAN_channel_num(const TPCANHandle handle)
{
	if(handle < 0x100)
		return (BYTE)(handle) & 0xF;
	else
		return (BYTE)(handle) & 0xFF;
}

void
PCAN_scan()
{
	for (int i=0; i<PCAN_handles_len; i++)
	{
        int buf;
        TPCANStatus res;

        res = PCAN_FP_GetValue((TPCANHandle)PCAN_handles[i], PCAN_CHANNEL_CONDITION, (void*)&buf, sizeof(buf));
        if ((res == PCAN_ERROR_OK) && ((buf & PCAN_CHANNEL_AVAILABLE) == PCAN_CHANNEL_AVAILABLE))
        {
            res = PCAN_FP_GetValue((TPCANHandle)PCAN_handles[i], PCAN_CHANNEL_FEATURES, (void*)&buf, sizeof(buf));
            channel_info[i].availaible = 1;
            channel_info[i].fd = (res == PCAN_ERROR_OK) && (buf & FEATURE_FD_CAPABLE);
        } else {
            channel_info[i].availaible = 0;
            channel_info[i].init = 0;
        }
	}
}

void
PCAN_uninit_all()
{
    for(unsigned i=0; i<=PCAN_handles_len; i++)
    {
        if(channel_info[i].init)
        {
            PCAN_FP_UnInitialize(PCAN_handles[i]);
            channel_info[i].init = 0;
        }
    }
}
