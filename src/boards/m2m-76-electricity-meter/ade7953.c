/*!
 * \file      ade7953.c
 *
 * \brief     
 *
 * \copyright 
 *
 * \author    Roman Filatov ( PromElectronics )
 */
#include <stdlib.h>
#include "utilities.h"
#include "board-config.h"
#include "delay.h"
#include "ade7953.h"

/*
 * Public global variables
 */
const unsigned int adeRD = 0b10000000;  //This value tells the ADE7953 that data is to be read from the requested register.
const unsigned int adeWR = 0b00000000; //This value tells the ADE7953 that data is to be written to the requested register.
int ADE7953MODE = 0; // not init mode
/*!
 * Meter hardware and global parameters
 */
ADE7953_t ADE7953;

void ADE7953Init( void )
{
	uint32_t config;
	uint32_t mode;
	uint32_t version;
	uint32_t data;
    // Set NSS1 pin to 1
    GpioInit( &ADE7953.NSS1, ADE7953_NSS1, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );
    GpioInit( &ADE7953.NSS2, PB_12, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );

	SpiInit( &ADE7953.Spi, SPI_2, ADE7953_MOSI, ADE7953_MISO, ADE7953_SCLK, NC );

		ADE7953Write( CONFIG_16, 0x0080, 2 ); // ADE soft reset
	    // Wait 500 ms
	    DelayMs( 500 );

//Write 0x00AD to Register Address 0x00FE to unlocks Register 0x120 - datasheet
	    ADE7953Write( 0x00FE, 0x00AD, 2 );

//Write 0x0030 to Register Address 0x0120 to configures the optimum settings - datasheet
	    ADE7953Write( 0x0120, 0x0030, 2 );

//Read 0x0102 CONFIG register to check initial value 0x8004
	    config = ADE7953Read( CONFIG_16, 2 );
	    mode = ADE7953Read( CFMODE_16, 2 );
	    version = ADE7953Read( Version_8, 1 );

	    DelayMs( 10 );

	    data = ADE7953Read( LINECYC_16, 2 );
	    ADE7953Write( LINECYC_16, 0x000f, 2 );
	    data = ADE7953Read( LINECYC_16, 2 );

	    if (config!=0x8004){
	    	ADE7953MODE = 0xFF; // reset,init error
	    }
	    else { ADE7953MODE = 0x01; // reset,init ok
	    }
	    while(1){
	    data = ADE7953Read( VRMS_24, 3);
	    data = ADE7953Read( IRMSA_24, 3);

	    }
}

void ADE7953Reset( void )
{
    // Set RESET pin to 0
    GpioInit( &ADE7953.Reset, ADE7953_RESET, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );
    GpioWrite( &ADE7953.Reset, 1 );
    DelayMs( 10 );

    GpioWrite( &ADE7953.Reset, 0 );
    DelayMs( 10 );

    GpioWrite( &ADE7953.Reset, 1 );

//    // Configure RESET as input
//    GpioInit( &ADE7953.Reset, ADE7953_RESET, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );

    // Wait 16 ms
    DelayMs( 16 );
}

void ADE7953Write( uint16_t addr, uint32_t data, uint8_t size )
{
   uint8_t i;
   uint16_t  x;
//Write address MSB - first
   	 GpioWrite( &ADE7953.NSS1, 0 );
    	    SpiInOut( &ADE7953.Spi, ((addr>>8) & 0xff) );
    	    SpiInOut( &ADE7953.Spi, ( addr & 0xff) );
    	    SpiInOut( &ADE7953.Spi, (uint16_t)adeWR );
//Write data MSB - first
    	    for( i = 0; i < size; i++ )
    	    {
    	    	x = (data >> (8*(size-1-i))) & 0x00ff;
    	    	SpiInOut( &ADE7953.Spi, x );
    	    }
    GpioWrite( &ADE7953.NSS1, 1 );
}

uint32_t ADE7953Read( uint16_t addr, uint8_t size )
{
	uint8_t i;
	uint8_t sd;
    uint32_t data=0;
//Write address MSB - first
      GpioWrite( &ADE7953.NSS1, 0 );
        	 SpiInOut( &ADE7953.Spi, ((addr>>8) & 0xff) );
        	 SpiInOut( &ADE7953.Spi, ( addr & 0xff) );
        	 SpiInOut( &ADE7953.Spi, (uint16_t)adeRD );
//Write data MSB - first
        	     	    for( i = 0; i < size; i++ )
        	     	    {
        	     	    	sd = (SpiInOut( &ADE7953.Spi, (uint16_t)adeWR)) & 0x00ff;
        	     	    	data += sd << (8*(size-1-i));
        	     	    }
      GpioWrite( &ADE7953.NSS1, 1 );
    return data;
}

void ADE7953WriteBuffer( uint16_t addr, uint8_t *buffer, uint8_t size )
{

}

void ADE7953ReadBuffer( uint16_t addr, uint8_t *buffer, uint8_t size )
{

}

