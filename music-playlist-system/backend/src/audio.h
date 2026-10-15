/* audio.h - builds a playable WAV in memory for the built-in demo songs */
#ifndef AUDIO_H
#define AUDIO_H
#include <stddef.h>
const unsigned char *audio_demo_wav(int seed, size_t *len);   /* cached, never free */
#endif
