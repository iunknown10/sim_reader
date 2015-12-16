/* iso7816.c 2005-10-8 */

#include <ioavr.h>
#include "iso7816.h"

#define WAIT_TIME_FLAG        while(TIFR1_Bit1 == 0){}
#define TIMER_COMP_FLAG_CLR   TIFR1_Bit1 = 0x01

/* variable from OCR0Ax_FREQUENCY to ERROR_CODE must be continuous in memory */
uchar OCR0Ax_FREQUENCY = ORC0Ax;
uchar TIMEOUT_COUNTE = 100;
uchar TIMEOUT_COUNTL = (60000&0xff);
uchar TIMEOUT_COUNTH = (60000>>8);
uchar RESEND_TIMES = 2;
uchar TMR1_VOLATL = (124*(ORC0Ax+1)&0xff);
uchar TMR1_VOLATH = 1;/* (124*(ORC0Ax+1)>>8); hacking for these variables continuous store*/
uchar TMR1_0P5ETUL = (372*(ORC0Ax+1)&0xff);
uchar TMR1_0P5ETUH = (372*(ORC0Ax+1)>>8);
uchar TMR1_1P0ETUL = (744*(ORC0Ax+1)&0xff);
uchar TMR1_1P0ETUH = (744*(ORC0Ax+1)>>8);
uchar ERROR_CODE = 1;/* hacking for these variables continuous store*/
uchar ERROR_STATUS;

uchar skipProcedureNull();
static uchar iso7816_getbytes(uchar *data, uchar len)
{
  uchar i = len;

  while(i--)
  {
    *data++ = GetChar();
    if(ERROR_STATUS != 0)
      return 1;
  }
  return 0;
}

static uchar iso7816_sendbytes(uchar *data, uchar len, uchar cmd)
{
  uchar i = len;
  uchar proc;
  uchar ch;

  while(i--)
  {
    ch = *data++;
    if(SendChar(ch) != ch)
      return 1;
    if(GPIOR0 & (1<<ONE_BY_ONE_FLAG))
    {
      proc = skipProcedureNull();
      if(proc == cmd)
        GPIOR0 &= ~(1<<ONE_BY_ONE_FLAG);
    }
  }
  return 0;
}

static inline void PowerPinInit()
{
  /* Power Pin init*/

  DDRC |= 0x1c;
  DDRD &= ~(1<<C3_CLK);
  DDRB &= ~(1<<C2_RESET);
}

static inline void PowerOn()
{
  PORTC = (PINC|0x1c);
  DDRD |= (1<<C3_CLK);
  DDRB |= (1<<C2_RESET);
}

__version_1 void load_timer1_count(uint n)
{
  TCNT1H = (n>>8);
  TCNT1L = (n&0xff);
}

void iso7816_init()
{
  TMR1_VOLATH = (124*(ORC0Ax+1)>>8);/* restore the value.;hacking for these variables continuous store*/
  /* set led pin direction out */
  LED_INIT;

  /* enable INT1 interrupt */
  EICRA_Bit3 = 1; EICRA_Bit2 = 0; /* falling edge */
  EIMSK_Bit1 = 1;

  PORTB |= (1<<C2_RESET);
  PowerPinInit();

  /* timer1 related stuff */
  TCCR1A = 0;
  timer1_stop();

  /* timer0 related stuff */
  TCCR0B = 0;
  OCR0A = ORC0Ax;
  TCCR0A = 0x42;
  TCCR0B = 0x01;
}

void icc_reset()
{
  GPIOR0 = 0;
  PowerOn();

  PORTB &= ~(1<<C2_RESET);

  load_timer1_count(14880);
  TIMER_COMP_FLAG_CLR;
  timer1_start();
  WAIT_TIME_FLAG;
  timer1_stop();

  load_one_etu();
  TIMER_COMP_FLAG_CLR;

  PORTB |= (1<<C2_RESET);
}

uchar read_atr(uchar *data)
{
  unsigned char ret,tmp_len,tmpchar,	historical_len;
  unsigned char have_Check = 0;

  tmp_len = RESEND_TIMES;
  RESEND_TIMES = 0;
  tmpchar = GetChar();
  RESEND_TIMES = tmp_len;
  if(tmpchar == 0x03)
    GPIOR0 |= (1<<REVERSE_FLAG);
  else if(tmpchar != 0x3b)
    return 0;
  ret = 2;
  data[0] = tmpchar;
  tmpchar = data[1] = GetChar();
  if(GPIOR0 & (1<<REVERSE_FLAG))
    tmpchar = ReverseByte(tmpchar);
  historical_len = (tmpchar & 0xf);
  while(tmpchar & 0x80)
  {
    tmp_len = ((tmpchar >> 4) & 1) + ((tmpchar >> 5) & 1) + ((tmpchar >> 6) & 1) + ((tmpchar >> 7) & 1);
    iso7816_getbytes(&(data[ret]),tmp_len);
    ret += tmp_len;
    tmpchar = data[ret-1];//must be TDi
    if(GPIOR0 & (1<<REVERSE_FLAG))
      tmpchar = ReverseByte(tmpchar);
    if(tmpchar&0xf)
      have_Check = 1;
  }
  tmp_len = ((tmpchar >> 4) & 1) + ((tmpchar >> 5) & 1) + ((tmpchar >> 6) & 1) + historical_len + have_Check;
  iso7816_getbytes(&(data[ret]),tmp_len);
  return ret + tmp_len;
}

uchar skipProcedureNull()
{
  uchar proc;
  uchar nullbyte;

  if(GPIOR0 & (1<<REVERSE_FLAG))
    nullbyte = 0xf9;
  else
    nullbyte = 0x60;
  do{
    proc = GetChar();
  }while(proc == nullbyte);
  return proc;
}

int t0_tpdu(uchar *cmd_apdu, uint cmd_len, uchar *res_apdu)
{
  uchar proc;
  if(*cmd_apdu == 0xff)/* is PTS*/
  {
    if(iso7816_sendbytes(cmd_apdu,cmd_len,0))
     goto error_end;
    if(iso7816_getbytes(res_apdu,cmd_len))
     goto error_end;
    return cmd_len;
  }else if(cmd_len == 5)/* read data from smart card */
  {
    if(iso7816_sendbytes(cmd_apdu,5,0))
     goto error_end;
    proc = skipProcedureNull();
    if( (proc == cmd_apdu[1])
        || (proc == (cmd_apdu[1]+1))
        || (proc == (uchar)~cmd_apdu[1])
        || (proc == (uchar)~(cmd_apdu[1]+1)) )
    {
        uchar res_len = cmd_apdu[4];
        if(GPIOR0 & (1<<REVERSE_FLAG))
          res_len = ReverseByte(res_len);
        res_len+=2;
        if(iso7816_getbytes(res_apdu,res_len))
          goto error_end;
        return res_len;
    }else{/* is SW1 */
      res_apdu[0] = proc;
      res_apdu[1] = GetChar();
      return 2;
    }
  }else if(cmd_len > 5)/* write data to smart card */
  {
      if(iso7816_sendbytes(cmd_apdu,5,0))
        goto error_end;
      proc = skipProcedureNull();
      if(proc == (uchar)~cmd_apdu[1]
         || proc == (uchar)~(cmd_apdu[1]+1)
         || proc == cmd_apdu[1]
         || proc == (cmd_apdu[1]+1))
      {
        if(proc == (uchar)~cmd_apdu[1] || proc == (uchar)~(cmd_apdu[1]+1))
        {
          /* Next data byte is transferred subsequently */
          GPIOR0 |= (1<<ONE_BY_ONE_FLAG);
        }else
        {/* All remaining data bytes are transferred subsequently */
          GPIOR0 &= ~(1<<ONE_BY_ONE_FLAG);
        }
        uchar data_len = cmd_apdu[4];
        if(GPIOR0 & (1<<REVERSE_FLAG))
          data_len = ReverseByte(data_len);
        if(iso7816_sendbytes(cmd_apdu+5,data_len,cmd_apdu[1]))
          goto error_end;
        /* Get Status Word */
        proc = skipProcedureNull();
        res_apdu[0] = proc;
        res_apdu[1] = GetChar();
        return 2;
      }else{/* is SW1 */
        res_apdu[0] = proc;
        res_apdu[1] = GetChar();
        return 2;
    }
  }
error_end:
  return 0;
}


#pragma vector=INT1_vect
__interrupt void INT1Handler(void)
{
  PowerOff();
  LED_OFF;
}

#include <string.h>
uchar GEM12_atr[22] = {0x3B,0x9F,0x95,0x80,0x1F,0xC3,0x80,0x31,0xE0,0x73,0xFE,
                        0x21,0x1B,0xB3,0xE2,0x01,0x74,0x83,0x0F,0x90,0x00,0x88};
uchar cmd_select_mf[] = {0xa0,0xa4,0x00,0x00,0x02,0x3f,0x00};
uchar cmd_get_res[] = {0xa0,0xc0,0x00,0x00,0x16};
uchar atr[33];
uchar ret_atr;

char test_iso7816()
{
  char ret;
  iso7816_init();
  icc_reset();
  ret_atr = read_atr(atr);
  ret = memcmp(atr,GEM12_atr,22);
  if(ret)
    return ret;
  if(t0_tpdu(cmd_select_mf,7,atr) == 0)
    return (char)-1;
  if(atr[0] != 0x9f || atr[1] != 0x16)
    return (char)-1;
  if(t0_tpdu(cmd_get_res,5,atr) != 0x18)
    return (char)-1;
  return 0;
}








