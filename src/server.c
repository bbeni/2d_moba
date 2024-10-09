#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdbool.h>

#include "common.h"

SOCKET setup_server(const char* ip_address, int port) {
	WSADATA wsaData = {0};
	int iResult = 0;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        wprintf(L"WSAStartup failed: %d\n", iResult);
        return INVALID_SOCKET;
    }

	// The socket for incomming connections
    SOCKET ListenSocket = INVALID_SOCKET;
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return INVALID_SOCKET;
    }
	
    struct sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_port = htons(port);
    inet_pton(AF_INET, ip_address, &service.sin_addr);

    if (bind(ListenSocket,
             (SOCKADDR *) & service, sizeof (service)) == SOCKET_ERROR) {
        wprintf(L"bind failed with error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    if (listen(ListenSocket, 1) == SOCKET_ERROR) {
        wprintf(L"listen failed with error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

	return ListenSocket;
}

void shutdown_server(SOCKET listenSocket) {
	closesocket(listenSocket);
    WSACleanup();
}


int main(int argc, char** argv) {

	SOCKET ListenSocket = setup_server(SERVER, PORT);

	for (int i=0; i<3; i++) {	
		SOCKET AcceptSocket;
		wprintf(L"Waiting for client to connect on %s:%d ...\n", SERVER, PORT);
		AcceptSocket = accept(ListenSocket, NULL, NULL);
		if (AcceptSocket == INVALID_SOCKET) {
			wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
		} else {
			wprintf(L"Client connected.\n");
		}

		State_Sync s = {
			STATE_SYNC,
			123,
			"Hello this message is from the server! Welome home!",
		};
		
		Message msg = serialize_state_sync(&s);
		send(AcceptSocket, msg.data, msg.length, 0);
	}

	shutdown_server(ListenSocket);

    return 0;

}
