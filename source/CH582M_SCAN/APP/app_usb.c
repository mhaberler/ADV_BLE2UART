/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_usb.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/01/19
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "CONFIG.h"
#include "stdint.h"
#include "observer.h"
#include "app_usb.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
uint8_t DevConfig, Ready;
uint8_t SetupReqCode;
UINT16 SetupReqLen;
const uint8_t *pDescr;

#define DevEP0SIZE  0x40
#define DevEP2SIZE  0x20
// 设备描述符
const uint8_t MyDevDescr[] = { 0x12,0x01,0x10,0x01,0xFF,0x00,0x00,DevEP0SIZE,
                             0x86,0x1A,0x23,0x75,0x63,0x02,0x00,0x02,
                             0x00,0x01 };
// 配置描述符
const uint8_t MyCfgDescr[] = {   0x09,0x02,0x27,0x00,0x01,0x01,0x00,0x80,0xf0,              //配置描述符，接口描述符,端点描述符
                                 0x09,0x04,0x00,0x00,0x03,0xff,0x01,0x02,0x00,
                                 0x07,0x05,0x82,0x02,DevEP2SIZE,0x00,0x00,                        //批量上传端点
                                 0x07,0x05,0x02,0x02,DevEP2SIZE,0x00,0x00,                        //批量下传端点
                                 0x07,0x05,0x81,0x03,0x08,0x00,0x01};                       //中断上传端点
// 语言描述符
const uint8_t MyLangDescr[] = { 0x04, 0x03, 0x09, 0x04 };
// 厂家信息
const uint8_t MyManuInfo[] = { 0x0E, 0x03, 'w', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'n', 0 };
// 产品信息
const uint8_t MyProdInfo[] = { 0x0C, 0x03, 'C', 0, 'H', 0, '5', 0, '8', 0, 'x', 0 };
/*产品描述符*/
const uint8_t StrDesc[28] =
{
  0x1C,0x03,0x55,0x00,0x53,0x00,0x42,0x00,
  0x32,0x00,0x2E,0x00,0x30,0x00,0x2D,0x00,
  0x53,0x00,0x65,0x00,0x72,0x00,0x69,0x00,
  0x61,0x00,0x6C,0x00
};

const uint8_t Return1[2] = {0x31,0x00};
const uint8_t Return2[2] = {0xC3,0x00};
const uint8_t Return3[2] = {0x9F,0xEE};

/*********************************************************************
 * LOCAL VARIABLES
 */

/******** 用户自定义分配端点RAM ****************************************/
__attribute__((aligned(4)))  uint8_t EP0_Databuf[64 + 64 + 64];    //ep0(64)+ep4_out(64)+ep4_in(64)
__attribute__((aligned(4)))  uint8_t EP1_Databuf[64 + 64];    //ep1_out(64)+ep1_in(64)
__attribute__((aligned(4)))  uint8_t EP2_Databuf[64 + 64];    //ep2_out(64)+ep2_in(64)
__attribute__((aligned(4)))  uint8_t EP3_Databuf[64 + 64];    //ep3_out(64)+ep3_in(64)

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      app_usb_init
 *
 * @brief   初始化usb
 *
 * @return  none
 */
void app_usb_init()
{
    pEP0_RAM_Addr = EP0_Databuf;
    pEP1_RAM_Addr = EP1_Databuf;
    pEP2_RAM_Addr = EP2_Databuf;
    pEP3_RAM_Addr = EP3_Databuf;

    USB_DeviceInit();
    PFIC_EnableIRQ( USB_IRQn );
}

/*********************************************************************
 * @fn      USBSendData
 *
 * @brief   Send data to the host
 *
 * @return  none
 */
uint8_t USBSendData(void)
{
    if((R8_UEP2_CTRL & MASK_UEP_T_RES) == UEP_T_RES_ACK)
    {
        return FAILURE;
    }
    uint16_t len = app_drv_fifo_length(&app_tx_fifo);
    if(len) {
        if(len > DevEP2SIZE)
            len = DevEP2SIZE;
        if(app_drv_fifo_read(&app_tx_fifo, pEP2_IN_DataBuf, &len) == APP_DRV_FIFO_RESULT_SUCCESS) {
            DevEP2_IN_Deal(len);
        }
    }
    return SUCCESS;
}

/*********************************************************************
 * @fn      DevEP1_OUT_Deal
 *
 * @brief   Endpoint 1 data processing
 *
 * @return  none
 */
void DevEP1_OUT_Deal( uint8_t l )
{ /* User customizable */
}

/*********************************************************************
 * @fn      DevEP2_OUT_Deal
 *
 * @brief   Endpoint 2 data processing
 *
 * @return  none
 */
void DevEP2_OUT_Deal( uint8_t l )
{ /* User customizable */
    if(app_cmd_len == 0 && l)
    {
        if(l > APP_RX_BUFFER_LENGTH)
            app_cmd_len = APP_RX_BUFFER_LENGTH;
        else
            app_cmd_len = l;
        memcpy(app_cmd_buf, pEP2_OUT_DataBuf, app_cmd_len);
        tmos_set_event(ObserverTaskId, NEW_USBDATA_EVT);
    }
}

/*********************************************************************
 * @fn      DevEP3_OUT_Deal
 *
 * @brief   Endpoint 3 data processing
 *
 * @return  none
 */
void DevEP3_OUT_Deal( uint8_t l )
{ /* User customizable */
}

/*********************************************************************
 * @fn      DevEP4_OUT_Deal
 *
 * @brief   Endpoint 4 data processing
 *
 * @return  none
 */
void DevEP4_OUT_Deal( uint8_t l )
{ /* User customizable */
}

/*********************************************************************
 * @fn      USB_DevTransProcess
 *
 * @brief   USB Transfer processing function
 *
 * @return  none
 */
void USB_DevTransProcess( void )
{
  uint8_t len, chtype;
  uint8_t intflag, errflag = 0;

  intflag = R8_USB_INT_FG;
  if ( intflag & RB_UIF_TRANSFER )
  {
    if ( ( R8_USB_INT_ST & MASK_UIS_TOKEN ) != MASK_UIS_TOKEN )    // Non-idle
    {
      switch ( R8_USB_INT_ST & ( MASK_UIS_TOKEN | MASK_UIS_ENDP ) )
      // Analyze the operation token and endpoint number
      {
        case UIS_TOKEN_IN :
        {
          switch ( SetupReqCode )
          {
            case USB_GET_DESCRIPTOR :
              len = SetupReqLen >= DevEP0SIZE ?
                  DevEP0SIZE : SetupReqLen;    // length of this transmission
              memcpy( pEP0_DataBuf, pDescr, len ); /* Loading Upload Data */
              SetupReqLen -= len;
              pDescr += len;
              R8_UEP0_T_LEN = len;
              R8_UEP0_CTRL ^= RB_UEP_T_TOG; // Flip
              break;
            case USB_SET_ADDRESS :
              R8_USB_DEV_AD = ( R8_USB_DEV_AD & RB_UDA_GP_BIT ) | SetupReqLen;
              R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
              break;
            default :
              R8_UEP0_T_LEN = 0;  // The status stage is completed and the interrupt is completed or the zero-length data packet is forced to upload to end the control transmission.
              R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
              break;
          }
        }
          break;

        case UIS_TOKEN_OUT :
        {
          len = R8_USB_RX_LEN;
        }
          break;

        case UIS_TOKEN_OUT | 1 :
        {
          if ( R8_USB_INT_ST & RB_UIS_TOG_OK )
          { // Out-of-sync packets will be discarded
            len = R8_USB_RX_LEN;
            DevEP1_OUT_Deal( len );
          }
        }
          break;

        case UIS_TOKEN_IN | 1 :
          R8_UEP1_CTRL = ( R8_UEP1_CTRL & ~MASK_UEP_T_RES ) | UEP_T_RES_NAK;
          break;

        case UIS_TOKEN_OUT | 2 :
        {
          if ( R8_USB_INT_ST & RB_UIS_TOG_OK )
          { // Out-of-sync packets will be discarded
            len = R8_USB_RX_LEN;
            DevEP2_OUT_Deal( len );
          }
        }
          break;

        case UIS_TOKEN_IN | 2 :
          R8_UEP2_CTRL = ( R8_UEP2_CTRL & ~MASK_UEP_T_RES ) | UEP_T_RES_NAK;
          break;

        case UIS_TOKEN_OUT | 3 :
        {
          if ( R8_USB_INT_ST & RB_UIS_TOG_OK )
          { // Out-of-sync packets will be discarded
            len = R8_USB_RX_LEN;
            DevEP3_OUT_Deal( len );
          }
        }
          break;

        case UIS_TOKEN_IN | 3 :
          R8_UEP3_CTRL = ( R8_UEP3_CTRL & ~MASK_UEP_T_RES ) | UEP_T_RES_NAK;
          break;

        case UIS_TOKEN_OUT | 4 :
        {
          if ( R8_USB_INT_ST & RB_UIS_TOG_OK )
          {
            R8_UEP4_CTRL ^= RB_UEP_R_TOG;
            len = R8_USB_RX_LEN;
            DevEP4_OUT_Deal( len );
          }
        }
          break;

        case UIS_TOKEN_IN | 4 :
          R8_UEP4_CTRL ^= RB_UEP_T_TOG;
          R8_UEP4_CTRL = ( R8_UEP4_CTRL & ~MASK_UEP_T_RES ) | UEP_T_RES_NAK;
          break;

        default :
          break;
      }
      R8_USB_INT_FG = RB_UIF_TRANSFER;
    }
    if ( R8_USB_INT_ST & RB_UIS_SETUP_ACT )                  // Setup package processing
    {
      R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
      SetupReqLen = pSetupReqPak->wLength;
      SetupReqCode = pSetupReqPak->bRequest;
      chtype = pSetupReqPak->bRequestType;

      len = 0;
      errflag = 0;
      if ( ( pSetupReqPak->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD )
      {
        if( pSetupReqPak->bRequestType == 0xC0 )
        {
          if(SetupReqCode==0x5F)
          {
            pDescr = Return1;
            len = sizeof(Return1);
          }
          else if(SetupReqCode==0x95)
          {
            if((pSetupReqPak->wValue)==0x18)
            {
              pDescr = Return2;
              len = sizeof(Return2);
            }
            else if((pSetupReqPak->wValue)==0x06)
            {
              pDescr = Return3;
              len = sizeof(Return3);
            }
          }
          else
          {
            errflag = 0xFF;
          }
          memcpy(pEP0_DataBuf,pDescr,len);
        }
        else
        {
          len = 0;
        }
      }
      else /* Standard Request */
      {
        switch ( SetupReqCode )
        {
          case USB_GET_DESCRIPTOR :
          {
            switch ( ( ( pSetupReqPak->wValue ) >> 8 ) )
            {
              case USB_DESCR_TYP_DEVICE :
              {
                pDescr = MyDevDescr;
                len = sizeof(MyDevDescr);
              }
                break;

              case USB_DESCR_TYP_CONFIG :
              {
                pDescr = MyCfgDescr;
                len = sizeof(MyCfgDescr);
              }
                break;

              case USB_DESCR_TYP_REPORT :
//              {
//                if ( ( ( pSetupReqPak->wIndex ) & 0xff ) == 0 )                             //接口0报表描述符
//                {
//                  pDescr = KeyRepDesc;                                  //数据准备上传
//                  len = sizeof( KeyRepDesc );
//                }
//                else if ( ( ( pSetupReqPak->wIndex ) & 0xff ) == 1 )                        //接口1报表描述符
//                {
//                  pDescr = MouseRepDesc;                                //数据准备上传
//                  len = sizeof( MouseRepDesc );
//                  Ready = 1;                                            //如果有更多接口，该标准位应该在最后一个接口配置完成后有效
//                }
//                else
//                  len = 0xff;                                           //本程序只有2个接口，这句话正常不可能执行
//              }
                break;

              case USB_DESCR_TYP_STRING :
              {
                switch ( ( pSetupReqPak->wValue ) & 0xff )
                {
                  case 1 :
                    pDescr = MyManuInfo;
                    len = MyManuInfo[0];
                    break;
                  case 2 :
                    pDescr = StrDesc;
                    len = StrDesc[0];
                    break;
                  case 0 :
                    pDescr = MyLangDescr;
                    len = MyLangDescr[0];
                    break;
                  default :
                    errflag = 0xFF;                               // Unsupported string descriptor
                    break;
                }
              }
                break;

              default :
                errflag = 0xff;
                break;
            }
            if ( SetupReqLen > len )
              SetupReqLen = len;      // actual total length to be uploaded
            len = ( SetupReqLen >= DevEP0SIZE ) ?
                DevEP0SIZE : SetupReqLen;
            memcpy( pEP0_DataBuf, pDescr, len );
            pDescr += len;
          }
            break;

          case USB_SET_ADDRESS :
            SetupReqLen = ( pSetupReqPak->wValue ) & 0xff;
            break;

          case USB_GET_CONFIGURATION :
            pEP0_DataBuf[0] = DevConfig;
            if ( SetupReqLen > 1 )
              SetupReqLen = 1;
            break;

          case USB_SET_CONFIGURATION :
            DevConfig = ( pSetupReqPak->wValue ) & 0xff;
            break;

          case USB_CLEAR_FEATURE :
          {
            if ( ( pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )    // 端点
            {
              switch ( ( pSetupReqPak->wIndex ) & 0xff )
              {
                case 0x82 :
                  R8_UEP2_CTRL = ( R8_UEP2_CTRL & ~( RB_UEP_T_TOG | MASK_UEP_T_RES ) ) | UEP_T_RES_NAK;
                  break;
                case 0x02 :
                  R8_UEP2_CTRL = ( R8_UEP2_CTRL & ~( RB_UEP_R_TOG | MASK_UEP_R_RES ) ) | UEP_R_RES_ACK;
                  break;
                case 0x81 :
                  R8_UEP1_CTRL = ( R8_UEP1_CTRL & ~( RB_UEP_T_TOG | MASK_UEP_T_RES ) ) | UEP_T_RES_NAK;
                  break;
                case 0x01 :
                  R8_UEP1_CTRL = ( R8_UEP1_CTRL & ~( RB_UEP_R_TOG | MASK_UEP_R_RES ) ) | UEP_R_RES_ACK;
                  break;
                default :
                  errflag = 0xFF;                                 // Unsupported endpoint
                  break;
              }
            }
            else
              errflag = 0xFF;
          }
            break;

          case USB_GET_INTERFACE :
            pEP0_DataBuf[0] = 0x00;
            if ( SetupReqLen > 1 )
              SetupReqLen = 1;
            break;

          case USB_GET_STATUS :
            pEP0_DataBuf[0] = 0x00;
            pEP0_DataBuf[1] = 0x00;
            if ( SetupReqLen > 2 )
              SetupReqLen = 2;
            break;

          default :
            errflag = 0xff;
            break;
        }
      }
      if ( errflag == 0xff )        // Error or not supported
      {
//                  SetupReqCode = 0xFF;
        R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;    // STALL
      }
      else
      {
        if ( chtype & 0x80 )     // Upload
        {
          len = ( SetupReqLen > DevEP0SIZE ) ?
              DevEP0SIZE : SetupReqLen;
          SetupReqLen -= len;
        }
        else
          len = 0;        // Download
        R8_UEP0_T_LEN = len;
        R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;    // default data packet is DATA1
      }

      R8_USB_INT_FG = RB_UIF_TRANSFER;
    }
  }
  else if ( intflag & RB_UIF_BUS_RST )// USB bus reset
  {
    R8_USB_DEV_AD = 0;
    R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_USB_INT_FG = RB_UIF_BUS_RST;
//    tmos_set_event(ObserverTaskId, STOP_DISCOVERY_EVT);
  }
  else if ( intflag & RB_UIF_SUSPEND ) // USB bus suspend/wake completed
  {
    if ( R8_USB_MIS_ST & RB_UMS_SUSPEND ) // Suspend
    {
        tmos_set_event(ObserverTaskId, STOP_DISCOVERY_EVT);
    }
    else // wake
    {
      ;
    }
    R8_USB_INT_FG = RB_UIF_SUSPEND;
  }
  else
  {
    R8_USB_INT_FG = intflag;
  }
}

/*********************************************************************
 * @fn      USB_IRQHandler
 *
 * @brief   USB中断函数
 *
 * @return  none
 */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void USB_IRQHandler( void ) /* USB中断服务程序,使用寄存器组1 */
{
  USB_DevTransProcess();
}


/*********************************************************************
*********************************************************************/
