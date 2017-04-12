#ifndef _CAN_BSP_H
#define _CAN_BSP_H

int canInit(void);

void* CanTxThread(void *argv);
void* CanRxThread(void *argv);

#endif
