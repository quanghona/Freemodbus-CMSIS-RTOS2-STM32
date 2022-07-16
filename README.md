![Awesome](https://cdn.rawgit.com/sindresorhus/awesome/d7305f38d29fed78fa85652e3a63e154dd8e8829/media/badge.svg)

Translation: [cn](./README_zh.md)

# Freemodbus-CMSIS-RTOS2-STM32

A port of Freemodbus for CMSIS-RTOS2

## Other ports (a tiny awesome Freemodbus dual modes):
| Repository                                                                                       | Repository owner | OS         | Comment                                                       |
|--------------------------------------------------------------------------------------------------|------------------|------------|---------------------------------------------------------------|
| [FreeModbus_Slave-Master-RTT-STM32](https://github.com/armink/FreeModbus_Slave-Master-RTT-STM32) | Armink           | RT-Thread  | The original Freemodbus master mode                           |
| [FreeModbus_Slave-Master-RTT-STM32](https://gitee.com/Armink/FreeModbus_Slave-Master-RTT-STM32)  | Armink           | RT-Thread  | Same repository but hosted on gitee                           |
| [STM32-FreeModbus-Example](https://github.com/ADElectronics/STM32-FreeModbus-Example)            | ADElectronics    | None       | A simple example for both master and slave mode of modbus RTU |
| [HAL_FreeRTOS_Modbus](https://github.com/Alidong/HAL_FreeRTOS_Modbus)                            | Alidong          | FreeRTOS   | A FreeRTOS port                                               |
| [chibi-modbus-master](https://github.com/AndruPol/chibi-modbus-master)                           | AndruPol         | chibiOS    | A ChibiOS port                                                |
| [freemodbus_master_slave](https://github.com/connictro/freemodbus_master_slave)                  | connictro        | ESP32 RTOS | A FreeRTOS port for ESP32                                     |

* For other ports, please see [#freemodbus](https://github.com/topics/freemodbus) topic or [instant search](https://github.com/search?q=freemodbus) on github. Some may lacks of support for master mode.

## Rules
This port is base on some rules:
- **Independent**: Each file in porting layer use it own variables. Except for portserial.c and portserial_m.c, which need to use UART instance from HAL library. But other variable in those 2 files still remain static inside the file.
- **Minimize time and execution in ISR**: A message (send or receive) is handle in one call. Specifically, the serialport send a whole message in one go. And when receiving character, it save data to a buffer. Until a RX line idle or full message received, a message handler is delegated to a thread to process the message.
- **Message timeout**: The port wait for response until timeout. After that, it will skip the current request and return control to system.

## Porting steps

Let's make Armink's repository as starting point.

1. In Freemodbus middleware, changes the port/rtt folder to our OS, in this case *cmsis_rtos2*.
2. Inside port/cmsis_rtos2 folder, changes port.c to portcritical.c as the build, the system is misunderstand with the port.c in RTOS (at least it happened in my project).
3. Implements `EnterCriticalSection` and `ExitCriticalSection` functions using CMSIS-RTOS2 kernel control API.
4. Implement [portevent.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portevent.c) and [portevent_m.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portevent_m.c) using CMSIS-RTOS2 Event flags.
5. Implement [porttimer.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/porttimer.c) and [porttimer_m.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/porttimer_m.c) using CMSIS-RTOS2 Timer or hardware timer (see ADElectronic's [repository](https://github.com/ADElectronics/STM32-FreeModbus-Example/blob/master/F401_MASTER_RTU/Middlewares/FreeModbus/port/porttimer_m.c) to see how to use hardware timer).
6. Implements [portserial.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial.c) and [portserial_m.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial_m.c) using target HAL UART (or LL UART). We also implement UART ISRs in this file.
7. Use [ADElectronics's repository](https://github.com/ADElectronics/STM32-FreeModbus-Example) idea to transfer message in one call. In mbport.h, add `xMBPortSerialPutBytes` and `xMBMasterPortSerialPutBytes` prototypes and implement it in [portserial.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial.c) and [portserial_m.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial_m.c) respectively.
8. In [mbrtu.c](Middlewares/Third_Party/FreeModbus/modbus/rtu/mbrtu.c) and [mbrtu_m.c](Middlewares/Third_Party/FreeModbus/modbus/rtu/mbrtu_m.c), calls the `xMBMasterPortSerialPutBytes` function inside `xMBMasterRTUTransmitFSM` at state `STATE_M_TX_XMIT`. Replaces the `xMBMasterPortSerialPutByte` or uses the below snippet:
```c
/* Defines the MASTER_SEND_ALL_BYTES_IN_ONE_CALL macro yourself */
#if MASTER_SEND_ALL_BYTES_IN_ONE_CALL > 0
			xMBMasterPortSerialPutBytes(pucMasterSndBufferCur, usMasterSndBufferCount);
			usMasterSndBufferCount = 0;
#else
            xMBMasterPortSerialPutByte( ( CHAR )*pucMasterSndBufferCur );
            pucMasterSndBufferCur++;  /* next byte in send buffer. */
            usMasterSndBufferCount--;
#endif
```
9.  Implement [interrupt handler](Core/Src/stm32f4xx_it.c) modbus(es). The handler function is in [portserial.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial.c) and [portserial_m.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial_m.c), which its prototypes are in [port.h](Middlewares/Third_Party/FreeModbus/port/port.h) (stated in 6.). We focus on 3 interrupt flags:
    - RXNE - receive buffer not empty or new character is received, we save it in a buffer;
    - IDLE - idle flags, indicating a whole message is received;
    - TC - transfer complete, a whole message is sent.
* Note that we do not handle TXE interrupt and leave it to the HAL library.
10. Create thread(s) for modbus polling following a simple code snippet:
```c
void MBMasterPollTask(void *argument)
{
    eMBMasterInit(MB_RTU, 6, 9600, MB_PAR_NONE);
    eMBMasterEnable();
    for(;;)
    {
        eMBMasterPoll();
    }
}
```
11. Update [Makefile](Makefile) (or other toolchain/IDE) to include the modbus library.

## Project configuration

- Build tools: CubeMX + Makefile
- MCU: STM32F407ZET6 (See [Porting steps](#porting-steps) for other MCU)
- Modbus serial ports: USART6 (master) and USART3 (slave). To change to other port, for each mode ([portserial.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial.c) and [portserial_m.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial_m.c)), change 2 places:
```c
extern UART_HandleTypeDef huart6;
// ...
#define huart                                   huart6
```
Replace the correct UART port for your application.
