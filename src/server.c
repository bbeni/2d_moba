#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdbool.h>

#include "common.h"
#include "mathematics.h"

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
    Player_Input input;
    bool is_up_to_date;
    // TODO: add mutex
} Connected_Player;

// should be in sync with g_world.player_...
static Connected_Player connected_players[MAX_PLAYERS] = { 0 };

bool add_player_connection(SOCKET socket) {

    if (g_world.player_count >= MAX_PLAYERS) return false;

    connected_players[g_world.player_count] = (Connected_Player) {
        g_world.player_count,
        socket,
    };

    return true;
}

void handle_inputs() {
    for (int i = 0; i < g_world.player_count; i++) {
        g_world.player_target_angles[i] = connected_players[i].input.target_angle;
    }
}


DWORD WINAPI player_connection_thread(LPVOID passed_socket) {

    SOCKET socket = (SOCKET)passed_socket;

    if (!add_player_connection(socket)) {
        // TODO: reject new player with a message!
        wprintf(L"Rejected player as we are already full\n");
        closesocket(socket);
    }

    add_player();

    printf("Player connected %llu/%d\n", g_world.player_count, MAX_PLAYERS);

    Message msg = {0};
    extend_message_capacity(&msg, MESSAGE_MAX_LEN);

    size_t player_id = g_world.player_count - 1;
    
    while(true) {

        if (!connected_players[player_id].is_up_to_date) {
            connected_players[player_id].is_up_to_date = true;
            
            printf("Sending State_Sync to player with id %llu\n", player_id);
            State_Sync s = (State_Sync){
                161,
                player_id,
                g_world.player_count,
                g_world.ticks,
                (float)g_world.time.accumulated_time,
            };
            
            memcpy(s.xs, g_world.player_xs, 4*MAX_PLAYERS);
            memcpy(s.ys, g_world.player_ys, 4*MAX_PLAYERS);
            memcpy(s.angles, g_world.player_angles, 4*MAX_PLAYERS);
            memcpy(s.target_angles, g_world.player_target_angles, 4*MAX_PLAYERS);

            serialize_State_Sync(&msg, &s);
            send_message(socket, &msg, STATE_SYNC);
            
        } else if(false) {
            
            u_long bytes_available = 0;
            ioctlsocket(socket, FIONREAD, &bytes_available);
            
            if (bytes_available > 0) {
                int recv_code = recv(socket, msg.data, MESSAGE_MAX_LEN, 0);
                if ( recv_code == 0 ) {
                    printf("Player connection closed\n");
                    break;
                } else if ( recv_code < 0) {
                    printf("recv failed: %d\n", WSAGetLastError());
                    closesocket(socket);
                    WSACleanup();
                    break;
                }

                // we have a message from the client to handle
                msg.length = recv_code;
                Message_Type type = extract_message_type(&msg);
                assert(type == PLAYER_INPUT);

                connected_players[player_id].input = deserialize_Player_Input(&msg);
                
            }
        }
        
        Sleep(5);
    }

    closesocket(socket);
    WSACleanup();
    return 0;
}

void shutdown_server(SOCKET socket) {
    closesocket(socket);
    WSACleanup();

}

DWORD WINAPI connection_manager_thread(LPVOID passed_socket) {
    SOCKET connection_socket = (SOCKET)passed_socket;

    while (true) {
        SOCKET AcceptSocket;
        wprintf(L"connection_manager: waiting for client to connect on %s:%d ...\n", SERVER, PORT);
        AcceptSocket = accept(connection_socket, NULL, NULL);
        if (AcceptSocket == INVALID_SOCKET) {
            wprintf(L"connection_manager: accept failed with error: %ld\n", WSAGetLastError());
        } else {
            CreateThread(NULL, 0, player_connection_thread, (LPVOID)AcceptSocket, 0, NULL);
        }
        
        Sleep(1);
    }
}


int main(int argc, char** argv) {

    SOCKET connection_socket = setup_server(SERVER, PORT);
    CreateThread(NULL, 0, connection_manager_thread, (LPVOID)connection_socket, 0, NULL);

    if (!start_game_time()) return 1;
    
    while(true) {

        while(g_world.time.accumulated_time > g_world.ticks * TICK_TIME) {
            handle_inputs();
            tick();
        
            if (g_world.ticks % 50 == 0) {
                printf("Requesting sync tick=%u\n", g_world.ticks);
                for (int i = 0; i < g_world.player_count; i++) {
                    connected_players[i].is_up_to_date = false;
                }
            }
        }
        
        wait_game_time();
    }

    shutdown_server(connection_socket);

    return 0;

}
