/*
 *  Copyright 2010 by Spectrum Digital Incorporated.
 *  All rights reserved. Property of Spectrum Digital Incorporated.
 */


#include "stdio.h"
#include "usbstk5515.h"
#include "usbstk5515_gpio.h"
#include "usbstk5515_i2c.h"

#define AIC3204_I2C_ADDR 0x18
#define Xmit 0x20
#define Rcv 0x08

#define ORDER_IIR 3 //Length of both numerator and denominator z^(-1) polynomials.

#define freq_6_857 0xF1
#define freq_8 0xE1
#define freq_9_6 0xD1
#define freq_12 0xC1
#define freq_16 0xB1
#define freq_24 0xA1
#define freq_48 0x91

Int16 direct_form_1(Int16 i,Int16 input);

Int16 AIC3204_rget(  Uint16 regnum, Uint16* regval )
{
    Int16 retcode = 0;
    Uint8 cmd[2];

    cmd[0] = regnum & 0x007F;       // 7-bit Register Address
    cmd[1] = 0;

    retcode |= USBSTK5515_I2C_write( AIC3204_I2C_ADDR, cmd, 1 );
    retcode |= USBSTK5515_I2C_read( AIC3204_I2C_ADDR, cmd, 1 );

    *regval = cmd[0];
    USBSTK5515_wait( 10 );
    return retcode;
}

Int16 AIC3204_rset( Uint16 regnum, Uint16 regval )
{
    Uint8 cmd[2];
    cmd[0] = regnum & 0x007F;       // 7-bit Register Address
    cmd[1] = regval;                // 8-bit Register Data

    return USBSTK5515_I2C_write( AIC3204_I2C_ADDR, cmd, 2 );
} 

void AIC3204_config(Uint8 sampling_freq)
{
	/* Configure AIC3204 */
	AIC3204_rset( 0, 0 );          // Select page 0
    AIC3204_rset( 1, 1 );          // Reset codec
    AIC3204_rset( 0, 1 );          // Select page 1
    AIC3204_rset( 1, 8 );          // Disable crude AVDD generation from DVDD
    AIC3204_rset( 2, 1 );          // Enable Analog Blocks, use LDO power
    AIC3204_rset( 0, 0 );          // Select page 0
    /* PLL and Clocks config and Power Up  */
    AIC3204_rset( 27, 0x0d );      // BCLK and WCLK is set as o/p to AIC3204(Master)
    AIC3204_rset( 28, 0x00 );      // Data ofset = 0
    AIC3204_rset( 4, 3 );          // PLL setting: PLLCLK <- MCLK, CODEC_CLKIN <-PLL CLK
    AIC3204_rset( 6, 7 );          // PLL setting: J=7
    AIC3204_rset( 7, 0x06 );       // PLL setting: HI_BYTE(D=1680)
    AIC3204_rset( 8, 0x90 );       // PLL setting: LO_BYTE(D=1680)
    AIC3204_rset( 30, 0x88 );      // For 32 bit clocks per frame in Master mode ONLY
                                   // BCLK=DAC_CLK/N =(12288000/8) = 1.536MHz = 32*fs
    AIC3204_rset( 5, sampling_freq);       // PLL setting: Power up PLL, P=1 and R=1
    AIC3204_rset( 13, 0 );         // Hi_Byte(DOSR) for DOSR = 128 decimal or 0x0080 DAC oversamppling
    AIC3204_rset( 14, 0x80 );      // Lo_Byte(DOSR) for DOSR = 128 decimal or 0x0080
    AIC3204_rset( 20, 0x80 );      // AOSR for AOSR = 128 decimal or 0x0080 for decimation filters 1 to 6
    AIC3204_rset( 11, 0x82 );      // Power up NDAC and set NDAC value to 2
    AIC3204_rset( 12, 0x87 );      // Power up MDAC and set MDAC value to 7
    AIC3204_rset( 18, 0x87 );      // Power up NADC and set NADC value to 7
    AIC3204_rset( 19, 0x82 );      // Power up MADC and set MADC value to 2
    /* DAC ROUTING and Power Up */
    AIC3204_rset(  0, 0x01 );      // Select page 1
    AIC3204_rset( 12, 0x08 );      // LDAC AFIR routed to HPL
    AIC3204_rset( 13, 0x08 );      // RDAC AFIR routed to HPR
    AIC3204_rset(  0, 0x00 );      // Select page 0
    AIC3204_rset( 64, 0x02 );      // Left vol=right vol
    AIC3204_rset( 65, 0x00 );      // Left DAC gain to 0dB VOL; Right tracks Left
    AIC3204_rset( 63, 0xd4 );      // Power up left,right data paths and set channel
    AIC3204_rset(  0, 0x01 );      // Select page 1
    AIC3204_rset( 16, 0x00 );      // Unmute HPL , 0dB gain
    AIC3204_rset( 17, 0x00 );      // Unmute HPR , 0dB gain
    AIC3204_rset(  9, 0x30 );      // Power up HPL,HPR
    AIC3204_rset(  0, 0x00 );      // Select page 0
    USBSTK5515_wait( 500 );        // Wait
    
    /* ADC ROUTING and Power Up */
    AIC3204_rset( 0, 1 );          // Select page 1
    AIC3204_rset( 0x34, 0x30 );    // STEREO 1 Jack
		                           // IN2_L to LADC_P through 40 kohm
    AIC3204_rset( 0x37, 0x30 );    // IN2_R to RADC_P through 40 kohmm
    AIC3204_rset( 0x36, 3 );       // CM_1 (common mode) to LADC_M through 40 kohm
    AIC3204_rset( 0x39, 0xc0 );    // CM_1 (common mode) to RADC_M through 40 kohm
    AIC3204_rset( 0x3b, 0 );       // MIC_PGA_L unmute
    AIC3204_rset( 0x3c, 0 );       // MIC_PGA_R unmute
    AIC3204_rset( 0, 0 );          // Select page 0
    AIC3204_rset( 0x51, 0xc0 );    // Powerup Left and Right ADC
    AIC3204_rset( 0x52, 0 );       // Unmute Left and Right ADC
    
    AIC3204_rset( 0, 0 );    
    USBSTK5515_wait( 200 );        // Wait
    /* I2S settings */
    I2S0_SRGR = 0x0;
    I2S0_CR = 0x8010;    // 16-bit word, slave, enable I2C
    I2S0_ICMR = 0x3f;    // Enable interrupts
    
}

Int16 y_IIR[ORDER_IIR]={0,0,0};
Int16 den_coeff[ORDER_IIR]={100,50,40};
Int16 num_coeff[ORDER_IIR]={49,79,49};
Int16 x_IIR[ORDER_IIR]={0,0,0};
Int16 syn_input[7]={10, 20, -10, 0, 30, 40, 10};

Int16 direct_form_1(Int16 i,Int16 input)
{	
	int p, k;
	Int16 acc=0;
	
	x_IIR[i]=input/64; //Buffering the latest input

    /*****************************
     * Your code goes here *
     *****************************/
     for(p=0; p<ORDER_IIR; p++)
     {
     	acc = acc + x_IIR[p] * num_coeff[(i-p+ORDER_IIR)%ORDER_IIR];
     }
//     for(k=1; k<ORDER_IIR; k++)
//     {
//     	acc = acc - den_coeff[k] * y_IIR[(i+k+2+ORDER_IIR)%ORDER_IIR];
//     }  
     acc=acc-  (den_coeff[1]*y_IIR[(i+2)%3])-(den_coeff[2]*y_IIR[(i+1)%3]);
 	acc = acc/den_coeff[0];
	y_IIR[i]=(Int16)(acc); //Buffering the latest output
	return(y_IIR[i]);
}

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  main( )                                                                 *
 *                                                                          *
 * ------------------------------------------------------------------------ */
 Int16 recent_output[100];
 Int16 recent_input[100];
 Int16 t =0;
int main( void )
{
    /* Initialize BSL */
    Int16 i,j=0;
    Int16 input,output;
    SYS_EXBUSSEL = 0x6100;
    USBSTK5515_init( );
    AIC3204_config(freq_48);
   
    while(t<100)
    {    	
    	for(i=0;i<ORDER_IIR;i++)
    	{
    		while((Rcv & I2S0_IR) == 0);
    		input = I2S0_W0_MSW_R;
			input = I2S0_W1_MSW_R;
      	    
    		output=1000*direct_form_1(i,input);//Passing the value of 'i' also. One can also maintain a global counter instead.
    		t++;
			while((Xmit & I2S0_IR) == 0);
    		I2S0_W0_MSW_W = output;  // 16 bit left channel transmit audio data
      		I2S0_W1_MSW_W = output; // 16 bit right channel transmit audio data
      	     
      	    recent_input[j]=input;
      	    recent_output[j]=output;
      	    j++;
      	    if(j>99)
      	    	j=0;
    	}
	}	
	
    /* Disble I2S */
    I2S0_CR = 0x00;
    return 0;
}
