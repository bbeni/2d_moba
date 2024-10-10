/* STBI type library using X macros for struct serialization example useage:

#define SERIALIZER_IMPLEMENTATION
#include "serializer.h"

#define SER_STRUCT_NAME Test_Data
#define SER_FIELDS         \
    SER_FIELD(char,     a) \
    SER_FIELD(float,    b) \
    SER_FIELD(uint32_t, c)
#define SER_CREATE
#include "serializer.h"

#define SER_STRUCT_NAME Vec2
#define SER_FIELDS         \
    SER_FIELD(float, x)    \
    SER_FIELD(float, y)
#define SER_CREATE
#include "serializer.h"

int main() {
    Test_Data t  = {"x", 4.56f, 123};
    Message m    = serialize_Test_Data(&t);
    Test_Data t2 = deserialize_Test_Data(m);

    printf("t2.a = %d\n", t2.a);
    printf("t2.b = %d\n", t2.b);
    printf("t2.c = %d\n", t2.c);
}

*/

#ifndef _SERIALIZER_H_
#define _SERIALIZER_H_

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#define MESSAGE_INITIAL_CAPACITY 4
#ifndef MESSAGE_REALLOC
#  include <stdlib.h>
#  define MESSAGE_REALLOC realloc
#endif

typedef struct Message {
    char*  data;
    size_t length;
    size_t capacity;
} Message;

void extend_message_capacity(Message* message, size_t size);
void serialize_char(Message* message, char val);
void serialize_uint32_t(Message* message, uint32_t val);
void serialize_float(Message* message, float val);
void* consume_uint32_t(void* buf, uint32_t* out);
void* consume_float(void* buf, float* out);
void* consume_char(void* buf, char* out);

#endif // _SERIALIZER_H_

#ifdef SERIALIZER_IMPLEMENTATION
#undef SERIALIZER_IMPLEMENTATION

// TODO: endianness conversion!!
/* static inline bool is_little_endian() {
    volatile uint32_t i=0x01234567;
    return (*((uint8_t*)(&i))) == 0x67;
}*/

uint32_t pack_uint32_t(uint32_t val) {return val;};
uint32_t pack_float(float val) {
    return *(uint32_t*)&val;
};

uint32_t unpack_uint32_t(uint32_t in) {return in;};
float    unpack_float(uint32_t in) {
    return *(float*)&in;
};

void extend_message_capacity(Message* message, size_t size) {
    if (message->length + size > message->capacity) {
        if (message->capacity == 0) {
            message->capacity = MESSAGE_INITIAL_CAPACITY;
        } else {
            message->capacity *= 2;
        }
        message->data = MESSAGE_REALLOC(message->data, message->capacity);
        assert(message->data != NULL && "cant realloc more space...");
    }
}

void serialize_char(Message* message, char val) {
    extend_message_capacity(message, 1);
    message->data[0] = val;
    message->length += 1;
}

void serialize_uint32_t(Message* message, uint32_t val) {
    extend_message_capacity(message, 4);
    uint32_t* buf = (uint32_t*)(&message->data[message->length]);
    message->length += 4;
    buf[0] = pack_uint32_t(val);
}

void serialize_float(Message* message, float val) {
    extend_message_capacity(message, 4);
    uint32_t* buf = (uint32_t*)(&message->data[message->length]);
    message->length += 4;
    buf[0] = pack_float(val);
}

void* consume_uint32_t(void* buf, uint32_t* out) {
    *out = unpack_uint32_t(((uint32_t*)buf)[0]);
    return buf + 4;
}

void* consume_float(void* buf, float* out) {
    *out = unpack_float(((uint32_t*)buf)[0]);
    return buf + 4;
}

void* consume_char(void* buf, char* out) {
    *out = ((char*)buf)[0];
    return buf + 1;
}

#endif // SERIALIZER_IMPLEMENTATION

//
// WELCOME TO MACRO LAND ! <3
//

#ifdef SER_CREATE
#undef SER_CREATE

#define SER_FIELD(type, name) type name;
typedef struct SER_STRUCT_NAME {
    SER_FIELDS
} SER_STRUCT_NAME;
#undef SER_FIELD

#define SER_FIELD(type, name) serialize ## _ ## type(&message, data->name);
#define MAKE_NAME(prefix, struct_name) prefix ## _ ## struct_name
#define SER_DECLAREFUNC(prefix, struct_name) Message MAKE_NAME(prefix, struct_name)(struct_name* data)
SER_DECLAREFUNC(serialize, SER_STRUCT_NAME) {
    Message message = {0};
    SER_FIELDS
    return message;
};
#undef SER_FIELD

#define SER_FIELD(type, name) buf = consume ## _ ## type(buf, &data.name);
#define SER_DECLAREFUNC2(prefix, struct_name) struct_name MAKE_NAME(prefix, struct_name)(Message* message)

SER_DECLAREFUNC2(deserialize, SER_STRUCT_NAME) {
    SER_STRUCT_NAME data = {0};
    void* buf = message->data;
    SER_FIELDS
    return data;
};

#undef SER_FIELD
#undef SER_FIELDS
#undef SER_STRUCT_NAME

#endif // SER_CREATE
