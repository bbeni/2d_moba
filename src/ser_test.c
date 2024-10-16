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
    Test_Data t = {'x', 4.56f, 123, "hello from the test data struct! :D"};
    Message m = {0};
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
