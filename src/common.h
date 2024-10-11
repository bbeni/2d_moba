#ifndef _COMMON_H_
#define _COMMON_H_

#include "winsock2.h"
#include <stdint.h>
#include "serializer.h"

#define SERVER "127.0.0.1"
#define PORT 27015
#define MAX_PLAYERS 24

typedef enum Message_Type: char {
    STATE_SYNC,     // TODO: remove or rename it
    PLAYER_INPUT,
} Message_Type;

#define SER_STRUCT_NAME State_Sync
#define SER_FIELDS                         \
    SER_FIELD(uint32_t, server_id)         \
    SER_ARRAY(char,     server_name, 36)   \
    SER_FIELD(uint32_t, number_of_players) \
    SER_ARRAY(float,    xs, MAX_PLAYERS)   \
    SER_ARRAY(float,    ys, MAX_PLAYERS)   \
    SER_FIELD(uint32_t, player_id)
#define SER_CREATE
#include "serializer.h"    

typedef enum : uint32_t {
    NOTHING_DOWN   = 0x0,
    PRIMARY_DOWN   = 0x1,
    SECONDARY_DOWN = 0x2,
    L_DOWN         = 0x4,
    R_DOWN         = 0x8,
} Input_Flags;

typedef struct Player_Input {
    uint32_t input_flags;
    float    target_angle;
} Player_Input;

void send_message(SOCKET socket, Message* msg, Message_Type type);
Message_Type extract_message_type(Message* msg);

// my temp storage solution

void temp_init(size_t capacity);
void* temp_malloc(size_t size);
void temp_reset();
void temp_deinit();

#endif // _COMMON_H_
