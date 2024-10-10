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

typedef struct Connected_Player {
    uint32_t id;
    SOCKET socket;
    int x;
    int y;
} Connected_Player;

#define MAX_PLAYERS 64

static Connected_Player connected_players[MAX_PLAYERS];
static int player_count = 0;

bool add_player(SOCKET socket) {

    if (player_count >= MAX_PLAYERS) {
        return false;
    }

    connected_players[player_count++] = (Connected_Player) {
        player_count,
        socket,
        0,
        0,
    };

    return true;
}


DWORD WINAPI player_connection_thread(LPVOID passed_socket) {

    SOCKET socket = (SOCKET)passed_socket;

    if (!add_player(socket)) {
        // TODO: reject new player with a message!
        wprintf(L"Rejected player as we are already full\n");
        closesocket(socket);
    }

    printf("Player connected %d/%d\n", player_count, MAX_PLAYERS);

    while(1) {
        State_Sync s = {
            STATE_SYNC,
            161,
            "Hello this message is a routine message from the server! <3",
        };

        Message msg = serialize_state_sync(&s);
        send(socket, msg.data, msg.length, 0);
        Sleep(100);
    }

    closesocket(socket);
    WSACleanup();
}

void shutdown_server(SOCKET socket) {
    closesocket(socket);
    WSACleanup();

}

int main(int argc, char** argv) {

    SOCKET ListenSocket = setup_server(SERVER, PORT);

    while (true) {
        SOCKET AcceptSocket;
        wprintf(L"Waiting for client to connect on %s:%d ...\n", SERVER, PORT);
        AcceptSocket = accept(ListenSocket, NULL, NULL);
        if (AcceptSocket == INVALID_SOCKET) {
            wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
        } else {
            CreateThread(NULL, 0, player_connection_thread, (LPVOID)AcceptSocket, 0, NULL);
        }

    }

    shutdown_server(ListenSocket);

    return 0;

}
