#include "ofxWinTouchHook.h"

// static touch events
ofEvent<ofTouchEventArgs>	ofxWinTouchHook::touchDown;
ofEvent<ofTouchEventArgs>	ofxWinTouchHook::touchUp;
ofEvent<ofTouchEventArgs>	ofxWinTouchHook::touchMoved;

#ifdef TARGET_WIN32
#include <Windows.h>

// variable to store the HANDLE to the hook. Don't declare it anywhere else then globally
// or you will get problems since every function uses this variable.
HHOOK _hook;

// This struct contains the data received by the hook callback. As you see in the callback function
// it contains the thing you will need: vkCode = virtual key code.
KBDLLHOOKSTRUCT kbdStruct;

// This is the callback function. Consider it the event that is raised when, in this case, 
// a key is pressed.
LRESULT __stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{

		// this doesnt fire with WH_CALLWNDPROC, as it's before the message is processed
		// http://cinematography-project-m2-charles.googlecode.com/svn/trunk/Touch2Tuio/TouchHook/TouchHook.cpp
		LPMSLLHOOKSTRUCT msg = (LPMSLLHOOKSTRUCT)lParam;
		if (msg->flags & LLMHF_INJECTED) // block injected events (in most cases generated by touches)
		{
			printf("\nBlocked injected (touch) mouse event %d", wParam);
			/*sprintf_s(s_buf, "Blocked injected (touch) mouse event\n");
			WriteConsole(s_out, s_buf, strlen(s_buf), &s_ccount, 0);
			//return CallNextHookEx(0, nCode, wParam, lParam);*/
			return 1;
		}

		PCWPSTRUCT pStruct = (PCWPSTRUCT)lParam;
		UINT message = pStruct->message;// ((PCWPSTRUCT)lParam)->message;
		
		// WM_POINTER events dont fire because GLFW is compiled for windows xp, instead of win 8 (WINVER 0x0602)
		if (message == WM_POINTERDOWN) {
			printf("\nWM_POINTER down %d", wParam);
		}
		else if (message == WM_POINTERUPDATE) {
			printf("\nWM_POINTER move %d", wParam);
		}
		else if (message == WM_POINTERUP) {
			printf("\nWM_POINTER up %d", wParam);
		} 
		else if (message == WM_TOUCH) {

			POINT p;
			UINT numInputs = (UINT)pStruct->wParam; // Number of actual per-contact messages
			TOUCHINPUT* touchInput = new TOUCHINPUT[numInputs]; // Allocate the storage for the parameters of the per-contact messages

			// Unpack message parameters into the array of TOUCHINPUT structures, each
			// representing a message for one single contact.
			HTOUCHINPUT touchLParam = (HTOUCHINPUT)pStruct->lParam;
			if (GetTouchInputInfo(touchLParam, numInputs, touchInput, sizeof(TOUCHINPUT)))
			{
				// For each contact, dispatch the message to the appropriate message
				// handler.
				for (unsigned int i = 0; i<numInputs; ++i)
				{
					// Capture x,y of touch event before iteration through objects
					p.x = TOUCH_COORD_TO_PIXEL(touchInput[i].x);// (touchInput[i].x) / 100;
					p.y = TOUCH_COORD_TO_PIXEL(touchInput[i].y);//(touchInput[i].y) / 100;

					// native touch to screen conversion, alt use ofGetWindowPosition offsets
					ScreenToClient(pStruct->hwnd, &p);

					// OF touch event (only using x,y,and id)
					// only interested in basic down, move, and up events
					ofTouchEventArgs touchEventArgs;
					touchEventArgs.x = p.x;
					touchEventArgs.y = p.y;
					touchEventArgs.id = touchInput[i].dwID;

					if (touchInput[i].dwFlags & TOUCHEVENTF_DOWN)
					{
						//printf("\nDOWN %ld %ld", p.x, p.y);						
						//ofNotifyEvent(ofEvents().touchDown, touchEventArgs);
						ofNotifyEvent(ofxWinTouchHook::touchDown, touchEventArgs);
					}
					else if (touchInput[i].dwFlags & TOUCHEVENTF_MOVE)
					{
						//printf("\nMOVE %ld %ld", p.x, p.y);
						//ofNotifyEvent(ofEvents().touchMoved, touchEventArgs);
						ofNotifyEvent(ofxWinTouchHook::touchMoved, touchEventArgs);
					}
					else if (touchInput[i].dwFlags & TOUCHEVENTF_UP)
					{
						//printf("\nUP %ld %ld", p.x, p.y);
						//ofNotifyEvent(ofEvents().touchUp, touchEventArgs);
						ofNotifyEvent(ofxWinTouchHook::touchUp, touchEventArgs);
					} 

				}
			}
			CloseTouchInputHandle(touchLParam);
			delete[] touchInput;
			
		}
		
	}

	// call the next hook in the hook chain. This is nessecary or your hook chain will break and the hook stops
	return CallNextHookEx(_hook, nCode, wParam, lParam);
}

#endif



//--------------------------------------------------------------
void ofxWinTouchHook::EnableTouch() {

	#ifdef TARGET_WIN32

	// WM_pointer not working
	EnableMouseInPointer(FALSE);

	// for WM_TOUCH - win7+8
	if (RegisterTouchWindow(ofGetWin32Window(), TWF_WANTPALM) == 0) {
		ofLogError() << "Failed to register touch";
	}
	else {
		// Set the hook and set it to use the callback function above
		// WH_KEYBOARD_LL means it will set a low level keyboard hook. More information about it at MSDN.
		// The last 2 parameters are NULL, 0 because the callback function is in the same thread and window as the
		// function that sets and releases the hook. If you create a hack you will not need the callback function 
		// in another place then your own code file anyway. Read more about it at MSDN.
		//if (!(_hook = SetWindowsHookEx(WH_MOUSE_LL, HookCallback, NULL, 0)))
		//hook = SetWindowsHookEx(WH_CALLWNDPROC, LaunchListener, thisModule, threadID);
		// catch before window: http://stackoverflow.com/questions/21069643/is-it-possible-to-remove-touch-messages-wm-pointerdown-etc-that-an-applicatio
		if (!(_hook = SetWindowsHookEx(WH_CALLWNDPROC, HookCallback, GetModuleHandle(NULL), GetCurrentThreadId()))) {
			//MessageBox(NULL, "Failed to install hook!", "Error", MB_ICONERROR);
			ofLogError() << "Failed to install hook!";
		}
	}

	#else
	ofLog() << "Not a windows target";
	#endif
}

void ofxWinTouchHook::DisableTouch() {

	#ifdef TARGET_WIN32
	UnhookWindowsHookEx(_hook);
	#endif
}
