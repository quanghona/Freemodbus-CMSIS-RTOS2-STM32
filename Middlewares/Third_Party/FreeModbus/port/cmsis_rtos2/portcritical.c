/**
* @file portcritical.c
* @brief FreeModbus Library: CMSIS-RTOS2 Port. Critical section functions definition
*
* @details
* @date 2022 Jun 30
* @author Ly Hon Quang
* @e-mail lyhonquang@gmail.com
*
*/

/* ----------------------- System includes --------------------------------*/

/* ----------------------- Modbus includes ----------------------------------*/
#include "port.h"
#include "cmsis_os2.h"

/* ----------------------- Variables ----------------------------------------*/
static int32_t lock;

/* ----------------------- Start implementation -----------------------------*/
void EnterCriticalSection(void)
{
    lock = osKernelLock();
}

void ExitCriticalSection(void)
{
    osKernelRestoreLock(lock);
}
