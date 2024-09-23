#include "BG95AT.h"
#include <errno.h>
#define AT_TERMINATOR '\r\n'

bool (*parseMQTTmessage)(uint8_t, String, String);
void (*tcpOnClose)(uint8_t clientID);

void BG95AT::begin(Stream &port) {
  _port = &port;
  _output.begin(LOG_LEVEL_VERBOSE, this, false);
#if _BG95ESP32_DEBUG
  _debug.begin(LOG_LEVEL_VERBOSE, &Serial, false);
#endif  // _BG95ESP32_DEBUG
}

void BG95AT::flushInput() {
  uint16_t timeout = 0;
  while (readNext(replyBuffer, BUFFER_SIZE, &timeout));
  memset(replyBuffer, 0, BUFFER_SIZE);
}

size_t BG95AT::readNext(char *buffer, size_t size, uint16_t *timeout, char stop) {
  size_t i = 0;
  bool exit = false;

  do {
    while (!exit && i < size - 1 && available()) {
      char c = read();
      buffer[i] = c;
      i++;

      exit |= stop && c == stop;
    }

    if (timeout) {
      if (*timeout) {
        delay(1);
        (*timeout)--;
      }

      if (!(*timeout)) {
        break;
      }
    }
  } while (!exit && i < size - 1);

  buffer[i] = '\0';

  if (i) {
    RECEIVEARROW;
    BG95ESP32_PRINT(buffer);
  }

  return i > 0 ? i - 1 : i;
}

int8_t BG95AT::waitResponse(uint16_t timeout, ATConstStr s1, ATConstStr s2, ATConstStr s3, ATConstStr s4) {
  Log.notice(F("waitResponse: Entering function" NL));
  ATConstStr wantedTokens[4] = {s1, s2, s3, s4};
  size_t length;

  do {
    Log.notice(F("waitResponse: Clearing replyBuffer" NL));
    memset(replyBuffer, 0, BUFFER_SIZE);
    Log.notice(F("waitResponse: Reading next line" NL));
    length = readNext(replyBuffer, BUFFER_SIZE, &timeout, '\n');

    if (!length) {
      Log.notice(F("waitResponse: Read nothing, continuing" NL));
      continue;  //read nothing
    }
    if (wantedTokens[0] == NULL) {
      Log.notice(F("waitResponse: Looking for any line with content, returning 0" NL));
      return 0;  //looking for a line with any content
    }

    for (uint8_t i = 0; i < 4; i++) {
      if (wantedTokens[i]) {
        Log.notice(F("waitResponse: Checking token %d" NL), i);
        char *p = strstr_P(replyBuffer, TO_P(wantedTokens[i]));
        if (replyBuffer == p) {
          Log.notice(F("waitResponse: Found token %d, returning %d" NL), i, i);
          return i;
        }
      }
    }
  } while (timeout);

  Log.notice(F("waitResponse: Timeout reached, returning -1" NL));
  return -1;
}

size_t BG95AT::copyCurrentLine(char *dst, size_t dstSize, uint16_t shift) {
  Log.notice(F("copyCurrentLine: Copying current line with shift %d" NL), shift);
  char *p = dst;
  char *p1;

  Log.notice(F("copyCurrentLine: Copying buffer content to destination" NL));
  p += safeCopy(replyBuffer + shift, p, dstSize);  // copy the current buffer content

  //copy the rest of the line if any
  if (!strchr(dst, '\n')) {
    Log.notice(F("copyCurrentLine: No newline found, reading next part of the line" NL));
    uint16_t timeout = 2000;
    p += readNext(p, dstSize - (p - dst), &timeout, '\n');
  }

  // terminating the string no matter what
  p1 = strchr(dst, '\n');
  p = p1 ? p1 : p;
  *p = '\0';

  Log.notice(F("copyCurrentLine: Final copied line: %s" NL), dst);
  return strlen(dst);
}

size_t BG95AT::safeCopy(const char *src, char *dst, size_t dstSize) {
  Log.notice(F("safeCopy: Copying from source to destination with size %d" NL), dstSize);
  size_t len = strlen(src);
  if (dst != NULL) {
    size_t maxLen = min(len + 1, dstSize);
    strlcpy(dst, src, maxLen);
    Log.notice(F("safeCopy: Copied string: %s" NL), dst);
  }

  return len;
}

char *BG95AT::find(const char *str, char divider, uint8_t index) {
  Log.notice(F("find: Searching for divider '%c' at index %d in string: %s" NL), divider, index, str);
  char *p = strchr(str, ':');
  if (p == NULL) {
    Log.notice(F("find: ':' not found, searching for first character '%c'" NL), str[0]);
    p = strchr(str, str[0]);  // ditching eventual response header
  }

  if (p != NULL) {
    p++;
    for (uint8_t i = 0; i < index; i++) {
      p = strchr(p, divider);
      if (p == NULL) {
        Log.notice(F("find: Divider '%c' not found at index %d" NL), divider, i);
        return NULL;
      }
      p++;
    }
    Log.notice(F("find: Found divider '%c' at index %d" NL), divider, index);
  } else {
    Log.notice(F("find: Initial search character not found" NL));
  }

  return p;
}

bool BG95AT::parse(const char *str, char divider, uint8_t index, uint8_t *result) {
  Log.notice("Parsing uint8_t from string: %s" NL, str);
  uint16_t tmpResult;
  if (!parse(str, divider, index, &tmpResult)) {
    return false;
  }

  *result = (uint8_t)tmpResult;
  Log.notice("Parsed uint8_t result: %u" NL, *result);
  return true;
}

bool BG95AT::parse(const char *str, char divider, uint8_t index, int8_t *result) {
  Log.notice("Parsing int8_t from string: %s" NL, str);
  int16_t tmpResult;
  if (!parse(str, divider, index, &tmpResult)) {
    return false;
  }

  *result = (int8_t)tmpResult;
  Log.notice("Parsed int8_t result: %d" NL, *result);
  return true;
}

bool BG95AT::parse(const char *str, char divider, uint8_t index, uint16_t *result) {
  Log.notice("Parsing uint16_t from string: %s" NL, str);
  char *p = find(str, divider, index);
  if (p == NULL) {
    return false;
  }

  errno = 0;
  *result = strtoul(p, NULL, 10);
  Log.notice("Parsed uint16_t result: %u" NL, *result);

  return errno == 0;
}

#if defined(NEED_SIZE_T_OVERLOADS)
bool BG95AT::parse(const char *str, char divider, uint8_t index, size_t *result) {
  Log.notice("Parsing size_t from string: %s", str);
  char *p = find(str, divider, index);
  if (p == NULL) {
    return false;
  }

  errno = 0;
  *result = strtoull(p, NULL, 10);
  Log.notice("Parsed size_t result: %zu", *result);

  return errno == 0;
}
#endif

bool BG95AT::parse(const char *str, char divider, uint8_t index, int16_t *result) {
  Log.notice("Parsing int16_t from string: %s" NL, str);
  char *p = find(str, divider, index);
  if (p == NULL) {
    return false;
  }

  errno = 0;
  *result = strtol(p, NULL, 10);
  Log.notice("Parsed int16_t result: %d" NL, *result);

  return errno == 0;
}

bool BG95AT::parse(const char *str, char divider, uint8_t index, float *result) {

  /* 
//from BG95 library

String BG95AT::get_command(String command, uint32_t timeout){

	send_command(command);
	delay(AT_WAIT_RESPONSE);

	String data = "";
	timeout += millis();
	while (timeout >= millis()) {
		if (available()) {
			String response = _port->readStringUntil(AT_TERMINATOR);
			response.trim();

			if (response.length() == 0) continue; // garbage

			#ifdef DEBUG_BG95_HIGH
				log("<< " +response);
			#endif

			data += parse_command_line(response);

			//if(response.indexOf("OK") > -1 && data.length() > 0)
			if(response.indexOf("OK") > -1)
				return data;

			if(response.indexOf("ERROR") > -1)
				return "ERROR";
		}

		delay(AT_WAIT_RESPONSE);
	}

	return data;
}

// filters the rcv response and waits for OK to validate it
String BG95AT::get_command(String command, String filter, uint32_t timeout){

	send_command(command);
	delay(AT_WAIT_RESPONSE);

	String data = "";
	timeout += millis();
	while (timeout >= millis()) {
		if (available()) {
			String response = _port->readStringUntil(AT_TERMINATOR);

			response.trim();

			if (response.length() == 0) continue; // garbage

			#ifdef DEBUG_BG95_HIGH
				log("<< " +response);
			#endif

			parse_command_line(response);

			if(response.startsWith(filter))
				data = response.substring(filter.length());

			//
			//if(response.indexOf("OK") > -1)
			//return data;
			//
			if(response.indexOf("ERROR") > -1)
				return "";
		}else{
			if(data.length() > 0)
				return data;
		}

		delay(AT_WAIT_RESPONSE);
	}

	return data;
}

// filters the rcv response and waits for OK to validate it
String BG95AT::get_command_critical(String command, String filter, uint32_t timeout){

	send_command(command);
	delay(AT_WAIT_RESPONSE);

	String data = "";
	timeout += millis();
	while (timeout >= millis()) {
		if (available()) {
			String response = _port->readStringUntil(AT_TERMINATOR);

			response.trim();

			if (response.length() == 0) continue; // garbage

			#ifdef DEBUG_BG95_HIGH
				log("<< " +response);
			#endif

			//parse_command_line(response);

			if(response.startsWith(filter))
				data = response.substring(filter.length());

			if(response.indexOf("OK") > -1 && data.length() > 0)
				return data;

			if(response.indexOf("ERROR") > -1)
				return "";
		}else{
			if(data.length() > 0)
				return data;
		}

		delay(AT_WAIT_RESPONSE);
	}

	return data;
}

// filters the rcv response
String BG95AT::get_command_no_ok(String command, String filter, uint32_t timeout){

	send_command(command);
	delay(AT_WAIT_RESPONSE);

	String data = "";
	timeout += millis();
	while (timeout >= millis()) {
		if (available()) {
			String response = _port->readStringUntil(AT_TERMINATOR);

			response.trim();

			if (response.length() == 0) continue; // garbage

			#ifdef DEBUG_BG95_HIGH
				log("<< " +response);
			#endif

			parse_command_line(response);

			if(response.startsWith(filter)){
				data = response.substring(filter.length());
				return data;
			}

			if(response.indexOf("ERROR") > -1)
				return "";
		}

		delay(AT_WAIT_RESPONSE);
	}

	return data;
}

// filters the rcv response
String BG95AT::get_command_no_ok_critical(String command, String filter, uint32_t timeout){

	send_command(command);
	delay(AT_WAIT_RESPONSE);

	String data = "";
	timeout += millis();
	while (timeout >= millis()) {
		if (available()) {
			String response = _port->readStringUntil(AT_TERMINATOR);

			response.trim();

			if (response.length() == 0) continue; // garbage

			#ifdef DEBUG_BG95_HIGH
				log("<< " +response);
			#endif

			response = parse_command_line(response,true);

			if(response.startsWith("+QMTRECV:")){ // parse MQTT received messages
				mqtt_message_received(response);
			}else if(response.startsWith(filter)){
				data = response.substring(filter.length());
				return data;
			}

			if(response.indexOf("ERROR") > -1)
				return "";
		}

		delay(AT_WAIT_RESPONSE);
	}

	return data;
}

// wait for at command status
bool BG95AT::wait_command(String filter, uint32_t timeout){

	delay(AT_WAIT_RESPONSE);

	String data = "";
	timeout += millis();
	while (timeout >= millis()) {
		if (available()) {
			String response = _port->readStringUntil(AT_TERMINATOR);

			response.trim();

			if (response.length() == 0) continue; // garbage

			#ifdef DEBUG_BG95_HIGH
				log("<< " +response);
			#endif

			parse_command_line(response);

			if(response.startsWith(filter)){
				return true;
			}

		}

		delay(AT_WAIT_RESPONSE);
	}

	return false;
}

// use it when OK comes in the end
bool BG95AT::check_command(String command, String ok_result, uint32_t wait) {
	bool response_expected = false;
	send_command(command);
	delay(AT_WAIT_RESPONSE);

	uint32_t timeout = millis() + wait;

	while (timeout >= millis()) {
		if (available()) {
			String response = _port->readStringUntil(AT_TERMINATOR);

			response.trim();

			if (response.length() == 0) continue;

			#ifdef DEBUG_BG95_HIGH
				log("<< " +response);
			#endif

			parse_command_line(response);

			if (response == ok_result) {
				response_expected = true;
				//break;
			}

			if (response == "ERROR") {
				break;
			}

			if (response.indexOf("+CME ERROR") > -1)
				break;

			if (response.indexOf("OK") > -1)
				break;

		}

		delay(AT_WAIT_RESPONSE);
	}
	
	// if(response_expected){
	// 	// check if there is an ok in the end of sentence
	// 	String response = _port->readStringUntil(AT_TERMINATOR);
	// 	response.trim();
	// 	#ifdef DEBUG_BG95_HIGH
	// 		log("<< " +response);
	// 	#endif
	// }
	
	return response_expected;
}

// use it when OK comes in the end
bool BG95AT::check_command(String command, String ok_result, String error_result, uint32_t wait) {
	bool response_expected = false;
	send_command(command);
	delay(AT_WAIT_RESPONSE);

	uint32_t timeout = millis() + wait;

	while (timeout >= millis()) {
		if (available()) {
			String response = _port->readStringUntil(AT_TERMINATOR);

			response.trim();

			if (response.length() == 0) continue;

			#ifdef DEBUG_BG95_HIGH
				log("<< " +response);
			#endif

			parse_command_line(response);

			if (response == ok_result) {
				response_expected = true;
			}

			if (response == error_result) {
				break;
			}

			if (response.indexOf("+CME ERROR") > -1)
				break;

			if (response.indexOf("OK") > -1)
				break;

		}

		delay(AT_WAIT_RESPONSE);
	}

	return response_expected;
}

// use it when OK comes before the ok_result
bool BG95AT::check_command_no_ok(String command, String ok_result, uint32_t wait) {
	bool response_expected = false;
	send_command(command);
	delay(AT_WAIT_RESPONSE);

	uint32_t timeout = millis() + wait;

	while (timeout >= millis()) {
		if (available()) {
			String response = _port->readStringUntil(AT_TERMINATOR);

			response.trim();

			if (response.length() == 0) continue;

			#ifdef DEBUG_BG95_HIGH
				log("<< " +response);
			#endif

			parse_command_line(response);

			if (response == ok_result) {
				response_expected = true;
				break;
			}

			if (response == "ERROR") {
				break;
			}

			if (response.indexOf("+CME ERROR") > -1)
				break;

		}

		delay(AT_WAIT_RESPONSE);
	}

	return response_expected;
}

// use it when OK comes before the ok_result
bool BG95AT::check_command_no_ok(String command, String ok_result, String error_result, uint32_t wait) {
	bool response_expected = false;
	send_command(command);
	delay(AT_WAIT_RESPONSE);

	uint32_t timeout = millis() + wait;

	while (timeout >= millis()) {
		if (available()) {
			String response = _port->readStringUntil(AT_TERMINATOR);

			response.trim();

			if (response.length() == 0) continue;

			#ifdef DEBUG_BG95_HIGH
				log("<< " +response);
			#endif

			parse_command_line(response);

			if (response == ok_result) {
				response_expected = true;
				break;
			}

			if (response == error_result) {
				break;
			}

			if (response.indexOf("+CME ERROR") > -1)
				break;

		}

		delay(AT_WAIT_RESPONSE);
	}

	return response_expected;
}

void BG95AT::check_commands() {

	if (_port->available()) {
		String response = _port->readStringUntil(AT_TERMINATOR);

		response.trim();

		#ifdef DEBUG_BG95_HIGH
			log("<< " +response);
		#endif

		parse_command_line(response);
	}

}

String BG95AT::parse_command_line(String line, bool set_data_pending) {
	//log("parse: "+line);

	String _cgreg = "+CGREG: ";
	String _cereg = "+CEREG: ";
	String _creg = "+CREG: ";
	int8_t index = -1;

	if(line.startsWith("AT+")){
		log("echo is enabled, disable it");
		send_command("ATE0");
	}

	if (line.startsWith(_cgreg)) {
		index = line.indexOf(",");
		if(index > -1)
			line = line.substring(index+1,index+2);
		else line = line.substring(_cgreg.length(),_cgreg.length()+1);
		if(isNumeric(line)){
			int radio_state = line.toInt();
			#ifdef DEBUG_BG95
			switch(radio_state){
				case 0:
					log("EGPRS not registered");
					break;
				case 1:
					log("EGPRS register");
					break;
				case 2:
					log("EGPRS connecting");
					break;
				case 3:
					log("EGPRS registration denied");
					break;
				case 4:
					log("EGPRS Unknow");
					break;
				case 5:
					log("EGPRS registered, roaming");
					break;
			}
			#endif
		}
		return "";
	}else if (line.startsWith(_cereg)) {
		index = line.indexOf(",");
		if(index > -1)
			line = line.substring(index+1,index+2);
		else line = line.substring(_cereg.length(),_cereg.length()+1);
		if(isNumeric(line)){
			int8_t radio_state = line.toInt();
			switch(radio_state){
				case 0:
					log("LTE not registered");
					break;
				case 1:
					log("LTE registered");
					break;
				case 2:
					log("LTE connecting");
					break;
				case 3:
					log("LTE registration denied");
					break;
				case 4:
					log("LTE Unknow");
					break;
				case 5:
					log("LTE registered, roaming");
					break;
			}
		}
		return "";
	}else if (line.startsWith(_creg)) {
		String connected_ = "";
		String technology_ = "";

		index = line.indexOf(",");
		if(index > -1){
			line = line.substring(index+1);
			index = line.indexOf(",");
			if(index > -1){
				connected_ = line.substring(0,index);
				//log("connected: "+String(connected_));
				line = line.substring(index+1);
			}else{
				connected_ = line;
				//log("connected: "+String(connected_));
			}
		}else return "";

		index = line.indexOf(",");
		if(index > -1){
			line = line.substring(index+1);
			index = line.indexOf(",");
			if(index > -1){
				technology_ = line.substring(index+1);
				//log("technology: "+String(technology_));
			}else return "";
		}else return "";

		if(isNumeric(connected_)){
			int8_t act = -1;
			int8_t radio_state = connected_.toInt();
			if(technology_ != "")
				act = technology_.toInt();
			else act = -1;

			switch(radio_state){
				case 0:
					 //st.apn_lte = false;
					 //st.apn_gsm = false;
					break;
				case 1:
					if(act == 0){
						//st.apn_gsm = true;
						//st.apn_lte = false;
					}else if(act == 9){
						//st.apn_gsm = false;
						//st.apn_lte = true;
					}
					break;
				case 2:
					if(act == 0){
						////st.apn_gsm = 2;
						//st.apn_gsm = false;
						//st.apn_lte = false;
					}else if(act == 9){
						//st.apn_gsm = false;
						////st.apn_lte = 2;
						//st.apn_lte = false;
					}
					break;
				case 3:
					//st.apn_gsm = false;
					//st.apn_lte = false;
					break;
				case 4:
					//st.apn_gsm = false;
					//st.apn_lte = false;
					break;
				case 5:
					if(act == 0){
						//st.apn_gsm = true;
						//st.apn_lte = false;
					}else if(act == 9){
						//st.apn_gsm = false;
						//st.apn_lte = true;
					}
					break;
			}
		}

	}else if (line.startsWith("+QIOPEN:")) {

		int8_t index = line.indexOf(",");
		uint8_t cid = 0;
		int8_t state = -1;
		if(index > -1){
			cid = line.substring(index-1,index).toInt();
			state = line.substring(index+1).toInt();
		}
		if(cid >= MAX_TCP_CONNECTIONS)
			return "";

		if(state == 0){
			#ifdef DEBUG_BG95
			log("TCP is connected");
			#endif
			tcp[cid].connected = true;
		}else{
			log("Error opening TCP");
			tcp[cid].connected = false;
		}
		
		// for (uint8_t index = 0; index < MAX_CONNECTIONS; index++) {
		// 	if (!line.startsWith("C: " + String(index) + ",")) continue;

		// 	connected_until[index] = millis() + CONNECTION_STATE;
		// 	connected_state[index] = line.endsWith("\"CONNECTED\"");

		// 	#ifdef DEBUG_BG95
		// 	if (connected_state[index]) log("socket " + String(index) + " = connected");
		// 	#endif
		// }
		
		return "";
	}else if (line.startsWith("+QIURC: \"recv\",") ) {
		String cid_str = line.substring(15);
		uint8_t cid = cid_str.toInt();
		if(cid >= MAX_TCP_CONNECTIONS)
			return "";
		if (set_data_pending) {
			data_pending[cid] = true;
			return "";
		} else {
			tcp_read_buffer(cid);
			return "";
		}
	}else if (line.startsWith("+QIURC: \"closed\",") ) {
		#ifdef DEBUG_BG95
		log("QIURC closed: "+line);
		#endif
		int8_t index = line.indexOf(",");
		int8_t cid = -1;
		if(index > -1){
			state = state.substring(index+1,index+1);
			cid = state.toInt();
			if(cid >= MAX_TCP_CONNECTIONS)
				return "";
			#ifdef DEBUG_BG95_HIGH
			log("connection: "+String(cid)+" closed");
			#endif
			tcp[cid].connected = false;
			tcp_close(cid);
		}
	}else if (line.startsWith("+QSSLURC: \"recv\",") ) {
		String cid_str = line.substring(17);
		uint8_t cid = cid_str.toInt();
		if(cid >= MAX_TCP_CONNECTIONS)
			return "";
		if (set_data_pending) {
			data_pending[cid] = true;
			return "";
		} else {
			tcp_read_buffer(cid);
			return "";
		}
	}else if (line.startsWith("+QSSLURC: \"closed\",") ) {
		#ifdef DEBUG_BG95
		log("QIURC closed: "+line);
		#endif
		int8_t index = line.indexOf(",");
		int8_t cid = -1;
		if(index > -1){
			state = state.substring(index+1,index+1);
			cid = state.toInt();
			if(cid >= MAX_TCP_CONNECTIONS)
				return "";
			#ifdef DEBUG_BG95_HIGH
			log("connection: "+String(cid)+" closed");
			#endif
			tcp[cid].connected = false;
			tcp_close(cid);
		}
	}else if (line.startsWith("+CMTI")){
		check_sms();
		return "";
	}else if (line.startsWith("+QIACT: ")) {
		line = line.substring(8);
		int8_t index = line.indexOf(",");
		uint8_t cid = 0;
		if(index > -1){
			cid = line.substring(0,index).toInt(); // connection
			line = line.substring(index+1);
		}else{
			return "";
		}

		if(cid == 0 || cid > MAX_CONNECTIONS)
			return "";

		int8_t state = line.substring(0,1).toInt(); // connection
		if(state == 1){
			#ifdef DEBUG_BG95_HIGH
			log("network connected: "+String(cid));
			#endif
			apn[cid-1].connected = true;
			apn[cid-1].retry = 0;
		}else{
			#ifdef DEBUG_BG95
			log("network disconnected: "+String(cid));
			#endif
			apn[cid-1].connected = false;
		}
		line = line.substring(index+1);

		index = line.lastIndexOf(",");
		if(index > -1){
			String ip = line.substring(index+1);
			ip.replace("\"","");
			memset(apn[cid-1].ip,0,sizeof(apn[cid-1].ip));
			memcpy(apn[cid-1].ip,ip.c_str(),ip.length());
		}else{
			return "";
		}

	}else if(line.startsWith("+QMTSTAT")){
		// error ocurred, channel is disconnected
		String filter = "+QMTSTAT: ";
		uint8_t index = line.indexOf(filter);
		line = line.substring(index);
		index = line.indexOf(",");
		if(index > -1){
			String client = line.substring(filter.length(),index);
			#ifdef DEBUG_BG95_HIGH
			log("client: "+client);
			#endif
			if(isNumeric(client)){
				uint8_t id = client.toInt();
				if(id <= MAX_MQTT_CONNECTIONS){
					mqtt[id].socket_state = MQTT_STATE_DISCONNECTED;
					mqtt[id].connected = false;
				}
				#ifdef DEBUG_BG95
				log("MQTT closed");
				#endif
			}
		}
	}else if(line.startsWith("+QMTRECV:")){
		return mqtt_message_received(line);
	}else if (line.startsWith("+QMTCONN: ")) {
		String filter = "+QMTCONN: ";
		line = line.substring(filter.length());
		index = line.indexOf(",");
		if(index > -1){
			uint8_t cidx = line.substring(0,index).toInt();
			if(cidx < MAX_MQTT_CONNECTIONS){
				if(line.lastIndexOf(",") != index){
					index = line.lastIndexOf(",");
					String state = line.substring(index+1,index+2);
					if((int)state.toInt() == 0){
						mqtt[cidx].socket_state = MQTT_STATE_CONNECTED;
						mqtt[cidx].connected = true;
						mqtt[cidx].unknow_counter = 0;
					}else{
						mqtt[cidx].socket_state = MQTT_STATE_DISCONNECTED;
						mqtt[cidx].connected = false;
						switch((int)state.toInt()){
							case 0:
								log("mqtt client "+String(cidx)+" Connection refused - Unacceptable Protocol Version");
							case 1:
								log("mqtt client "+String(cidx)+" Connection refused - Identifier Rejected");
							case 2:
								log("mqtt client "+String(cidx)+" Connection refused - Server Unavailable");
							case 3:
								log("mqtt client "+String(cidx)+" Connection refused - Not Authorized");
						}
					}
				}else{
					String state = line.substring(index+1,index+2);
					if(isdigit(state.c_str()[0])){
						mqtt[cidx].socket_state = (int)state.toInt();
						mqtt[cidx].connected = (int)(state.toInt()==MQTT_STATE_CONNECTED);
						if(mqtt[cidx].connected)
							mqtt[cidx].unknow_counter = 0;
						#ifdef DEBUG_BG95
						if(mqtt[cidx].connected)
						log("mqtt client "+String(cidx)+" is connected");
						else
						log("mqtt client "+String(cidx)+" is disconnected");
						#endif
						return "";
					}
				}
			}
		}
	}else if(line.startsWith("OK"))
		return "";

	return line;
}

boolean BG95AT::isNumeric(String str) {

    unsigned int StringLength = str.length();

    if (StringLength == 0) {
        return false;
    }

    boolean seenDecimal = false;

    for(unsigned int i = 0; i < StringLength; ++i) {
        if (isDigit(str.charAt(i))) {
            continue;
        }

        if (str.charAt(i) == '.') {
            if (seenDecimal) {
                return false;
            }
            seenDecimal = true;
            continue;
        }

				if (str.charAt(i) == '-')
            continue;

        return false;
    }
    return true;
}

void BG95AT::log(String text) {
	//log_output->println("[" + date() + "] bgxx: " + text);
	//log_output->println("bgxx: " + text);
	Log.notice(S_F("bgxx: \"%s\""  NL), text);
}

// --- TCP ---

void BG95AT::tcp_set_callback_on_close(void(*callback)(uint8_t clientID)){

	tcpOnClose = callback;
}
// 
// * connect to a host:port
// *
// * @clientID - connection id 1-11, yet it is limited to MAX_TCP_CONNECTIONS
// * @host - can be IP or DNS
// * @wait - maximum time to wait for at command response in ms
// *
// * return true if connection was established
// 
bool BG95AT::tcp_connect(uint8_t clientID, String host, uint16_t port, uint16_t wait) {

	uint8_t contextID = 1;
	if(apn_connected(contextID) != 1)
		return false;

	if (clientID >= MAX_TCP_CONNECTIONS) return false;

	memset(tcp[clientID].server,0,sizeof(tcp[clientID].server));
	memcpy(tcp[clientID].server,host.c_str(),host.length());
	tcp[clientID].port = port;
	tcp[clientID].active = true;
	tcp[clientID].ssl = false;

	if(check_command_no_ok("AT+QIOPEN="+String(contextID)+","+String(clientID) + ",\"TCP\",\"" + host + "\","
	+ String(port),"+QIOPEN: "+String(clientID)+",0","ERROR"),wait){
		tcp[clientID].connected = true;
		return true;
	}else tcp_close(clientID);

	get_command("AT+QIGETERROR");

	return false;
}


// * connect to a host:port
// *
// * @ccontextID - context id 1-16, yet it is limited to MAX_CONNECTIONS
// * @clientID - connection id 0-11, yet it is limited to MAX_TCP_CONNECTIONS
// * @host - can be IP or DNS
// * @wait - maximum time to wait for at command response in ms
// *
// * return true if connection was established

bool BG95AT::tcp_connect(uint8_t contextID, uint8_t clientID, String host, uint16_t port, uint16_t wait) {
	if(apn_connected(contextID) != 1)
		return false;

	if (contextID == 0 || contextID > MAX_CONNECTIONS) return false;

	if (clientID >= MAX_TCP_CONNECTIONS) return false;

	memset(tcp[clientID].server,0,sizeof(tcp[clientID].server));
	memcpy(tcp[clientID].server,host.c_str(),host.length());
	tcp[clientID].port = port;
	tcp[clientID].active = true;
	tcp[clientID].ssl = false;

	if(check_command_no_ok("AT+QIOPEN="+String(contextID)+","+String(clientID) + ",\"TCP\",\"" + host + "\","
	+ String(port),"+QIOPEN: "+String(clientID)+",0","ERROR"),wait){
		tcp[clientID].connected = true;
		return true;
	}else tcp_close(clientID);

	get_command("AT+QIGETERROR");

	return false;
}


// * connect to a host:port using ssl
// *
// * @contextID - context id 1-16, yet it is limited to MAX_CONNECTIONS
// * @sslClientID - id 0-5
// * @clientID - connection id 0-11, yet it is limited to MAX_TCP_CONNECTIONS
// * @host - can be IP or DNS
// * @wait - maximum time to wait for at command response in ms
// *
// * return true if connection was established

bool BG95AT::tcp_connect_ssl(uint8_t contextID, uint8_t sslClientID, uint8_t clientID, String host, uint16_t port, uint16_t wait) {
	if(apn_connected(contextID) != 1)
		return false;

	if (contextID == 0 || contextID > MAX_CONNECTIONS) return false;

	if (clientID >= MAX_TCP_CONNECTIONS) return false;

	memset(tcp[clientID].server,0,sizeof(tcp[clientID].server));
	memcpy(tcp[clientID].server,host.c_str(),host.length());
	tcp[clientID].port = port;
	tcp[clientID].active = true;
	tcp[clientID].ssl = true;
	tcp[clientID].sslClientID = sslClientID;

	if(check_command_no_ok("AT+QSSLOPEN="+String(contextID)+","+String(sslClientID)+","+String(clientID) + ",\"" + host + "\","
	+ String(port),"+QSSLOPEN: "+String(clientID)+",0","ERROR"),wait){
		tcp[clientID].connected = true;
		return true;
	}else tcp_close(clientID);

	get_command("AT+QIGETERROR");

	return false;
}


// return tcp connection status

bool BG95AT::tcp_connected(uint8_t clientID) {

	if (clientID >= MAX_TCP_CONNECTIONS) return false;
	return tcp[clientID].connected;
}

// * close tcp connection
// *
// * return tcp connection status

bool BG95AT::tcp_close(uint8_t clientID) {

	if(clientID >= MAX_TCP_CONNECTIONS)
		return false;

	tcp[clientID].active = false;
	connected_since[clientID] = 0;
	data_pending[clientID] = false;

	if(tcp[clientID].ssl){
		if(check_command("AT+QSSLCLOSE=" + String(clientID),"OK", "ERROR", 10000)){
			tcp[clientID].connected = false;
			if(tcpOnClose != NULL)
			tcpOnClose(clientID);
		}
	}else{
		if(check_command("AT+QICLOSE=" + String(clientID),"OK", "ERROR", 10000)){
			tcp[clientID].connected = false;
			if(tcpOnClose != NULL)
			tcpOnClose(clientID);
		}
	}

	return tcp[clientID].connected;
}

// * send data through open channel
// *
// * returns true if succeed

bool BG95AT::tcp_send(uint8_t clientID, const char *data, uint16_t size) {
	if (clientID >= MAX_CONNECTIONS) return false;
	if (tcp_connected(clientID) == 0) return false;

	while (_port->available()) {
		String line = _port->readStringUntil(AT_TERMINATOR);

		line.trim();

		parse_command_line(line, true);
	}

	while (_port->available()) _port->read(); // delete garbage on buffer

	if(tcp[clientID].ssl){
		if (!check_command_no_ok("AT+QSSLSEND=" + String(clientID) + "," + String(size), ">", "ERROR"))
			return false;
	}else{
		if (!check_command_no_ok("AT+QISEND=" + String(clientID) + "," + String(size), ">", "ERROR"))
			return false;
	}

	send_command(data, size);
	delay(AT_WAIT_RESPONSE);

	uint32_t timeout        = millis() + 10000;
	String new_data_command = "";

	while (timeout >= millis()) {
		if (_port->available()) {
			String line = _port->readStringUntil(AT_TERMINATOR);

			line.trim();

			if (line.length() == 0) continue;

			//log("[send] ? '" + line + "'");
			#ifdef DEBUG_BG95
			log("parse: "+line);
			#endif
			parse_command_line(line, true);

			if (line.indexOf("SEND OK") > -1 || line.indexOf("OK") > -1) return true;

		}

		delay(AT_WAIT_RESPONSE);
	}

	tcp_check_data_pending();

	return false;
}

// * copies data to pointer if available
// *
// * returns len of data copied

uint16_t BG95AT::tcp_recv(uint8_t clientID, char *data, uint16_t size) {

	if (clientID >= MAX_TCP_CONNECTIONS) return false;

	if (buffer_len[clientID] == 0) return 0;

	uint16_t i;

	if (buffer_len[clientID] < size)
		size = buffer_len[clientID];

	for (i = 0; i < size; i++) {
		data[i] = buffers[clientID][i];
	}

	if (buffer_len[clientID] > size){
		for (i = size; i < buffer_len[clientID]; i++) {
			buffers[clientID][i - size] = buffers[clientID][i];
		}
	}

	buffer_len[clientID] -= size;

	return size;
}
// 
// * returns len of data available for clientID
// 
uint16_t BG95AT::tcp_has_data(uint8_t clientID){

	if (clientID >= MAX_TCP_CONNECTIONS) return 0;

	return buffer_len[clientID];
}

void BG95AT::check_sms() {
	send_command("AT+CMGL=\"ALL\"");
	//send_command("AT+CMGL=\"REC UNREAD\"");
	delay(AT_WAIT_RESPONSE);

	String   sms[7]; // index, status, origin, phonebook, date, time, msg
	//uint8_t  index     = 0;
	uint32_t timeout   = millis() + 10000;
	bool     found_sms = false;
	uint8_t counter = 0;

	while (timeout >= millis()) {
		if (_port->available()) {
			String ret = _port->readStringUntil(AT_TERMINATOR);

			ret.trim();
			if (ret.length() == 0) continue;
			if (ret == "OK") break;
			if (ret == "ERROR") break;

			log("[sms] '" + ret + "'");

			String index = "", msg_state = "", origin = "", msg = "";

			if (!ret.startsWith("+CMGL:")) {
				parse_command_line(ret, true);
			}

			// 
			// if (found_sms) continue;

			// found_sms = true;
			// 
			ret = ret.substring(6);
			ret.trim();

			uint8_t word = 0, last_i = 0;
			for (uint8_t i = 0; i < ret.length(); i++) {
				if (ret.charAt(i) == ',') {
					switch (word){
						case 0:
							message[counter].used   = true;
							index = ret.substring(0,i);
							#ifdef DEBUG_BG95
							log("index: "+index);
							#endif
							message[counter].index  = (uint8_t) index.toInt();
							last_i = i;
							break;
						case 1:
							msg_state = ret.substring(last_i,i);
							#ifdef DEBUG_BG95
							log("msg_state: "+msg_state);
							#endif
							last_i = i;
							break;
						case 2:
							origin = ret.substring(last_i,i);
							origin.replace("\"","");
							origin.replace(",","");
							//origin.replace("+","");
							#ifdef DEBUG_BG95
							log("origin: "+origin);
							#endif
							memcpy(message[counter].origin,origin.c_str(),i-last_i);
							last_i = i;
							break;
						default:
							break;
					}
					word++;
				}
			}

			if (_port->available()) {
				String ret = _port->readStringUntil(AT_TERMINATOR);
				ret.trim();
				msg = ret;
				#ifdef DEBUG_BG95
				log("msg: "+msg);
				#endif
				memcpy(message[counter].msg,msg.c_str(),ret.length());
			}

			if(counter < MAX_SMS-1)
				counter++;
			else
				continue;

		} else {
			delay(AT_WAIT_RESPONSE);
		}
	}

	for(uint8_t i = 0; i<counter; i++){
		process_sms(i);
		message[i].used = false;
		memset(message[i].origin,0,20);
		memset(message[i].msg,0,256);
	}
	return;
}

bool BG95AT::apn_connected(uint8_t contextID) {
	if(contextID == 0 || contextID > MAX_CONNECTIONS)
		return false;

	return apn[contextID-1].connected;
}

void BG95AT::process_sms(uint8_t index) {
	if (message[index].used && message[index].index >= 0) {
		message[index].used = false;

		sms_handler_func(message[index].index, String(message[index].origin), String(message[index].msg));
	}
}
 */
  Log.notice("Parsing float from string: %s", str);
  char *p = find(str, divider, index);
  if (p == NULL) {
    return false;
  }

  errno = 0;
  *result = strtod(p, NULL);
  Log.notice("Parsed float result: %f", *result);

  return errno == 0;
}

bool BG95AT::reset() {

  // APN reset
  op.ready = false;

  for (uint8_t i = 0; i < MAX_CONNECTIONS; i++) {
    data_pending[i] = false;
    apn[i].connected = false;
  }
  for (uint8_t i = 0; i < MAX_TCP_CONNECTIONS; i++) {
    tcp[i].connected = false;
  }
  for (uint8_t i = 0; i < MAX_MQTT_CONNECTIONS; i++) {
    mqtt[i].connected = false;
  }

  return true;
}
