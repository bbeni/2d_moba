// windows specific implementations
#ifdef _WIN32
#include "platform.h"

#define NOGDI             // All GDI defines and routines
#define NOUSER            // All USER defines and routines
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#undef near
#undef far

#include <assert.h>
#include <stdio.h>

bool open_connection(Socket* sock, const char* address, uint32_t port) {
    *sock = INVALID_SOCKET;

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        printf("newtorking.c: WSAStartup function failed with error: %d\n", iResult);
        return false;
    }

    Socket ConnectSocket = INVALID_SOCKET;
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("networking.c: socket function failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return false;
    }

    struct sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr(address);
    clientService.sin_port = htons(port);

    iResult = connect(ConnectSocket, (SOCKADDR *) & clientService, sizeof (clientService));
    if (iResult == SOCKET_ERROR) {
        printf("networking.c: connect function failed with error: %d\n", WSAGetLastError());
        iResult = closesocket(ConnectSocket);
        if (iResult == SOCKET_ERROR)
        printf("networking.c: closesocket function failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return false;
    }
    
    *sock = ConnectSocket;
    return true;
}

bool close_connection(Socket sock) {
    int status = shutdown(sock, SD_BOTH);
    if (status != 0) {
        printf("close_connection(): shutdown() failed on windows with code %d\n", status);
        WSACleanup();
        return false;
    }
    status = closesocket(sock);
    if (status != 0) {
        printf("close_connection(): closesocket() failed on windows with code %d, WSAGetLastError() returned %d\n", status, WSAGetLastError());
        WSACleanup();
        return false;
    }

    WSACleanup();
    return true;
}

bool set_non_block(Socket sock) {
    u_long option = 1;
    int error = ioctlsocket(sock, FIONBIO, &option);
    if (error != 0) {
        printf("set_non_block(): failed: ioctlsocket failed with error: %d, %d\n", error, WSAGetLastError());
        return false;
    }
    return true;
}


void sleep_ms(int sleepMs) {
    Sleep(sleepMs);
}

void create_thread(Thread (*thread_func)(), Socket sock) {
    CreateThread(NULL, 0, thread_func, (LPVOID)sock, 0, NULL);
}


#endif // _WIN32
