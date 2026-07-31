/******************************************************************************
  Filename:       GenericApp.c
  Revised:        $Date: 2012-03-07 01:04:58 -0800 (Wed, 07 Mar 2012) $
  Revision:       $Revision: 29656 $

  Description:    Generic Application (no Profile).


  Copyright 2004-2012 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License"). You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product. Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
******************************************************************************/

/*********************************************************************
  本应用不实现复杂功能，仅作为应用程序结构的简单示例。

  每隔5秒向另一个"Generic"应用发送"Hello World"，同时也能
  接收"Hello World"数据包。

  "Hello World"消息以MSG类型消息收发。

  本应用没有Profile，所有操作直接处理。

  按键功能：
    SW1: （未使用）
    SW2: 发起终端设备绑定
    SW3: （未使用）
    SW4: 发起匹配描述符请求
*********************************************************************/

/*********************************************************************
 * 头文件包含
 */
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"

#include "GenericApp.h"
#include "DebugTrace.h"
#include "stdio.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
#define MT_UART_DEFAULT_BAUDRATE  HAL_UART_BR_9600
#include "MT_UART.h"

/* RTOS */
#if defined( IAR_ARMCM3_LM )
#include "RTOS_App.h"
#endif  

#include "stdio.h"
#include "string.h"

// 自定义串口回调：接收STM32原始数据，发送给应用任务
extern byte GenericApp_TaskID;
static void App_UartCallback(uint8 port, uint8 event)
{
  if (event & (HAL_UART_RX_FULL | HAL_UART_RX_ABOUT_FULL | HAL_UART_RX_TIMEOUT))
  {
    uint16 rxBufLen = Hal_UART_RxBufLen(port);
    if (rxBufLen > 0 && GenericApp_TaskID)
    {
      osal_event_hdr_t *msg_ptr = (osal_event_hdr_t *)osal_msg_allocate(rxBufLen + sizeof(osal_event_hdr_t));
      if (msg_ptr)
      {
        msg_ptr->event = SPI_INCOMING_ZAPP_DATA;
        msg_ptr->status = rxBufLen;
        HalUARTRead(port, (uint8 *)(msg_ptr + 1), rxBufLen);
        osal_msg_send(GenericApp_TaskID, (uint8 *)msg_ptr);
      }
    }
  }
}

void CoorToEndDevice(uint16 nwkDevAddr,uint16 cid,char* pmsg,uint8 len);//向终端节点发送ACK消息

uint16 Led_Time = 500;//LED闪烁时间间隔(ms)

struct NODEX
{
  uint16 node_addr;//终端节点的地址
  char msg[64]; //要发送的数据
}recv_node1;
   
char lcd_buff[64] = "\0";

// 记住最近一次上报的终端节点地址，用于转发串口指令
static uint16 last_enddevice_addr = 0;
static uint8  last_enddevice_addr_valid = 0;
char lcd_send_cmd[32] = "\0";  // LCD第5行：发送给STM32的指令


//传感器数据结构体（紧凑对齐，避免两端字节偏移不一致）
#pragma pack(1)
struct tagSensor_Nodex
{
  uint8 rgb_state;//RGB颜色状态
};
struct tagNodex //节点数据
{
  uint16 nwkDevAddress;//节点地址
  struct tagSensor_Nodex sensor_val;//传感器数据
}nodex;
#pragma pack()


union tagNodex_BUFF
{
  unsigned char buff[32];
  struct tagSensor_Nodex upval;
}up_nodex;//����λ���������ݵĹ�����

/*********************************************************************
 * 宏定义
 */

/*********************************************************************
 * 常量定义
 */

/*********************************************************************
 * 类型定义
 */

/*********************************************************************
 * 全局变量
 */
// 应用专用的Cluster ID列表
const cId_t GenericApp_ClusterList[GENERICAPP_MAX_CLUSTERS] =
{
  GENERICAPP_CLUSTERID
};

const SimpleDescriptionFormat_t GenericApp_SimpleDesc =
{
  GENERICAPP_ENDPOINT,              //  端点号
  GENERICAPP_PROFID,                //  应用Profile ID
  GENERICAPP_DEVICEID,              //  设备ID
  GENERICAPP_DEVICE_VERSION,        //  设备版本号
  GENERICAPP_FLAGS,                 //  应用标志位
  GENERICAPP_MAX_CLUSTERS,          //  输入Cluster数量
  (cId_t *)GenericApp_ClusterList,  //  输入Cluster列表指针
  GENERICAPP_MAX_CLUSTERS,          //  输出Cluster数量
  (cId_t *)GenericApp_ClusterList   //  输出Cluster列表指针
};

// 端点/接口描述符，在GenericApp_Init()中填充。
// 也可以在这里定义为const常量存入代码空间。
// 当前定义在RAM中。
endPointDesc_t GenericApp_epDesc;

/*********************************************************************
 * 外部变量
 */

/*********************************************************************
 * 外部函数
 */

/*********************************************************************
 * 局部变量
 */
byte GenericApp_TaskID;   // 任务ID，用于内部任务/事件处理
                          // GenericApp_Init()调用时由OSAL分配
devStates_t GenericApp_NwkState;


byte GenericApp_TransID;  // 唯一消息ID（递增计数器）

afAddrType_t GenericApp_DstAddr;

/*********************************************************************
 * 局部函数声明
 */
static void GenericApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
static void GenericApp_HandleKeys( byte shift, byte keys );
static void GenericApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
static void GenericApp_SendTheMessage( void );

#if defined( IAR_ARMCM3_LM )
static void GenericApp_ProcessRtosMessage( void );
#endif

/*
nwkDevAddr -- �˵��ַ
cid �C ��id
pmsg �C ָ��Ҫ���͵���������
len �C ���ݳ���
*/
void CoorToEndDevice(uint16 nwkDevAddr,uint16 cid,char* pmsg,uint8 len)
{
  GenericApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  GenericApp_DstAddr.endPoint = GENERICAPP_ENDPOINT;
  GenericApp_DstAddr.addr.shortAddr = nwkDevAddr;
  
  AF_DataRequest( &GenericApp_DstAddr, &GenericApp_epDesc,
                        cid,
                       (byte)len + 1,
                       (byte *)pmsg,
                       &GenericApp_TransID,
                       AF_DISCV_ROUTE, AF_DEFAULT_RADIUS);
}


/*********************************************************************
 * 网络层回调函数
 */

/*********************************************************************
 * 公共函数
 */

/*********************************************************************
 * @fn      GenericApp_Init
 *
 * @brief   Generic App任务初始化函数。
 *          在初始化期间调用，应包含应用特定的初始化操作
 *          （如硬件初始化/配置、表初始化、上电通知等）。
 *
 * @param   task_id - OSAL分配的任务ID，用于发送消息和设置定时器
 *
 * @return  none
 */
void GenericApp_Init( uint8 task_id )
{
  GenericApp_TaskID = task_id;
  GenericApp_NwkState = DEV_INIT;
  GenericApp_TransID = 0;

  // 硬件初始化可在此处或main()（Zmain.c）中添加
  // 应用专用硬件初始化放这里，其他硬件初始化放main()

  // 串口初始化：先用MT初始化，再重新配置为原始数据模式
  MT_UartInit();
  {
    halUARTCfg_t uartConfig;
    uartConfig.configured           = TRUE;
    uartConfig.baudRate             = MT_UART_DEFAULT_BAUDRATE;
    uartConfig.flowControl          = FALSE;
    uartConfig.flowControlThreshold = 0;
    uartConfig.rx.maxBufSize        = 128;
    uartConfig.tx.maxBufSize        = 128;
    uartConfig.idleTimeout          = 100;
    uartConfig.intEnable            = TRUE;
    uartConfig.callBackFunc         = App_UartCallback;
    HalUARTOpen(MT_UART_DEFAULT_PORT, &uartConfig);
    // 清空串口缓冲区残留数据
    { uint8 trash[64]; while (Hal_UART_RxBufLen(MT_UART_DEFAULT_PORT) > 0) HalUARTRead(MT_UART_DEFAULT_PORT, trash, sizeof(trash)); }
  }

  GenericApp_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  GenericApp_DstAddr.endPoint = 0;
  GenericApp_DstAddr.addr.shortAddr = 0;

  // 填充端点描述符
  GenericApp_epDesc.endPoint = GENERICAPP_ENDPOINT;
  GenericApp_epDesc.task_id = &GenericApp_TaskID;
  GenericApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&GenericApp_SimpleDesc;
  GenericApp_epDesc.latencyReq = noLatencyReqs;

  // 向AF注册端点描述符
  afRegister( &GenericApp_epDesc );

  // 注册按键事件，本应用将处理所有按键事件
  RegisterForKeys( GenericApp_TaskID );

  // 更新LCD显示
#if defined ( LCD_SUPPORTED )
  //ע�����ݴӵ����п�ʼ��ʾ��ǰ���лᱻ��ʼ��ʧ����ʾ���ǣ�����
  HalLcdWriteString( "GenericApp", HAL_LCD_LINE_1 );
  HalLcdWriteString( "zhengyuntong", HAL_LCD_LINE_3 );
#endif

  ZDO_RegisterForZDOMsg( GenericApp_TaskID, End_Device_Bind_rsp );
  ZDO_RegisterForZDOMsg( GenericApp_TaskID, Match_Desc_rsp );

//����Led�ƶ�ʱ������
   osal_start_timerEx(GenericApp_TaskID,
                      GENERICAPP_LED_EVT,
                      Led_Time);

#if defined( IAR_ARMCM3_LM )
  // 向RTOS任务发起者注册本任务
  RTOS_RegisterApp( task_id, GENERICAPP_RTOS_MSG_EVT );
#endif
}

/*********************************************************************
 * @fn      GenericApp_ProcessEvent
 *
 * @brief   Generic App任务事件处理器。处理所有任务事件，
 *          包括定时器、消息及其他用户定义事件。
 *
 * @param   task_id  - OSAL分配的任务ID
 * @param   events  - 待处理事件位图，可包含多个事件
 *
 * @return  none
 */
uint16 GenericApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  afDataConfirm_t *afDataConfirm;

  // 数据确认消息字段
  byte sentEP;
  ZStatus_t sentStatus;
  byte sentTransID;       // 应与发送时的值一致
  (void)task_id;  // 未使用的参数，避免编译警告

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( GenericApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZDO_CB_MSG:
          GenericApp_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          GenericApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case AF_DATA_CONFIRM_CMD:
          // 收到数据包发送确认消息
          // 状态类型为ZStatus_t（定义在ZComDef.h中）
          // 消息字段定义在AF.h中
          afDataConfirm = (afDataConfirm_t *)MSGpkt;
          sentEP = afDataConfirm->endpoint;
          sentStatus = afDataConfirm->hdr.status;
          sentTransID = afDataConfirm->transID;
          (void)sentEP;
          (void)sentTransID;

          // 收到确认后的处理
          if ( sentStatus != ZSuccess )
          {
            // 数据未送达，可在此处理错误
          }
          break;

        case AF_INCOMING_MSG_CMD:
          GenericApp_MessageMSGCB( MSGpkt );
          break;

        // 收到STM32串口发来的数据（LED控制指令）
        case SPI_INCOMING_ZAPP_DATA:
        {
          osal_event_hdr_t *pMsg = (osal_event_hdr_t *)MSGpkt;
          uint8 *pData = (uint8 *)(pMsg + 1);
          uint8  len   = pMsg->status;

          if(len > 0 && len < 64)
          {
            char cmd[64] = {0};
            char lcd_line5[80] = {0};
            memcpy(cmd, pData, len);
            sprintf(lcd_line5, "get: %s", cmd);
            HalLcdWriteString(lcd_line5, HAL_LCD_LINE_5);

            // 广播转发给所有终端设备（不需要知道具体地址）
            {
              afAddrType_t broadcastAddr;
              broadcastAddr.addrMode = (afAddrMode_t)AddrBroadcast;
              broadcastAddr.endPoint = GENERICAPP_ENDPOINT;
              broadcastAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
              AF_DataRequest( &broadcastAddr, &GenericApp_epDesc,
                              GENERICAPP_CLUSTERID,
                              (byte)len + 1,
                              (byte *)cmd,
                              &GenericApp_TransID,
                              AF_DISCV_ROUTE, AF_DEFAULT_RADIUS );
            }
          }
          break;
        }

        case ZDO_STATE_CHANGE:
          GenericApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
            if ( (GenericApp_NwkState == DEV_ZB_COORD) )
              {
               HalLcdWriteString("Net Creat Success",HAL_LCD_LINE_3);
               Led_Time = 1000;//��ɫС������--������������
              }
              else
              {
                HalLcdWriteString("Net Creat Failed",HAL_LCD_LINE_3);
               Led_Time = 100;//��ɫС�ƿ���--û�д���������
              }
          break;

        default:
          break;
      }

      // 释放消息内存
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // 接收下一条消息
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( GenericApp_TaskID );
    }

    // 返回未处理的事件
    return (events ^ SYS_EVENT_MSG);
  }

  // 发送消息事件 - 由定时器产生（在GenericApp_Init()中设置）
  if ( events & GENERICAPP_SEND_MSG_EVT )
  {
    // 发送消息
    GenericApp_SendTheMessage();

    // 设置下次发送消息的定时器
    osal_start_timerEx( GenericApp_TaskID,
                        GENERICAPP_SEND_MSG_EVT,
                        GENERICAPP_SEND_MSG_TIMEOUT );

    // 返回未处理的事件
    return (events ^ GENERICAPP_SEND_MSG_EVT);
  }
  
  //led�¼��ж�
  if ( events & GENERICAPP_LED_EVT )
  {
    // 发送消息
    HalLedSet(HAL_LED_1,HAL_LED_MODE_TOGGLE);
    HalLedSet(HAL_LED_2,HAL_LED_MODE_TOGGLE);
    HalLedSet(HAL_LED_3,HAL_LED_MODE_TOGGLE);

    // 设置下次发送消息的定时器
    osal_start_timerEx( GenericApp_TaskID,
                        GENERICAPP_LED_EVT,
                        Led_Time );

    // 返回未处理的事件
    return (events ^ GENERICAPP_LED_EVT);
  }


#if defined( IAR_ARMCM3_LM )
  // 从RTOS队列接收消息
  if ( events & GENERICAPP_RTOS_MSG_EVT )
  {
    // 处理RTOS队列中的消息
    GenericApp_ProcessRtosMessage();

    // 返回未处理的事件
    return (events ^ GENERICAPP_RTOS_MSG_EVT);
  }
#endif

  // 丢弃未知事件
  return 0;
}

/*********************************************************************
 * 事件处理函数
 */

/*********************************************************************
 * @fn      GenericApp_ProcessZDOMsgs()
 *
 * @brief   处理ZDO响应消息
 *
 * @param   none
 *
 * @return  none
 */
static void GenericApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
{
  switch ( inMsg->clusterID )
  {
    case End_Device_Bind_rsp:
      if ( ZDO_ParseBindRsp( inMsg ) == ZSuccess )
      {
        // 点亮LED
        HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
      }
#if defined( BLINK_LEDS )
      else
      {
        // LED闪烁表示绑定失败
        HalLedSet ( HAL_LED_4, HAL_LED_MODE_FLASH );
      }
#endif
      break;

    case Match_Desc_rsp:
      {
        ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( inMsg );
        if ( pRsp )
        {
          if ( pRsp->status == ZSuccess && pRsp->cnt )
          {
            GenericApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
            GenericApp_DstAddr.addr.shortAddr = pRsp->nwkAddr;
            // 取第一个端点，可修改为遍历所有端点
            GenericApp_DstAddr.endPoint = pRsp->epList[0];

            // 点亮LED
            HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
          }
          osal_mem_free( pRsp );
        }
      }
      break;
  }
}

/*********************************************************************
 * @fn      GenericApp_HandleKeys
 *
 * @brief   处理本设备的所有按键事件
 *
 * @param   shift - 是否按下Shift/Alt键
 * @param   keys  - 按键事件位域，有效值：
 *                  HAL_KEY_SW_4, HAL_KEY_SW_3,
 *                  HAL_KEY_SW_2, HAL_KEY_SW_1
 *
 * @return  none
 */
static void GenericApp_HandleKeys( uint8 shift, uint8 keys )
{
  zAddrType_t dstAddr;

  // Shift键用于切换按键的第二功能
  if ( shift )
  {
    if ( keys & HAL_KEY_SW_1 )
    {
    }
    if ( keys & HAL_KEY_SW_2 )
    {
    }
    if ( keys & HAL_KEY_SW_3 )
    {
    }
    if ( keys & HAL_KEY_SW_4 )
    {
    }
  }
  else
  {
    if ( keys & HAL_KEY_SW_1 )
    {
      // SW1未被其他功能占用时...
#if defined( SWITCH1_BIND )
      // 用SW1模拟SW2（仅有一个按键的设备）
      keys |= HAL_KEY_SW_2;
#elif defined( SWITCH1_MATCH )
      // 或用SW1模拟SW4（仅有一个按键的设备）
      keys |= HAL_KEY_SW_4;
#endif
    }

    if ( keys & HAL_KEY_SW_2 )
    {
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );

      // 对必选端点发起终端设备绑定请求
      dstAddr.addrMode = Addr16Bit;
      dstAddr.addr.shortAddr = 0x0000; // 协调器地址
      ZDP_EndDeviceBindReq( &dstAddr, NLME_GetShortAddr(),
                            GenericApp_epDesc.endPoint,
                            GENERICAPP_PROFID,
                            GENERICAPP_MAX_CLUSTERS, (cId_t *)GenericApp_ClusterList,
                            GENERICAPP_MAX_CLUSTERS, (cId_t *)GenericApp_ClusterList,
                            FALSE );
    }

    if ( keys & HAL_KEY_SW_3 )
    {
    }

    if ( keys & HAL_KEY_SW_4 )
    {
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
      // 发起匹配描述符请求（服务发现）
      dstAddr.addrMode = AddrBroadcast;
      dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
      ZDP_MatchDescReq( &dstAddr, NWK_BROADCAST_SHORTADDR,
                        GENERICAPP_PROFID,
                        GENERICAPP_MAX_CLUSTERS, (cId_t *)GenericApp_ClusterList,
                        GENERICAPP_MAX_CLUSTERS, (cId_t *)GenericApp_ClusterList,
                        FALSE );
    }
  }
}

/*********************************************************************
 * 局部函数声明
 */

/*********************************************************************
 * @fn      GenericApp_MessageMSGCB
 *
 * @brief   数据消息处理回调函数。处理来自其他设备的入站数据，
 *          根据Cluster ID执行相应操作。
 *
 * @param   none
 *
 * @return  none
 */
static void GenericApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  char lcd_buff[128] = "\0";
  char uart_msg[32] = "\0";
  switch ( pkt->clusterId )
  {
    case GENERICAPP_CLUSTERID:
      nodex = *((struct tagNodex *)pkt->cmd.Data);
      last_enddevice_addr = nodex.nwkDevAddress;
      last_enddevice_addr_valid = 1;
      sprintf(lcd_buff,"%X state:%d",nodex.nwkDevAddress,
                                      nodex.sensor_val.rgb_state);

      switch(nodex.sensor_val.rgb_state)
      {
        case 0:  strcpy(uart_msg, "off");       break;
        case 1:  strcpy(uart_msg, "red on");    break;
        case 2:  strcpy(uart_msg, "green on");  break;
        case 3:  strcpy(uart_msg, "blue on");   break;
        case 4:  strcpy(uart_msg, "yellow on"); break;
        default: strcpy(uart_msg, "off");       break;
      }
      break;
  }
  HalLcdWriteString(lcd_buff, HAL_LCD_LINE_7);
  CoorToEndDevice(nodex.nwkDevAddress,GENERICAPP_CLUSTERID,"ACK",osal_strlen("ACK"));
  if(uart_msg[0] != '\0')
  {
    strcpy(lcd_send_cmd, uart_msg);
    HalLcdWriteString(lcd_send_cmd, HAL_LCD_LINE_4);
  }
}
  }
  HalLcdWriteString(lcd_buff, HAL_LCD_LINE_7);
  CoorToEndDevice(nodex.nwkDevAddress,GENERICAPP_CLUSTERID,"ACK",osal_strlen("ACK"));
}

/*********************************************************************
 * @fn      GenericApp_SendTheMessage
 *
 * @brief   发送消息函数
 *
 * @param   none
 *
 * @return  none
 */
static void GenericApp_SendTheMessage( void )
{
  char theMessageData[] = "Hello World";

  if ( AF_DataRequest( &GenericApp_DstAddr, &GenericApp_epDesc,
                       GENERICAPP_CLUSTERID,
                       (byte)osal_strlen( theMessageData ) + 1,
                       (byte *)&theMessageData,
                       &GenericApp_TransID,
                       AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
    // 消息发送请求成功
  }
  else
  {
    // 消息发送请求失败
  }
}

#if defined( IAR_ARMCM3_LM )
/*********************************************************************
 * @fn      GenericApp_ProcessRtosMessage
 *
 * @brief   从RTOS队列接收消息并发送响应
 *
 * @param   none
 *
 * @return  none
 */
static void GenericApp_ProcessRtosMessage( void )
{
  osalQueue_t inMsg;

  if ( osal_queue_receive( OsalQueue, &inMsg, 0 ) == pdPASS )
  {
    uint8 cmndId = inMsg.cmnd;
    uint32 counter = osal_build_uint32( inMsg.cbuf, 4 );

    switch ( cmndId )
    {
      case CMD_INCR:
        counter += 1;  /* 递增接收到的计数器 */
                       /* 落入下一个case */

      case CMD_ECHO:
      {
        userQueue_t outMsg;

        outMsg.resp = RSP_CODE | cmndId;  /* 响应ID */
        osal_buffer_uint32( outMsg.rbuf, counter );    /* 递增计数器 */
        osal_queue_send( UserQueue1, &outMsg, 0 );  /* 发送回UserTask */
        break;
      }
      
      default:
        break;  /* 忽略未知命令 */    
    }
  }
}
#endif

/*********************************************************************
 */
