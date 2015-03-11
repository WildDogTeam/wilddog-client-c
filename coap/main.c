/*
 * main.c
 *
 *  Created on: 2014年12月10日
 *      Author: x
 */
#include "pdu.h"


int main(int argc, char **argv) {
	coap_pdu_t* pdu = coap_new_pdu();
	pdu->hdr->id=1;
	pdu->hdr->version=0;
	pdu->hdr->type=1;
	char path[]="xx/xx";
	char host[]="abc.com";
	char port[]="234";
	char data[]="this is data";
	coap_add_option(pdu,3,sizeof(host),host);
	coap_add_option(pdu,7,sizeof(port),port);
	coap_add_option(pdu,11,sizeof(path),path);
	coap_add_data(pdu,sizeof(data),data);



}

