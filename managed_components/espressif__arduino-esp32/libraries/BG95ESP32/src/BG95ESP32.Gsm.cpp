#include "BG95ESP32.h"

AT_COMMAND(SEND_SMS, "+CMGS=\"%s\"");

TOKEN_TEXT(CPIN, "+CPIN");
TOKEN_TEXT(QPINC, "+QPINC"); //Updated for BG95, AT+QPINC Display PIN Remainder Counter
TOKEN_TEXT(CSQ, "+CSQ");
TOKEN_TEXT(CMGS, "+CMGS");
//set a token for +QINISTAT
TOKEN_TEXT(QINISTAT, "+QINISTAT");


bool BG95ESP32::simUnlock(const char* pin)
{
	sendAT(TO_F(TOKEN_CPIN), TO_F(TOKEN_WRITE), pin);

	return waitResponse(5000L) == 0;
}

size_t BG95ESP32::getSimState(char *state, size_t stateSize)
{
	Log.notice("Sending AT command to get SIM state\n");
	sendAT(TO_F(TOKEN_QINISTAT));
	if(waitResponse(5000L, TO_F(TOKEN_QINISTAT)) != 0) {
		Log.error("Failed to get response for SIM state\n");
		return 0;
	}

	Log.notice("Response received, copying current line\n");
	char tempBuffer[32];
	copyCurrentLine(state, stateSize, strlen_P(TOKEN_QINISTAT) + 2);

	// Parse the number from the response
	Log.notice("Parsing the number from the response\n");
	char *numberStart = strchr(tempBuffer, ':');
	if (numberStart != nullptr) {
		numberStart += 2; // Skip ": " to get to the number
		strncpy(state, numberStart, stateSize - 1);
		state[stateSize - 1] = '\0'; // Ensure null-termination
		Log.notice("Parsed state: %s\n", state);
	} else {
		Log.error("Parsing failed\n");
		return 0; // Parsing failed
	}

	// Wait for the final response and return the length of the state string if it matches TOKEN_OK
	Log.notice("Waiting for final response\n");
	return waitResponse() == 0 ? strlen(state) : 0;
}

size_t BG95ESP32::getImei(char *imei, size_t imeiSize)
{
	//AT+GSN does not have a response prefix, so we need to flush input
	//before sending the command
	flushInput();

	sendAT(S_F("+GSN"));	
	waitResponse(BG95AT_DEFAULT_TIMEOUT, NULL); //consuming an extra line before the response. Undocumented

	if(waitResponse(BG95AT_DEFAULT_TIMEOUT, NULL) != 0) return 0;
	copyCurrentLine(imei, imeiSize);

	return waitResponse() == 0?
		strlen(imei) :
		0;
}

BG95ESP32SignalQualityReport BG95ESP32::getSignalQuality()
{
	uint8_t quality;
	uint8_t errorRate;

	BG95ESP32SignalQualityReport report = {99, 99, 1};

	sendAT(TO_F(TOKEN_CSQ));
	if(waitResponse(TO_F(TOKEN_CSQ)) != 0 ||
		!parseReply(',', (uint8_t)BG95ESP32SignalQualityResponse::SignalStrength, &quality) ||
		!parseReply(',', (uint8_t)BG95ESP32SignalQualityResponse::BitErrorrate, &errorRate) ||
		waitResponse())
		return report;

	report.rssi = quality;
	report.ber = errorRate;

	if (quality == 0) report.attenuation = -115;
	else if (quality == 1) report.attenuation = -111;
	else if (quality == 31) report.attenuation = -52;
	else if (quality > 31) report.attenuation = 1;
	else report.attenuation = map(quality, 2, 30, -110, -54);

	return report;
}

bool BG95ESP32::setSmsMessageFormat(BG95ESP32SmsMessageFormat format)
{
	sendAT(S_F("+CMGF="), (uint8_t)format);
	return waitResponse() == 0;
}

bool BG95ESP32::sendSms(const char *addr, const char *msg)
{
	if (!setSmsMessageFormat(BG95ESP32SmsMessageFormat::Text)) return false;
	sendFormatAT(TO_F(AT_COMMAND_SEND_SMS), addr);

	if (!waitResponse(S_F(">")) == 0) return false;

	SENDARROW;
	print(msg);
	print((char)0x1A);

	return waitResponse(60000L, TO_F(TOKEN_CMGS)) == 0 &&
		waitResponse() == 0;
}