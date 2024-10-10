#include "common.h"
#include "winsock2.h"
#include <assert.h>
#include <stdio.h>

char* serialize_type(char* buf, enum Message_Type type) {
    buf[0] = (char)type;
    return buf + 1;
}

char* serialize_uint(char* buf, uint32_t val) {
    uint32_t* b2 = (uint32_t*)buf;
    b2[0] = htonl(val);
    return buf + 4;
}

char* serialize_str(char* buf, char* val, int len) {
    // TODO: maybe use memcopy?
    for (int i =0; i < len; i++) {
        buf[i] = val[i];
    }
    return buf + len;
}

Message serialize_state_sync(State_Sync* state_sync) {
    Message msg = {0};
    msg.length = sizeof(Message_Type) + sizeof(state_sync->server_id) + sizeof(char) * 256;

    // TODO: handle free
    char* buffer = malloc(msg.length);
    msg.data = buffer;

    buffer = serialize_type(buffer, STATE_SYNC);
    buffer = serialize_uint(buffer, state_sync->server_id);
    buffer = serialize_str(buffer, state_sync->welcome_string, 256);

    // printf("%lld, %lld\n", msg.data - buffer,  msg.length);
    assert(buffer - msg.data == msg.length);

    return msg;
}

char* deserialize_uint(char* buf, uint32_t* out) {
    *out = ntohl(((uint32_t*)buf)[0]);
    return buf + 4;
}

char* deserialize_str(char* buf, char* out, uint32_t len) {
    // TODO: maybe use memcopy?
    for (int i =0; i < len; i++) {
        out[i] = buf[i];
    }
    return buf + len;
}

State_Sync deserialize_state_sync(Message message) {

    State_Sync state = {0};
    char* buffer = message.data + 1; // we skip first byte as it is the already known type..

    state.type = STATE_SYNC;
    buffer = deserialize_uint(buffer, &state.server_id);
    buffer = deserialize_str(buffer, state.welcome_string, 256);

    assert(buffer - message.data == message.length || message.data - buffer == message.length);

    return state;
}

Message_Type read_message_type(Message message) {
    assert(message.length > 0);
    return (Message_Type)message.data[0];
}
