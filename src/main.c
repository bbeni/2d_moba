#include "raylib.h"
#include <stdio.h>

#if defined(_WIN32)           
	#define NOGDI             // All GDI defines and routines
	#define NOUSER            // All USER defines and routines
#endif


//#include <Windows.h> // or any library that uses Windows.h ...
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#if defined(_WIN32)           // raylib uses these names as function parameters
	#undef near
	#undef far
#endif


#include "common.h"

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900

#define BG_COLOR CLITERAL(Color){0x18, 0x18, 0x18, 0xFF}


// Networking stuff
#define MESSAGE_LEN 512
SOCKET server_socket;

bool connect_to_server() {
	server_socket = INVALID_SOCKET;

	WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"WSAStartup function failed with error: %d\n", iResult);
        return false;
    }

    //----------------------
    // Create a SOCKET for connecting to server
    SOCKET ConnectSocket = INVALID_SOCKET;
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return false;
    }
	
    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    struct sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr(SERVER);
    clientService.sin_port = htons(PORT);

    //----------------------
    // Connect to server.
    iResult = connect(ConnectSocket, (SOCKADDR *) & clientService, sizeof (clientService));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"connect function failed with error: %ld\n", WSAGetLastError());
        iResult = closesocket(ConnectSocket);
        if (iResult == SOCKET_ERROR)
            wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return false;
    }

	
	/* close 
    iResult = closesocket(ConnectSocket);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return false;
    }

    WSACleanup();
	*/
    wprintf(L"Connected to server.\n");

	server_socket = ConnectSocket;
    return true;
}


int main() {

	while (!connect_to_server()) {
		printf("Connecting to server...\n");
		Sleep(1000);
	}

	
	Message received = {0};
	received.data = malloc(MESSAGE_LEN);

    // Receive until the peer closes the connection
	int iResult = 0;
	do {
		received.length = 0; // reset
        iResult = recv(server_socket, received.data, MESSAGE_LEN, 0);
        if ( iResult > 0 ) {
			printf("Bytes received: %d\n", iResult);
			received.length = iResult;
			State_Sync state_sync = deserialize_state_sync(received);
			printf("Server id: %u\n", state_sync.server_id);
			printf("Welcome string: %s\n", state_sync.welcome_string);
			
		} else if ( iResult == 0 ) {	
            printf("Connection closed\n");
        } else {
            printf("recv failed: %d\n", WSAGetLastError());
		}
		Sleep(10);
		
	} while( iResult > 0 );

	free(received.data);
	
	return 0;

	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello");

	int w = WINDOW_WIDTH;
	int h = WINDOW_HEIGHT;

	
	while(!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BG_COLOR);
		DrawCircle(w/2, h/2, 200.0f, RED);
		EndDrawing();
	}
}
