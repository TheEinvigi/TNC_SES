#include "http_client/http_client.h"
#include "http_client/http_client_rx_callback.h"

#include "gsm/sim800c_tcpip.h"

#include <string.h>

typedef enum http_client_state {
	HTTP_CLIENT_UNITIALIZED,
	HTTP_CLIENT_READY,
	HTTP_CLIENT_CONNECTED_IDLE,
	HTTP_CLIENT_WAITING_POST,
	HTTP_CLIENT_WAITING_GET
} http_client_state_t;

http_client_state_t http_client_state = HTTP_CLIENT_UNITIALIZED;

/**
 * Content lenght received from HTTP response headers or chunked encoding
 */
uint16_t http_client_content_lenght = 0;

/**
 *	HTTP code returned by the latest query. It is zeroed after each successful call to async
 *	function. This indicate that a request is currently in progress. Negative values means some
 *	non HTTP error, like communication timeout or response longer than expected
 */
int16_t http_client_http_code = 0;

/**
 * Temporary buffer for processing
 */
char http_client_header_buffer[HEADER_BUFFER_LN];

/**
 * Index used to walk through 'http_client_header_buffer'
 */
uint8_t http_client_header_index = 0;

/**
 * SIM800 state and serial context used to communication with gsm module.
 */
gsm_sim800_state_t * http_client_deticated_sim800_state;
srl_context_t * http_client_deticated_serial_context;

/**
 * If this is set to non zero, the library will stop response processing after HTTP code different from <200, 299>
 */
uint8_t http_client_ignore_content_on_http_error;

/**
 * Default port for http
 */
const char * http_client_default_port = "80";

/**
 * Local, static buffers to fetch domain name or IP address from URI
 */
#define HOSTNAME_LN 	32
#define PORT_LN			5
static char http_client_hostname[HOSTNAME_LN];
static char http_client_port[PORT_LN];

/**
 * This functions splits the URL string into hostname and path
 *
 * It return a split point index which in case of
 * http://pogoda.cc:8080/meteo_backend/station/z_gss_zar/summary
 * will return an index of '/' after 8080
 *
 *  */
static uint16_t http_client_split_hostname_and_path(char * input, uint16_t input_ln) {

	uint16_t out = 0xFFFF;

	uint16_t iterator = 7;

	// check if URL starts correctly
	if (*input == 'h' && *(input + 1) == 't'  && *(input + 2) == 't'  && *(input + 3) == 'p'  && *(input + 4) == ':'  && *(input + 5) == '/'  && *(input + 6) == '/') {
		for (; iterator < input_ln; iterator++) {
			if (*(input + iterator) == '/') {

				out = iterator;

				break;
			}
		}
	}

	return out;
}

static void http_client_get_port_from_url(char * input, uint16_t input_ln, char * port, uint16_t port_ln) {

	char temp[5] = {0, 0, 0, 0, 0};

	// get split point
	uint16_t split_point = http_client_split_hostname_and_path(input, input_ln);

	if (split_point != 0xFFFF) {
		// check last character before split point
		char last_character = *(input + split_point - 1);

		// clear target buffer
		memset (port, 0x00, port_ln);

		// if any port has been provided by a user
		if (last_character >= '0' && last_character <= '9' ) {

			// copy maximum of 5 characters until ':'
			for (short i = 1; i < 6; i++) {

				// get current character
				last_character = *(input + split_point - i);

				// break on any non digit character
				if (last_character < '0' || last_character > '9') {
					break;
				}

				// copy to temporary buffer
				temp[i - 1] = last_character;
			}

			// copy port number into target buffer
			memcpy(port, temp, 5);
		}
		else {
			// copy default port
			memcpy(port, http_client_default_port, 2);
		}

	}


}

static void http_client_get_address_from_url(char * input, uint16_t input_ln, char * address, uint16_t address_ln) {

	int first_index = 0, last_index = 0;

	// get split point
	uint16_t split_point = http_client_split_hostname_and_path(input, input_ln);

	// if split point is valid it also means that URL starts from 'http://'
	if (split_point != 0xFFFF) {

		// first position of an URL
		first_index = 7;

		// rewind to either ':' or split point
		for (last_index = first_index; last_index <= split_point; last_index ++ ) {

			// fetch current character
			char current_ = *(input + last_index);

			// check if
			if (current_ == ':' || current_ == '/') {

				// rewind one character
				current_--;
				break;
			}
		}

		// check if output fits data
		if (last_index - first_index < address_ln) {
			// if yes copy it
			memcpy(address, input + first_index, last_index - first_index);
		}
	}

}

void http_client_init(gsm_sim800_state_t * state, srl_context_t * serial_context, uint8_t ignore_content_on_http_error) {

	http_client_deticated_sim800_state = state;

	http_client_state = HTTP_CLIENT_READY;
}


uint8_t http_client_async_get(char * url, uint8_t url_ln, uint16_t response_ln_limit, uint8_t force_disconnect_on_busy) {

	uint16_t split_point = http_client_split_hostname_and_path(url, url_ln);

	uint8_t out = 0;

	uint8_t connect_result = -1;

	// simple check if url seems to be corrected or not
	if (split_point != 0xFFFF && http_client_state == HTTP_CLIENT_READY ) {

		// clear local buffers
		memset(http_client_hostname, 0x00, HOSTNAME_LN);
		memset(http_client_port, 0x00, PORT_LN);

		// check if module is busy on other TCP/IP connection
		if (*http_client_deticated_sim800_state == SIM800_TCP_CONNECTED && force_disconnect_on_busy != 0) {
			// if client is connected end a user wants to force disconnect
			gsm_sim800_tcpip_close(http_client_deticated_serial_context, http_client_deticated_sim800_state);
		}
		else if (*http_client_deticated_sim800_state == SIM800_TCP_CONNECTED && force_disconnect_on_busy == 0) {
			out = HTTP_CLIENT_RET_TCPIP_BSY;
		}

		// get hotsname from URL (URI)
		http_client_get_address_from_url(url, url_ln, http_client_hostname, HOSTNAME_LN);
		http_client_get_port_from_url(url, url_ln, http_client_port, PORT_LN);

		// establish TCP connection to HTTP server
		connect_result = gsm_sim800_tcpip_connect(http_client_hostname, HOSTNAME_LN, http_client_port, PORT_LN, http_client_deticated_serial_context, http_client_deticated_sim800_state);

		// if connection has been established
		if (connect_result == 0) {
			// set appropriate state
			http_client_state = HTTP_CLIENT_CONNECTED_IDLE;
		}
	}
	else if (split_point == 0xFFFF) {
		out = HTTP_CLIENT_RET_WRONG_URL;
	}
	else if (http_client_state != HTTP_CLIENT_READY) {
		out = HTTP_CLIENT_RET_UNITIALIZED;
	}

	return out;
}

uint8_t http_client_async_post(char * url, uint8_t url_ln, char * data_to_post, uint8_t data_ln, uint8_t force_disconnect_on_busy) {
	return 0;
}


char * http_client_get_server_response() {
	return 0;
}

uint16_t http_client_get_latest_http_code() {
	return http_client_http_code;
}