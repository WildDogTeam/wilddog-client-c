#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include "pdu.h"
#include "option.h"
#define SERVER_IP "192.168.1.188"
#define SERVER_PORT 5683
coap_pdu_t* requestPdu() {
	coap_pdu_t* pdu = coap_new_pdu();
	pdu->hdr->id = htons(123);
	pdu->hdr->version = 1;
	pdu->hdr->type = 0;
	pdu->hdr->code = 3; //get

	char path[] = "hello";
	char host[] = SERVER_IP;
	char data[] = "this is data";
	char token[2];
	token[0] = 127;
	token[1] = 0;
	unsigned short port = htons(SERVER_PORT);
	coap_add_token(pdu, 2, token);
	coap_add_option(pdu, 3, strlen(host), host);
	coap_add_option(pdu, 7, sizeof(port), (char*) &port);
	coap_add_option(pdu, 11, strlen(path), path);

	coap_add_data(pdu, strlen(data), data);
	return pdu;
}
coap_pdu_t * new_ack(coap_pdu_t* request) {
	coap_pdu_t *pdu = coap_new_pdu();

	if (pdu) {
		pdu->hdr->type = COAP_MESSAGE_ACK;
		pdu->hdr->code = 0;
		pdu->hdr->id = request->hdr->id;
	}

	return pdu;
}
int main(void) {
	int ret = EXIT_SUCCESS;
	int fd = -1;
	int n = 0;
	struct timeval tv_out;
	struct sockaddr_in servAddr;

	// create socket
	if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		return EXIT_FAILURE;
	}

	// set socket
	tv_out.tv_sec = 20;
	tv_out.tv_usec = 0;

	if ((ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv_out, sizeof(tv_out)))
			< 0) {
		goto finally;
	}

	if ((ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out)))
			< 0) {
		goto finally;
	}

	// connect
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	//servAddr.sin_addr.s_addr = inet_addr("180.150.179.62");
	servAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	servAddr.sin_port = htons(SERVER_PORT);
	if ((ret = connect(fd, (struct sockaddr*) &servAddr, sizeof(servAddr)))
			< 0) {
		goto finally;
	}
	coap_pdu_t* request;
	request = requestPdu();
	printf("length:%d", request->length);

	sendto(fd, request->hdr, request->length, 0, (struct sockaddr*) &servAddr,
			sizeof(servAddr));
	coap_delete_pdu(request);
	printf("send end \n");

	finally: close(fd);
	return ret;
}
