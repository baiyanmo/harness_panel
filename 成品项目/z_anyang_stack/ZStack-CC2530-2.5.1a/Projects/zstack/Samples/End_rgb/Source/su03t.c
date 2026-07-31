#include "su03t.h"
#include "MT.h"
#include "MT_UART.h"
#include "string.h"

/*
在应用任务的系统事件中，添加 CMD_SERIAL_MSG 串口的消息事件
并在这个事件内接收数据 Su03t_RecvData
做一个定时器，周期性检查数据是否接收完成，并解析数据 Su03t_DataAnalysis
*/


static Su03tRecvDef su03t = {0};
static mtOSALSerialData_t *su03tMsg = NULL;
extern byte GenericApp_TaskID;//将串口的事件绑定到该任务下

void Su03t_Config(void)
{
  //加上串口相关的初始化及事件注册函数
  MT_UartInit();
  MT_UartRegisterTaskID(GenericApp_TaskID);
}



void Su03t_RecvData(afIncomingMSGPacket_t * MSGpkt)
{
  su03tMsg = (mtOSALSerialData_t *)MSGpkt;//UartMsg->msg[0] -- 接收到的数据长度 
  su03t.time = osal_GetSystemClock();
  for(int i=0; i<su03tMsg->msg[0]; i++) {
    su03t.buff[su03t.cnt++] = su03tMsg->msg[i+1];
  } 
}

uint8 Su03t_DataAnalysis(void)
{
  uint8 ret = 0;
  if(osal_GetSystemClock() - su03t.time > 10) {//10ms就判定结束
    if(su03t.cnt > 0) {  
      if(su03t.buff[0] == 0xAA && su03t.buff[1] == 0x55) {
        switch(su03t.buff[2]) {
          case 1:ret = 1;break;//打开氛围灯
          case 2:ret = 2;break;//关闭氛围灯
          case 3:ret = 3;break;//打开红灯
          case 4:ret = 4;break;//打开绿灯
          case 5:ret = 5;break;//打开蓝灯
        }
      }
      memset((uint8*)&su03t, 0, sizeof(su03t));
    }
  }
  return ret;
}




