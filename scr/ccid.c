#include <inavr.h>
#include <ioavr.h>
#include "iso7816.h"
#include "ccid.h"

USB_CCID_BUF ccid_buf;

uchar ccid_PowerOn()
{
  uchar ret;
  icc_reset();
  ret = read_atr(ccid_buf.Response.Data);
  ccid_buf.Response.ResponseHeader.wLength = ret+8;/* in order to compatible with asm version only */
  ccid_buf.Response.ResponseHeader.bError = ERROR_CODE;
  ccid_buf.Response.ResponseHeader.bStatus = ERROR_STATUS;
  LED_ON;
  return ret+8;
}

uchar ccid_PowerOff()
{
  PowerOff();
  LED_OFF;
  ccid_buf.Response.ResponseHeader.wLength = 0;
  ccid_buf.Response.ResponseHeader.bError = 0;
  ccid_buf.Response.ResponseHeader.bStatus = 0;
  return 8;
}

int ccid_XfrBlock()
{
  return 0;
}

int ccid_XfrBlockRead()
{
  int ret;
  ret = t0_tpdu(ccid_buf.Request.Data,ccid_buf.Request.RequestHeader.wLength,ccid_buf.Response.Data);
  ccid_buf.Response.ResponseHeader.wLength = ret;
  ccid_buf.Response.ResponseHeader.bError = ERROR_CODE;
  ccid_buf.Response.ResponseHeader.bStatus = ERROR_STATUS;
  return ret+8;
}

uchar ccid_GetSlotStatus()
{
  if(PIND & (1<<CARD_STATUS))
    ccid_buf.Response.ResponseHeader.bStatus = ICC_PRESENT;
  else
     ccid_buf.Response.ResponseHeader.bStatus = ICC_ABSENT;
  ccid_buf.Response.ResponseHeader.wLength = 0;
  ccid_buf.Response.ResponseHeader.bError = 0;
  return 8;
}

uchar ccid_SetParameters(uchar n)
{
  uchar *dst = &OCR0Ax_FREQUENCY;
  uchar *src = ccid_buf.Request.Data;
  do
  {
    *dst++ = *src++;
  }while(--n != 0);

  OCR0A = OCR0Ax_FREQUENCY;
  load_one_etu();
  return 0;
}

uchar ccid_GetParameters()
{
  uchar *src = &OCR0Ax_FREQUENCY;
  uchar *dst = ccid_buf.Response.Data;
  uchar n = &ERROR_CODE - src;
  uchar ret = n;

  while(n--)
  {
    *dst++ = *src++;
  }
  ccid_buf.Response.ResponseHeader.wLength = ret;
  ccid_buf.Response.ResponseHeader.bError = ERROR_CODE;
  ccid_buf.Response.ResponseHeader.bStatus = ERROR_STATUS;
  return ret+8;
}

extern int    USBwriteLen;
extern int    USBwrittenLen;
extern uchar  lastRequest;
int ccid_process(PUSB_CCID_REQUEST pCCIDRequest)
{
  int len = 0;
  int wLength = pCCIDRequest->RequestHeader.wLength;
  uchar bRequestType = pCCIDRequest->RequestHeader.bMessageType;

  /* reset global error code */
  ERROR_CODE = 0;
  ERROR_STATUS = 0;
  ccid_buf.Response.ResponseHeader.bMessageType = bRequestType;
  ccid_buf.Response.ResponseHeader.bSeq = pCCIDRequest->RequestHeader.bSeq;

  if(bRequestType == PC_to_RDR_IccPowerOn)
  {
    len = ccid_PowerOn();
  }else if(bRequestType == PC_to_RDR_IccPowerOff)
  {
    len = ccid_PowerOff();
  }else if(bRequestType == PC_to_RDR_XfrBlock)
  {
    USBwriteLen = wLength;
    ccid_buf.Request.RequestHeader.wLength = wLength;
    USBwrittenLen = 0;
    lastRequest = PC_to_RDR_XfrBlock;
    len = -1;
  }else if(bRequestType == PC_to_RDR_XfrBlockRead)
  {
    if((lastRequest == PC_to_RDR_XfrBlock) && (USBwriteLen == USBwrittenLen))
    {
      LED_TOGGLE;
      __disable_interrupt(); /* FIXME: should disable INT0 only */
      len = ccid_XfrBlockRead();
      /* clear the INT0 pending bit */
      EIFR |= (1<<INTF0);
      __enable_interrupt();
      lastRequest = PC_to_RDR_XfrBlockRead;
    }else if(lastRequest == PC_to_RDR_XfrBlockRead)
    {
      len = ccid_buf.Request.RequestHeader.wLength+8;
    }
  }else if(bRequestType == PC_to_RDR_GetSlotStatus)
  {
    len = ccid_GetSlotStatus();
  }else if(bRequestType == PC_to_RDR_GetParameters)
  {
    len = ccid_GetParameters();
  }else if(bRequestType == PC_to_RDR_SetParameters)
  {
    USBwriteLen = wLength;
    USBwrittenLen = 0;
    lastRequest = PC_to_RDR_SetParameters;
    len = -1;
  }else{
    len = -1;
  }
  return len > wLength ? wLength : len;
}




