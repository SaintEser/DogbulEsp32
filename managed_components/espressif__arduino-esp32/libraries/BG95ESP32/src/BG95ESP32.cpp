#include "BG95ESP32.h"

TOKEN(RDY);

BG95ESP32::BG95ESP32(uint8_t resetPin, uint8_t pwrKeyPin)
{
	_resetPin = resetPin;
	_pwrKeyPin = pwrKeyPin;

	if(_pwrKeyPin != BG95ESP32_UNAVAILABLE_PIN) pinMode(_pwrKeyPin, OUTPUT);
	if(_pwrKeyPin != BG95ESP32_UNAVAILABLE_PIN) digitalWrite(_pwrKeyPin, HIGH);
	if(_resetPin != BG95ESP32_UNAVAILABLE_PIN) pinMode(_resetPin, OUTPUT);
	if(_resetPin != BG95ESP32_UNAVAILABLE_PIN) digitalWrite(_resetPin, HIGH);
}

BG95ESP32::~BG95ESP32() { }

//#pragma region Public functions

void BG95ESP32::init()
{
	powerOnOff(1);
	BG95ESP32_PRINT_SIMPLE_P("Init...");
	//setEcho(BG95ESP32Echo::On);
	//reset();
	waitForReady();
	delay(1500);
	setEcho(BG95ESP32Echo::Off);
}

void BG95ESP32::reset()
{
	digitalWrite(_resetPin, HIGH);
	delay(10);
	digitalWrite(_resetPin, LOW);
	delay(200);
	digitalWrite(_resetPin, HIGH);
}

void BG95ESP32::waitForReady()
{
	do
	{
		//BG95ESP32_PRINT_SIMPLE_P("Waiting for OK...");
		sendAT(S_F(""));
	} 
	while (waitResponse(TO_F(TOKEN_OK)) != 0);
	// we got OK, waiting for RDY
	//while (waitResponse(TO_F(TOKEN_RDY)) != 0);
}

bool BG95ESP32::setEcho(BG95ESP32Echo mode)
{
	sendAT(S_F("E"), (uint8_t)mode);

	return waitResponse() == 0;
}

size_t BG95ESP32::sendCommand(const char *cmd, char *response, size_t responseSize)
{
	flushInput();
	sendAT(cmd);
	
	uint16_t timeout = BG95AT_DEFAULT_TIMEOUT;
	readNext(response, responseSize, &timeout);
	return 0;
}

size_t BG95ESP32::sendCommandW(const char *cmd, char *response, size_t responseSize, uint16_t timeout)
{
	flushInput();
	sendAT(cmd);
	
	
	readNext(response, responseSize, &timeout);
	return 0;
}

//#pragma endregion


