#include "BG95ESP32.h"

AT_COMMAND(SET_HTTP_PARAMETER_STRING, "+HTTPPARA=\"%S\",\"%s\"");
AT_COMMAND(SET_HTTP_PARAMETER_STRING_PROGMEM, "+HTTPPARA=\"%S\",\"%S\"");
AT_COMMAND(SET_HTTP_PARAMETER_INT, "+HTTPPARA=\"%S\",\"%d\"");
AT_COMMAND(HTTP_DATA, "+HTTPDATA=%d,%d");
AT_COMMAND(HTTP_READ, "+HTTPREAD=%d,%d");

TOKEN_TEXT(HTTP_DATA, "+HTTPDATA");
TOKEN_TEXT(HTTP_ACTION, "+HTTPACTION");
TOKEN_TEXT(HTTP_READ, "+HTTPREAD");
TOKEN_TEXT(HTTP_INIT, "+HTTPINIT");
TOKEN_TEXT(HTTP_SSL, "+HTTPSSL");
TOKEN_TEXT(HTTP_TERM, "+HTTPTERM");
TOKEN(DOWNLOAD);

AT_COMMAND_PARAMETER(HTTP, CONTENT);
AT_COMMAND_PARAMETER(HTTP, REDIR);
AT_COMMAND_PARAMETER(HTTP, CID);
AT_COMMAND_PARAMETER(HTTP, URL);
AT_COMMAND_PARAMETER(HTTP, UA);


bool BG95ESP32::setHttpParameter(ATConstStr parameter, ATConstStr value)
{
	sendFormatAT(TO_F(AT_COMMAND_SET_HTTP_PARAMETER_STRING_PROGMEM), parameter, value);
	return waitResponse() == 0;
}

#if defined(__AVR__)

bool BG95ESP32::setHttpParameter(ATConstStr parameter, const char * value)
{
	sendFormatAT(TO_F(AT_COMMAND_SET_HTTP_PARAMETER_STRING), parameter, value);
	return waitResponse() == 0;
}

#endif

bool BG95ESP32::setHttpParameter(ATConstStr parameter, uint8_t value)
{
	sendFormatAT(TO_F(AT_COMMAND_SET_HTTP_PARAMETER_INT), parameter, value);
	return waitResponse() == 0;
}

uint16_t BG95ESP32::httpGet(const char *url, char *response, size_t responseSize)
{
	uint16_t statusCode = 0;
	size_t dataSize = 0;

	bool result = setupHttpRequest(url) &&
		fireHttpRequest(BG95ESP32HttpAction::Get, &statusCode, &dataSize) &&
		readHttpResponse(response, responseSize, dataSize) &&
		httpEnd();

	return statusCode;
}

uint16_t BG95ESP32::httpPost(const char *url, ATConstStr contentType, const char *body, char *response, size_t responseSize)
{
	uint16_t statusCode = 0;
	size_t dataSize = 0;

	bool result = setupHttpRequest(url) &&
		setHttpParameter(TO_F(AT_COMMAND_PARAMETER_HTTP_CONTENT), contentType) &&
		setHttpBody(body) &&
		fireHttpRequest(BG95ESP32HttpAction::Post, &statusCode, &dataSize) &&
		readHttpResponse(response, responseSize, dataSize) &&
		httpEnd();

	return statusCode;
}

bool BG95ESP32::setupHttpRequest(const char* url)
{
	httpEnd();

	return httpInit() &&
		setHttpParameter(TO_F(AT_COMMAND_PARAMETER_HTTP_REDIR), 1) &&
		setHttpParameter(TO_F(AT_COMMAND_PARAMETER_HTTP_CID), 1) &&
		setHttpParameter(TO_F(AT_COMMAND_PARAMETER_HTTP_URL), url) &&
		(url[4] != 's' || (sendAT(TO_F(TOKEN_HTTP_SSL), TO_F(TOKEN_WRITE), 1), waitResponse() == 0)) &&
		(_userAgent == NULL || setHttpParameter(TO_F(AT_COMMAND_PARAMETER_HTTP_UA), _userAgent));
}

bool BG95ESP32::httpInit()
{
	return (sendAT(TO_F(TOKEN_HTTP_INIT)), waitResponse() == 0);
}

bool BG95ESP32::httpEnd()
{
	return (sendAT(TO_F(TOKEN_HTTP_TERM)), waitResponse() == 0);
}

bool BG95ESP32::setHttpBody(const char* body)
{
	sendFormatAT(TO_F(AT_COMMAND_HTTP_DATA), strlen(body), 10000L);
	
	if(waitResponse(TO_F(TOKEN_DOWNLOAD)) != 0) return false;		

	SENDARROW;
	print(body);

	if(waitResponse() != 0) return false;
	return true;
}

bool BG95ESP32::fireHttpRequest(const BG95ESP32HttpAction action, uint16_t *statusCode, size_t *dataSize)
{
	sendAT(TO_F(TOKEN_HTTP_ACTION), TO_F(TOKEN_WRITE), (uint8_t)action);

	return waitResponse(HTTP_TIMEOUT, TO_F(TOKEN_HTTP_ACTION)) == 0 &&
		parseReply(',', (uint8_t)BG95ESP32HttpActionResponse::StatusCode, statusCode) &&
		parseReply(',', (uint8_t)BG95ESP32HttpActionResponse::DataLen, dataSize);
}

bool BG95ESP32::readHttpResponse(char *response, size_t responseSize, size_t dataSize)
{
	size_t readSize = min(responseSize - 1, dataSize);

	sendFormatAT(TO_F(AT_COMMAND_HTTP_READ), 0, readSize);
	if(waitResponse(TO_F(TOKEN_HTTP_READ)) != 0) return false;

	readNext(response, readSize + 1); // taking in account the string term
	return waitResponse() == 0;
}
