#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

#define SERVER "127.0.0.1"
#define PORT 27015

typedef enum Message_Type: char {
	STATE_SYNC,
	JOINED,
	MOVED,
} Message_Type;

typedef struct Message {
	size_t length;
	char* data;
} Message;

typedef struct State_Sync {
	enum Message_Type type;
	
	uint32_t server_id;
	char welcome_string[512];
} State_Sync;

Message_Type read_message_type(Message message);
Message serialize_state_sync(State_Sync* state_sync);
State_Sync deserialize_state_sync(Message message);

#endif // _COMMON_H_
