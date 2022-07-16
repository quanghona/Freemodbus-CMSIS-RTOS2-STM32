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
 * File: $Id: porttimer.c,v 1.60 2013/08/13 15:07:05 Armink $
 *            porttimer.c,v 1.60 2022/07/17          quanghona <lyhonquang@gmail.com> CMSIS-RTOS2 port $
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"
#include "cmsis_os2.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- static functions ---------------------------------*/
static osTimerId_t xTimerId;
static uint32_t ulTimeout;
static void prvvTIMERExpiredISR(void* argument);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortTimersInit(USHORT usTim1Timerout50us)
{
    ulTimeout = 50 * usTim1Timerout50us * osKernelGetTickFreq() / 1000000 + 1;
    xTimerId = osTimerNew(prvvTIMERExpiredISR, osTimerOnce, NULL, NULL);
    return xTimerId != NULL;
}

void vMBPortTimersEnable()
{
    osTimerStart(xTimerId, ulTimeout);
}

void vMBPortTimersDisable()
{
    osTimerStop(xTimerId);
}

static void prvvTIMERExpiredISR(void* argument)
{
    ( void )pxMBPortCBTimerExpired(  );
}
