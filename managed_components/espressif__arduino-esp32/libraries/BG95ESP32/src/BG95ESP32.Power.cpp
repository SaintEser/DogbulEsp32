#include "BG95ESP32.h"

TOKEN_TEXT(CBC, "+CBC");
TOKEN_TEXT(CFUN, "+CFUN");
TOKEN_TEXT(SPIC, "+SPIC");

bool BG95ESP32::powered() {
  sendAT();
  return waitResponse(BG95AT_DEFAULT_TIMEOUT) != -1;
  return 1;
}

void BG95ESP32::powertoggle() {
  digitalWrite(_pwrKeyPin, HIGH);
  delay(750);
  digitalWrite(_pwrKeyPin, LOW);
	delay(3000);
}

bool BG95ESP32::powerOnOff(bool power) {
  if (_pwrKeyPin == BG95ESP32_UNAVAILABLE_PIN) {
    return false;  //Corrected the logic error
  }

  bool currentlyPowered = powered();
  if (currentlyPowered == power) {
    return true;
  }

  BG95ESP32_PRINT_P("powerOnOff: %t", power);
 	powertoggle();

  uint16_t timeout = 5000;
  do {
		BG95ESP32_PRINT_P("powerOnOff: %t", power);
    powertoggle();
    delay(500);
    timeout -= 150;
    currentlyPowered = powered();
  } while (currentlyPowered != power && timeout > 0);  

  return currentlyPowered == power;
}

BG95ESP32ChargingStatus BG95ESP32::getChargingState() {
  uint8_t state;
  uint8_t level;
  uint16_t voltage;

  sendAT(TO_F(TOKEN_CBC));

  if (waitResponse(TO_F(TOKEN_CBC)) == 0 && parseReply(',', (uint8_t)BG95ESP32BatteryChargeField::Bcs, &state)
      && parseReply(',', (uint8_t)BG95ESP32BatteryChargeField::Bcl, reinterpret_cast<int8_t *>(&level))
      && parseReply(',', (uint16_t)BG95ESP32BatteryChargeField::Voltage, &voltage) && waitResponse() == 0) {
    return {(BG95ESP32ChargingState)state, static_cast<int8_t>(level), static_cast<int16_t>(voltage)};
  }

  return {BG95ESP32ChargingState::Error, 0, 0};
}

BG95ESP32PhoneFunctionality BG95ESP32::getPhoneFunctionality() {
  uint8_t state;

  sendAT(TO_F(TOKEN_CFUN), TO_F(TOKEN_READ));

  if (waitResponse(10000L, TO_F(TOKEN_CFUN)) == 0 && parseReply(',', 0, &state) && waitResponse() == 0) {
    return (BG95ESP32PhoneFunctionality)state;
  }

  return BG95ESP32PhoneFunctionality::Fail;
}

bool BG95ESP32::setPhoneFunctionality(BG95ESP32PhoneFunctionality fun) {
  sendAT(TO_F(TOKEN_CFUN), TO_F(TOKEN_WRITE), (uint8_t)fun);

  return waitResponse(10000L) == 0;
}

bool BG95ESP32::setSlowClock(BG95ESP32SlowClock mode) {
  sendAT(S_F("+CSCLK"), TO_F(TOKEN_WRITE), (uint8_t)mode);

  return waitResponse() == 0;
}

BG95ESP32PinStatus BG95ESP32::getPinState() {
  int8_t pincount, pukcount, pin2count, puk2count;

  sendAT(TO_F(TOKEN_SPIC));

  if (waitResponse(10000L, TO_F(TOKEN_SPIC)) == 0 && parseReply(',', (uint8_t)BG95ESP32PinField::Pin, &pincount)
      && parseReply(',', (uint8_t)BG95ESP32PinField::Puk, &pukcount) && parseReply(',', (uint8_t)BG95ESP32PinField::Pin2, &pin2count)
      && parseReply(',', (uint8_t)BG95ESP32PinField::Puk2, &puk2count) && waitResponse() == 0) {
    return {pincount, pukcount, pin2count, puk2count};
  }
  return {-1, -1, -1, -1};
}
