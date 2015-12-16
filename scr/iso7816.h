/* iso7816.h 2005-10-8 */

#ifndef __iso7816_h_included__
#define __iso7816_h_included__

#define ICC_PRESENT       0x1
#define ICC_ABSENT        0x2
#define	PROCESSED_FAILED  0x40
/*(FEh) CCID timed out while talking to the ICC*/
#define	ICC_MUTE          -2

#define C7_DATA   3
#define C3_CLK    6
#define C2_RESET  4

/* PIND_bit3 indicate card present or absent */
#define CARD_STATUS   3

/* we use GPIOR0 as global status register*/
#define REVERSE_FLAG    0
#define ONE_BY_ONE_FLAG 1

#define ORC0Ax 1/* smart card clock:0 for 6M Hz,1 for 3M Hz,3 for 1.5M Hz */

#ifndef __IAR_SYSTEMS_ASM__

#ifndef uchar
#define	uchar unsigned char
#endif
#ifndef uint
#define uint  unsigned int
#endif
extern uchar OCR0Ax_FREQUENCY;
extern uchar ERROR_CODE;
extern uchar ERROR_STATUS;

extern __version_1 uchar ReverseByte(uchar ch);
extern __version_1 char SendChar(uchar ch);
extern __version_1 unsigned char GetChar();
extern __version_1 void timer1_start();
extern __version_1 void timer1_stop();
extern __version_1 void load_one_etu();
extern __version_1 void load_half_etu();
extern __version_1 void load_custom_etu();
extern __version_1 void load_timer1_count(uint n);

static inline void PowerOff()
{
  DDRD &= ~(1<<C3_CLK);
  DDRB &= ~(1<<C2_RESET);
  PORTC = (PINC&~0x1c);
}
extern void iso7816_init();
extern void icc_reset();
extern uchar read_atr(uchar *data);
extern int t0_tpdu(uchar *cmd_apdu, uint cmd_len, uchar *res_apdu);

#define LED_INIT    DDRB_Bit5 = 1
#ifndef LED_ON
#define LED_ON 	    PORTB_Bit5 = 1
#endif
#ifndef LED_OFF
#define LED_OFF     PORTB_Bit5 = 0
#endif
#ifndef LED_TOOGLE
#define LED_TOGGLE  PORTB_Bit5 = !PINB_Bit5
#endif
#endif /* __IAR_SYSTEMS_ASM__ */

#endif /*__iso7816_h_included__*/
