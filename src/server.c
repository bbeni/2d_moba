#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdbool.h>

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

#define SERVER "127.0.0.1"
#define PORT 27015

int main(int argc, char** argv) {

	SOCKET ListenSocket = setup_server(SERVER, PORT);

	for (int i=0; i<3; i++) {	
		SOCKET AcceptSocket;
		wprintf(L"Waiting for client to connect on %s:%d ...\n", SERVER, PORT);
		AcceptSocket = accept(ListenSocket, NULL, NULL);
		if (AcceptSocket == INVALID_SOCKET) {
			wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		} else {
			wprintf(L"Client connected.\n");
		}
	}

	shutdown_server(ListenSocket);

    return 0;

}
