/*
  USB SmartCard reader with built in 3gpp-aka
  yanglq 2005-10-5
*/
#include <string.h>
#include <inavr.h>
#include <ioavr.h>
#include "usbdrv/usbdrv.h"
#include "scr/ccid.h"
#include "scr/iso7816.h"

int    USBwriteLen;
int    USBwrittenLen;
uchar  lastRequest;

uchar usbFunctionSetup(uchar data[8])
{
  int len = 0;

  if(data[1] > 0x60)
  {
    if(data[5] == 1)
    {
      len = iccd_process((PUSB_CCID_REQUEST)data);
      usbMsgPtr = (uchar *)&ccid_buf;
    }else{
      len = ccid_process((PUSB_CCID_REQUEST)data);
      usbMsgPtr = (uchar *)&ccid_buf;
    }
  }else{
    len = -1;
  }
  return len;
}

uchar usbFunctionRead(uchar *data, uchar len)
{
  return 0xff;
}

uchar usbFunctionWrite(uchar *data, uchar len)
{
  uchar i;
  if((lastRequest != PC_to_RDR_XfrBlock) && (lastRequest != PC_to_RDR_SetParameters))
    return 0xff;

  uchar *dst = ccid_buf.Request.Data+USBwrittenLen;
  for(i=0; i<len; i++)
  {
    *dst++ = *data++;
  }
  USBwrittenLen += len;
  if((lastRequest == PC_to_RDR_SetParameters) && (USBwrittenLen == USBwriteLen))
  {
    ccid_SetParameters(USBwrittenLen);
  }

  return 0;
}
/* for test only */
extern void test_aka();
extern char test_iso7816();
void main(void)
{
   /* test start */
  //test_aka();
  //if(test_iso7816())
  //{//test iso7816 error.
  //    LED_ON;
  //}
  /* test end*/
  iso7816_init();
  DDRC_Bit1 = 1;
  PORTC_Bit1 = 1;            /* pull up D- resistor */
  usbInit();
  __enable_interrupt();
  for(;;){	        /* main event loop */
    usbPoll();
  }
 }
