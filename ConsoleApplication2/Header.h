#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "portaudio.h"
#include "pa_ringbuffer.h"
#include "pa_util.h"
#include <Windows.h>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <string>
#include "Recorder.h"

#define SAMPLE_RATE  (44100)
#define SAMPLE_SIZE 4
#define FRAMES_PER_BUFFER (512)
#define NUM_SECONDS     (10)
#define NUM_CHANNELS    (2)
#define NUM_WRITES_PER_BUFFER   (4)
/* #define DITHER_FLAG     (paDitherOff) */
#define DITHER_FLAG     (0) /**/

#if 1
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#elif 1
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE  paInt8
typedef char SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#else
#define PA_SAMPLE_TYPE  paUInt8
typedef unsigned char SAMPLE;
#define SAMPLE_SILENCE  (128)
#define PRINTF_S_FORMAT "%d"
#endif

class Button {
public:
	int keycode = 0x60;
	int time_pressed = 0;
};

class AudioManager {
public:
    static std::map<int, Button> key_map;

    static bool recording;

    HHOOK hhookKbdListener;

    Button NewKey(int keycode = 0x60);

    LRESULT static CALLBACK KeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam);

    static int passthroughAudio();

    int Start();

private:
    static Recorder* recorder;
};