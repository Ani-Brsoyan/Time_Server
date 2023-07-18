//The programm is organized so that both Windows and Unix based platforms can run it. Here we are using TCP.

//When you run the programm it will wait for the connection. You should open a web browser and navigate to http://127.0.0.1:8080.


//the parts with WIN32 are for Windows
#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

//Linux and macOS
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#endif

//defining some macros to abstract out some differences between Berkeley socket and Winsock APIs
#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>

int main() {

//if we are compiling on Windows first we should initialize Winsock
#if defined(_WIN32)
	WSADATA d;
	if(WSAStartup(MAKEWORD(2, 2), &d)) {
		fprint(stderr, "Failed to initialize.\n");
		return 1;
	}
#endif

//figuring out the local address our server should bind to
	printf("Configuring local address...\n");
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; //for IPv4
	hints.ai_socktype = SOCK_STREAM; //for TCP
	hints.ai_flags = AI_PASSIVE; //to enable server to listen()
	
	struct addrinfo* bind_address;
	getaddrinfo(0, "8080", &hints, &bind_address); //it generates an address that is suitable for bind()

//creating socket
	printf("Creating socket...\n");
	SOCKET socket_listen;
	socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);

//checking if the socket is successfully called
	if(!ISVALIDSOCKET(socket_listen)) {
		fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
	}

//Calling bind() to associate it with our address from getaddrinfo()
	printf("Binding socket to local address...\n");
	//bind() returns 0 on sucess
	if(bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
		fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
		return 1;
	}
	freeaddrinfo(bind_address);

//Start to listening
	printf("Listening...\n");
	//10 shows how many connections listen() is allowed to queue up
	if(listen(socket_listen, 10) < 0) {
		fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
		return 1;
	}

//Accepting any incoming connections
	printf("Waiting for connection...\n");
	struct sockaddr_storage client_address;
	socklen_t client_len = sizeof(client_address);
	//this new socket is made to send and receive data
	SOCKET socket_client = accept(socket_listen, (struct sockaddr*) &client_address, &client_len);
	if(!ISVALIDSOCKET(socket_client)) {
		fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
		return 1;
	}

//We've estableshed TCP connection to a remote client
//Printing clients address [optional]
	printf("Client is connected... ");
	char address_buffer[100];
	getnameinfo((struct sockaddr*) &client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
	printf("%s\n", address_buffer);

//Reading request via recv()
	printf("Reading request...\n");
	char request[1024];
	int bytes_received = recv(socket_client, request, 1024, 0);
	printf("Received %d bytes. \n", bytes_received);

//As our programm only tells the time, assume that web browser has sent its request and we send our response back
	printf("Sending response...\n");
	const char* response = 
		"HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"Local time is: ";
	int bytes_sent = send(socket_client, response, strlen(response), 0);        printf("Sent %d of %d bytes. \n", bytes_sent, (int)strlen(response));

//Now sending local time
	time_t timer;
	time(&timer);
	char* time_msg = ctime(&timer);
	bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);
	printf("Sent %d of %d bytes. \n", bytes_sent, (int)strlen(time_msg));

//Closing the client connection
	printf("Closing connection...\n");
	CLOSESOCKET(socket_client);

	printf("Closing listening socket...\n");
	CLOSESOCKET(socket_listen);

#if defined(_WIN32)
	WSACleanup();
#endif

	printf("Finished.\n");
	
	return 0;

}
