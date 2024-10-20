#ifndef _COMMON_H_
#define _COMMON_H_

#include <winsock2.h>
#include <stdint.h>
#include <stdbool.h>

#include "serializer.h"
#include "mathematics.h"

// Configuration

#define SERVER "127.0.0.1"
#define PORT 27015
#define MAX_PLAYERS 24
#define MAX_SHOTS 128
#define TICK_TIME 0.02      // seconds
#define SPEED 9.5f          // units per tick
#define SHOT_SPEED 20.0f    // units per tick
#define SHOT_COOLDOWN 5     // ticks
#define ANGLE_SPEED 0.08f   // radians per tick
#define WORLD_WIDTH 1024
#define WORLD_HEIGHT 576
#define SERVER_CONNECTION_THREAD_SLEEP 10 // millis

// Game stuff

typedef struct Game_Time {
    LARGE_INTEGER frequency;
    LARGE_INTEGER global_start_time;
    LARGE_INTEGER frame_start_time;
    LARGE_INTEGER frame_end_time;
    double frame_time, accumulated_time;
} Game_Time;

bool start_game_time();
void wait_game_time();

typedef enum : uint32_t {
    NOTHING_DOWN   = 0x0,
    PRIMARY_DOWN   = 0x1,
    SECONDARY_DOWN = 0x2,
    L_DOWN         = 0x4,
    R_DOWN         = 0x8,
} Input_Flags;

#define SER_STRUCT_NAME Player_Input
#define SER_FIELDS \
    SER_FIELD(uint32_t, flags) \
    SER_FIELD(float, target_angle)
#define SER_CREATE
#include "serializer.h"

void handle_game_input(Player_Input input, size_t player_id);

typedef struct Shots {
    size_t count;
    Vec2 positions[MAX_SHOTS];
    Vec2 directions[MAX_SHOTS];
    size_t shooter_ids[MAX_SHOTS];
} Shots;

typedef struct Game_World {
    size_t player_count;
    uint32_t ticks;
    Game_Time time;
    float player_xs[MAX_PLAYERS];
    float player_ys[MAX_PLAYERS];
    float player_angles[MAX_PLAYERS];
    float player_target_angles[MAX_PLAYERS];
    Player_Input player_inputs[MAX_PLAYERS];
    Shots shots;
} Game_World;

extern Game_World g_world;

void tick();
void add_player();

// Networking stuff

#define MESSAGE_MAX_LEN 4096

typedef enum Message_Type: char {
    LOBBY_TOGGLE_ACCEPT,
    LOBBY_SYNC,
    STATE_SYNC,
    PLAYER_INPUT,
} Message_Type;

#define SER_STRUCT_NAME State_Sync
#define SER_FIELDS                           \
    SER_FIELD(uint32_t, server_id)           \
    SER_FIELD(uint32_t, player_id)           \
    SER_FIELD(uint32_t, number_of_players)   \
    SER_FIELD(uint32_t, ticks)               \
    SER_FIELD(float,    accumulated_time)    \
    SER_ARRAY(float,    xs, MAX_PLAYERS)     \
    SER_ARRAY(float,    ys, MAX_PLAYERS)     \
    SER_ARRAY(float,    angles, MAX_PLAYERS) \
    SER_ARRAY(float,    target_angles, MAX_PLAYERS) \
    SER_FIELD(uint32_t, number_of_shots) \
    SER_ARRAY(uint32_t, shots_player_ids, MAX_SHOTS) \
    SER_ARRAY(float,    shots_xs, MAX_SHOTS) \
    SER_ARRAY(float,    shots_ys, MAX_SHOTS) \
    SER_ARRAY(float,    shots_direction_xs, MAX_SHOTS) \
    SER_ARRAY(float,    shots_direction_ys, MAX_SHOTS)
#define SER_CREATE
#include "serializer.h"

#define SER_STRUCT_NAME Lobby_Sync
#define SER_FIELDS \
    SER_FIELD(uint32_t, number_of_players) \
    SER_ARRAY(bool, rdy, MAX_PLAYERS)
#define SER_CREATE
#include "serializer.h"

void send_message(SOCKET socket, Message* msg, Message_Type type);
Message_Type extract_message_type(Message* msg);

// my temp storage solution

void temp_init(size_t capacity);
void* temp_malloc(size_t size);
void temp_reset();
void temp_deinit();

#endif // _COMMON_H_
