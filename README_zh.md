![Awesome](https://cdn.rawgit.com/sindresorhus/awesome/d7305f38d29fed78fa85652e3a63e154dd8e8829/media/badge.svg)

Translation: [en](./README.md)

# Freemodbus-CMSIS-RTOS2-STM32

Freemodbus 移植， 以 CMSIS-RTOS2 为实时操作系统。

## 其他移植版（小小的awesome Freemodbus）

| 储库                                                                                             | 原作者        | 系统       | 说明                                      |
|--------------------------------------------------------------------------------------------------|---------------|------------|-------------------------------------------|
| [FreeModbus_Slave-Master-RTT-STM32](https://github.com/armink/FreeModbus_Slave-Master-RTT-STM32) | Armink        | RT-Thread  | Freemodbus主机的原作者                    |
| [FreeModbus_Slave-Master-RTT-STM32](https://gitee.com/Armink/FreeModbus_Slave-Master-RTT-STM32)  | Armink        | RT-Thread  | 也是原作者的储库，但是在gitee             |
| [STM32-FreeModbus-Example](https://github.com/ADElectronics/STM32-FreeModbus-Example)            | ADElectronics | 无         | 一个简单的移植，实现modbus RTU 主机和从机 |
| [HAL_FreeRTOS_Modbus](https://github.com/Alidong/HAL_FreeRTOS_Modbus)                            | Alidong       | FreeRTOS   | FreeRTOS移植版                            |
| [chibi-modbus-master](https://github.com/AndruPol/chibi-modbus-master)                           | AndruPol      | chibiOS    | ChibiOS移植版                             |
| [freemodbus_master_slave](https://github.com/connictro/freemodbus_master_slave)                  | connictro     | ESP32 RTOS | FreeRTOS ESP32移植版                      |

* 关于其它移植，请在github上查[#freemodbus](https://github.com/topics/freemodbus)主题或者搜索[freemodbus](https://github.com/search?q=freemodbus)。大部分的储库都是freertos移植，或者只有一种模式（从机或主机）。

## 规律
这个移植版遵守以下的规律：
- **独立性**：除了portserial.c和portserial_m.c之外，每个文件都是用它内在的变数。portserial.c和portserial_m.c必须用HAL的串口变数来传达信息。但是其它的变数也是static的。
- **减少ISR的执行时间**：每个信息都是一次性的解决。发送信息时，把所有的字节一起性的发送出去。详细请看下面的移植。接收字节时，所有数据保存在一个缓冲器里面。等到IDLE了，信息处理过程就交给一个线程来解决。
- **信息过时**：每个信息请求有一个限定的时间。超过这个时间时，程序会放弃请求和等待。

## 移植

用Armink的源代码作为起点

1. 改名port/rtt为我们用的系统 - *cmsis_rtos2*
2. 在port/cmsis_rtos2里面，改名port.c为portcritical.c。因为在编译的时候会发生冲突。实时操作系统的代码里面也有一个port.c的文件。
3. 实现`EnterCriticalSection` 和 `ExitCriticalSection`接口使用CMSIS-RTOS2 kernel control API。
4. 实现[portevent.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portevent.c)和[portevent_m.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portevent_m.c)使用CMSIS-RTOS2 Event flags。
5. 实现[porttimer.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/porttimer.c)和[porttimer_m.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/porttimer_m.c)使用CMSIS-RTOS2定时器或者硬件定时器（硬件定时器版可以参考ADElectronic的[储库](https://github.com/ADElectronics/STM32-FreeModbus-Example/blob/master/F401_MASTER_RTU/Middlewares/FreeModbus/port/porttimer_m.c)）。
6.  实现[portserial.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial.c)和[portserial_m.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial_m.c)使用HAL UART(或者LL UART)。这些文件也实现了串口的中断服务程序接口。
7.  在[mbrtu.c](Middlewares/Third_Party/FreeModbus/modbus/rtu/mbrtu.c)和[mbrtu_m.c](Middlewares/Third_Party/FreeModbus/modbus/rtu/mbrtu_m.c)里面，把`xMBMasterPortSerialPutBytes`接口应用到`xMBMasterRTUTransmitFSM`里面的`STATE_M_TX_XMIT`状态。可以代替`xMBMasterPortSerialPutByte`或者用以下的片段：
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
8.  用ADElectronics的[储库](https://github.com/ADElectronics/STM32-FreeModbus-Example)的部分实现方法来把整条信息内容一次性的发送出去。在mbport.h里面加`xMBPortSerialPutBytes`和`xMBMasterPortSerialPutBytes`接口定义。然后在[portserial.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial.c)和[portserial_m.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial_m.c)实现它们。
9.  实现[系统的中断处理程序](Core/Src/stm32f4xx_it.c)。详细的中断服务程序已经实现在[portserial.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial.c)和[portserial_m.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial_m.c)和它们的定义在[port.h](Middlewares/Third_Party/FreeModbus/port/port.h)。我们只要把它们叫出来就好。我们只关心以下的三个中断旗：
    - RXNE - 接收缓冲区不空或者新的字节已接收，我们把它转移到内存的缓冲器；
    - IDLE - 闲旗，表示全部信息内容已收到；
    - TC - 送达完成，所有发送内容已发出去
* 注： 我们不处理TXE中断，由下面的HAL来处理。
10.  创造modbus polling线程,使用以下的简单程序片段：
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

11. 把需要的modbus文件加到[Makefile](Makefile)(或者其它toolchain/IDE)里面。

## 项目配置

- 工具： CubeMX + Makefile
- 微控制器：STM32F407ZET6(其它的微控制器请看[移植](#移植))
- Modbus串口： USART6(主机)和USART3(从机)。若想要用其它串口，在([portserial.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial.c)和[portserial_m.c](Middlewares/Third_Party/FreeModbus/port/cmsis_rtos2/portserial_m.c))里面，该以下的UART片段成为你想要的串口。
```c
extern UART_HandleTypeDef huart6;
// ...
#define huart                                   huart6
```
