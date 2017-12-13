#include "CAN.h"

#include <stdio.h>
#include "c11threads.h"

#include "PCANUtil.h"

#include "config.h"
#include "parameters.h"
#include "logging.h"

#define DMS_BASE_ID 0x451

static int read_thread_continue;
static thrd_t read_thread;
static struct timeval last_dms_msg;

void
message_handler(unsigned channel, TPCANMsg msg, TPCANTimestamp ts)
{
    //printf("got a can\n");
    switch(msg.ID)
    {
    //DMS-State Message
    case DMS_BASE_ID + 0x000:
        gettimeofday(&last_dms_msg, 0);

        log_put(0, LOG_INT, (union log_data){.i = 1});
        break;
    //Attention State
    case DMS_BASE_ID + 0x001:
    {
        static int attnID_last = 0;
        gettimeofday(&last_dms_msg, 0);

        unsigned data = msg.DATA[0];
        log_put(1, LOG_INT, (union log_data){.i = data});
        if(data > 1 && attnID_last != data)
        {
            attnID_last = data;
            char buf[11];
            snprintf(buf, sizeof(buf), "AttnID_%i", data);
        }
        break;
    }
    //PVOE Message
    case DMS_BASE_ID + 0x00C:
    {
        static unsigned pvoe_state_last = 0;

        gettimeofday(&last_dms_msg, 0);
        unsigned pvoe_state = msg.DATA[0] << 7;
        uint16_t pvoe_length = ((uint16_t)msg.DATA[0] | (uint16_t)msg.DATA[1]) << 1;
        pvoe_length |= ((uint16_t)msg.DATA[2]) >> 7;
        log_put(2, LOG_INT, (union log_data){.i = pvoe_state});
        if(pvoe_state && !pvoe_state_last && (pvoe_length > 20))
        {
            pvoe_state_last = 1;
        } else if(pvoe_state_last && !pvoe_state)
        {
            pvoe_state_last = 0;
        }
        break;
    }
    //Head Rotation
    case DMS_BASE_ID + 0x003:
    {
        long x = msg.DATA[0] | (msg.DATA[1] << 8);
        /*
        gettimeofday(&last_can_msg, 0);
        if(!f.payload().at(6) & 1)
            break;

        uint16_t x = (uint16_t)f.payload().at(0) | (uint16_t)f.payload().at(1) << 8;
        double yes_pos = *reinterpret_cast<int16_t *>(&x)*.0001;

        uint16_t y = (uint16_t)f.payload().at(2) | (uint16_t)f.payload().at(3) << 8;
        double no_pos = *reinterpret_cast<int16_t *>(&y)*.0001;

        float no_hp = no_lastbig.update(no_pos);
        no_hp = no_avg.update(no_hp);
        no_detector.update(no_hp);
        no_ed.update(no_detector.status());

        qDebug() << ": " <<  no_hp;

        float yes_hp = yes_lastbig.update(yes_pos);
        yes_hp = yes_avg.update(yes_hp);
        yes_detector.update(yes_hp);
        yes_ed.update(yes_detector.status());

        if(no_ed.isRising())
        {
            no_detector.reset();
            DMSSoundEngine::instance()->play("no");
        }

        if(yes_ed.isRising())
        {
            yes_detector.reset();
            DMSSoundEngine::instance()->play("yes");
        }*/
        break;
    }
    default:
        break;
    }

    //printf("%i\n", channel_dms);
}

int
read_thread_run(void *p)
{
    while(read_thread_continue)
    {
        for(unsigned i=0; i<PCAN_handles_len; i++)
        {
            TPCANMsg msg;
            TPCANTimestamp timestamp;
            TPCANStatus res;

            if(channel_info[i].init)
                while((res = PCAN_FP_Read(PCAN_handles[i], &msg, &timestamp)) == PCAN_ERROR_OK)
                    message_handler(i, msg, timestamp);
        }

        thrd_yield();
    }

    return 0;
}

int
CAN_init()
{
    if(!PCAN_load())
    {
        fprintf(stderr, "Error loading PCANBasic.dll\r\n");
        return 0;
    }

    PCAN_scan();

    for(unsigned i=25; i<=28; i++)//USB Channels 1-4
    {
        if(channel_info[i].availaible && !channel_info[i].fd)
        {
            //printf("Found USB Channel\n");
            int buf;
            PCAN_FP_GetValue(PCAN_handles[i], PCAN_BITRATE_ADAPTING, &buf, sizeof(buf));
            TPCANStatus res = PCAN_FP_Initialize(PCAN_handles[i], PCAN_BAUD_500K, PCAN_USB, 0, 0);
            if(res == PCAN_ERROR_OK )
            {
                channel_info[i].init = 1;
                break; //only eat 1 channel so DMS can still connect!!!
            }
        }
    }

    read_thread_continue = 1;
    thrd_create(&read_thread, &read_thread_run, 0);

    return 0;
}

void
CAN_cleanup()
{
    read_thread_continue = 0;
    thrd_join(read_thread, 0);

    PCAN_uninit_all();
	PCAN_unload();
}
