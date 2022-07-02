/*
 * FreeModbus Libary: RT-Thread Port
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
 * File: $Id: portserial_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions $
 */

#include "port.h"
#include "main.h"
#include "cmsis_os2.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Static variables ---------------------------------*/
static volatile uint8_t rx_buff[256];
static int rx_index;
static int put_index;
static osThreadId_t receiveDataHandle;
static osEventFlagsId_t xSerialEventHandle;

extern UART_HandleTypeDef huart6;

/* ----------------------- Defines ------------------------------------------*/
#define EVENT_MBMASTER_HANDLE_RECEIVED_DATA     0x00000001UL
#define huart                                   huart6
#define RS485_MASTER_TX_MODE                        LL_GPIO_SetOutputPin(RS485_IN_CTRL_GPIO_Port, RS485_IN_CTRL_Pin)
#define RS485_MASTER_RX_MODE                        LL_GPIO_ResetOutputPin(RS485_IN_CTRL_GPIO_Port, RS485_IN_CTRL_Pin)

/* ----------------------- static functions ---------------------------------*/
static void handleReceivedDataTask(void *argument);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits,
        eMBParity eParity)
{
    /* set serial configure parameter */
    huart.Init.BaudRate = ulBaudRate;
    huart.Init.WordLength = ucDataBits == 8 ? UART_WORDLENGTH_8B : UART_WORDLENGTH_9B;
    huart.Init.StopBits = UART_STOPBITS_1;
    switch (eParity)
    {
        case MB_PAR_NONE:
            huart.Init.Parity = UART_PARITY_NONE;
            break;

        case MB_PAR_ODD:
            huart.Init.Parity = UART_PARITY_ODD;
            break;

        case MB_PAR_EVEN:
            huart.Init.Parity = UART_PARITY_EVEN;
            break;

        default:
            return FALSE;
    }
    huart.Init.Mode = UART_MODE_TX_RX;
    huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart.Init.OverSampling = UART_OVERSAMPLING_16;
    /* open serial device */
    if (HAL_UART_Init(&huart) != HAL_OK)
    {
        Error_Handler();
        return FALSE;
    }
    /* software initialize */
    __HAL_UART_ENABLE_IT(&huart, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart, UART_IT_TC);
    __HAL_UART_ENABLE_IT(&huart, UART_IT_IDLE);
    receiveDataHandle = osThreadNew(handleReceivedDataTask, NULL, NULL);

    return TRUE;
}

void vMBMasterPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable)
{
    if (xRxEnable)
    {
        /* switch 485 to receive mode */
        RS485_MASTER_RX_MODE;
    }
    else
    {
        /* switch 485 to transmit mode */
        RS485_MASTER_TX_MODE;
    }
    if (xTxEnable)
    {
        /* start serial transmit */
        rx_index = 0;
        put_index = 0;
        pxMBMasterFrameCBTransmitterEmpty();
    }
    else
    {
        /* stop serial transmit */
        HAL_UART_AbortTransmit_IT(&huart);
    }
}

void vMBMasterPortClose(void)
{
	HAL_UART_AbortReceive_IT(&huart);
	HAL_UART_AbortTransmit_IT(&huart);
}

BOOL xMBMasterPortSerialPutByte(CHAR ucByte)
{
    HAL_UART_Transmit_IT(&huart, (uint8_t*)&ucByte, 1);
    return TRUE;
}

BOOL xMBMasterPortSerialPutBytes(volatile UCHAR *ucByte, USHORT usSize)
{
	HAL_UART_Transmit_IT(&huart, (uint8_t *)ucByte, usSize);
	return TRUE;
}

BOOL xMBMasterPortSerialGetByte(CHAR * pucByte)
{
    *pucByte = rx_buff[rx_index++];

    return TRUE;
}

/*
 * Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBMasterFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call
 * xMBPortSerialPutByte( ) to send the character.
 */
void prvvMBMasterUARTTxReadyISR(void)
{
    pxMBMasterFrameCBTransmitterEmpty();
}

/*
 * Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
void prvvMBMasterUARTRxISR(void)
{
    osEventFlagsSet(xSerialEventHandle, EVENT_MBMASTER_HANDLE_RECEIVED_DATA);
}

void prvvMBMasterUARTRxReceiveCharISR(CHAR data)
{
    rx_buff[put_index++] = data;
}

static void handleReceivedDataTask(void *argument)
{
    while (1)
    {
        osEventFlagsWait(xSerialEventHandle, EVENT_MBMASTER_HANDLE_RECEIVED_DATA, osFlagsWaitAny, osWaitForever);
        while (put_index > 0) {
            pxMBMasterFrameCBByteReceived();
            put_index--;
        }
    }
}

#endif
