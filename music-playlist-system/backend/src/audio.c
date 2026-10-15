#include "audio.h"
#include "platform.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SR 16000
#define SECS 24
#define MAXSEED 64
static unsigned char *cache[MAXSEED];
static mutex_t mu; static int mu_ready = 0;

static void le32(unsigned char *p, uint32_t v) { p[0] = v & 255; p[1] = (v >> 8) & 255; p[2] = (v >> 16) & 255; p[3] = (v >> 24) & 255; }
static void le16(unsigned char *p, uint16_t v) { p[0] = v & 255; p[1] = v >> 8; }

static unsigned char *synth(int seed, size_t *len) {
    const int n = SR * SECS;
    size_t total = 44 + (size_t)n * 2;
    unsigned char *b = malloc(total);
    memcpy(b, "RIFF", 4); le32(b + 4, (uint32_t)(36 + n * 2)); memcpy(b + 8, "WAVEfmt ", 8);
    le32(b + 16, 16); le16(b + 20, 1); le16(b + 22, 1); le32(b + 24, SR); le32(b + 28, SR * 2);
    le16(b + 32, 2); le16(b + 34, 16); memcpy(b + 36, "data", 4); le32(b + 40, (uint32_t)(n * 2));
    int sc[8] = {0, 2, 4, 7, 9, 12, 14, 16};
    for (int i = 0; i < 8; i++) sc[i] += (seed % 5) * 2;
    int st = (int)floor((double)SR * 30.0 / (90 + seed * 8));
    for (int i = 0; i < n; i++) {
        int k = i / st;
        double f = 220.0 * pow(2.0, sc[(k * (3 + seed % 3) + (k >> 2) * seed) % 8] / 12.0);
        double t = (double)(i % st) / st, e = exp(-3.0 * t), ph = 2.0 * M_PI * f * i / SR;
        double s = (sin(ph) * .6 + sin(2 * ph) * .2 + sin(ph / 2) * .2) * e * 9000.0;
        le16(b + 44 + (size_t)i * 2, (uint16_t)(int16_t)s);
    }
    *len = total;
    return b;
}
const unsigned char *audio_demo_wav(int seed, size_t *len) {
    if (!mu_ready) { mutex_init_(&mu); mu_ready = 1; }
    if (seed < 0) seed = -seed;
    seed %= MAXSEED;
    mutex_lock_(&mu);
    if (!cache[seed]) { size_t l; cache[seed] = synth(seed, &l); }
    mutex_unlock_(&mu);
    *len = 44 + (size_t)SR * SECS * 2;
    return cache[seed];
}
