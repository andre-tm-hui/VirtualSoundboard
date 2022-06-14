# VirtualSoundboard
A Virtual Soundboard built in C++ for Windows, capable of recording input and system audio, and outputting to a virtual cable (see https://vb-audio.com/Cable/).

It uses a keyboard hook to register key events, and assigns audio events to a set of given keys. Audio events are handled using the portaudio (https://github.com/PortAudio/portaudio/) and libsndfile (https://github.com/libsndfile/libsndfile/) libraries.

Usage:
Build in Visual Studio 22 and run. Currently, hotkeys are assigned to numbers. This can be changed by changing line 81 in BackgroundManager.cpp. For example, to use the NUMPAD keys:

      for (int i = 0x60; i < 0x6A; i++) {
      
To record, press and hold a key, and release to stop recording. If there is audio recorded to the key already, press Alt+key to start re-recording.
To play, once audio has been recorded to a key, press the key to playback the audio.

TODO:
  - GUI elements:
      - Selecting input/output/virtual audio devices
      - Adding/removing/setting hotkeys
      - Minimize to hidden/background
  - Audio Effects?:
      - Assign audio effects to hotkeys
      - Hotkeys toggle the audio effect - applies to input device only
      - Effects may include pitch, distortion etc.
  - QOL/Features:
      - On/off hotkey
      - Customize channels to record per key
