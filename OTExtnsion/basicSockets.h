/*
 * basicSockets.h
 *
 *  Created on: Aug 3, 2015
 *      Author: froike (Roi Inbar) 
 * 	Modified: Aner Ben-Efraim
 * 
 */
#include <stdio.h>
#include <stropts.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include "secCompMultiParty.h"
using namespace std;

#ifndef BMRNET_H_
#define BMRNET_H_

#ifdef _WIN32
 #include<winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <stdbool.h>
#endif
/*GLOBAL VARIABLES - LIST OF IP ADDRESSES*/
extern char** localIPaddrs;
extern int numberOfAddresses;
//gets the list of IP addresses
char** getIPAddresses();
int getPartyNum(char* filename);



class BmrNet {
private:
	char * host;
	unsigned int port;
	bool is_JustServer;
	int socketFd;
	#ifdef _WIN32
	    PCSTR Cport;
		WSADATA wsa;
		DWORD dwRetval;
	#endif


public:
	/**
	 * Constructor for servers and clients, got the host and the port for connect or listen.
	 * After creation call listenNow() or connectNow() function.
	 */
	BmrNet(char * host, int port);

	/**
	 * Constructor for servers only. got the port it will listen to.
	 * After creation call listenNow() function.
	 */
	BmrNet(int portno);

	/**
	 * got data and send it to the other side, wait for response and return it.
	 * return pointer for the data that recived.
	 */
	void* sendAndRecive(const void* data, int get_size, int send_size);

	
	virtual ~BmrNet();

	/**
	 * Start listen on the given port.
	 */
	bool listenNow();

	/**
	 * Try to connect to server by given host and port.
	 * return true for success or false for failure.
	 */
	bool connectNow();

	/**
	 * Send Data to the other side.
	 * return true for success or false for failure.
	 */
	bool sendMsg(const void* data, int size);

	/**
	 * Recive data from other side.
	 * return true for success or false for failure.
	 */
	bool reciveMsg(void* buff, int buffSize);


};



#endif /* BMRNET_H_ */
