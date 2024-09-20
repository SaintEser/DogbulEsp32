#pragma once

#include <Arduino.h>
#include <ArduinoLog.h>
#include "BG95AT.Common.h"

#define _BG95ESP32_DEBUG 1//_DEBUG

#if _BG95ESP32_DEBUG
	#define BG95ESP32_PRINT(...) _debug.verbose(__VA_ARGS__)
	#define BG95ESP32_PRINT_CHAR(x) Serial.print((char)x)
	#define BG95ESP32_PRINT_P(fmt, ...) _debug.verbose(S_F(fmt "\n"), __VA_ARGS__)
	#define BG95ESP32_PRINT_SIMPLE_P(fmt) _debug.verbose(S_F(fmt "\n"))

	#define RECEIVEARROW _debug.verbose(S_F("<--"))
	#define SENDARROW _debug.verbose(S_F("\n-->"))
#else
	#define BG95ESP32_PRINT(...)
	#define BG95ESP32_PRINT_CHAR(x)
	#define BG95ESP32_PRINT_P(x, ...)
	#define BG95ESP32_PRINT_SIMPLE_P(x)
	
	#define RECEIVEARROW 
	#define SENDARROW
#endif // _DEBUG

#if SIZE_MAX > UINT16_MAX
	#define NEED_SIZE_T_OVERLOADS
#endif

#define BUFFER_SIZE 256
#define BG95AT_DEFAULT_TIMEOUT 1000
#define AT_WAIT_RESPONSE 10

#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS				  4
#endif
#ifndef MAX_TCP_CONNECTIONS
#define MAX_TCP_CONNECTIONS			2
#endif
#ifndef MAX_MQTT_CONNECTIONS
#define MAX_MQTT_CONNECTIONS		2
#endif
#ifndef CONNECTION_BUFFER
#define CONNECTION_BUFFER			  650 // bytes
#endif
#ifndef CONNECTION_STATE
#define CONNECTION_STATE			  10000 // millis
#endif
#ifndef SMS_CHECK_INTERVAL
#define SMS_CHECK_INTERVAL			30000 // milli
#endif
#ifndef MQTT_RECV_MODE
#define MQTT_RECV_MODE				  1 // 0 - for urc containing payload; 1 - to use buffers
#endif

#define GSM 	1
#define GPRS 	2
#define NB 		3
#define CATM1 	4
#define AUTO 	5

#define NOT_REGISTERED	0
#define REGISTERED		1
#define CONNETING		2
#define DENIED			3
#define UNKNOW			4
#define ROAMING			4

#define MQTT_STATE_DISCONNECTED 	0
#define MQTT_STATE_INITIALIZING 	1
#define MQTT_STATE_CONNECTING 		2
#define MQTT_STATE_CONNECTED 		3
#define MQTT_STATE_DISCONNECTING 	4

// CONSTANTS
#define   AT_WAIT_RESPONSE      	10 // milis
#define   AT_TERMINATOR     		'\r\n'

#define MAX_SMS 10

class BG95AT : public Stream
{
private:

				struct SMS {
			bool		used;
			uint8_t index;
			char 		origin[20];
			char 		msg[256];
		};

		// configurations
		struct Modem {
			uint8_t pwkey;
			bool ready;
			bool did_config;
			bool sim_ready;
			uint8_t radio;
			uint16_t cops;
			bool force;
			char tech_string[16];
			uint8_t technology;
		};

		struct APN {
			char name[64];
			uint8_t contextID; // context id 1-16
			bool active;
			bool connected;
			uint32_t retry;
			char ip[15];
		};

		struct TCP {
			char server[64];
			uint16_t port;
			uint8_t contextID; // context id 1-16
			uint8_t connectID; // connect id 0-11
			bool ssl;
			uint8_t sslClientID;
			uint8_t socket_state;
			bool active;
			bool connected;
		};

		struct MQTT {
			char host[64];
			uint8_t contextID; // index for TCP tcp[] 1-16, limited to MAX_CONNECTIONS
			uint8_t clientID; // client id 0-5 (limited to MAX_MQTT_CONNECTIONS)
			uint8_t socket_state;
			bool active;
			bool connected;
			uint8_t unknow_counter;
		};

		struct HTTP{
				uint16_t body_len;
				char md5[16];
				char responseStatus[32];
				char contentType[32];
		};

		Modem op = {
			/* pwkey */ 			0,
			/* ready */ 			false,
			/* did_config */ 	false,
			/* sim_ready */ 	false,
			/* radio */			 	0,
			/* cops */				0,
			/* force */				false,
			/* tech_string */	"",
			/* technology */	0
		};

		// State
		String state;
		// IMEI
		String imei;
		// IP address
		String ip_address;
		// pending texts
		SMS message[MAX_SMS];

		int8_t mqtt_buffer[5] = {-1,-1,-1,-1,-1}; // index of msg to read
		int8_t mqtt_tries[5] = {0,0,0,0,0}; // index of msg to read

		APN apn[MAX_CONNECTIONS];
		TCP tcp[MAX_TCP_CONNECTIONS];
		MQTT mqtt[MAX_MQTT_CONNECTIONS];
		HTTP http;

		//mbedtls_md_context_t ctx;

		int32_t tz = 0;

		uint8_t cereg; // Unsolicited LTE commands
		uint8_t cgreg; // Unsolicited GPRS commands

		// --- TCP ---
		// size of each buffer
		uint16_t buffer_len[MAX_TCP_CONNECTIONS];
		// data pending of each connection
		bool data_pending[MAX_TCP_CONNECTIONS];
		// validity of each connection state
		uint32_t connected_until[MAX_TCP_CONNECTIONS];
		// last connection start
		uint32_t connected_since[MAX_TCP_CONNECTIONS];
		// data buffer for each connection
		char buffers[MAX_TCP_CONNECTIONS][CONNECTION_BUFFER];
		// --- --- ---

		uint32_t rssi_until = 20000;
		uint32_t loop_until = 0;
		uint32_t ready_until = 15000;

		// last rssi
		int16_t rssi_last = 99;

		// validity of sms check
		uint32_t sms_until = 60000;

		void (*sms_handler_func)(uint8_t, String, String) = NULL;

		uint32_t next_retry = 0;
		uint32_t clock_sync_timeout = 0;

		bool mqtt_pool = false;
		uint32_t mqtt_pool_timeout = 0;


protected:
	Stream* _port;
	Logging _output;
#if _BG95ESP32_DEBUG
	Logging _debug;
#endif

	char replyBuffer[BUFFER_SIZE];
	
	template<typename T> void writeStream(T last)
	{
		print(last);
	}

	template<typename T, typename... Args> void writeStream(T head, Args... tail)
	{
		print(head);
		writeStream(tail...);
	}

	template<typename... Args> void sendAT(Args... cmd)
	{
		SENDARROW;
		writeStream(TO_F(TOKEN_AT), cmd..., TO_F(TOKEN_NL));
	}

	template<typename T, typename... Args> void sendFormatAT(T format, Args... args)
	{
		SENDARROW;
		writeStream(TO_F(TOKEN_AT));
		_output.verbose(format, args...);
		writeStream(TO_F(TOKEN_NL));
	}

	/**
	 * Read and discard all content already waiting to be parsed. 
	 */
	void flushInput();
	/**
	 * Read at max size available chars into buffer until either the timeout is exhausted or
	 * the stop character is encountered. timeout and char are optional
	 * 
	 */
	size_t readNext(char * buffer, size_t size, uint16_t * timeout = NULL, char stop = 0);

	int8_t waitResponse(
		ATConstStr s1 = TO_F(TOKEN_OK),
		ATConstStr s2 = TO_F(TOKEN_ERROR),
		ATConstStr s3 = NULL,
		ATConstStr s4 = NULL) {
			return waitResponse(BG95AT_DEFAULT_TIMEOUT, s1, s2, s3, s4);
		};

	int8_t waitResponse(uint16_t timeout, 
		ATConstStr s1 = TO_F(TOKEN_OK),
		ATConstStr s2 = TO_F(TOKEN_ERROR),
		ATConstStr s3 = NULL,
		ATConstStr s4 = NULL);
		
	/**
	 * Read the current response line and copy it in response. Start at replyBuffer + shift
	 */
	size_t copyCurrentLine(char *dst, size_t dstSize, uint16_t shift = 0);
	
	size_t safeCopy(const char *src, char *dst, size_t dstSize);
	
	/**
	 * Find and return a pointer to the nth field of a string.
	 */
	char* find(const char* str, char divider, uint8_t index);
	/**
	 * Parse the nth field of a string as a uint8_t.
	 */
	bool parse(const char* str, char divider, uint8_t index, uint8_t* result);
	/**
	 * Parse the nth field of a string as a int8_t.
	 */
	bool parse(const char* str, char divider, uint8_t index, int8_t* result);
	/**
	 * Parse the nth field of a string as a uint16_t.
	 */
	bool parse(const char* str, char divider, uint8_t index, uint16_t* result);
#if defined(NEED_SIZE_T_OVERLOADS)
	/**
	 * Parse the nth field of a string as a size_t.
	 */
	bool parse(const char* str, char divider, uint8_t index, size_t* result);
#endif
	/**
	 * Parse the nth field of a string as a int16_t.
	 */
	bool parse(const char* str, char divider, uint8_t index, int16_t* result);
	/**
	 * Parse the nth field of a string as a float.
	 */
	bool parse(const char* str, char divider, uint8_t index, float* result);

	/**
	 * Parse the nth field of the reply buffer as a uint8_t.
	 */
	bool parseReply(char divider, uint8_t index, uint8_t* result) { return parse(replyBuffer, divider, index, result); }
	/**
	 * Parse the nth field of the reply buffer as a int8_t.
	 */
	bool parseReply(char divider, uint8_t index, int8_t* result) { return parse(replyBuffer, divider, index, result); }
	/**
	 * Parse the nth field of the reply buffer as a uint16_t.
	 */
	bool parseReply(char divider, uint8_t index, uint16_t* result) { return parse(replyBuffer, divider, index, result); }
#if defined(NEED_SIZE_T_OVERLOADS)
	/**
	 * Parse the nth field of the reply buffer as a size_t.
	 */
	bool parseReply(char divider, uint8_t index, size_t* result) { return parse(replyBuffer, divider, index, result); }
#endif	
	/**
	 * Parse the nth field of the reply buffer as a int16_t.
	 */
	bool parseReply(char divider, uint8_t index, int16_t* result) { return parse(replyBuffer, divider, index, result); }
	/**
	 * Parse the nth field of the reply buffer as a float.
	 */
	bool parseReply(char divider, uint8_t index, float* result) { return parse(replyBuffer, divider, index, result); }
	/**
	 * Parse the PIN fields.
	 */
	void parsePIN(const char *str);

	//void check_modem_buffers();
		String check_messages();

		String parse_command_line(String line, bool set_data_pending = true);
		void read_data(uint8_t index, String command, uint16_t bytes);

		// run a command and check if it matches an OK or ERROR result String
		bool check_command(String command, String ok_result, uint32_t wait = 5000);
		bool check_command(String command, String ok_result, String error_result, uint32_t wait = 5000);
		bool check_command_no_ok(String command, String ok_result, uint32_t wait = 5000);
		bool check_command_no_ok(String command, String ok_result, String error_result, uint32_t wait = 5000);

		// send a command (or data for that matter)
		void send_command(uint8_t *command, uint16_t size);
		void send_command(String command, bool mute = false);

		String get_command(String command, uint32_t timeout = 300);
		String get_command(String command, String filter, uint32_t timeout = 300);
		String get_command_critical(String command, String filter, uint32_t timeout = 300);
		String get_command_no_ok(String command, String filter, uint32_t timeout = 300);
		String get_command_no_ok_critical(String command, String filter, uint32_t timeout = 300);
		String mqtt_message_received(String line);

		bool wait_command(String command, uint32_t timeout = 300);
		void check_commands();
		//logging BG95 lib
		void log(String text);
		String date();
		String pad2(int value);
		boolean isNumeric(String str);

		int str2hex(String str);

		

		// --- TCP ---
		void tcp_set_callback_on_close(void(*callback)(uint8_t clientID));
		bool tcp_connect(uint8_t clientID, String host, uint16_t port, uint16_t wait = 10000);
		bool tcp_connect(uint8_t contextID, uint8_t clientID, String host, uint16_t port, uint16_t wait = 10000);
		bool tcp_connect_ssl(uint8_t contextID, uint8_t sslClientID, uint8_t clientID, String host, uint16_t port, uint16_t wait = 10000);
		bool tcp_connected(uint8_t clientID);
		bool tcp_close(uint8_t clientID);
		bool tcp_send(uint8_t clientID, const char *data, uint16_t size);
		uint16_t tcp_recv(uint8_t clientID, char *data, uint16_t size);
		uint16_t tcp_has_data(uint8_t clientID);
		void tcp_check_data_pending();

				// --- TCP ---
		

	/**
	 * Perform the minmum commands needed to get the device communication up and running.
	 */
	virtual void init()=0;
public:	  
	/**
	 * Begin communicating with the device.
	 */
	void begin(Stream& port);
	void tcp_read_buffer(uint8_t index, uint16_t wait = 100);
	void check_sms();
	
	bool apn_connected(uint8_t cid = 1);	//check if modem is connected to apn

	void process_sms(uint8_t index);	// process pending SMS messages

//#pragma region Stream implementation
#ifdef _MSC_VER
#pragma region Stream
#endif

	int available() { return _port->available(); }
	size_t write(uint8_t x) { BG95ESP32_PRINT_CHAR(x); return _port->write(x); }
	int read() { return _port->read(); }
	int peek() { return _port->peek(); }
	void flush() { return _port->flush(); }

#ifdef _MSC_VER
#pragma endregion
#endif

//#pragma endregion

};

