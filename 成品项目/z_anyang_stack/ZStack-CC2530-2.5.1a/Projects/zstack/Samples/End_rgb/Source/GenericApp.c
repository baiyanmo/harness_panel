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
  PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
  本应用程序不执行任何实际功能，仅作为应用程序结构的简单示例。

  本应用每隔5秒向另一个"Generic"应用发送"Hello World"消息，
  同时也接收来自其他设备的"Hello World"消息。

  "Hello World"消息以MSG类型消息收发。

  本应用没有Profile，因此所有处理都由自身直接完成。

  按键控制：
    SW1:
    SW2:  发起终端设备绑定
    SW3:
    SW4:  发起匹配描述符请求
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
#include "string.h"
#include "rgb.h"
#include "su03t.h"
#include "MT.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* 硬件抽象层 */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
#include "MT_UART.h"

/* 实时操作系统 */
#if defined( IAR_ARMCM3_LM )
#include "RTOS_App.h"
#endif



uint16 Led_Time = 500;//LED闪烁时间片(毫秒)

struct NODEX
{
  uint16 node_addr;//远程节点的地址
  char msg[64]; //远程节点要发送的数据信息
}node1 = {.msg = "Welcome to China!"};


//传感器数据结构体（紧凑对齐，避免两端字节偏移不一致）
#pragma pack(1)
struct tagSensor_Nodex
{
   uint8 rgb_state; //rgb灯的当前状态
};
struct tagNodex //温湿度计
{
    uint16 nwkDevAddress; //节点地址
    struct tagSensor_Nodex sensor_val; //从设备传递来的数据信息
}nodex;
#pragma pack()

char lcd_buff[60] = {0};

/*********************************************************************
 * 宏定义
 */

/*********************************************************************
 * 常量
 */

/*********************************************************************
 * 类型定义
 */

/*********************************************************************
 * 全局变量
 */
// 应用特定的Cluster ID列表
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
  GENERICAPP_FLAGS,                 //  设备标志位
  GENERICAPP_MAX_CLUSTERS,          //  输入Cluster数量
  (cId_t *)GenericApp_ClusterList,  //  输入Cluster列表
  GENERICAPP_MAX_CLUSTERS,          //  输出Cluster数量
  (cId_t *)GenericApp_ClusterList   //  输出Cluster列表
};

// 端点/接口描述符，在这里定义，但具体赋值在GenericApp_Init()中完成。
// 另一种方式是直接在这里初始化并设为const（存放在代码空间）。
// 本示例中定义在RAM中。
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
                          // 该变量在GenericApp_Init()被调用时赋值
devStates_t GenericApp_NwkState;


byte GenericApp_TransID;  // 消息ID计数器（唯一标识）

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

/*********************************************************************
 * 网络层回调函数
 */

/*********************************************************************
 * 公共函数
 */

/*********************************************************************
 * @fn      GenericApp_Init
 *
 * @brief   通用应用任务的初始化函数。
 *          在系统初始化时被调用，应包含所有应用特定的初始化操作
 *          （如硬件初始化/配置、表初始化、上电通知等）。
 *
 * @param   task_id - OSAL分配的任务ID。该ID应用于发送消息和设置定时器。
 *
 * @return  无
 */
void GenericApp_Init( uint8 task_id )
{
  GenericApp_TaskID = task_id;
  GenericApp_NwkState = DEV_INIT;
  GenericApp_TransID = 0;

  // 设备硬件初始化可以在这里或在 main() (Zmain.c) 中添加
  // 如果硬件与应用相关，在这里添加
  // 如果是设备的其他部分，在 main() 中添加

  MT_UartInit();//串口初始化
  HalUARTWrite(0,"jfjidij\n\r",sizeof("jfjidij"));//串口测试

  RGB_Config();//RGB灯初始化
  Su03t_Config();//su-03t语音模块初始化

//  GenericApp_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
//  GenericApp_DstAddr.endPoint = 0;
//  GenericApp_DstAddr.addr.shortAddr = 0;
  GenericApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  GenericApp_DstAddr.endPoint = GENERICAPP_ENDPOINT;
  GenericApp_DstAddr.addr.shortAddr = 0;

  // 填充端点描述符
  GenericApp_epDesc.endPoint = GENERICAPP_ENDPOINT;
  GenericApp_epDesc.task_id = &GenericApp_TaskID;
  GenericApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&GenericApp_SimpleDesc;
  GenericApp_epDesc.latencyReq = noLatencyReqs;

  // 向AF层注册端点描述符
  afRegister( &GenericApp_epDesc );

  // 注册按键事件 - 本应用将处理所有按键事件
  RegisterForKeys( GenericApp_TaskID );

  // 更新显示
#if defined ( LCD_SUPPORTED )
  //注意：数据从第2行开始显示，第1行会被初始化失败信息占用，不影响
  HalLcdWriteString( "GenericApp", HAL_LCD_LINE_1 );
  HalLcdWriteString( "zhengyuntong", HAL_LCD_LINE_3 );
#endif

  ZDO_RegisterForZDOMsg( GenericApp_TaskID, End_Device_Bind_rsp );
  ZDO_RegisterForZDOMsg( GenericApp_TaskID, Match_Desc_rsp );

//启动LED灯的定时器
   osal_start_timerEx(GenericApp_TaskID,
                      GENERICAPP_LED_EVT,
                      Led_Time);

////启动传感器采集数据的定时器
//  osal_start_timerEx(GenericApp_TaskID,
//                     GENERICAPP_SENSOR_EVT,
//                     GENERICAPP_SENSOR_TIMEOUT);

#if defined( IAR_ARMCM3_LM )
  // 向RTOS任务发起者注册本任务
  RTOS_RegisterApp( task_id, GENERICAPP_RTOS_MSG_EVT );
#endif
}

/*********************************************************************
 * @fn      GenericApp_ProcessEvent
 *
 * @brief   通用应用任务的事件处理器。该函数被调用来处理任务的所有事件，
 *          事件包括定时器、消息以及任何其他用户定义的事件。
 *
 * @param   task_id  - OSAL分配的任务ID。
 * @param   events   - 待处理的事件位图，可以同时包含多个事件。
 *
 * @return  未处理的事件
 */
uint16 GenericApp_ProcessEvent( uint8 task_id, uint16 events )
{
  mtOSALSerialData_t *UartMsg; //用于保存串口接收到的串口数据消息
  afIncomingMSGPacket_t *MSGpkt;
  afDataConfirm_t *afDataConfirm;

  // 数据确认消息字段
  byte sentEP;
  ZStatus_t sentStatus;
  byte sentTransID;       // 应与发送时的值匹配
  (void)task_id;  // 故意未引用的参数

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
          // 状态为ZStatus_t类型（定义在ZComDef.h中）
          // 消息字段定义在AF.h中
          afDataConfirm = (afDataConfirm_t *)MSGpkt;
          sentEP = afDataConfirm->endpoint;
          sentStatus = afDataConfirm->hdr.status;
          sentTransID = afDataConfirm->transID;
          (void)sentEP;
          (void)sentTransID;

          // 收到确认后采取的操作
          if ( sentStatus != ZSuccess )
          {
            // 数据未送达 -- 进行错误处理
          }
          break;

        case AF_INCOMING_MSG_CMD:
          GenericApp_MessageMSGCB( MSGpkt );
          break;

        case ZDO_STATE_CHANGE:
          GenericApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
            if ((GenericApp_NwkState == DEV_END_DEVICE) )
              {
                HalLcdWriteString("End Jion Net Success",HAL_LCD_LINE_3);
                Led_Time=1000;//绿灯慢闪---节点加入网络成功

                nodex.nwkDevAddress = _NIB.nwkDevAddress;//将协议栈本地设备地址保存到信息节点的地址字段

                //显示pan_id 和 信道
                sprintf(lcd_buff,"PD:%X XD:%d",_NIB.nwkPanId,_NIB.nwkLogicalChannel);
                HalLcdWriteString(lcd_buff,HAL_LCD_LINE_4);

//                node1.node_addr = _NIB.nwkDevAddress;////将协议栈本地设备地址保存到信息节点的地址字段
//
//                //再次启动发送数据信息的定时器，每隔5s发送一次
//                osal_start_timerEx( GenericApp_TaskID,
//                                    GENERICAPP_SEND_MSG_EVT,
//                                    GENERICAPP_SEND_MSG_TIMEOUT );
              }
              else
              {
                 HalLcdWriteString("End Jion Net Failed",HAL_LCD_LINE_3);
                 Led_Time=100;//小灯快闪---节点加入网络失败
              }
          break;

          //-------------------------------------------
    case CMD_SERIAL_MSG:
              //将从串口接收的数据解析并发送给协调器的接口，处理数据的环节
              UartMsg = (mtOSALSerialData_t *)MSGpkt;//msg[0] -- 接收到的数据长度
              switch(UartMsg->msg[1])
              {
                    case 01:RGB_Mode(RGB_MODE_R);
                  nodex.sensor_val.rgb_state = RGB_MODE_R;
                  break;
                case 03:RGB_Mode(RGB_MODE_G);
                  nodex.sensor_val.rgb_state = RGB_MODE_G;
                  break;
                case 02:RGB_Mode(RGB_MODE_B);
                  nodex.sensor_val.rgb_state = RGB_MODE_B;
                  break;
                case 04:RGB_Mode(RGB_MODE_ON);
                  nodex.sensor_val.rgb_state = RGB_MODE_ON;
                  break;
                case 05:RGB_Mode(RGB_MODE_OFF);
                  nodex.sensor_val.rgb_state = RGB_MODE_OFF;
                  break;
          }
             //3、在屏幕上显示传感器采集到的信息
             sprintf(lcd_buff,"RGB_State:%d",nodex.sensor_val.rgb_state);
             HalLcdWriteString(lcd_buff,HAL_LCD_LINE_5);
             //4、将传感器信息发送给协调器
            if ( AF_DataRequest( &GenericApp_DstAddr, &GenericApp_epDesc,
                             GENERICAPP_CLUSTERID,
                             (byte)sizeof(nodex) + 1,
                             (byte *)&nodex,
                             &GenericApp_TransID,
                             AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
            {
               HalLcdWriteString( "Send OK",HAL_LCD_LINE_7);
            }
            else
            {
              HalLcdWriteString( "Send Fail",HAL_LCD_LINE_7);
            }
            break;
              //-------------------------------------------

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

  // 发送消息 - 该事件由定时器产生（在GenericApp_Init()中设置）
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

  // LED事件处理
  if ( events & GENERICAPP_LED_EVT )
  {
    // 翻转三个LED的状态
    HalLedSet(HAL_LED_1,HAL_LED_MODE_TOGGLE);
    HalLedSet(HAL_LED_2,HAL_LED_MODE_TOGGLE);
    HalLedSet(HAL_LED_3,HAL_LED_MODE_TOGGLE);

    // 重新设置LED定时器
    osal_start_timerEx( GenericApp_TaskID,
                        GENERICAPP_LED_EVT,
                        Led_Time );

    // 返回未处理的事件
    return (events ^ GENERICAPP_LED_EVT);
  }

//  //传感器事件处理
//if(events & GENERICAPP_SENSOR_EVT)
//  {
//    //  1、获取传感器采集到的数据信息
//    static uint8 t = 0;
//    static uint8 h = 0;
//    t++;
//    h++;
//    //  2、将传感器数据放入消息中
//    nodex.sensor_val.t_val = t;
//    nodex.sensor_val.h_val = h;
//    //  3、在屏幕上显示传感器采集到的信息
//    sprintf(lcd_buff,"T:%d,H:%d",nodex.sensor_val.t_val,nodex.sensor_val.h_val);
//    HalLcdWriteString(lcd_buff,HAL_LCD_LINE_5);
//    //  4、将传感器信息发送给协调器
//    if ( AF_DataRequest( &GenericApp_DstAddr, &GenericApp_epDesc,
//                     GENERICAPP_CLUSTERID,
//                     (byte)sizeof(nodex) + 1,
//                     (byte *)&nodex,
//                     &GenericApp_TransID,
//                     AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
//    {
//      // 数据请求发送成功
//       HalLcdWriteString( "Sensor Send Sucess",HAL_LCD_LINE_7);
//    }
//    else
//    {
//      // 数据请求发送失败
//      HalLcdWriteString( "Sensor Send Failed",HAL_LCD_LINE_7);
//    }
//    //  5、再次启动传感器采集数据的定时器
//     osal_start_timerEx(GenericApp_TaskID,
//                     GENERICAPP_SENSOR_EVT,
//                     GENERICAPP_SENSOR_TIMEOUT);
//    //  6、清除已处理的事件位
//    return (events ^ GENERICAPP_SENSOR_EVT);
//  }


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
 * 事件生成函数
 */

/*********************************************************************
 * @fn      GenericApp_ProcessZDOMsgs()
 *
 * @brief   处理ZDO响应消息
 *
 * @param   无
 *
 * @return  无
 */
static void GenericApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
{
  switch ( inMsg->clusterID )
  {
    case End_Device_Bind_rsp:
      if ( ZDO_ParseBindRsp( inMsg ) == ZSuccess )
      {
        // 绑定成功，点亮LED
        HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
      }
#if defined( BLINK_LEDS )
      else
      {
        // 绑定失败，闪烁LED提示
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
            // 取第一个端点，也可以遍历所有端点进行搜索
            GenericApp_DstAddr.endPoint = pRsp->epList[0];

            // 匹配成功，点亮LED
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
 * @param   shift - 是否按下Shift/Alt键（双功能键）。
 * @param   keys  - 按键事件位域。有效值：
 *                  HAL_KEY_SW_4
 *                  HAL_KEY_SW_3
 *                  HAL_KEY_SW_2
 *                  HAL_KEY_SW_1
 *
 * @return  无
 */
static void GenericApp_HandleKeys( uint8 shift, uint8 keys )
{
  zAddrType_t dstAddr;

  // Shift用于实现每个按键的双功能
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
      // SW1在本应用中未用于其他功能
#if defined( SWITCH1_BIND )
      // 可以用SW1模拟SW2的功能（用于只有一个按键的设备）
      keys |= HAL_KEY_SW_2;
#elif defined( SWITCH1_MATCH )
      // 或者用SW1模拟SW4的功能（用于只有一个按键的设备）
      keys |= HAL_KEY_SW_4;
#endif
    }

    if ( keys & HAL_KEY_SW_2 )
    {
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );

      // 向协调器发起终端设备绑定请求
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
 * 局部函数
 */

/*********************************************************************
 * @fn      GenericApp_MessageMSGCB
 *
 * @brief   数据消息处理回调函数。处理来自协调器的入站数据，
 *          根据Cluster ID执行相应操作：解析数据、控制RGB灯、
 *          LCD显示、并回复ACK确认。
 *
 * @param   pkt - 接收到的消息包
 *
 * @return  无
 */
static void GenericApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  char lcd_buff[60] = {0};

  switch ( pkt->clusterId )
  {
    case GENERICAPP_CLUSTERID:
    {
      // 获取接收到的数据长度和内容
      uint8 rxLen = pkt->cmd.DataLength;
      uint8 *rxData = pkt->cmd.Data;

      // ---------- 1. 判断是否为字符串数据（如协调器回复的"ACK"） ----------
      if ( rxLen > 0 && rxData[0] != '{' )
      {
        // 协调器发来的是字符串（例如 "ACK"、"red on" 等）
        // 在LCD第6行显示收到的字符串
        HalLcdWriteString((char *)rxData, HAL_LCD_LINE_6);

        // 如果协调器发来的是RGB控制指令字符串，则解析并执行
        // 支持两种格式：STM32的"on1"/"on2"/"on3" 和 协调器的"red on"/"green on"/"blue on"
        if ( strcmp((char *)rxData, "on1") == 0 ||
             strcmp((char *)rxData, "red on") == 0 )
        {
          RGB_Mode(RGB_MODE_R);
          nodex.sensor_val.rgb_state = RGB_MODE_R;
        }
        else if ( strcmp((char *)rxData, "on2") == 0 ||
                  strcmp((char *)rxData, "green on") == 0 )
        {
          RGB_Mode(RGB_MODE_G);
          nodex.sensor_val.rgb_state = RGB_MODE_G;
        }
        else if ( strcmp((char *)rxData, "on3") == 0 ||
                  strcmp((char *)rxData, "blue on") == 0 )
        {
          RGB_Mode(RGB_MODE_B);
          nodex.sensor_val.rgb_state = RGB_MODE_B;
        }
        else if ( strcmp((char *)rxData, "off") == 0 ||
                  strcmp((char *)rxData, "OFF") == 0 )
        {
          RGB_Mode(RGB_MODE_OFF);
          nodex.sensor_val.rgb_state = RGB_MODE_OFF;
        }
      }
      // ---------- 2. 判断是否为结构体数据（nodex格式） ----------
      else if ( rxLen >= sizeof(struct tagNodex) )
      {
        // 协调器发来的是结构体数据，进行解析
        struct tagNodex *pRemote = (struct tagNodex *)rxData;

        // 根据协调器下发的rgb_state控制本地RGB灯
        RGB_Mode(pRemote->sensor_val.rgb_state);
        nodex.sensor_val.rgb_state = pRemote->sensor_val.rgb_state;

        // LCD显示收到的节点地址和状态
        sprintf(lcd_buff, "Coor->%X s:%d", pRemote->nwkDevAddress,
                                            pRemote->sensor_val.rgb_state);
        HalLcdWriteString(lcd_buff, HAL_LCD_LINE_6);
      }

      // ---------- 3. LCD更新RGB当前状态 ----------
      sprintf(lcd_buff, "RGB:%d", nodex.sensor_val.rgb_state);
      HalLcdWriteString(lcd_buff, HAL_LCD_LINE_7);

      // ---------- 4. 向协调器回复ACK确认 ----------
      if ( AF_DataRequest( &GenericApp_DstAddr, &GenericApp_epDesc,
                           GENERICAPP_CLUSTERID,
                           (byte)(osal_strlen("ACK") + 1),
                           (byte *)"ACK",
                           &GenericApp_TransID,
                           AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
      {
        HalLcdWriteString("ACK Sent", HAL_LCD_LINE_5);
      }
      else
      {
        HalLcdWriteString("ACK Failed", HAL_LCD_LINE_5);
      }
      break;
    }

    default:
      break;
  }
}

/*********************************************************************
 * @fn      GenericApp_SendTheMessage
 *
 * @brief   发送消息
 *
 * @param   无
 *
 * @return  无
 */
static void GenericApp_SendTheMessage( void )
{
  //char theMessageData[] = "Hello World";

  if ( AF_DataRequest( &GenericApp_DstAddr, &GenericApp_epDesc,
                       GENERICAPP_CLUSTERID,
                       (byte)sizeof(node1) + 1,
                       (byte *)&node1,
                       &GenericApp_TransID,
                       AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
    // 数据请求发送成功
     HalLcdWriteString( "Send Sucess",HAL_LCD_LINE_7);
  }
  else
  {
    // 数据请求发送失败
    HalLcdWriteString( "Send Failed",HAL_LCD_LINE_7);
  }
}

#if defined( IAR_ARMCM3_LM )
/*********************************************************************
 * @fn      GenericApp_ProcessRtosMessage
 *
 * @brief   从RTOS队列接收消息并发送响应
 *
 * @param   无
 *
 * @return  无
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
                       /* 故意落入下一个case */

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
