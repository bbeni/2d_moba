#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>
#include "serializer.h"

#define SERVER "127.0.0.1"
#define PORT 27015

typedef enum Message_Type: char {
    STATE_SYNC,     // TODO: remove or rename it
    PLAYER_INPUT,
} Message_Type;

#define SER_STRUCT_NAME State_Sync
#define SER_FIELDS                          \
    SER_FIELD(uint32_t, server_id)          \
    SER_ARRAY(char,     server_name, 36)    \
    SER_FIELD(uint32_t, number_of_players) 
#define SER_CREATE
#include "serializer.h"    

typedef enum Input_Flags: uint32_t {
    NOTHING_DOWN   = 0x0,
    PRIMARY_DOWN   = 0x1,
    SECONDARY_DOWN = 0x2,
    L_DOWN         = 0x4,
    R_DOWN         = 0x8,
} Input_Flags;

typedef struct Player_Input {
    Input_Flags input_flags;
    float       target_angle;
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
