#include "BG95ESP32.h"
#include "..\..\main\ledmanage.h"

AT_COMMAND(SEND_SMS, "+CMGS=\"%s\"");

TOKEN_TEXT(CPIN, "+CPIN");
TOKEN_TEXT(QPINC, "+QPINC");  //Updated for BG95, AT+QPINC Display PIN Remainder Counter
TOKEN_TEXT(CSQ, "+CSQ");
TOKEN_TEXT(CMGS, "+CMGS");
//set a token for +QINISTAT
TOKEN_TEXT(QINISTAT, "+QINISTAT");
AT_COMMAND(SIM_DETECTOR, "+QSIMDET=%d,%d");

bool BG95ESP32::simUnlock(const char *pin) {
  sendAT(TO_F(TOKEN_CPIN), TO_F(TOKEN_WRITE), pin);

  return waitResponse(5000L) == 0;
  return false;  // Default return value if no conditions are met
}

//AT+QSIMDET=1,0 must be set when sim is inserted first time. AT+QSIMSTAT? to get sim state, if received +QSIMSTAT: 1,1 and +CPIN: READY then sim is ready.
uint8_t BG95ESP32::getSimState() {
  uint8_t simState;
  Log.notice("Sending AT command to get SIM state\n");
  sendAT(TO_F(TOKEN_QINISTAT));
  if (waitResponse(2000L, TO_F(TOKEN_QINISTAT)) != 0) {
    Log.error("Failed to get response for SIM state\n");
    return 0;
  }

  Log.notice("Response received, copying current line\n");
  char tempBuffer[32];
  copyCurrentLine(tempBuffer, sizeof(tempBuffer), strlen_P(TOKEN_QINISTAT));  // Copy to tempBuffer

  // Parse the number from the response
  Log.notice("Parsing the number from the response\n");
  char *numberStart = strchr(tempBuffer, ':');
  if (numberStart != nullptr) {
    numberStart += 2;  // Skip ": " to get to the number
    strncpy(tempBuffer, numberStart, sizeof(tempBuffer) - 1);
    tempBuffer[sizeof(tempBuffer) - 1] = '\0';  // Ensure null-termination
    Log.notice("Parsed state: %s\n", tempBuffer);
    //convert state to uint8_t:
    simState = atoi(tempBuffer);
    return simState;  // Return the state string
  } else {
    ESP_LOGE("BG95", "Parsing failed");
    return 0;  // Parsing failed
  }
  // Wait for the final response and return the length of the state string if it matches TOKEN_OK
  ESP_LOGI("BG95", "Response: %s ", tempBuffer);
  return simState;
}

//checks if sim card is inserted or not
bool BG95ESP32::check_sim(uint8_t buffer) {
  if (buffer == 0 || buffer == NULL) {  // Check if the buffer starts with '0'
    getRgbValue(LED_COLOR_RED, 16);
    return false;
  } else if (buffer > 0) {  // Check if the buffer is not empty
    getRgbValue(LED_COLOR_GREEN, 16);
    return true;
  }
  return false;
  getRgbValue(LED_COLOR_RED, 16);
  ESP_LOGI("BG95", "Response: %d ", buffer);
}

size_t BG95ESP32::getImei(char *imei, size_t imeiSize) {
  //AT+GSN does not have a response prefix, so we need to flush input
  //before sending the command
  flushInput();

  sendAT(S_F("+GSN"));
  waitResponse(BG95AT_DEFAULT_TIMEOUT, NULL);  //consuming an extra line before the response. Undocumented

  if (waitResponse(BG95AT_DEFAULT_TIMEOUT, NULL) != 0) {
    return 0;
  }
  copyCurrentLine(imei, imeiSize);

  return waitResponse() == 0 ? strlen(imei) : 0;
}

BG95ESP32SignalQualityReport BG95ESP32::getSignalQuality() {
  uint8_t quality;
  uint8_t errorRate;

  BG95ESP32SignalQualityReport report = {99, 99, 1};

  sendAT(TO_F(TOKEN_CSQ));
  if (waitResponse(TO_F(TOKEN_CSQ)) != 0 || !parseReply(',', (uint8_t)BG95ESP32SignalQualityResponse::SignalStrength, &quality)
      || !parseReply(',', (uint8_t)BG95ESP32SignalQualityResponse::BitErrorrate, &errorRate) || waitResponse()) {
    return report;
  }

  report.rssi = quality;
  report.ber = errorRate;

  if (quality == 0) {
    report.attenuation = -115;
  } else if (quality == 1) {
    report.attenuation = -111;
  } else if (quality == 31) {
    report.attenuation = -52;
  } else if (quality > 31) {
    report.attenuation = 1;
  } else {
    report.attenuation = map(quality, 2, 30, -110, -54);
  }

  return report;
}

bool BG95ESP32::setSmsMessageFormat(BG95ESP32SmsMessageFormat format) {
  sendAT(S_F("+CMGF="), (uint8_t)format);
  return waitResponse() == 0;
}

bool BG95ESP32::sendSms(const char *addr, const char *msg) {

  if (!setSmsMessageFormat(BG95ESP32SmsMessageFormat::Text)) {
    return false;
  }
  sendFormatAT(TO_F(AT_COMMAND_SEND_SMS), addr);

  if (!waitResponse(S_F(">")) == 0) {
    return false;
  }

  SENDARROW;
  print(msg);
  print((char)0x1A);

  return waitResponse(60000L, TO_F(TOKEN_CMGS)) == 0 && waitResponse() == 0;
}

//initialize sim detection with AT+QSIMDET=1,0 command
bool BG95ESP32::initSimDetection(bool enable, bool pinbehave) {
  if (enable) {
    if (sendFormatAT(TO_F(AT_COMMAND_SIM_DETECTOR), 1, 0), waitResponse(2000L) != -1) {
      return waitResponse(TOKEN_OK) == 0;
    }
  } else {
    if (sendFormatAT(TO_F(AT_COMMAND_SIM_DETECTOR), 0, 0), waitResponse(2000L) != -1) {
      return waitResponse(TOKEN_OK) == 0;
    }
  }
  return false;
}
