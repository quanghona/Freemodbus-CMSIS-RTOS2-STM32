![status](https://img.shields.io/badge/status-updating-yellow)

Translation: [cn](./README_zh.md)

# Freemodbus-CMSIS-RTOS2-STM32
A port of Freemodbus for CMSIS-RTOS2

## Other ports (a tiny awesome Freemodbus):

| Repository                                                                                       | Repository owner | OS        | Comment                                                       |
|--------------------------------------------------------------------------------------------------|------------------|-----------|---------------------------------------------------------------|
| [FreeModbus_Slave-Master-RTT-STM32](https://github.com/armink/FreeModbus_Slave-Master-RTT-STM32) | Armink           | RT-Thread | The original Freemodbus master mode                           |
| [FreeModbus_Slave-Master-RTT-STM32](https://gitee.com/Armink/FreeModbus_Slave-Master-RTT-STM32)  | Armink           | RT-Thread | Same repository but in gitee                                  |
| [STM32-FreeModbus-Example](https://github.com/ADElectronics/STM32-FreeModbus-Example)            | ADElectronics    | None      | A simple example for both master and slave mode of modbus RTU |
| [HAL_FreeRTOS_Modbus](https://github.com/Alidong/HAL_FreeRTOS_Modbus)                            | Alidong          | FreeRTOS  | A FreeRTOS port                                               |
| [chibi-modbus-master](https://github.com/AndruPol/chibi-modbus-master)                           | AndruPol         | chibiOS   | A ChibiOS port                                                |

## Rules
This port is base on some rules:
- **Independent**: Each file in poring layer use it own variables. Except for portserial.c and portserial_m.c, which need to use UART instance from HAL library. But other variable in those 2 files still remain static inside the file.
- **Minimize time and execution in ISR**: A message (send or receive) is handle in one call. Specifically, the serialport send a whole message in one go. And when receiving character, it save data to a buffer. Until a RX line idle or full message received, a message handler is delegated to a thread to process the message.
- **Message timeout**: The port wait for response until timeout. After that, it will skip the current request and return control to system.

## Porting steps

Let's make Armink's repository as starting point.

1. In Freemodbus middleware, change the port/rtt folder to our OS, in this case cmsis_rtos2.
2. Inside port/cmsis_rtos2 folder, change port.c to portcritical.c as the build system is misunderstand with the port.c in RTOS (at least it happened in my project).
3. Implement `EnterCriticalSection` and `ExitCriticalSection` functions using CMSIS-RTOS2 kernel control API.
4. Implement portevent.c and portevent_m.c using CMSIS-RTOS2's Event flags.
5. Implement porttimer.c and porttimer_m.c using CMSIS-RTOS2's Timer or hardware timer.
6. Implement portserial.c and portserial_m.c using target HAL UART (or LL UART). We also implement UART ISRs in this file.
7. Use [ADElectronics's repository](https://github.com/ADElectronics/STM32-FreeModbus-Example) idea to transfer message in one call. In mbport.h, add `xMBPortSerialPutBytes` and `xMBMasterPortSerialPutBytes` function declarations and implement it in portserial.c and portserial_m.c respectively.
8. Implement interrupt handler modbus(es). We focus on 3 interrupt flags:
- RXNE - receive buffer not empty or new character is received, we save it in a buffer;
- IDLE - idle flags, indicating a whole message is received;
- TC - transfer complete, a whole message is sent.
* Note that we do not handle TXE interrupt and leave it to the HAL library.
9. Create thread(s) for modbus polling following a simple template:
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

## Project configuration
- Build tools: CubeMX + Makefile
- MCU: STM32F407ZET6 (See [Porting steps](#porting-steps) for other MCU)
- Modbus serialports: USART6 (master) and USART3 (slave). To change to other port, for each mode (portserial.c and portserial_m.c), change 2 places:
```c
extern UART_HandleTypeDef huart6;
// ...
#define huart                                   huart6
```
Replace the correct UART port for your application.
