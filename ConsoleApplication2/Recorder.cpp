#include "Recorder.h"

Recorder::Recorder() {
    err = Pa_Initialize();
    if (err != paNoError) {
        done(); return;
    }

    // Setup the default input device (mic in)
    inputParameters.device = Pa_GetDefaultInputDevice();
    if (inputParameters.device == paNoDevice) {
        fprintf(stderr, std::to_string(Pa_GetDeviceCount()).c_str());
        fprintf(stderr, "Error: No default input device.\n");
        done();
        return;
    }
    
    inputParameters.channelCount = 2;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    sfdata.mainInput = true;

    // Setup the default output device (speaker out)
    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
        for (int i = 0; i < 5; i++) {
            fprintf(stderr, std::to_string(i).c_str());
        }

        fprintf(stderr, "Error: No default output device.\n");
        done();
        return;
    }
    outputParameters.channelCount = 2;                     /* stereo output */
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    fprintf(stderr, (std::string(Pa_GetDeviceInfo(outputParameters.device)->name) + "\n").c_str());

    // Grab the loopback of the default output device if available
    loopbackParameters.device = paNoDevice;
    for (int i = 0; i < Pa_GetDeviceCount(); i++) {
        std::string name = Pa_GetDeviceInfo(i)->name;
        if (name.find(Pa_GetDeviceInfo(outputParameters.device)->name) != std::string::npos && name.find("[Loopback]") != std::string::npos) {
            loopbackParameters.device = i;
            break;
        }
    }
    loopbackParameters.channelCount = 2;
    loopbackParameters.sampleFormat = PA_SAMPLE_TYPE;
    loopbackParameters.suggestedLatency = Pa_GetDeviceInfo(loopbackParameters.device)->defaultLowInputLatency;
    loopbackParameters.hostApiSpecificStreamInfo = NULL;
    loopdata.mainInput = false;

    // Grab the loopback of the default output device if available
    virtualMicParameters.device = paNoDevice;
    for (int i = 0; i < Pa_GetDeviceCount(); i++) {
        std::string name = Pa_GetDeviceInfo(i)->name;
        if (name.find("CABLE Input") != std::string::npos) {
            virtualMicParameters.device = i;
            break;
        }
    }
    virtualMicParameters.channelCount = 2;
    virtualMicParameters.sampleFormat = PA_SAMPLE_TYPE;
    virtualMicParameters.suggestedLatency = Pa_GetDeviceInfo(virtualMicParameters.device)->defaultLowInputLatency;
    virtualMicParameters.hostApiSpecificStreamInfo = NULL;

    // Open and start the passthrough stream permanently
    err = Pa_OpenStream(
        &virtualMic,
        &inputParameters,
        &virtualMicParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,
        passthroughCallback,
        NULL
    );
    if (err != paNoError) {
        done(); return;
    }

    err = Pa_StartStream(virtualMic);
    fprintf(stderr, "stream started\n");
    if (err != paNoError) {
        done(); return;
    }

    // Open and start the file playback monitoring stream
    err = Pa_OpenStream(
        &filePlay,
        NULL,
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,
        playCallback,
        NULL
    );
    if (err != paNoError) {
        done(); return;
    }

    err = Pa_StartStream(filePlay);
    fprintf(stderr, "stream started\n");
    if (err != paNoError) {
        done(); return;
    }

    // Get the default path to save all files to
    WCHAR profilePath[MAX_PATH];
    HRESULT result = SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, profilePath);
    if (SUCCEEDED(result)) {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)profilePath, -1, NULL, 0, NULL, NULL);
        char* out_string = new char[size_needed + 1];
        WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)profilePath, -1, out_string, size_needed, NULL, NULL);

        appdata = out_string;
        fprintf(stderr, appdata.c_str());
        
        std::string dir = appdata + folderName;
        if (CreateDirectory(std::wstring(dir.begin(), dir.end()).c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError()) {

        }
    }
}

Recorder::~Recorder() {
    // shutdown
    done();
}

void Recorder::Record(int keycode) {
    // Create a filename based on the keycode
    std::string FILE_NAME = appdata + folderName + std::to_string(keycode) + ".wav";
    if (loopbackParameters.device != paNoDevice) {
        // We change the filename in cases where a loopback stream is available
        FILE_NAME = appdata + folderName + std::to_string(keycode) + "b.wav";
    }
    // Check if we're already recording, to avoid recording multiple files at once
    if (!recording) {
        // Set the sndfile info to be a .wav file
        sfdata.info.samplerate = SAMPLE_RATE;
        sfdata.info.channels = 2;
        sfdata.info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_32;

        // Open the file
        sfdata.file = sf_open(FILE_NAME.c_str(), SFM_WRITE, &sfdata.info);
        if (sf_error(sfdata.file) != SF_ERR_NO_ERROR) {
            fprintf(stderr, (sf_strerror(sfdata.file)));
            return;
        }
        
        // Open and start the stream to record from
        err = Pa_OpenStream(
            &stream,
            &inputParameters,
            NULL,
            SAMPLE_RATE,
            FRAMES_PER_BUFFER,
            paClipOff,
            recordCallback,
            &sfdata
        );
        if (err != paNoError) {
            done(); return;
        }

        err = Pa_StartStream(stream);
        fprintf(stderr, "stream started\n");
        if (err != paNoError) {
            done(); return;
        }

        // Start a separate, nearly identical stream to record the loopback if available
        if (loopbackParameters.device != paNoDevice) {
            FILE_NAME = appdata + folderName + std::to_string(keycode) + "a.wav";
            loopdata.info.samplerate = SAMPLE_RATE;
            loopdata.info.channels = 2;
            loopdata.info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_32;


            loopdata.file = sf_open(FILE_NAME.c_str(), SFM_WRITE, &loopdata.info);
            if (sf_error(loopdata.file) != SF_ERR_NO_ERROR) {
                fprintf(stderr, (sf_strerror(loopdata.file)));
                return;
            }

            err = Pa_OpenStream(
                &loopstream,
                &loopbackParameters,
                NULL,
                SAMPLE_RATE,
                FRAMES_PER_BUFFER,
                paClipOff,
                recordCallback,
                &loopdata
            );
            if (err != paNoError) {
                done(); return;
            }

            err = Pa_StartStream(loopstream);
            fprintf(stderr, "stream started\n");
            if (err != paNoError) {
                done(); return;
            }
        }
        recording = true;
    }
}

void Recorder::Stop(int keycode) {
    // Only call if we're recording
    if (recording) {
        // Close the file being written to
        sf_close(sfdata.file);

        // Close the stream
        err = Pa_CloseStream(stream);
        if (err != paNoError) {
            done(); return;
        }

        // Check if loopback is available
        if (loopbackParameters.device != paNoDevice) {
            // Set the temporary loop buffer to all 0s
            float* tempBuffer = new float[loopdata.info.channels * FRAMES_PER_BUFFER];
            memset(tempBuffer, 0, sizeof(float) * loopdata.info.channels * FRAMES_PER_BUFFER);
            // Write the 0 buffer for every missed frame (see more in record callback)
            for (int i = 0; i < framesMissed; i++) {
                sf_write_float(loopdata.file, tempBuffer, loopdata.info.channels * FRAMES_PER_BUFFER);
            }
            framesMissed = -1;
            // Close the file and stream
            sf_close(loopdata.file);
            delete[] tempBuffer;

            err = Pa_CloseStream(loopstream);
            if (err != paNoError) {
                done(); return;
            }

            // Load both recorded files and combine into a single output
            SndfileHandle fileA, fileB, fileOut;
            short bufferA[1024], bufferB[1024], bufferOut[1024];
            fileA = SndfileHandle(appdata + folderName + std::to_string(keycode) + "a.wav");
            fileB = SndfileHandle(appdata + folderName + std::to_string(keycode) + "b.wav");
            fileOut = SndfileHandle(
                appdata + folderName + std::to_string(keycode) + ".wav",
                SFM_WRITE,
                SF_FORMAT_WAV | SF_FORMAT_PCM_32,
                2,
                SAMPLE_RATE);
            sf_count_t countA;
            sf_count_t countB;
            

            while (1) {
                countA = fileA.read(bufferA, 1024);
                countB = fileB.read(bufferB, 1024);

                for (int i = 0; i < countB/2; i++) {
                    float toMono = *(bufferB + (2 * i)) + *(bufferB + (2 * i) + 1);
                    *(bufferB + (2 * i)) = toMono / 2;
                    *(bufferB + (2 * i) + 1) = toMono / 2;
                }

                // Combine the two buffers and write
                for (int i = 0; i < 1024; i++) {
                    bufferOut[i] = 0.5 * (bufferA[i] + bufferB[i]);
                }
                fileOut.write(bufferOut, 1024);

                // Check for EOF, and pad out the rest of the buffer to match the file lengths
                /*if (countA < 1024) {
                    memset(bufferA, 0, sizeof(float) * loopdata.info.channels * FRAMES_PER_BUFFER);
                }
                else {
                    countA = fileA.read(bufferA, 1024);
                }
                
                if (countB < 1024) {
                    memset(bufferB, 0, sizeof(float) * loopdata.info.channels * FRAMES_PER_BUFFER);
                }
                else {
                    countB = fileA.read(bufferB, 1024);
                }*/
                // Break the while loop if both files reach EOF
                if (countA < 1024) {
                    break;
                }
            }

            // Cleanup
            std::remove((appdata + folderName + std::to_string(keycode) + "a.wav").c_str());
            std::remove((appdata + folderName + std::to_string(keycode) + "b.wav").c_str());
        }

        recording = false;
    }
}

void Recorder::Play(int keycode) {
    // Set the file name and sndfile info
    std::string FILE_NAME = appdata + folderName + std::to_string(keycode) + ".wav";
    SF_INFO info;
    info.channels = 2;
    info.samplerate = SAMPLE_RATE;
    SNDFILE* file = sf_open(FILE_NAME.c_str(), SFM_READ, &info);
    // Add the file to a map containing all files
    if (loadedFiles.size() < 5) {
        loadedFiles[file] = 0;
    }
}

int Recorder::recordCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    // Create some variables
    float* in;
    callback_data_s* p_data = (callback_data_s*)userData;
    sf_count_t num_written;
    // Assign params to variables
    in = (float*)inputBuffer;
    p_data == (callback_data_s*)userData;
    // Check if this call is from the main input stream
    if (!p_data->mainInput) {
        /* Since this is not the main input stream, and rather the loopback, it is possible for the callback to not be called
        *  when no audio is playing on the system. This causes desync issues between the input and loopback channels. 
        *  tempBuffer is always an empty buffer, used to pad out the file to re-sync the two files.                             */
        float* tempBuffer = new float[framesPerBuffer * p_data->info.channels];
        memset(tempBuffer, 0, sizeof(float) * framesPerBuffer * p_data->info.channels);
        // For every frame missed in the loopback stream, we pad the output with empty buffers
        for (int i = 0; i < framesMissed; i++) {
            sf_write_float(p_data->file, tempBuffer, framesPerBuffer * p_data->info.channels);
        }
        // Write the actual loopback audio
        num_written = sf_write_float(p_data->file, in, framesPerBuffer * p_data->info.channels);
        // Set frames missed to -2, since it is possible for the main input stream to be called twice in the time it takes for the loopback stream to be called once.
        // This avoids distortion in the audio.
        framesMissed = -2;
        delete[] tempBuffer;
    }
    else {
        // Write the data from the main input stream to the output file.
        num_written = sf_write_float(p_data->file, in, framesPerBuffer * p_data->info.channels);
        sf_write_sync(p_data->file);
        // Increment the frames missed, such that if too many frames are missed, the loopback stream writes empty buffers
        framesMissed++;
    }

    return paContinue;
}

bool Recorder::CanPlay(int keycode) {
    std::string FILE_NAME = appdata + folderName + std::to_string(keycode) + ".wav";
    struct stat buffer;
    return (stat(FILE_NAME.c_str(), &buffer) == 0);
}

int Recorder::playCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* data)
{
    // Create some variables
    float* out;
    float* read = new float[framesPerBuffer * 2];
    sf_count_t num_read;
    // Assign params to variables
    out = (float*)outputBuffer;
    // Clear the output buffer first
    memset(out, 0, sizeof(float) * framesPerBuffer * 2);
    // Iterate through every file
    for (auto const& [file, timeAlive] : loadedFiles) {
        // Files are available if timeAlive > -1
        if (timeAlive != -1) {
            // Read the frames to a temporary read buffer
            num_read = sf_read_float(file, read, framesPerBuffer * 2);
            // Add the read buffer to the aggregate read buffer
            for (int i = 0; i < num_read; i++) {
                *(outputReadBuffer + i) += *(read + i);
            }
            // Close the file if EOF is reached, or if the clip is too long
            if (num_read < framesPerBuffer || timeAlive > 5 * SAMPLE_RATE) {
                sf_close(file);
                loadedFiles[file] = -1;
            }
            else {
                // Keep track of how much of the file has been read
                loadedFiles[file] += framesPerBuffer;
            }
        }
    }
    // Copy the aggregate read buffer to the output buffer
    memcpy(out, outputReadBuffer, sizeof(float) * framesPerBuffer * 2);
    // Clear the aggregate read buffer, so that the next callback can use it
    memset(outputReadBuffer, 0, sizeof(float) * framesPerBuffer * 2);
    // Unassign the memory allocation for the read buffer
    delete [] read;

    return paContinue;
}

int Recorder::passthroughCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* data)
{
    // Declare some variables
    float* in;
    float* out;
    // Assign params to variables
    in = (float*)inputBuffer;
    out = (float*)outputBuffer;
    // Copy the input buffer directly to the output buffer
    memcpy(out, in, sizeof(float) * framesPerBuffer * 2);
    // Add the aggregate read buffer to the output if an input is available
    if (in) {
        for (int i = 0; i < framesPerBuffer * 2; i++) {
            *(out + i) += *(outputReadBuffer + i);
        }
    }

    return paContinue;
}

int Recorder::done() {
    // Called if an error occurs, and shuts down portaudio
    Pa_Terminate();
    printf("Done called. Terminated.\n"); fflush(stdout);
    if (err != paNoError)
    {
        fprintf(stderr, "An error occured while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", err);
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
        err = 1;          /* Always return 0 or 1, but no other return codes. */
    }
    Pa_CloseStream(virtualMic);
    Pa_CloseStream(filePlay);
    return err;
}

float* Recorder::outputReadBuffer = new float[2 * FRAMES_PER_BUFFER];
int Recorder::framesMissed = 0;
std::map<SNDFILE*, int> Recorder::loadedFiles;