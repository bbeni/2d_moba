#ifndef _SPECIFIC_H_
#define _SPECIFIC_H_
// unified API to platform specific stuff here that is not from raylib
// like for now: networking, sleep, threads
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
typedef long long unsigned int Socket;
typedef long unsigned int Thread;
#else
typedef uint32_t Thread;
typedef int Socket;
#endif

bool open_connection(Socket* sock, const char* address, uint32_t port);
bool close_connection(Socket sock);
bool set_non_block(Socket sock);

void create_thread(Thread (*thread_func)(), Socket sock);
void sleep_ms(int ms);

#endif // _NETWORKING_H_

