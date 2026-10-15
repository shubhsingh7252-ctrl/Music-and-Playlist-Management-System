/* platform.h - tiny portability layer (Windows/MinGW + Linux/macOS) */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stddef.h>
#ifdef _WIN32
  #ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0601
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  typedef SOCKET sock_t;
  typedef CRITICAL_SECTION mutex_t;
  #define SOCK_BAD INVALID_SOCKET
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <pthread.h>
  typedef int sock_t;
  typedef pthread_mutex_t mutex_t;
  #define SOCK_BAD (-1)
#endif

typedef void (*task_fn)(void *);

int  net_init(void);
void sock_close_(sock_t s);
void sock_timeout(sock_t s, int seconds);
void mutex_init_(mutex_t *m);
void mutex_lock_(mutex_t *m);
void mutex_unlock_(mutex_t *m);
int  spawn_detached(task_fn fn, void *arg);
void rand_bytes(unsigned char *buf, size_t n);
long long now_ms(void);
void make_dir(const char *path);
int  file_replace(const char *tmp, const char *dst);

#endif
