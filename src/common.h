#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

#define SERVER "127.0.0.1"
#define PORT 27015

typedef enum Message_Type: char {
    STATE_SYNC,     // TODO: remove or rename it
    PLAYER_INPUT,
} Message_Type;

typedef struct Message {
    size_t length;
    char* data;
} Message;

typedef struct State_Sync {
    Message_Type type;

    uint32_t server_id;
    char welcome_string[512];
} State_Sync;

typedef enum Input_Flags: uint32_t {
    NOTHING_DOWN   = 0x0,
    PRIMARY_DOWN   = 0x1,
    SECONDARY_DOWN = 0x2,
    L_DOWN         = 0x4,
    R_DOWN         = 0x8,
} Input_Flags;

typedef struct Player_Input {
    Message_Type type;

    Input_Flags input_flags;
    float       angle_target;
} Player_Input;

Message_Type read_message_type(Message message);

Message serialize_state_sync(State_Sync* state_sync);
State_Sync deserialize_state_sync(Message message);

Message serialize_player_input(Player_Input* player_input);
Player_Input deserialize_player_input(Message message);


// my temp storage solution

void temp_init(size_t capacity);
void* temp_malloc(size_t size);
void temp_reset();
void temp_deinit();

#endif // _COMMON_H_
