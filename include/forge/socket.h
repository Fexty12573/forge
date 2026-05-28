#pragma once

#include "nn/socket.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Socket {
    int fd;
} Socket;

// Library lifecycle
int forge_socket_initDefault(void);
int forge_socket_init(void* pool, size_t pool_size, size_t allocator_pool_size, int concurrency_count);
void forge_socket_deinit(void);

// Socket lifecycle
Socket forge_socket_create(int domain, int type, int protocol);
void forge_socket_destroy(Socket* socket);

// Connection management
int forge_socket_bind(const Socket* socket, const struct sockaddr* addr, socklen_t addrlen);
int forge_socket_connect(const Socket* socket, const struct sockaddr* addr, socklen_t addrlen);
int forge_socket_listen(const Socket* socket, int backlog);
Socket forge_socket_accept(const Socket* socket, struct sockaddr* addr, socklen_t* addrlen);
int forge_socket_shutdown(const Socket* socket, int how);

// I/O
ssize_t forge_socket_send(const Socket* socket, const void* buf, size_t len, int flags);
ssize_t forge_socket_recv(const Socket* socket, void* buf, size_t len, int flags);

// Socket options
int forge_socket_getSockOpt(const Socket* socket, int level, int optname, void* optval, socklen_t* optlen);
int forge_socket_setSockOpt(const Socket* socket, int level, int optname, const void* optval, socklen_t optlen);

#ifdef __cplusplus
}
#endif
