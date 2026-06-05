#pragma once

#include "switch/types.h"
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Lifecycle
int nnsocketInitialize(void* pool, size_t pool_size, size_t allocator_pool_size, int concurrency_count);
int nnsocketFinalize(void);
int nnsocketOpen(void);

// Cancel support
int nnsocketCancel(int sockfd);
int nnsocketRequestCancelHandle(int* handle);

// Core socket operations
int nnsocketAccept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
int nnsocketBind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int nnsocketClose(int sockfd);
int nnsocketConnect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int nnsocketDuplicateSocket(int oldfd, int newfd);
int nnsocketFcntl(int fd, int cmd, ...);
int nnsocketIoctl(int fd, unsigned long request, ...);
int nnsocketListen(int sockfd, int backlog);
int nnsocketShutdown(int sockfd, int how);
int nnsocketSockAtMark(int sockfd);
int nnsocketSocket(int domain, int type, int protocol);

// I/O
ssize_t nnsocketRead(int fd, void* buf, size_t count);
ssize_t nnsocketWrite(int fd, const void* buf, size_t count);
ssize_t nnsocketRecv(int sockfd, void* buf, size_t len, int flags);
ssize_t nnsocketRecvFrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
ssize_t nnsocketRecvMsg(int sockfd, struct msghdr* msg, int flags);
ssize_t nnsocketSend(int sockfd, const void* buf, size_t len, int flags);
ssize_t nnsocketSendMsg(int sockfd, const struct msghdr* msg, int flags);
ssize_t nnsocketSendTo(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen);

// Multiplexing
int nnsocketPoll(struct pollfd* fds, nfds_t nfds, int timeout);
int nnsocketSelect(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);

// Socket options and naming
int nnsocketGetSockOpt(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
int nnsocketSetSockOpt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);
int nnsocketGetPeerName(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
int nnsocketGetSockName(int sockfd, struct sockaddr* addr, socklen_t* addrlen);

// Name resolution
int nnsocketGetAddrInfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res);
int nnsocketGetAddrInfoWithOptions(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res, int options);
void nnsocketFreeAddrInfo(struct addrinfo* res);
const char* nnsocketGAIStrError(int ecode);
int nnsocketGetNameInfo(const struct sockaddr* sa, socklen_t salen, char* host, socklen_t hostlen, char* serv, socklen_t servlen, int flags);
int nnsocketGetNameInfoWithOptions(const struct sockaddr* sa, socklen_t salen, char* host, socklen_t hostlen, char* serv, socklen_t servlen, int flags, int options);
struct hostent* nnsocketGetHostByAddr(const void* addr, socklen_t len, int type);
struct hostent* nnsocketGetHostByAddrWithOptions(const void* addr, socklen_t len, int type, int options);
struct hostent* nnsocketGetHostByName(const char* name);
struct hostent* nnsocketGetHostByNameWithOptions(const char* name, int options);

// Error handling
int nnsocketGetHErrno(void);
int nnsocketGetLastErrno(void);
void nnsocketSetLastErrno(int err);
const char* nnsocketHStrError(int err);

// Address conversion
int nnsocketInetAton(const char* cp, struct in_addr* inp);
char* nnsocketInetNtoa(struct in_addr in);
const char* nnsocketInetNtop(int af, const void* src, char* dst, socklen_t size);
int nnsocketInetPton(int af, const char* src, void* dst);
uint32_t nnsocketInetHtonl(uint32_t hostlong);
uint16_t nnsocketInetHtons(uint16_t hostshort);
uint32_t nnsocketInetNtohl(uint32_t netlong);
uint16_t nnsocketInetNtohs(uint16_t netshort);

// Sysctl
int nnsocketSysctl(const int* name, unsigned int namelen, void* oldp, size_t* oldlenp, const void* newp, size_t newlen);

#ifdef __cplusplus
}
#endif
