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
#include "mathematics.h"

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900

#define BG_COLOR CLITERAL(Color){0x18, 0x18, 0x18, 0xFF}

#define XBOX360_LEGACY_NAME_ID  "Xbox Controller"
#define XBOX360_NAME_ID         "Xbox 360 Controller"
#define PS3_NAME_ID             "Sony PLAYSTATION(R)3 Controller"
static int gamepad = 0;

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
    CONNECTED_LOBBY,
    CONNECTED_GAME,
    RECONNECTING,
};

static enum State state = WAITING_TO_CONNECT;
static size_t my_id;

// input queue for the network
struct {
    #define QUEUE_SIZE 24
    Player_Input data[QUEUE_SIZE]; // FIFO queue
    size_t tail; // append to tail
    size_t head; // eat from head
} input_queue = {0};

void maybe_add_to_input_queue(Player_Input input) {
    Player_Input current_input = g_world.player_inputs[my_id];
    if ((current_input.flags == input.flags) && (current_input.target_angle == input.target_angle)) return;
    input_queue.data[input_queue.tail] = input;
    input_queue.tail = (input_queue.tail + 1) % QUEUE_SIZE;
    g_world.player_inputs[my_id] = input;
};

bool consume_from_input_queue(Player_Input* out) {
    if (input_queue.head == input_queue.tail) {
        return false; // we are empty
    }

    *out = input_queue.data[input_queue.head];
    input_queue.head = (input_queue.head + 1) % QUEUE_SIZE;
    return true;
}

Lobby_Sync lobby_state = {0};

DWORD WINAPI connection_thread() {

    while (!connect_to_server()) {
        printf("connection_thread: try connecting to server...\n");
        Sleep(1000);
    }

    state = CONNECTED_LOBBY;
    printf("connection_thread: connected, starting\n");

    Message received = {0};
    extend_message_capacity(&received, MESSAGE_MAX_LEN);

    Message to_send = {0};
    extend_message_capacity(&to_send, MESSAGE_MAX_LEN);

    Player_Input current_input = {0};

    // set non-blocking
    u_long option = 1;
    int error = ioctlsocket(server_socket, FIONBIO, &option);
    while (error != 0) {
        printf("setting non-block socket failed: ioctlsocket failed with error: %d, %d\n", error, WSAGetLastError());
        Sleep(1000);
        error = ioctlsocket(server_socket, FIONBIO, &option);
    }

    while(true) {

        received.length = 0; // reset
        to_send.length = 0;  // reset

        int recv_code = recv(server_socket, received.data, MESSAGE_MAX_LEN, 0);

        if (recv_code == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
            // we have no message so we could send something else!
            switch(state) {
            case CONNECTED_GAME: {
                if (consume_from_input_queue(&current_input)) {
                    serialize_Player_Input(&to_send, &current_input);
                    send_message(server_socket, &to_send, PLAYER_INPUT);
                }
            } break;
            case CONNECTED_LOBBY: {
                Player_Input last_input = current_input;
                if (consume_from_input_queue(&current_input)) {

                    bool did_click = (!(last_input.flags & PRIMARY_DOWN) && (current_input.flags & PRIMARY_DOWN));
                    if (did_click) {
                        send_message(server_socket, &to_send, LOBBY_TOGGLE_ACCEPT);
                    }
                }
            } break;
            default:
                assert(false && "unhandled state in connection_thread()");
                break;
            }

        } else if ( recv_code > 0 ) {
            // we got a message yay !
            received.length = recv_code;
            Message_Type type = extract_message_type(&received);

            switch(type) {
            case STATE_SYNC: {
                state = CONNECTED_GAME;
                State_Sync state_sync = deserialize_State_Sync(&received);
                /*printf("Server_Sync{n_players: %d, my_id: %d, my position: (%f, %f), time: %f, ticks: %d}\n",
                    state_sync.number_of_players,
                    state_sync.player_id,
                    state_sync.xs[state_sync.player_id],
                    state_sync.ys[state_sync.player_id],
                    state_sync.accumulated_time,
                    state_sync.ticks
                    );*/
                my_id = state_sync.player_id;
                g_world.time.accumulated_time = (double)state_sync.accumulated_time;
                g_world.player_count = state_sync.number_of_players;
                g_world.ticks = state_sync.ticks;
                memcpy(g_world.player_xs, state_sync.xs, 4*MAX_PLAYERS);
                memcpy(g_world.player_ys, state_sync.ys, 4*MAX_PLAYERS);
                memcpy(g_world.player_angles, state_sync.angles, 4*MAX_PLAYERS);
                memcpy(g_world.player_target_angles, state_sync.target_angles, 4*MAX_PLAYERS);

                // sync shots
                g_world.shots.count = state_sync.number_of_shots;
                printf("shots: %lld\n", g_world.shots.count);
                memcpy(g_world.shots.shooter_ids, state_sync.shots_player_ids, 4*MAX_SHOTS);
                for (int i=0; i<MAX_SHOTS; i++) {
                    g_world.shots.positions[i].x = state_sync.shots_xs[i];
                    g_world.shots.positions[i].y = state_sync.shots_ys[i];
                    g_world.shots.directions[i].x = state_sync.shots_direction_xs[i];
                    g_world.shots.directions[i].y = state_sync.shots_direction_ys[i];
                }

            } break;
            case LOBBY_SYNC: {
                lobby_state = deserialize_Lobby_Sync(&received);
                state = CONNECTED_LOBBY;
                //printf("Lobby_Sync{n_players: %d, rdy: %d}\n", lobby_state.number_of_players, lobby_state.rdy[my_id]);
            } break;
            default:
                printf("type of message: %d\n", type);
                assert(false && "connection_thread: got unknown type of message");
            }


        } else if ( recv_code == 0 ) {
            // the server wants to go
            printf("connection closed\n");
            break;
        } else {
            // it is and error!
            printf("recv failed: %d\n", WSAGetLastError());

            state = RECONNECTING;
            printf("Reconnecting...\n");
            closesocket(server_socket);
            WSACleanup();

            while (!connect_to_server()) {
                Sleep(2000);
            }

            state = CONNECTED_LOBBY;
            printf("Reconnected\n");

        }

        Sleep(10);
    }

    free(received.data);
    closesocket(server_socket);
    WSACleanup();

    return 0;
}

Color player_color(size_t player_id) {
    return (Color){(player_id*681)%(255), 50*player_id, 220, 255};
}

#define LOBBY_PLAYER_RADIUS 40
#define LOBBY_RADIUS 300
#define LOBBY_COLOR YELLOW

void draw_lobby() {
    int mid_x = WINDOW_WIDTH/2;
    int mid_y = WINDOW_HEIGHT/2;
    DrawCircle(mid_x, mid_y, LOBBY_RADIUS, LOBBY_COLOR);
    DrawCircle(mid_x, mid_y, LOBBY_RADIUS - 5, BG_COLOR);
    const char* lobby_text = "Lobby";
    DrawText(lobby_text, mid_x - MeasureText(lobby_text, 100)/2, mid_y - 100/2, 100, LOBBY_COLOR);
    int n = lobby_state.number_of_players;
    for (int i=0; i < n; i++) {
        float x = LOBBY_RADIUS * cosf(2*M_PI/n*i) + mid_x;
        float y = LOBBY_RADIUS * sinf(2*M_PI/n*i) + mid_y;
        DrawCircle(x, y, LOBBY_PLAYER_RADIUS, player_color(i));
        if (!lobby_state.rdy[i]) DrawCircle(x, y, LOBBY_PLAYER_RADIUS - 5, BG_COLOR);
    }
}

void draw_game() {
    size_t count = g_world.player_count;
    for (int i=0; i<count; i++) {
        float angle = g_world.player_angles[i];
        float x = g_world.player_xs[i];
        float y = g_world.player_ys[i];
        float dx = cosf(angle);
        float dy = sinf(angle);
        Vec2 dir = {dx, dy};
        Vec2 offset = scale(dir, 30.f);
        Vec2 right = {-offset.y, offset.x};
        Vec2 pos = {x, y};
        Vec2 nose = add(pos, offset);
        Vec2 rear_right = add(pos, right);
        rear_right = sub(rear_right, offset);
        Vec2 rear_left = sub(pos, right);
        rear_left = sub(rear_left, offset);

        DrawTriangle(
            (Vector2){nose.x, nose.y},
            (Vector2){rear_left.x, rear_left.y},
            (Vector2){rear_right.x, rear_right.y},
            player_color(i));

    }

    count = g_world.shots.count;
    for (int i=0; i<count; i++) {
        DrawCircle(g_world.shots.positions[i].x, g_world.shots.positions[i].y, 5, WHITE);
    }
}

void draw_gamepad() {
    int w = 1150;
    int h = 80;
    DrawRectangle(w+160, 0, 370, 130, BLACK);
    DrawCircle(w+259, 152 - h, 34, LIGHTGRAY);
    DrawCircle(w+259 + (int)(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X)*20), 152 - h + (int)(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y)*20), 25, RED);

    // Draw buttons: basic
    if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT)) DrawCircle(w+236, 150 - h, 9, RED);
    if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_LEFT)) DrawCircle(w+152, 150 - h, 9, RED);
    if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) DrawCircle(w+301, 151 - h, 15, BLUE);
    if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) DrawCircle(w+336, 187 - h, 15, LIME);
    if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) DrawCircle(w+372, 151 - h, 15, MAROON);
    if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP)) DrawCircle(w+336, 115 - h, 15, GOLD);


    // Draw axis: left-right triggers
    DrawRectangle(w+190, 30, 15, 70, GRAY);
    DrawRectangle(w+404, 30, 15, 70, GRAY);
    DrawRectangle(w+190, 30, 15, (int)(((1 + GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_TRIGGER))/2)*70), RED);
    DrawRectangle(w+404, 30, 15, (int)(((1 + GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_TRIGGER))/2)*70), RED);
}

void draw_stats() {
    Color text_color = ORANGE;
    if (IsGamepadAvailable(gamepad)) {
        DrawText(TextFormat("GP%d: %s", gamepad, GetGamepadName(gamepad)), 10, 10, 20, text_color);
    } else {
        DrawText(TextFormat("No gamepad found (%d)", gamepad) , 10, 10, 20, text_color);
    }
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 40, 20, text_color);
}

int main() {

    CreateThread(NULL, 0, connection_thread, (LPVOID)server_socket, 0, NULL);

    if (!start_game_time()) return 1;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello");

    int w = WINDOW_WIDTH;
    int h = WINDOW_HEIGHT;

    while(true) {

        // Handle Input

        // Change gamepad
        if (IsKeyPressed(KEY_LEFT) && gamepad > 0) gamepad--;
        if (IsKeyPressed(KEY_RIGHT)) gamepad++;

        // In game input (Player_Input)
        Player_Input input = g_world.player_inputs[my_id];
        input.target_angle = g_world.player_angles[my_id];
        if (state == CONNECTED_GAME) {
            if (my_id < g_world.player_count) {
                float x_axis;
                float y_axis;
                x_axis = IsKeyDown(KEY_D) - IsKeyDown(KEY_A);
                y_axis = IsKeyDown(KEY_S) - IsKeyDown(KEY_W);
                if (IsGamepadAvailable(gamepad)) {
                    x_axis = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
                    y_axis = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);
                }

                Vec2 pad_vec = (Vec2){x_axis, y_axis};
                if (length(pad_vec) > 0.35f) {
                    Vec2 dir = pad_vec;
                    normalize_or_y_axis(&dir);
                    const Vec2 right = (Vec2){1.0f, 0.0f};
                    float angle = -angle_between(dir, right);
                    input.target_angle = angle;
                }
            }

        }

        if (IsKeyDown(KEY_SPACE)) {
            input.flags |= PRIMARY_DOWN;
        }

        if (IsKeyUp(KEY_SPACE)){
            input.flags &= ~PRIMARY_DOWN;
        }

        if (IsGamepadAvailable(gamepad)) {
            input.flags &= ~PRIMARY_DOWN;
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                input.flags |= PRIMARY_DOWN;
            }
        }



        maybe_add_to_input_queue(input);
        handle_game_input(input, my_id);


        if (WindowShouldClose()) break;

        BeginDrawing();
        ClearBackground(BG_COLOR);
        draw_stats();

        if (IsGamepadAvailable(gamepad)) {
            draw_gamepad();
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
            case CONNECTED_LOBBY: {
                draw_lobby();
            } break;
            case CONNECTED_GAME: {
                //printf("acc time: %f, tick time done: %f\n", g_world.time.accumulated_time, g_world.ticks * TICK_TIME);
                while(g_world.time.accumulated_time > g_world.ticks * TICK_TIME) {
                    tick();
                }

                draw_game();
                DrawCircle(w/2, h/2, 50.0f, RED);
            } break;
        default:
            assert(false && "unhandeled state in main");
            break;
        }

        EndDrawing();

        wait_game_time();
    }

    return 0;
}
