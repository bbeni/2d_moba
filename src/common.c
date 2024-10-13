#include "common.h"
#define SERIALIZER_IMPLEMENTATION
#include "serializer.h"
#include "mathematics.h"

#include <assert.h>
#include <stdio.h>

// game_stuff

Game_World g_world = {0};

void tick() {
    int count = g_world.player_count;
    for (int i=0; i<count; i++) {
        move_towards_on_circle(
            &g_world.player_angles[i],
            g_world.player_target_angles[i],
            ANGLE_SPEED,
            1.0f);
    }

    for (int i=0; i<count; i++) {
        float angle = g_world.player_angles[i];
        float dx = cosf(angle) * SPEED;
        float dy = sinf(angle) * SPEED;
        g_world.player_xs[i] += dx;
        g_world.player_ys[i] += dy;

        // periodic boundary
        if (g_world.player_xs[i] > WORLD_WIDTH) {
            g_world.player_xs[i] -= WORLD_WIDTH;
        };        
        if (g_world.player_xs[i] < 0) {
            g_world.player_xs[i] += WORLD_WIDTH;
        };        
        if (g_world.player_ys[i] > WORLD_HEIGHT) {
            g_world.player_ys[i] -= WORLD_HEIGHT;
        };        
        if (g_world.player_ys[i] < 0) {
            g_world.player_ys[i] += WORLD_HEIGHT;
        };        
    }
    
    g_world.ticks++;
    //printf("Tick Tock: %u\n", g_world.ticks);
};

void add_player() {
    int index = g_world.player_count;
    if (index >= MAX_PLAYERS) {
        printf("MAX_PLAYERS already full.\n");
        return;
    }
    g_world.player_count++;

    float x = (index*123819 + 2) % WORLD_WIDTH;
    float y = (index*index*1238 + 4) % WORLD_HEIGHT;

    // make sure all fields are set!
    g_world.player_xs[index] = x;
    g_world.player_ys[index] = y;
    g_world.player_angles[index] = 0.0f;
    g_world.player_target_angles[index] = M_PI * 0.5f;   
}

// time stuff

bool start_game_time() {
    if (!QueryPerformanceFrequency(&g_world.time.frequency)) {
        printf("QueryPerformanceFrequency failed!\n");
        return false;
    }
    QueryPerformanceCounter(&g_world.time.global_start_time);
    QueryPerformanceCounter(&g_world.time.frame_start_time);
    return true;
};

void wait_game_time() {

    Game_Time* gt = &g_world.time;

    //printf("Accumulated %f time\n", gt->accumulated_time);

    QueryPerformanceCounter(&gt->frame_end_time);    
    gt->frame_time = (double)(gt->frame_end_time.QuadPart - gt->frame_start_time.QuadPart) / gt->frequency.QuadPart;
    double sleep_time = TICK_TIME - gt->frame_time;
    double busy_wait_time = gt->frame_time;

    if (sleep_time > 0) {
        Sleep((DWORD)(sleep_time * 900)); // Sleep for 90% of the remaining time
        do {
            QueryPerformanceCounter(&gt->frame_end_time);
            busy_wait_time = (double)(gt->frame_end_time.QuadPart - gt->frame_start_time.QuadPart) / gt->frequency.QuadPart;
        } while (busy_wait_time < TICK_TIME);
    }
    QueryPerformanceCounter(&g_world.time.frame_start_time);
    gt->accumulated_time += busy_wait_time;
}


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

