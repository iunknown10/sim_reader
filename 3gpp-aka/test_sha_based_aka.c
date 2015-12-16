/* test_sha_based_aka.c */

#include <stdio.h>
#include <string.h>
#include "aka.h"

static uchar K[]={0xad,0x1b,0x5a,0x15,0x9b,0xe8,0x6b,0x2c,
		   0xa6,0x6c,0x7a,0xe4,0x0b,0xba,0x9b,0x9d};
static uchar Fmk[L_FMK] = { 'A', 'H', 'A', 'G' };
__eeprom volatile uchar SQNms[L_SQN]={0x00,0x00,0x00,0x00,0x00,0x00};
static uchar fi0=0x41;
static uchar fi1=0x42;
static uchar fi1star=0x43;
static uchar fi2=0x44;
static uchar fi3=0x45;
static uchar fi4=0x46;
static uchar fi5=0x47;
static uchar fi5star=0x48;
__eeprom volatile unsigned char ee_var = 0xff;

void test_eeprom()
{
  unsigned char temp;

  temp = ee_var;
  PORTB = temp;
}

uchar run_aka(uchar *rand_autn, uchar len, uchar *res)
{
  uchar AK[L_AK];
  uchar SQN[L_SQN];
  uchar XMAC[L_MACA];
  uchar *RAND = &rand_autn[1];
  uchar *AMF = &rand_autn[1+16+1+6];
  uchar *MAC = &rand_autn[1+16+1+6+2];
  int i;

  f5(K,fi5,RAND,Fmk,AK);
  for(i=0; i<L_SQN; i++)
  {
    SQN[i] = AK[i] ^ rand_autn[i+18];
  }
  f1(K,fi1,RAND,Fmk,SQN,AMF,XMAC);
  //verify MAC
  if(memcmp(XMAC,MAC,8))
  {//"MAC != XMAC"
    *res++ = 0x98;
    *res++ = 0x62;
    return 2;
  }else{//MAC ok
    //Verify that SQN is in the correct range
    for(i=0; i<L_SQN; i++)
    {
      if(SQN[i] - SQNms[i] == 1)
      {
        for(i=i+1; i<L_SQN; i++)
        {
          if(SQNms[i] - SQN[i] != 0xff)
            goto resync;
        }
        //if SQN - SQNms == 1, i think SQN is correct
        for(i=0; i<L_SQN; i++)
          SQNms[i] = SQN[i];
        *res++ = 0xdb;
        *res++ = 16;
        f2(K,fi2,RAND,Fmk,res,L_RES);
        res += 16;
        *res++ = 16;
	f3(K,fi3,RAND,Fmk,res);
        res += 16;
        *res++ = 16;
	f4(K,fi4,RAND,Fmk,res);
        return 52;
      }
    }
resync:
    *res++ = 0xdc;
    *res++ = 14;
    f5star(K,fi5star,RAND,Fmk,AK);
    for(i=0; i<L_SQN; i++)
    {
      *res++ = SQNms[i] ^ AK[i];
      SQN[i] = SQNms[i];
    }
    f1star(K,fi1star,RAND,Fmk,SQN,AMF,XMAC);
    for(i=0; i<8; i++)
      *res++ = XMAC[i];
    return 16;
  }
}

//#define DEBUG
#ifdef DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif
unsigned char *msg_data =
	"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

void pause(void)
{
	dbg_printf("Press any key to continue\n");
#ifdef DEBUG
	getchar();
#endif
}


void test_aka()
{
        uchar seed[]={0xb0,0xab,0xb9,0x9d,0x6a,0xc6,0xa7,0x4e,
			0xb9,0x8e,0xb6,0xc2,0xda,0xb1,0xa5,0x51};

	uchar RAND[L_RAND];

	uchar CK[L_CK];
	uchar IK[L_IK];
        uchar UAK[L_UAK];

	uchar MACA[L_MACA];
	uchar MACS[L_MACS];
	uchar RES[L_RES];
	uchar AK[L_AK];
	uchar AKS[L_AKS];

	uchar SQN[L_SQN]={0x00,0x00,0x00,0x00,0x00,0x01};
	uchar fi11;

	uchar buff1[L_RAND/2],buff2[L_RAND/2];

	uchar AMF[2];
        uchar RAND_AUTN[34];
	int i;

        fi11 = 0x50;
	dbg_printf("test vector for f0:\n");
	dbg_printf("input section\n");
	dbg_printf("seed is:	");
	for(i=0;i<L_KEY;i++)
		dbg_printf("%02x ",seed[i]);
	dbg_printf("\n");
	dbg_printf("fi0 is:		%02x\n",fi0);
	dbg_printf("Fmk is:		");
	for (i=0;i<L_FMK;i++)
		dbg_printf("%02x ",Fmk[i]);
	dbg_printf("\n");

	dbg_printf("\n");

	f0(seed,fi0,Fmk,buff1);
	f0(seed,fi0,Fmk,buff2);

	dbg_printf("output section\n");
	dbg_printf("f0 	RAND:	");
	for (i=0;i<L_RAND/2;i++)
		dbg_printf("%02x ",buff1[i]);
	for (i=0;i<L_RAND/2;i++)
		dbg_printf("%02x ",buff2[i]);
	dbg_printf("\n");

	pause();

	/* reuse RAND generated for the subsequent function calls*/
	for (i=0;i<L_RAND/2;i++)
		RAND[i] = buff1[i];
	for (i=0;i<L_RAND/2;i++)
		RAND[i+L_RAND/2] = buff2[i];

	AMF[0]=0x00;
	AMF[1]=0x01;

	dbg_printf("\n");
	dbg_printf("\n");
	dbg_printf("test vector for f1:\n");
	dbg_printf("input section\n");
	dbg_printf("K is:		");
	for(i=0;i<L_KEY;i++)
		dbg_printf("%02x ",K[i]);
	dbg_printf("\n");
	dbg_printf("fi1 is:		%02x\n",fi1);
	dbg_printf("RAND is:	");
	for(i=0;i<L_RAND;i++)
		dbg_printf("%02x ",RAND[i]);
	dbg_printf("\n");
	dbg_printf("Fmk is:		");
	for(i=0;i<L_FMK;i++)
		dbg_printf("%02x ",Fmk[i]);
	dbg_printf("\n");
	dbg_printf("SQN is:		");
	for(i=0;i<L_SQN;i++)
		dbg_printf("%02x ",SQN[i]);
	dbg_printf("\n");
	dbg_printf("AMF is:		%02x %02x\n",AMF[0],AMF[1]);

	f1(K,fi1,RAND,Fmk,SQN,AMF,MACA);
	dbg_printf("\n");
	dbg_printf("output section\n");
	dbg_printf("f1 	MACA:	");
	for (i=0;i<L_MACA;i++)
		dbg_printf("%02x ",MACA[i]);
	dbg_printf("\n");

	pause();

	dbg_printf("\n");
	dbg_printf("\n");
	dbg_printf("test vector for f5\n");
	dbg_printf("input section\n");
	dbg_printf("K is:		");
	for(i=0;i<L_KEY;i++)
		dbg_printf("%02x ",K[i]);
	dbg_printf("\n");
	dbg_printf("fi5 is:		%02x\n",fi5);
	dbg_printf("RAND is:	");
	for(i=0;i<L_RAND;i++)
		dbg_printf("%02x ",RAND[i]);
	dbg_printf("\n");
	dbg_printf("Fmk is:		");
	for(i=0;i<L_FMK;i++)
		dbg_printf("%02x ",Fmk[i]);
	dbg_printf("\n");
	dbg_printf("\n");
	f5(K,fi5,RAND,Fmk,AK);
	dbg_printf("output section\n");
	dbg_printf("f5 	AK:	");
	for (i=0;i<L_AK;i++)
		dbg_printf("%02x ",AK[i]);
	dbg_printf("\n");
        RAND_AUTN[0] = 16;
        for(i=0; i<16; i++)
          RAND_AUTN[1+i] = RAND[i];
        RAND_AUTN[17] = 16;
        for(i=0; i<6; i++)
          RAND_AUTN[18+i] = SQN[i] ^ AK[i];
        RAND_AUTN[24] = AMF[0];
        RAND_AUTN[25] = AMF[1];
        for(i=0; i<8; i++)
          RAND_AUTN[26+i] = MACA[i];
        run_aka(RAND_AUTN,34,RAND_AUTN);

	pause();
	dbg_printf("\n");
	dbg_printf("\n");
	dbg_printf("test vector for f1*:\n");
	dbg_printf("input section\n");
	dbg_printf("K is:		");
	for(i=0;i<L_KEY;i++)
		dbg_printf("%02x ",K[i]);
	dbg_printf("\n");
	dbg_printf("fi1star is:	%02x\n",fi1star);
	dbg_printf("RAND is:	");
	for(i=0;i<L_RAND;i++)
		dbg_printf("%02x ",RAND[i]);
	dbg_printf("\n");
	dbg_printf("Fmk is:		");
	for(i=0;i<L_FMK;i++)
		dbg_printf("%02x ",Fmk[i]);
	dbg_printf("\n");
	dbg_printf("SQN is:		");
	for(i=0;i<L_SQN;i++)
		dbg_printf("%02x ",SQN[i]);
	dbg_printf("\n");
	dbg_printf("AMF is:		%02x %02x\n",AMF[0],AMF[1]);

	dbg_printf("\n");
	f1star(K,fi1star,RAND,Fmk,SQN,AMF,MACS);
	dbg_printf("output section\n");
	dbg_printf("f1* 	MACS:	");
	for(i=0;i<L_MACS;i++)
		dbg_printf("%02x ",MACS[i]);
	for(i=0;i<L_SQN;i++)
		SQN[i]=0x00;
	AMF[0]=AMF[1]=0;
	dbg_printf("\n");

	pause();

	dbg_printf("\n");
	dbg_printf("\n");
	dbg_printf("test vector for f2:\n");
	dbg_printf("input section\n");
	dbg_printf("K is:		");
	for(i=0;i<L_KEY;i++)
		dbg_printf("%02x ",K[i]);
	dbg_printf("\n");
	dbg_printf("fi2 is:		%02x\n",fi2);
	dbg_printf("RAND is:	");
	for(i=0;i<L_RAND;i++)
		dbg_printf("%02x ",RAND[i]);
	dbg_printf("\n");
	dbg_printf("Fmk is:		");
	for(i=0;i<L_FMK;i++)
		dbg_printf("%02x ",Fmk[i]);
	dbg_printf("\n");
	dbg_printf("\n");
	f2(K,fi2,RAND,Fmk,RES,L_RES);
	dbg_printf("output section\n");
	dbg_printf("f2 	RES:	");
	for (i=0;i<L_RES;i++)
		dbg_printf("%02x ",RES[i]);
	dbg_printf("\n");

	pause();

	dbg_printf("\n");
	dbg_printf("\n");
	dbg_printf("test vector for f3:\n");
	dbg_printf("input section\n");
	dbg_printf("K is:		");
	for(i=0;i<L_KEY;i++)
		dbg_printf("%02x ",K[i]);
	dbg_printf("\n");
	dbg_printf("fi3 is:		%02x\n",fi3);
	dbg_printf("RAND is:	");
	for(i=0;i<L_RAND;i++)
		dbg_printf("%02x ",RAND[i]);
	dbg_printf("\n");
	dbg_printf("Fmk is:		");
	for(i=0;i<L_FMK;i++)
		dbg_printf("%02x ",Fmk[i]);
	dbg_printf("\n");

	dbg_printf("\n");
	f3(K,fi3,RAND,Fmk,CK);
	dbg_printf("output section\n");
	dbg_printf("f3 	CK:	");
	for (i=0;i<L_CK;i++)
		dbg_printf("%02x ",CK[i]);
	dbg_printf("\n");

	pause();

	dbg_printf("\n");
	dbg_printf("\n");
	dbg_printf("test vector for f4:\n");
	dbg_printf("input section\n");
	dbg_printf("K is:		");
	for(i=0;i<L_KEY;i++)
		dbg_printf("%02x ",K[i]);
	dbg_printf("\n");
	dbg_printf("fi4 is:		%02x\n",fi4);
	dbg_printf("RAND is:	");
	for(i=0;i<L_RAND;i++)
		dbg_printf("%02x ",RAND[i]);
	dbg_printf("\n");
	dbg_printf("Fmk is:		");
	for(i=0;i<L_FMK;i++)
		dbg_printf("%02x ",Fmk[i]);
	dbg_printf("\n");

	dbg_printf("\n");
	f4(K,fi4,RAND,Fmk,IK);
	dbg_printf("output section\n");
	dbg_printf("f4 	IK:	");
	for (i=0;i<L_IK;i++)
		dbg_printf("%02x ",IK[i]);
	dbg_printf("\n");

	pause();

	dbg_printf("\n");
	dbg_printf("\n");
	dbg_printf("test vector for f5*\n");
	dbg_printf("input section\n");
	dbg_printf("K is:		");
	for(i=0;i<L_KEY;i++)
		dbg_printf("%02x ",K[i]);
	dbg_printf("\n");
	dbg_printf("fi5* is:	%02x\n",fi5star);
	dbg_printf("RAND is:	");
	for(i=0;i<L_RAND;i++)
		dbg_printf("%02x ",RAND[i]);
	dbg_printf("\n");
	dbg_printf("Fmk is:		");
	for(i=0;i<L_FMK;i++)
		dbg_printf("%02x ",Fmk[i]);
	dbg_printf("\n");
	dbg_printf("\n");
	f5star(K,fi5star,RAND,Fmk,AKS);
	dbg_printf("output section\n");
	dbg_printf("f5* 	AKS:	");
	for (i=0;i<L_AKS;i++)
		dbg_printf("%02x ",AKS[i]);
	dbg_printf("\n");

	pause();
        dbg_printf("\n");
        dbg_printf("\n");
        dbg_printf("test vector for f11\n");
        dbg_printf("input section\n");
        dbg_printf("K is: ");
        for(i=0;i<L_KEY;i++)
          dbg_printf("%02x ",K[i]);
        dbg_printf("\n");
        dbg_printf("fi11 is: %02x\n",fi11);
        dbg_printf("RAND is: ");
        for(i=0;i<L_RAND;i++)
          dbg_printf("%02x ",RAND[i]);
        dbg_printf("\n");
        dbg_printf("Fmk is: ");
        for(i=0;i<L_FMK;i++)
          dbg_printf("%02x ",Fmk[i]);
        dbg_printf("\n");
        dbg_printf("\n");
        f11(K,fi11,RAND,Fmk,UAK);
        dbg_printf("output section\n");
        dbg_printf("f11 UAK: ");
        for (i=0;i<L_UAK;i++)
          dbg_printf("%02x ",UAK[i]);
        dbg_printf("\n");
        pause();
}
