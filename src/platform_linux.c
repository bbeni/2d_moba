#ifdef LINUX
// linux/unix platform specific implementations
#include "platform.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
#include <assert.h>
#include <stdio.h>

bool open_connection(Socket* sock, const char* address, uint32_t port) {
    assert(false && "conect_to_socket(): not implemented for unix like");
    return false;
}

bool close_connection(Socket sock) {
    int status = shutdown(sock, SHUT_RDWR);
    if (status != 0) {
        printf("close_connection(): shutdown() failed on unix-like with code %d\n", status);
        *sock = 
        return false;
    }
    status = close(sock);
    if (status != 0) {
        printf("close_connection(): close() failed on unix-like with code %d\n", status);
        return false;
    }
    return true;
}

bool set_non_block(Socket sock) {
    assert(false && "conect_to_socket(): not implemented for unix like");
    return false;
}

void sleep_ms(int sleepMs) {
    usleep(sleepMs * 1000);
}

void create_thread(Thread (*thread_func)(), Socket sock) {
    assert(false && "create_thread(): not implemented for unix-like");
}


#endif // LINUX
