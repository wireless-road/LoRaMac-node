/*
 * nmea_gps_private.h
 *
 *  Created on: 18 сент. 2019 г.
 *      Author: almaz
 */

#ifndef PERIPHERALS_NMEA_GPS_PRIVATE_H_
#define PERIPHERALS_NMEA_GPS_PRIVATE_H_

#include <nmea_gps.h>

#define NMEA_PACKET_MAX_LEN 128

#define NMEA_STARTING_SYMBOL '$'
#define NMEA_CMD_PREFIX_1 'G'
#define NMEA_CMD_PREFIX_2 'P'
#define NMEA_DELIMITER ','

typedef struct {
	uint8_t timestamp[12];
	uint8_t latitude[11];
	uint8_t n_s_indicator;
	uint8_t longtitude[12];
	uint8_t e_w_indicator;
	uint8_t fix_indicator;
	uint8_t sattelites_cnt;
	uint8_t updated_flag;
} gps_data_raw_t;


#endif /* PERIPHERALS_NMEA_GPS_PRIVATE_H_ */
