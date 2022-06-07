// ConsoleApplication2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
//
#include "Header.h"


Button AudioManager::NewKey(int keycode) {
	Button button;
	while (1) {
		if (key_map.find(keycode) != key_map.end()) {
			keycode++;
		}
		else {
			button.keycode = keycode;
			break;
		}
	}
	key_map[keycode] = button;
	return button;
}

/* Callback for the keyboard hook */
LRESULT CALLBACK AudioManager::KeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
		if (key_map.find(p->vkCode) != key_map.end()) {
			Button &button = key_map[p->vkCode];
			int passedTime = 0;
			switch (wParam) {
			case WM_KEYDOWN:
				if (!recording) {
					// Check if button has a recorded file, otherwise, start recording
					if (recorder->CanPlay(p->vkCode)) {
						std::cout << p->vkCode << " played\n";
						recorder->Play(p->vkCode);
					}
					// if not, start recording
					else {
						std::cout << p->vkCode << " recording\n";
						recorder->Record(p->vkCode);
						recording = true;
					}
				}
				break;
			case WM_KEYUP:
				// Stop recording when button is released
				passedTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() - button.time_pressed;
				if (recording) {
					std::cout << p->vkCode << " recording finished\n";
                    recorder->Stop(p->vkCode);
                    recording = false;
				}
				button.time_pressed = 0;
				break;
			case WM_SYSKEYDOWN:
				// Secondary record button - used to record over already existing clips
				if (!recording) {
					std::cout << p->vkCode << " recording\n";
					recorder->Record(p->vkCode);
					recording = true;
				}
				break;
			case WM_SYSKEYUP:
				// Stop recording when button is released
				if (recording) {
					std::cout << p->vkCode << " recording finished\n";
					recorder->Stop(p->vkCode);
					recording = false;
				}
				break;
			default:
				break;
			}
		}
	}
	return CallNextHookEx(0, nCode, wParam, lParam);
}

int AudioManager::Start()
{
	for (int i = 0x31; i < 0x3A; i++) {
		Button b = NewKey(i);
	}
	hhookKbdListener = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardEvent, 0, 0);

	MSG msg;
	while (!GetMessage(&msg, NULL, NULL, NULL)) {    //this while loop keeps the hook
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	UnhookWindowsHookEx(hhookKbdListener);
	return 0;
}

std::map<int, Button> AudioManager::key_map;
bool AudioManager::recording = false;
Recorder* AudioManager::recorder = new Recorder();



int main() {
	AudioManager am;
	am.Start();
	return 0;
}




// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
