#include "forge/socket.h"
#include "forge/mem.h"
#include "forge/types.h"

#include <stdlib.h>

#ifndef FORGE_SOCKET_USE_OWN_POOL
#define FORGE_SOCKET_USE_OWN_POOL 0
#endif

#define SOCKET_POOL_SIZE 0xC0000 // 768 KB (minimum)
#define SOCKET_ALLOC_POOL_SIZE 0x20000 // 128 KB

#define GAME_SOCKET_INIT_OFFSET 0x813B44
#define GAME_SOCKET_OBJECT_OFFSET 0x191EEE8

static u8* s_socketPool = NULL;
typedef void (*GameSocketInit)(void*, int*);

int forge_socket_initDefault(void)
{
#if FORGE_SOCKET_USE_OWN_POOL
    s_socketPool = aligned_alloc(PAGE_SIZE, SOCKET_POOL_SIZE);
    return forge_socket_init(s_socketPool, SOCKET_POOL_SIZE, SOCKET_ALLOC_POOL_SIZE, 4);
#else
    int result = 0;
    void* socketObj = *(void**)(g_mainTextAddr + GAME_SOCKET_OBJECT_OFFSET);
    void* realObj = *(void**)((u8*)socketObj + 0x2C);

    GameSocketInit init = (GameSocketInit)(g_mainTextAddr + GAME_SOCKET_INIT_OFFSET);

    init(realObj, &result);

    return result;
#endif
}

int forge_socket_init(void* pool, size_t pool_size, size_t allocator_pool_size, int concurrency_count)
{
    return nnsocketInitialize(pool, pool_size, allocator_pool_size, concurrency_count);
}

void forge_socket_deinit(void)
{
    nnsocketFinalize();
    free(s_socketPool);
    s_socketPool = NULL;
}

Socket forge_socket_create(int domain, int type, int protocol)
{
    return (Socket) { nnsocketSocket(domain, type, protocol) };
}

void forge_socket_destroy(Socket* socket)
{
    if (socket->fd < 0) {
        return;
    }

    nnsocketClose(socket->fd);
    socket->fd = -1;
}

int forge_socket_bind(const Socket* socket, const struct sockaddr* addr, socklen_t addrlen)
{
    return nnsocketBind(socket->fd, addr, addrlen);
}

int forge_socket_connect(const Socket* socket, const struct sockaddr* addr, socklen_t addrlen)
{
    return nnsocketConnect(socket->fd, addr, addrlen);
}

int forge_socket_listen(const Socket* socket, int backlog)
{
    return nnsocketListen(socket->fd, backlog);
}

Socket forge_socket_accept(const Socket* socket, struct sockaddr* addr, socklen_t* addrlen)
{
    return (Socket) { nnsocketAccept(socket->fd, addr, addrlen) };
}

int forge_socket_shutdown(const Socket* socket, int how)
{
    return nnsocketShutdown(socket->fd, how);
}

ssize_t forge_socket_send(const Socket* socket, const void* buf, size_t len, int flags)
{
    return nnsocketSend(socket->fd, buf, len, flags);
}

ssize_t forge_socket_recv(const Socket* socket, void* buf, size_t len, int flags)
{
    return nnsocketRecv(socket->fd, buf, len, flags);
}

int forge_socket_getSockOpt(const Socket* socket, int level, int optname, void* optval, socklen_t* optlen)
{
    return nnsocketGetSockOpt(socket->fd, level, optname, optval, optlen);
}

int forge_socket_setSockOpt(const Socket* socket, int level, int optname, const void* optval, socklen_t optlen)
{
    return nnsocketSetSockOpt(socket->fd, level, optname, optval, optlen);
}
