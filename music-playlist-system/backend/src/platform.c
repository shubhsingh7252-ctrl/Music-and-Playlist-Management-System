#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
typedef struct { task_fn fn; void *arg; } Tramp;
static DWORD WINAPI tramp(LPVOID p) { Tramp t = *(Tramp *)p; free(p); t.fn(t.arg); return 0; }

int net_init(void) { WSADATA w; return WSAStartup(MAKEWORD(2, 2), &w); }
void sock_close_(sock_t s) { closesocket(s); }
void sock_timeout(sock_t s, int sec) {
    DWORD ms = (DWORD)sec * 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&ms, sizeof ms);
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char *)&ms, sizeof ms);
}
void mutex_init_(mutex_t *m) { InitializeCriticalSection(m); }
void mutex_lock_(mutex_t *m) { EnterCriticalSection(m); }
void mutex_unlock_(mutex_t *m) { LeaveCriticalSection(m); }
int spawn_detached(task_fn fn, void *arg) {
    Tramp *t = malloc(sizeof *t);
    if (!t) return -1;
    t->fn = fn; t->arg = arg;
    HANDLE h = CreateThread(NULL, 0, tramp, t, 0, NULL);
    if (!h) { free(t); return -1; }
    CloseHandle(h);
    return 0;
}
void rand_bytes(unsigned char *buf, size_t n) {
    typedef BOOLEAN (WINAPI *RtlGenRandomFn)(PVOID, ULONG);
    HMODULE lib = LoadLibraryA("advapi32.dll");
    RtlGenRandomFn fn = lib ? (RtlGenRandomFn)GetProcAddress(lib, "SystemFunction036") : NULL;
    if (fn && fn(buf, (ULONG)n)) return;
    srand((unsigned)(time(NULL) ^ GetCurrentProcessId() ^ GetTickCount()));
    for (size_t i = 0; i < n; i++) buf[i] = (unsigned char)(rand() & 0xFF);
}
long long now_ms(void) { return (long long)GetTickCount64(); }
void make_dir(const char *p) { _mkdir(p); }
int file_replace(const char *tmp, const char *dst) { remove(dst); return rename(tmp, dst); }

#else
typedef struct { task_fn fn; void *arg; } Tramp;
static void *tramp(void *p) { Tramp t = *(Tramp *)p; free(p); t.fn(t.arg); return NULL; }

int net_init(void) { return 0; }
void sock_close_(sock_t s) { close(s); }
void sock_timeout(sock_t s, int sec) {
    struct timeval tv = { sec, 0 };
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
}
void mutex_init_(mutex_t *m) { pthread_mutex_init(m, NULL); }
void mutex_lock_(mutex_t *m) { pthread_mutex_lock(m); }
void mutex_unlock_(mutex_t *m) { pthread_mutex_unlock(m); }
int spawn_detached(task_fn fn, void *arg) {
    Tramp *t = malloc(sizeof *t);
    if (!t) return -1;
    t->fn = fn; t->arg = arg;
    pthread_t th; pthread_attr_t a;
    pthread_attr_init(&a);
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
    int r = pthread_create(&th, &a, tramp, t);
    pthread_attr_destroy(&a);
    if (r) { free(t); return -1; }
    return 0;
}
void rand_bytes(unsigned char *buf, size_t n) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (f) { size_t r = fread(buf, 1, n, f); fclose(f); if (r == n) return; }
    srand((unsigned)time(NULL));
    for (size_t i = 0; i < n; i++) buf[i] = (unsigned char)(rand() & 0xFF);
}
long long now_ms(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
void make_dir(const char *p) { mkdir(p, 0755); }
int file_replace(const char *tmp, const char *dst) { return rename(tmp, dst); }
#endif
