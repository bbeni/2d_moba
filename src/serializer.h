/*  serializer.h - copyright Benjamin Froelich <bbenif@gmail.com> 2024

    STBI type library using X macros for struct serialization example usage:

// Simple example for serializer.h 
#define SERIALIZER_IMPLEMENTATION
#include "serializer.h"
#include <stdio.h>
#include <string.h>

#define SER_STRUCT_NAME Test_Data
#define SER_FIELDS         \
    SER_FIELD(char,     a) \
    SER_FIELD(float,    b) \
    SER_FIELD(uint32_t, c) \
    SER_ARRAY(char,     d, 256)
#define SER_CREATE
#include "serializer.h"

#define SER_STRUCT_NAME Vec2
#define SER_FIELDS      \
    SER_FIELD(float, x) \
    SER_FIELD(float, y)
#define SER_CREATE
#include "serializer.h"

int main() {
    Message m = {0};

    Test_Data t = {'x', 4.56f, 123, "hello from the test data struct! :D"};
    serialize_Test_Data(&m, &t);
    Test_Data t2 = deserialize_Test_Data(&m);

    printf("t2.a = %c\n", t2.a);
    printf("t2.b = %f\n", t2.b);
    printf("t2.c = %d\n", t2.c);
    printf("t2.d = %s\n", t2.d);
    printf("\n");

    Vec2 pos = {-0.55f, -3.14159f};
    serialize_Vec2(&m, &pos);
    Vec2 pos2 = deserialize_Vec2(&m);

    printf("pos2.x = %f\n", pos2.x);
    printf("pos2.y = %f\n", pos2.y);
    return 0;
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
void serialize_bool(Message* message, bool val);
void serialize_uint32_t(Message* message, uint32_t val);
void serialize_float(Message* message, float val);
void* consume_uint32_t(void* buf, uint32_t* out);
void* consume_float(void* buf, float* out);
void* consume_char(void* buf, char* out);
void* consume_bool(void* buf, bool* out);

// TODO: make array versions for all above
void serialize_char_array(Message* message, char* val, size_t length);
void serialize_bool_array(Message* message, bool* val, size_t length);
void serialize_float_array(Message* message, float* val, size_t length);
void* consume_char_array(void* buf, char* out, size_t length);
void* consume_bool_array(void* buf, bool* out, size_t length);
void* consume_float_array(void* buf, float* out, size_t length);

#endif // _SERIALIZER_H_

#ifdef SERIALIZER_IMPLEMENTATION
#undef SERIALIZER_IMPLEMENTATION

// TODO: endianness conversion!!
/* static inline bool is_little_endian() {
    volatile uint32_t i=0x01234567;
    return (*((uint8_t*)(&i))) == 0x67;
}*/

uint32_t pack_uint32_t(uint32_t val)  {return val;};
uint32_t pack_float(float val)        {return *(uint32_t*)&val;};
uint32_t unpack_uint32_t(uint32_t in) {return in;};
float    unpack_float(uint32_t in)    {return *(float*)&in;};

void extend_message_capacity(Message* message, size_t size) {
    if (message->length + size > message->capacity) {
        if (message->capacity == 0) {
            message->capacity = MESSAGE_INITIAL_CAPACITY;
        }
        while(message->length + size > message->capacity) {
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

void serialize_bool(Message* message, bool val) {
    extend_message_capacity(message, 1);
    message->data[0] = (char)val;
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

void serialize_char_array(Message* message, char* val, size_t length) {
    extend_message_capacity(message, length);
    for (int i = 0; i < length; i++) {
        message->data[message->length + i] = val[i];
    }
    message->length += length;
}

void serialize_bool_array(Message* message, bool* val, size_t length) {
    serialize_char_array(message, (char*)val, length);
}

void serialize_float_array(Message* message, float* val, size_t length) {
    for (int i = 0; i < length; i++) {
        serialize_float(message, val[i]);
    }
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

void* consume_bool(void* buf, bool* out) {
    *out = (bool)consume_char(buf, (char*)out);
    return buf + 1;
}

void* consume_char_array(void* buf, char* out, size_t length) {
    for(int i = 0; i < length; i++) {
        out[i] = ((char*)buf)[i];
    }
    return buf + length;
}

void* consume_bool_array(void* buf, bool* out, size_t length) {
    return consume_char_array(buf, (char*)out, length);
}

void* consume_float_array(void* buf, float* out, size_t length) {
    for(int i = 0; i < length; i++) {
        buf = consume_float(buf, &out[i]);
    }
    return buf;
}

#endif // SERIALIZER_IMPLEMENTATION

//
// WELCOME TO MACRO LAND ! <3
//

// NOTE: i was too lazy to separate decalarations and implementations of
// the deserialize_ and serialize_ functions. to avoid undefined reference
// linker errors, the functions are now just static. this should be fixed
// but for now just use this as a workaround and ingore warnings here!
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#ifdef SER_CREATE
#undef SER_CREATE

#define SER_FIELD(type, name) type name;
#define SER_ARRAY(type, name, length) type name[length];
typedef struct SER_STRUCT_NAME {
    SER_FIELDS
} SER_STRUCT_NAME;
#undef SER_ARRAY
#undef SER_FIELD

#define SER_FIELD(type, name) serialize ## _ ## type(message, data->name);
#define SER_ARRAY(type, name, length) serialize ## _ ## type ## _array(message, data->name, length);
#define MAKE_NAME(prefix, struct_name) prefix ## _ ## struct_name
#define SER_DECLAREFUNC(prefix, struct_name) static void MAKE_NAME(prefix, struct_name)(Message* message, struct_name* data)
SER_DECLAREFUNC(serialize, SER_STRUCT_NAME) {
    message->length = 0; // reset so we dont realloc!
    SER_FIELDS
};
#undef SER_ARRAY
#undef SER_FIELD

#define SER_FIELD(type, name) buf = consume ## _ ## type(buf, &data.name);
#define SER_ARRAY(type, name, length) buf = consume ## _ ## type ## _array(buf, data.name, length);
#define SER_DECLAREFUNC2(prefix, struct_name) static struct_name MAKE_NAME(prefix, struct_name)(Message* message)

SER_DECLAREFUNC2(deserialize, SER_STRUCT_NAME) {
    SER_STRUCT_NAME data = {0};
    void* buf = message->data;
    SER_FIELDS
    return data;
};

#undef SER_ARRAY
#undef SER_FIELD
#undef SER_FIELDS
#undef SER_STRUCT_NAME

#endif // SER_CREATE

#pragma GCC diagnostic pop
