#include "common.h"
#define SERIALIZER_IMPLEMENTATION
#include "serializer.h"

#include <assert.h>
#include <stdio.h>


// network stuff

void send_message(SOCKET socket, Message* msg, Message_Type type) {
    extend_message_capacity(msg, 1);
    msg->data[msg->length++] = (char)type;
    send(socket, msg->data, msg->length, 0);
}

Message_Type extract_message_type(Message* msg) {
    return (Message_Type)msg->data[--msg->length];
}

// my temp storage solution

struct Temp_Storage {
    void* data;
    size_t capacity;
    size_t cursor;
};

struct Temp_Storage _temp_storage = {0};

void temp_init(size_t capacity) {
    _temp_storage.data = malloc(capacity);
    _temp_storage.capacity = capacity;
}

void* temp_malloc(size_t size) {
    size_t offset = _temp_storage.cursor;
    if (_temp_storage.cursor + size <= _temp_storage.capacity) {
        _temp_storage.cursor += size;
        return _temp_storage.data + offset;
    } else {
        printf("temp_malloc: overflow, not enough space\n");
        abort();
    }
}

void temp_reset() { // no free but reset everything once per frame
    _temp_storage.cursor = 0;
}

void temp_deinit() {
    temp_reset();
    if (_temp_storage.data) {
        free(_temp_storage.data);
        _temp_storage.capacity = 0;
    }
}

