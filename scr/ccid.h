/* ccid.h 2005-10-11 */

#ifndef __ccid_h_included__
#define __ccid_h_included__
#ifndef uchar
#define	uchar unsigned char
#endif
#ifndef uint
#define uint  unsigned int
#endif

#ifndef LED_INIT
#define LED_INIT    DDRB_Bit5 = 1
#endif
#ifndef LED_ON
#define LED_ON 	    PORTB_Bit5 = 1
#endif
#ifndef LED_OFF
#define LED_OFF     PORTB_Bit5 = 0
#endif
#ifndef LED_TOOGLE
#define LED_TOGGLE  PORTB_Bit5 = !PINB_Bit5
#endif

#define PC_to_RDR_IccPowerOn		0x62
#define PC_to_RDR_IccPowerOff		0x63
#define PC_to_RDR_GetSlotStatus		0x65
#define PC_to_RDR_XfrBlock		0x6F
#define PC_to_RDR_XfrBlockRead 		0x70
#define PC_to_RDR_GetParameters 	0x6C
#define PC_to_RDR_ResetParameters 	0x6D
#define PC_to_RDR_SetParameters 	0x61

typedef unsigned char 	UCHAR;
typedef unsigned int 	USHORT;

typedef struct _USB_CCID_REQUEST {
  union{
    struct {
      UCHAR bmRequest;
      UCHAR bRequest;
      USHORT wValue;
      USHORT wIndex;
      USHORT wLength;
    }SetupPacket;
    struct{
      UCHAR bmRequest;
      UCHAR bMessageType;
      UCHAR bSeq;
      UCHAR abMspec[3];
      USHORT wLength;
    }RequestHeader;
  };
  UCHAR Data[0];
} USB_CCID_REQUEST, *PUSB_CCID_REQUEST;	

typedef struct _USB_CCID_RESPONSE {
  struct {
    UCHAR bmRequestType;
    UCHAR bMessageType;
    UCHAR bSeq;
    UCHAR bStatus;
    UCHAR bError;
    UCHAR bMspec;
    USHORT wLength;
  }ResponseHeader;
  UCHAR Data[263];
} USB_CCID_RESPONSE, *PUSB_CCID_RESPONSE;

typedef struct _USB_CCID_BUF{
  union {
    USB_CCID_REQUEST  Request;
    USB_CCID_RESPONSE Response;
  };
} USB_CCID_BUF, *PUSB_CCID_BUF;

extern USB_CCID_BUF ccid_buf;
extern uchar ccid_PowerOn();
extern uchar ccid_PowerOff();
extern int ccid_XfrBlock();
extern int ccid_XfrBlockRead();
extern uchar ccid_GetSlotStatus();
extern uchar ccid_SetParameters(uchar n);
extern uchar ccid_GetParameters();
extern int ccid_process(PUSB_CCID_REQUEST pCCIDRequest);
extern int iccd_process(PUSB_CCID_REQUEST pCCIDRequest);
#endif
