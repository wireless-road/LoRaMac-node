#include <nmea_gps_private.h>

const uint8_t NMEA_GGA_CMD[3] = { 'G', 'G', 'A' };
const uint8_t NMEA_GLL_CMD[3] = { 'G', 'L', 'L' };
const uint8_t NMEA_GSA_CMD[3] = { 'G', 'S', 'A' };
const uint8_t NMEA_GSV_CMD[3] = { 'G', 'S', 'V' };
const uint8_t NMEA_RMC_CMD[3] = { 'R', 'M', 'C' };
const uint8_t NMEA_VTG_CMD[3] = { 'V', 'T', 'G' };

uint8_t timestamp[12];
uint8_t latitude[11];
uint8_t n_s_indicator;
uint8_t longtitude[12];
uint8_t e_w_indicator;
uint8_t fix_indicator;
uint8_t sattelites_cnt;

static uint32_t rx_cnt = 0;
static uint8_t rx_buf[NMEA_PACKET_MAX_LEN] = {0};
static uint8_t rx_buf_saved[NMEA_PACKET_MAX_LEN] = {0};
static gps_data_raw_t gps_raw_data = {
	.timestamp = 0,
	.latitude = 0,
	.n_s_indicator = 0,
	.longtitude = 0,
	.e_w_indicator = 0,
	.fix_indicator = Nmea_Fix_No_Data,
	.sattelites_cnt = 0
};
static gps_data_t gps_data = {
		.timestamp_hh = 0,
		.timestamp_mm = 0,
		.timestamp_ss = 0,
		.timestamp_ms = 0,
		.n_s_indicator = 0,
		.latitude = 0,
		.e_w_indicator = 0,
		.longtitude = 0,
		.fix_indicator = 0,
		.sattelites_cnt = 0,
		.updated_flag = 0
};

static uint32_t nmea_busy_flag = 0;

static void nmea_resulted_parse_GGA_cmd()
{
	if( gps_raw_data.timestamp[0] != 0) {
		gps_data.timestamp_hh = (gps_raw_data.timestamp[0]-0x30)*10 + (gps_raw_data.timestamp[1]-0x30);
		gps_data.timestamp_mm = (gps_raw_data.timestamp[2]-0x30)*10 + (gps_raw_data.timestamp[3]-0x30);
		gps_data.timestamp_ss = (gps_raw_data.timestamp[4]-0x30)*10 + (gps_raw_data.timestamp[5]-0x30);
		gps_data.timestamp_ms = (gps_raw_data.timestamp[7]-0x30)*100 + (gps_raw_data.timestamp[8]-0x30)*10 + (gps_raw_data.timestamp[9]-0x30)*10;
	}

	if( gps_raw_data.fix_indicator == '0') {
		return;
	}
	gps_data.fix_indicator = gps_raw_data.fix_indicator - 0x29;

	if( gps_raw_data.n_s_indicator == 'N') {
		gps_data.n_s_indicator = Nmea_Latitude_N;
	}
	if( gps_raw_data.n_s_indicator == 'S') {
		gps_data.n_s_indicator = Nmea_Latitude_S;
	}
	if( gps_raw_data.e_w_indicator == 'E') {
		gps_data.e_w_indicator = Nmea_Longtitude_E;
	}
	if( gps_raw_data.e_w_indicator == 'W') {
		gps_data.e_w_indicator = Nmea_Longtitude_W;
	}
	gps_data.latitude = (float)((gps_raw_data.latitude[0]-0x30)*10 + gps_raw_data.latitude[1]-0x30);
	gps_data.latitude += (float)((gps_raw_data.latitude[2]-0x30)*10 + (gps_raw_data.latitude[3]-0x30)) / 60.0;
	gps_data.latitude += (float)((gps_raw_data.latitude[5]-0x30)*1000 + (gps_raw_data.latitude[6]-0x30)*100 + (gps_raw_data.latitude[7]-0x30)*10 + (gps_raw_data.latitude[8]-0x30) ) / 600000.0;

	gps_data.longtitude = (float)((gps_raw_data.longtitude[0]-0x30)*100 + (gps_raw_data.longtitude[1]-0x30)*10 + gps_raw_data.longtitude[2]-0x30);
	gps_data.longtitude += (float)((gps_raw_data.longtitude[3]-0x30)*10 + (gps_raw_data.longtitude[4]-0x30)) / 60.0;
	gps_data.longtitude += (float)((gps_raw_data.longtitude[6]-0x30)*1000 + (gps_raw_data.longtitude[7]-0x30)*100 + (gps_raw_data.longtitude[8]-0x30)*10 + (gps_raw_data.longtitude[9]-0x30) ) / 600000.0;

	gps_data.sattelites_cnt = gps_raw_data.sattelites_cnt - 0x30;

//	uint32_t latitude_dec = (uint32_t)((gps_data.latitude - (uint32_t)gps_data.latitude)*10000);
//	uint32_t longtude_dec = (uint32_t)((gps_data.longtitude - (uint32_t)gps_data.longtitude)*10000);
//	printf("==== %d %d %d %d %d.%d %d %d.%d %d %d\r\n", gps_data.timestamp_hh, gps_data.timestamp_mm, gps_data.timestamp_ss, gps_data.timestamp_ms, (uint32_t)(gps_data.latitude), latitude_dec, gps_data.n_s_indicator, (uint32_t)(gps_data.longtitude), longtude_dec, gps_data.n_s_indicator, gps_data.e_w_indicator, gps_data.fix_indicator);

}

static void nmea_raw_parse_GGA_cmd()
{
	nmea_busy_flag = 1;
	gps_raw_data.updated_flag = 1;
	memcpy(rx_buf_saved, rx_buf, rx_cnt);
	uint8_t* cur_field = gps_raw_data.timestamp;
	uint8_t delimiter_cnt = 0;
	uint32_t parsed_flag = 0;
	for(uint32_t i=7; i<rx_cnt; i++) {
		if(rx_buf_saved[i] == NMEA_DELIMITER) {
			delimiter_cnt++;
			switch(delimiter_cnt)
			{
			case 1: *cur_field = 0; cur_field = gps_raw_data.latitude; 			break;
			case 2: 				cur_field = &gps_raw_data.n_s_indicator; 	break;
			case 3: *cur_field = 0; cur_field = gps_raw_data.longtitude; 		break;
			case 4: 				cur_field = &gps_raw_data.e_w_indicator; 	break;
			case 5: 				cur_field = &gps_raw_data.fix_indicator; 	break;
			case 6: parsed_flag = 1; break;
			}
		} else {
			if(parsed_flag)
				break;
			*cur_field++ = rx_buf_saved[i];
		}
	}

	if(parsed_flag)
		nmea_resulted_parse_GGA_cmd();

//	printf("==== %s %c %s %c %c\r\n", gps_data.latitude, gps_data.n_s_indicator, gps_data.longtitude, gps_data.e_w_indicator, gps_data.fix_indicator);
	nmea_busy_flag = 0;
}

uint32_t nmea_is_updated(void) {
	if(nmea_busy_flag) {
		return 0;
	} else if(gps_raw_data.fix_indicator == Nmea_Fix_No_Data) {
		return 0;
	} else {
		return 1;
	}
}

void nmea_parser(uint8_t data)
{
	if( data == NMEA_STARTING_SYMBOL ) {
		if(rx_cnt == 0) {
			rx_buf[rx_cnt] = data;
			rx_cnt++;
			return;
		}

		if( (rx_buf[1] != NMEA_CMD_PREFIX_1) || (rx_buf[2] != NMEA_CMD_PREFIX_2) || (rx_buf[6] != NMEA_DELIMITER) ) {
			rx_cnt = 0;
			rx_buf[rx_cnt] = data;
			rx_cnt++;
			// printf(" error " );
			return;
		}
		if( (rx_buf[3] == NMEA_GGA_CMD[0]) && (rx_buf[4] == NMEA_GGA_CMD[1]) && (rx_buf[5] == NMEA_GGA_CMD[2]) ) {
			nmea_raw_parse_GGA_cmd();
		}
		rx_cnt = 0;
	}
	rx_buf[rx_cnt] = data;
	rx_cnt++;
}

gps_data_t* nmea_getData(void) {
	return &gps_data;
}
