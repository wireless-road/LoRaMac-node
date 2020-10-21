/*!
 * \file      ade7953.h
 *
 * \brief
 *
 * \copyright
 *
 * \author    Roman Filatov ( PromElectronics )
 */
#ifndef __ADE7953_H__
#define __ADE7953_H__

#include <stdint.h>
#include <stdbool.h>
#include "gpio.h"
#include "spi.h"
#include "ade7953_Regs.h"

/*!
 * Radio LoRa packet handler state
 */
typedef struct
{
    int8_t GAValue;
    int16_t XBValue;
    uint8_t PH;
}MeterGain_t;

typedef struct
{
	MeterGain_t		Gain;
}MeterSettings_t;

typedef struct ADE7953_s
{
    Gpio_t        	Reset;
    Gpio_t        	NSS1;
    Gpio_t        	NSS2;
    Spi_t         	Spi;
    MeterSettings_t Settings;
}ADE7953_t;


/*!
 * \brief Initializes the ADE7953 I/Os pins interface
 */
void ADE7953Init ( void );

/*!
 * \brief Initializes the ADE7953
 */
void ADE7953Reset ( void );

/*!
 * \brief ADE7953 write - addr, data, size in bytes
 */
void ADE7953Write( uint16_t addr, uint32_t data, uint8_t size );

/*!
 * \brief ADE7953 read - addr, data, size in bytes
 */
uint32_t ADE7953Read( uint16_t addr, uint8_t size );


#endif
