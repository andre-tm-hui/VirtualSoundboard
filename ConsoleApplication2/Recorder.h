#pragma once
#include "portaudio.h"
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>
#include <ShlObj.h>
#include <filesystem>
#include <sndfile.h>
#include <sndfile.hh>
#include <map>
#include <sys/stat.h>

#define SAMPLE_RATE  (48000)
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

class Recorder {
public:
    Recorder();
    ~Recorder();
    void Record(int keycode);
    void Stop(int keycode);
    void Play(int keycode);
    bool CanPlay(int keycode);

    int sampleRate = 48000;

private:
    typedef struct {
        SNDFILE* file;
        SF_INFO info;
        bool mainInput;
    } callback_data_s;

    PaStreamParameters  inputParameters,
                        loopbackParameters,
                        outputParameters,
                        virtualMicParameters;
    PaStream* stream;
    PaStream* loopstream;
    PaStream* filePlay;
    PaStream* virtualMic;
    PaError             err = paNoError;
    callback_data_s     sfdata = { 0 };
    callback_data_s     loopdata = { 0 };
    bool                passthroughTrue = true;
    bool                passthroughFalse = true;
    unsigned            delayCntr;
    unsigned            numSamples;
    unsigned            numBytes;
    bool                recording;
    errno_t             ferr;
    std::string         appdata;
    std::string         folderName = "/Virtual Soundboard and Mixer/";
    static int          framesMissed;
    static std::map<SNDFILE*, int> loadedFiles;
    static float*       outputReadBuffer;

    static int recordCallback(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);

    static int playCallback(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);

    static int passthroughCallback(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);

    int done();
};