#ifndef __SU03T_H
#define __SU03T_H

#include "iocc2530.h"
#include "OnBoard.h"
#include "AF.h"

typedef struct {
  uint8 buff[256];
  uint8 cnt;
  uint32 time;
}Su03tRecvDef;

void Su03t_Config(void);

void Su03t_RecvData(afIncomingMSGPacket_t * MSGpkt);
uint8 Su03t_DataAnalysis(void);
#endif
