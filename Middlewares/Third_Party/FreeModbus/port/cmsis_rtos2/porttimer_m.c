/*
 * FreeModbus Libary: CMSIS-RTOS2 Port
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: porttimer_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 *            porttimer_m.c,v 1.60 2022/07/17          quanghona <lyhonquang@gmail.com> CMSIS-RTOS2 port $
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"
#include "cmsis_os2.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Variables ----------------------------------------*/
static osTimerId_t xTimerId;
static uint32_t ulTimeout;

static void prvvTIMERExpiredISR(void* argument);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortTimersInit(USHORT usTimeOut50us)
{
    /* backup T35 ticks */
    ulTimeout = 50 * usTimeOut50us * osKernelGetTickFreq() / 1000000 + 1;
    xTimerId = osTimerNew(prvvTIMERExpiredISR, osTimerOnce, NULL, NULL);
    return xTimerId != NULL;
}

void vMBMasterPortTimersT35Enable()
{
    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_T35);
    osTimerStart(xTimerId, ulTimeout);
}

void vMBMasterPortTimersConvertDelayEnable()
{
    uint32_t timer_ticks = MB_MASTER_DELAY_MS_CONVERT * osKernelGetTickFreq() / 1000;

    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_CONVERT_DELAY);
    osTimerStart(xTimerId, timer_ticks);
}

void vMBMasterPortTimersRespondTimeoutEnable()
{
    uint32_t timer_ticks = MB_MASTER_TIMEOUT_MS_RESPOND * osKernelGetTickFreq() / 1000;

    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_RESPOND_TIMEOUT);
    osTimerStart(xTimerId, timer_ticks);
}

void vMBMasterPortTimersDisable()
{
    osTimerStop(xTimerId);
}

static void prvvTIMERExpiredISR(void* argument)
{
    pxMBMasterPortCBTimerExpired();
}

#endif
