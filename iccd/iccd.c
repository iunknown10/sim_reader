#include <string.h>
#include <inavr.h>
#include <ioavr.h>
#include "../scr/ccid.h"
#include "../3gpp-aka/aka.h"

extern USB_CCID_BUF ccid_buf;
extern int    USBwriteLen;
extern int    USBwrittenLen;
extern uchar  lastRequest;

int iccd_XfrBlockRead()
{
  int ret = 2;
  uchar *apdu = ccid_buf.Request.Data;

  if(apdu[1] == 0x88 && apdu[3] == 0x81)
  {
    //3GPP-AKA
    if(apdu[4] == 0x22)
    {
      memcpy(apdu+200,apdu+5,0x22);
      ret = run_aka(apdu+200,0x22,apdu);
    }else{
      apdu[0] = 0x6c;//wrong length -  %d expected
      apdu[1] = 0x22;
    }
  }else{
    apdu[0] = 0x6d;//INS code not supported or invalid
    apdu[1] = 0x00;
  }
  ccid_buf.Response.ResponseHeader.wLength = ret;
  ccid_buf.Response.ResponseHeader.bError = 0;
  ccid_buf.Response.ResponseHeader.bStatus = 0;
  return ret+8;
}

int iccd_process(PUSB_CCID_REQUEST pCCIDRequest)
{
  int len = 0;
  int wLength = pCCIDRequest->RequestHeader.wLength;
  uchar bRequestType = pCCIDRequest->RequestHeader.bMessageType;

  /* reset global error code */
  ccid_buf.Response.ResponseHeader.bMessageType = bRequestType;
  ccid_buf.Response.ResponseHeader.bSeq = pCCIDRequest->RequestHeader.bSeq;

  if(bRequestType == PC_to_RDR_IccPowerOn)
  {
    ccid_buf.Response.Data[0] = 0x3b;
    ccid_buf.Response.Data[0] = 0x02;
    ccid_buf.Response.Data[0] = 0x0a;
    ccid_buf.Response.Data[0] = 0x9e;
    ccid_buf.Response.ResponseHeader.wLength = 4+8;/* in order to compatible with asm version only */
    ccid_buf.Response.ResponseHeader.bError = 0;
    ccid_buf.Response.ResponseHeader.bStatus = 0;
    LED_ON;
    return 12;
  }else if(bRequestType == PC_to_RDR_IccPowerOff)
  {
    LED_OFF;
    len = 8;
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
      len = iccd_XfrBlockRead();
      lastRequest = PC_to_RDR_XfrBlockRead;
    }else if(lastRequest == PC_to_RDR_XfrBlockRead)
    {
      len = ccid_buf.Request.RequestHeader.wLength+8;
    }
  }else{
    len = -1;
  }
  return len > wLength ? wLength : len;
}



