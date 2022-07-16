#include "port.h"
#include "main.h"
#include "cmsis_os2.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Static variables ---------------------------------*/
static volatile uint8_t rx_buff[256];
static int rx_index;
static int put_index;
static osThreadId_t receiveDataHandle;
static osEventFlagsId_t xSerialEventHandle;

extern UART_HandleTypeDef huart3;

/* ----------------------- Defines ------------------------------------------*/
#define EVENT_MBSLAVE_HANDLE_RECEIVED_DATA  0x00000001UL
#define huart                               huart3
#define RS485_SLAVE_TX_MODE()               LL_GPIO_SetOutputPin(RS485_SLAVE_CTRL_GPIO_Port, RS485_SLAVE_CTRL_Pin)
#define RS485_SLAVE_RX_MODE()               LL_GPIO_ResetOutputPin(RS485_SLAVE_CTRL_GPIO_Port, RS485_SLAVE_CTRL_Pin)

/* ----------------------- static functions ---------------------------------*/
static void handleReceivedDataTask(void *argument);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits,
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

void vMBPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable)
{
    if (xRxEnable)
    {
        /* switch 485 to receive mode */
        RS485_SLAVE_RX_MODE();
    }
    else
    {
        /* switch 485 to transmit mode */
        RS485_SLAVE_TX_MODE();
    }
    if (xTxEnable)
    {
        /* start serial transmit */
        rx_index = 0;
        put_index = 0;
        pxMBFrameCBTransmitterEmpty();
    }
    else
    {
        /* stop serial transmit */
        HAL_UART_AbortTransmit_IT(&huart);
    }
}

void vMBPortClose(void)
{
	HAL_UART_AbortReceive_IT(&huart);
	HAL_UART_AbortTransmit_IT(&huart);
}

BOOL xMBPortSerialPutByte(CHAR ucByte)
{
    HAL_UART_Transmit_IT(&huart, (uint8_t*)&ucByte, 1);
    return TRUE;
}

/*
 * A Function to send all bytes in one call.
 */
BOOL xMBPortSerialPutBytes(volatile UCHAR *ucByte, USHORT usSize)
{
	HAL_UART_Transmit_IT(&huart, (uint8_t *)ucByte, usSize);
	return TRUE;
}

BOOL xMBPortSerialGetByte(CHAR * pucByte)
{
    *pucByte = rx_buff[rx_index++];

    return TRUE;
}

/*
 * Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call
 * xMBPortSerialPutByte( ) to send the character.
 */
void prvvUARTTxReadyISR(void)
{
    pxMBFrameCBTransmitterEmpty();
}

/*
 * Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 *
 * quanghona: This function is called when IDLE is detected. thus will then
 * trigger the received message handler.
 */
void prvvUARTRxISR(void)
{
    osEventFlagsSet(xSerialEventHandle, EVENT_MBSLAVE_HANDLE_RECEIVED_DATA);
}

/*
 * Create an interrupt handler for the receive character for your target processor.
 * This function should store data to a buffer for received message thread to
 * handle later.
 */
void prvvUARTRxReceiveCharISR(CHAR data)
{
    rx_buff[put_index++] = data;
}

/*
 * A thread to handle the received message.
 * This thread is wait until a received message flag is set, then it will call
 * pxMBFrameCBByteReceived() until all characters in the receive buffer is handled.
 */
static void handleReceivedDataTask(void *argument)
{
    while (1)
    {
        osEventFlagsWait(xSerialEventHandle, EVENT_MBSLAVE_HANDLE_RECEIVED_DATA, osFlagsWaitAny, osWaitForever);
        while (put_index > 0) {
            pxMBFrameCBByteReceived();
            put_index--;
        }
    }
}
