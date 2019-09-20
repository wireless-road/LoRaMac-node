/*
 * nmea_gps.h
 *
 *  Created on: 18 сент. 2019 г.
 *      Author: almaz
 */
#include <stdint.h>
#include <stdlib.h>

#ifndef PERIPHERALS_NMEA_GPS_H_
#define PERIPHERALS_NMEA_GPS_H_


typedef enum{
	Nmea_Fix_No_Data = 0x00,
	Nmea_Fix_Not_Available = 0x01,
	Nmea_Fix_Gps_Sps_Mode,
	Nmea_Fix_Differential_Gps_Sps_Mode,
	Nmea_Fix_Gps_Pps_Mode
} Nmea_FixIndicator;

typedef enum{
	Nmea_Latitude_N = 0,
	Nmea_Latitude_S
} Nmea_Latitude;

typedef enum{
	Nmea_Longtitude_E = 0,
	Nmea_Longtitude_W
} Nmea_Longtitude;

typedef struct {
	uint8_t timestamp_hh;
	uint8_t timestamp_mm;
	uint8_t timestamp_ss;
	uint8_t timestamp_ms;
	Nmea_Latitude n_s_indicator;
	float latitude;
	Nmea_Longtitude e_w_indicator;
	float longtitude;
	Nmea_FixIndicator fix_indicator;
	uint8_t sattelites_cnt;
	uint8_t updated_flag;
} gps_data_t;


void nmea_parser(uint8_t data);
uint32_t nmea_is_updated(void);
gps_data_t* nmea_getData(void);

#endif /* PERIPHERALS_NMEA_GPS_H_ */
