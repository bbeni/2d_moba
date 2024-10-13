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

enum Server_State {
    IN_LOBBY,
    IN_GAME,
};

typedef struct Connected_Player {
    uint32_t id;
    SOCKET socket;
    Player_Input input;
    bool is_up_to_date;
    // TODO: add mutex
} Connected_Player;

// should be in sync with g_world.player_...
static Connected_Player connected_players[MAX_PLAYERS] = { 0 };
static enum Server_State server_state = IN_LOBBY;

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

    // set non-blocking
    u_long option = 1;
    int error = ioctlsocket(socket, FIONBIO, &option);
    while (error != 0) {
        printf("setting non-block socket failed: ioctlsocket failed with error: %d, %d\n", error, WSAGetLastError());
        Sleep(1000);
        error = ioctlsocket(socket, FIONBIO, &option);
    }

    while(true) {

        switch (server_state) {
        case IN_LOBBY: {
            printf("Sending State_Sync to player with id %llu\n", player_id);
            Lobby_Sync s = (Lobby_Sync){
                g_world.player_count,
            };
            serialize_Lobby_Sync(&msg, &s);
            send_message(socket, &msg, LOBBY_SYNC);
        } break;
        case IN_GAME: {
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

            } else {

                // we ccheck if we have a message from the clinet

                msg.length = 0; // reset
                int recv_code = recv(socket, msg.data, MESSAGE_MAX_LEN, 0);
                if (recv_code == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
                    // we have no message so we could send something else!
                    // here we just chill for now
                } else if ( recv_code > 0 ) {
                    // we got a message yay !
                    // we have a message from the client to handle
                    msg.length = recv_code;
                    Message_Type type = extract_message_type(&msg);
                    assert(type == PLAYER_INPUT);
                    connected_players[player_id].input = deserialize_Player_Input(&msg);
                    printf("got Player_Input{target_angle: %f}\n", connected_players[player_id].input.target_angle);
                    printf("Requesting sync tick=%u\n", g_world.ticks);
                    for (int i = 0; i < g_world.player_count; i++) {
                        connected_players[i].is_up_to_date = false;
                    }
                } else {
                    printf("error: got recv_code %d\n", recv_code);
                    goto end;
                }
            }
        } break;
        default:
            assert(false && "unhandled server state");
        }

        Sleep(10);
    }

end:
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
        SOCKET accept_socket;
        wprintf(L"connection_manager: waiting for client to connect on %s:%d ...\n", SERVER, PORT);
        accept_socket = accept(connection_socket, NULL, NULL);
        if (accept_socket == INVALID_SOCKET) {
            printf("connection_manager: accept failed with error: %d\n", WSAGetLastError());
        } else {
            CreateThread(NULL, 0, player_connection_thread, (LPVOID)accept_socket, 0, NULL);
        }

        Sleep(100);
    }
}


int main(int argc, char** argv) {

    SOCKET connection_socket = setup_server(SERVER, PORT);
    CreateThread(NULL, 0, connection_manager_thread, (LPVOID)connection_socket, 0, NULL);

    if (!start_game_time()) return 1;

    int lobby_ticks = 0;

    while(true) {

        switch (server_state) {
        case IN_LOBBY:
            Sleep(200);
            lobby_ticks++;
            if (lobby_ticks >= 30) { // TODO: delete this
                server_state = IN_GAME;
            }
            break;
        case IN_GAME: {
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
        } break;
        default:
            assert(false && "unhandled server state in main");
            break;
        }
    }

    shutdown_server(connection_socket);

    return 0;

}
