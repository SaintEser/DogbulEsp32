#include "BG95ESP32.h"

TOKEN_TEXT(GPS_POWER, "+CGNSPWR");
TOKEN_TEXT(GPS_INFO, "+CGNSINF");

bool BG95ESP32::powerOnOffGps(bool power)
{
	bool currentState;
	if(!getGpsPowerState(&currentState) || (currentState == power)) return false;

	sendAT(TO_F(TOKEN_GPS_POWER), TO_F(TOKEN_WRITE), (uint8_t)power);
	return waitResponse() == 0;
}

bool BG95ESP32::getGpsPosition(char *response, size_t responseSize)
{
	sendAT(TO_F(TOKEN_GPS_INFO));

	if(waitResponse(TO_F(TOKEN_GPS_INFO)) != 0)
		return false;

	// GPSINF response might be too long for the reply buffer
	copyCurrentLine(response, responseSize, strlen_P(TOKEN_GPS_INFO) + 2);
	return true;
}

void BG95ESP32::getGpsField(const char* response, BG95ESP32GpsField field, char** result) 
{
	char *pTmp = find(response, ',', (uint8_t)field);
	*result = pTmp;
}

bool BG95ESP32::getGpsField(const char* response, BG95ESP32GpsField field, uint16_t* result)
{
	if (field < BG95ESP32GpsField::Speed) return false;

	parse(response, ',', (uint8_t)field, result);
	return true;
}

bool BG95ESP32::getGpsField(const char* response, BG95ESP32GpsField field, float* result)
{
	if (field != BG95ESP32GpsField::Course && 
		field != BG95ESP32GpsField::Latitude &&
		field != BG95ESP32GpsField::Longitude &&
		field != BG95ESP32GpsField::Altitude &&
		field != BG95ESP32GpsField::Speed) return false;

	parse(response, ',', (uint8_t)field, result);
	return true;
}

BG95ESP32GpsStatus BG95ESP32::getGpsStatus(char * response, size_t responseSize, uint8_t minSatellitesForAccurateFix)
{	
	BG95ESP32GpsStatus result = BG95ESP32GpsStatus::NoFix;

	sendAT(TO_F(TOKEN_GPS_INFO));

	if(waitResponse(TO_F(TOKEN_GPS_INFO)) != 0)
		return BG95ESP32GpsStatus::Fail;

	uint16_t shift = strlen_P(TOKEN_GPS_INFO) + 2;

	if(replyBuffer[shift] == '0') result = BG95ESP32GpsStatus::Off;
	if(replyBuffer[shift + 2] == '1') // fix acquired
	{
		uint16_t satellitesUsed;
		getGpsField(replyBuffer, BG95ESP32GpsField::GnssUsed, &satellitesUsed);

		result = satellitesUsed > minSatellitesForAccurateFix ?
			BG95ESP32GpsStatus::AccurateFix :
			BG95ESP32GpsStatus::Fix;

		copyCurrentLine(response, responseSize, shift);
	}

	if(waitResponse() != 0) return BG95ESP32GpsStatus::Fail;

	return result;
}

bool BG95ESP32::getGpsPowerState(bool *state)
{
	uint8_t result;

	sendAT(TO_F(TOKEN_GPS_POWER), TO_F(TOKEN_READ));

	if(waitResponse(10000L, TO_F(TOKEN_GPS_POWER)) != 0 ||
		!parseReply(',', 0, &result) ||
		waitResponse())
		return false;

	*state = result;
	return true;
}
