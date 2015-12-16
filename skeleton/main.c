
#include <ioavr.h>

extern int callCount;

#pragma vector=INT0_vect
__interrupt void irqHandler(void)
{
  ++callCount;
}

unsigned usbCrc16(unsigned char *data, unsigned char len)
{

  return *data+len;
}

__version_1 char SendChar(unsigned char *data)
{
  return 0;
}

__version_1 unsigned char GetChar()
{
  return 0;
}
