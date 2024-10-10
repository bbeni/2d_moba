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

#define XBOX360_LEGACY_NAME_ID  "Xbox Controller"
#define XBOX360_NAME_ID         "Xbox 360 Controller"
#define PS3_NAME_ID             "Sony PLAYSTATION(R)3 Controller"
static int gamepad = 0;


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


enum State {
    WAITING_TO_CONNECT,
    CONNECTED,
    RECONNECTING,
};

static enum State state = WAITING_TO_CONNECT;

DWORD WINAPI connection_thread() {

    while (!connect_to_server()) {
        printf("Connecting to server...\n");
        Sleep(1000);
    }

    state = CONNECTED;
    printf("Connected\n");

    Message received = {0};
    received.data = malloc(MESSAGE_LEN);

    int result = 1;
    while(result > 0) {

        received.length = 0; // reset
        result = recv(server_socket, received.data, MESSAGE_LEN, 0);
        
        if ( result > 0 ) {
            //printf("Bytes received: %d\n", result);
            received.length = result;
            State_Sync state_sync = deserialize_State_Sync(&received);
            printf("Server{name: %s, id: %d, n_players: %d}\n",
                    state_sync.server_name, state_sync.server_id, state_sync.number_of_players);
        } else if ( result == 0 ) {
            printf("Connection closed\n");
        } else {
            printf("recv failed: %d\n", WSAGetLastError());

            state = RECONNECTING;
            printf("Reconnecting... \n");
            
            closesocket(server_socket);
            WSACleanup();

            while (!connect_to_server()) {
                Sleep(300);
            }

            state = CONNECTED;
            result = 1;
            printf("Connected\n");

        }
        Sleep(10);
    }

    free(received.data);
    closesocket(server_socket);
    WSACleanup();

    return 0;
}

int main() {

    CreateThread(NULL, 0, connection_thread, (LPVOID)server_socket, 0, NULL);

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello");

    int w = WINDOW_WIDTH;
    int h = WINDOW_HEIGHT;

    SetTargetFPS(60);

    while(!WindowShouldClose()) {
        
        BeginDrawing();
        ClearBackground(BG_COLOR);

        if (IsKeyPressed(KEY_LEFT) && gamepad > 0) gamepad--;
        if (IsKeyPressed(KEY_RIGHT)) gamepad++;
        
        if (IsGamepadAvailable(gamepad)) {
            DrawText(TextFormat("GP%d: %s", gamepad, GetGamepadName(gamepad)), 10, 10, 10, BLACK);
           
            DrawCircle(259, 152, 34, LIGHTGRAY);
            DrawCircle(259 + (int)(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X)*20), 152 + (int)(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y)*20), 25, RED);
            
            // Draw buttons: basic
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT)) DrawCircle(436, 150, 9, RED);
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_LEFT)) DrawCircle(352, 150, 9, RED);
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) DrawCircle(501, 151, 15, BLUE);
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) DrawCircle(536, 187, 15, LIME);
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) DrawCircle(572, 151, 15, MAROON);
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP)) DrawCircle(536, 115, 15, GOLD);


            // Draw axis: left-right triggers
            DrawRectangle(170, 30, 15, 70, GRAY);
            DrawRectangle(604, 30, 15, 70, GRAY);
            DrawRectangle(170, 30, 15, (int)(((1 + GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_TRIGGER))/2)*70), RED);
            DrawRectangle(604, 30, 15, (int)(((1 + GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_TRIGGER))/2)*70), RED);

        } else {
            DrawText(TextFormat("No gamepad found (%d)", gamepad) , 10, 10, 10, ORANGE);
        }

        switch (state) {
        case WAITING_TO_CONNECT:
            const char* connecting_string = "Connecting to server...";
            DrawText(connecting_string, w/2 - MeasureText(connecting_string, 100)/2, h/2 - 50, 100, GREEN);
            break;
        case RECONNECTING:
            const char* reconnecting_string = "Reconnecting...";
            DrawText(reconnecting_string, w/2 - MeasureText(reconnecting_string, 100)/2, h/2 - 50, 100, GREEN);
            break;
        case CONNECTED:
            DrawCircle(w/2, h/2, 200.0f, RED);
            break;
        }
                
        EndDrawing();
    }

    return 0;
}
