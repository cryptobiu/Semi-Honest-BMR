/*
 * basicSockets.cpp
 *
 *  Created on: Aug 3, 2015
 *      Author: froike(Roi Inbar) 
 * 	Modified: Aner Ben-Efraim
 * 
 */
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "basicSockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <iostream>
#include <netinet/tcp.h>

#define bufferSize 256

#ifdef __linux__
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#define Sockwrite(sock, data, size) write(sock, data, size) 
	#define Sockread(sock, buff, bufferSize) read(sock, buff, bufferSize)
	//#define socklen_t int
#elif _WIN32
	//#pragma comment (lib, "ws2_32.lib") //Winsock Library
	#pragma comment (lib, "Ws2_32.lib")
	//#pragma comment (lib, "Mswsock.lib")
	//#pragma comment (lib, "AdvApi32.lib")
	#include<winsock.h>
	//#include <ws2tcpip.h>
	#define socklen_t int
	#define close closesocket
	#define Sockwrite(sock, data, size) send(sock, (char*)data, size, 0)
	#define Sockread(sock, buff, bufferSize) recv(sock, (char*)buff, bufferSize, 0)
	
#endif

/*GLOBAL VARIABLES - LIST OF IP ADDRESSES*/
char** localIPaddrs;
int numberOfAddresses;


using namespace std;

char** getIPAddresses(const int domain)
{
  char** ans;
  int s;
  struct ifconf ifconf;
  struct ifreq ifr[50];
  int ifs;
  int i;

  s = socket(domain, SOCK_STREAM, 0);
  if (s < 0) {
    perror("socket");
    return 0;
  }

  ifconf.ifc_buf = (char *) ifr;
  ifconf.ifc_len = sizeof ifr;

  if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
    perror("ioctl");
    return 0;
  }

  ifs = ifconf.ifc_len / sizeof(ifr[0]);
  numberOfAddresses=ifs;
  ans= new char*[ifs];
  for (i = 0; i < ifs; i++) {
    char* ip=new char[INET_ADDRSTRLEN];
    struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;

    if (!inet_ntop(domain, &s_in->sin_addr, ip, INET_ADDRSTRLEN)) {
      perror("inet_ntop");
      return 0;
    }

    ans[i]=ip;
  }

  close(s);

  return ans;
}

int getPartyNum(char* filename)
{

	FILE * f = fopen(filename, "r");

	char buff[STRING_BUFFER_SIZE];
	char ip[STRING_BUFFER_SIZE];

	localIPaddrs=getIPAddresses(AF_INET);
	string tmp;
	int player = 0;
	while (true)
	{
		fgets(buff, STRING_BUFFER_SIZE, f);
		sscanf(buff, "%s\n", ip);
		for (int i = 0; i < numberOfAddresses; i++)
			if (strcmp(localIPaddrs[i], ip) == 0 || strcmp("127.0.0.1", ip)==0)
				return player;
		player++;
	}
	fclose(f);

}

BmrNet::BmrNet(char* host, int portno) {
	this->port = portno;
#ifdef _WIN32
	this->Cport = (PCSTR)portno;
#endif
	this->host = host;
	this->is_JustServer = false;
	this->socketFd = NULL;
#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d\n", WSAGetLastError());
	}
	else printf("WSP Initialised.\n");
#endif

}

BmrNet::BmrNet(int portno) {
	this->port = portno;
	this->host = "";
	this->is_JustServer = true;
	this->socketFd = NULL;
#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d\n", WSAGetLastError());
	}
	else printf("WSP Initialised.\n");
#endif
}

BmrNet::~BmrNet() {
	close(socketFd);
	#ifdef _WIN32
		WSACleanup();
	#endif
	printf("Closeing connection\n");
}

bool BmrNet::listenNow(){
	int serverSockFd;
	socklen_t clilen;

	struct sockaddr_in serv_addr, cli_addr;


	serverSockFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSockFd < 0){
		cout<<"ERROR opening socket"<<endl;
		return false;
	}
	memset(&serv_addr, 0,sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(this->port);
	
	int yes=1;
	
	if (setsockopt(serverSockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) 
	{
	perror("setsockopt");
	exit(1);
	}
	
	int testCounter=0;//
	while (bind(serverSockFd, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0 && testCounter<10){
		cout<<"ERROR on binding: "<< port <<endl;
		cout<<"Count: "<< testCounter<<endl;///
		perror("Binding");
		testCounter++;///
		sleep(2);
	}
	if (testCounter==10) return false;//
	///
	listen(serverSockFd,5);
	clilen = sizeof(cli_addr);
	printf("Start listening!");
	this->socketFd = accept(serverSockFd,
			(struct sockaddr *) &cli_addr,
			 &clilen);
	if (this->socketFd < 0){
		cout<<"ERROR on accept"<<endl;
		return false;
	}
	//use TCP_NODELAY on the socket 
	int flag = 1;
	int result = setsockopt(this->socketFd,            /* socket affected */
                          IPPROTO_TCP,     /* set option at TCP level */
                          TCP_NODELAY,     /* name of option */
                          (char *) &flag,  /* the cast is historical */
                          sizeof(int));    /* length of option value */
	if (result < 0) {
	    cout << "error setting NODELAY. exiting" << endl;
	    exit (-1);
	}

	printf("Connected!");

	close(serverSockFd);
	return true;
}


bool BmrNet::connectNow(){
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int n;

	if (is_JustServer){
		cout<<"ERROR: Never got a host... please use the second constructor"<<endl;;
		return false;
	}

	//fprintf(stderr,"usage %s hostname port\n", host);
	socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (socketFd < 0){
		cout<<("ERROR opening socket")<<endl;
		return false;
	}

	//use TCP_NODELAY on the socket 
	int flag = 1;
	int result = setsockopt(socketFd,            /* socket affected */
                          IPPROTO_TCP,     /* set option at TCP level */
                          TCP_NODELAY,     /* name of option */
                          (char *) &flag,  /* the cast is historical */
                          sizeof(int));    /* length of option value */
	if (result < 0) {
	    cout << "error setting NODELAY. exiting" << endl;
	    exit (-1);
       }
	

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= inet_addr(host);
	serv_addr.sin_port			= htons(port); 
	
	int count = 0;
	cout << "Trying to connect to server"<< endl; cout <<"IP:"<<host<<"port"<<port<< endl;
	while (connect(socketFd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
			count++;
			if (count%50==0)
			    cout << "Not managing to connect. " << "Count=" << count << endl;
			sleep(1);
	}

	printf("Connected! %d",count);

	return true;

}



bool BmrNet::sendMsg(const void* data, int size){
	int left = size;
	int n;
	while (left > 0)
	{
		n = Sockwrite(this->socketFd, &((char*)data)[size - left], left);
		if (n < 0) {
			cout << "ERROR writing to socket" << endl;
			return false;
		}
		left -= n;
	}
	return true;
}

bool BmrNet::reciveMsg(void* buff, int size){
	int left = size;
	int n;
	memset(buff, 0,(unsigned long)size);
	while (left > 0)
	{
		n = Sockread(this->socketFd, &((char*)buff)[size-left], left);
		if (n < 0) {
			cout << "ERROR reading from socket" << endl;
			cout << "Size = " << size << ", Left = " << left << ", n = " << n << endl;
			return false;
		}
		left -= n;
	}
	return true;
}




