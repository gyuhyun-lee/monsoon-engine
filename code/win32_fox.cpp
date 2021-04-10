/* 
 
Author : Gyuhyeon, Lee 
Email : weanother@gmail.com 
Copyright by GyuHyeon, Lee. All Rights Reserved. 
 
*/

// TODO : Get rid of stdio.h?
#include <stdio.h> 
#include <windows.h>
#include "fox.h"
#include "win32_fox.h"
#include <xinput.h>

global_variable b32 isGameRunning;
global_variable win32_screen_buffer globalScreenBuffer;
LARGE_INTEGER performanceFrequency;

#define XINPUT_GET_STATE(name) DWORD name(DWORD userID, XINPUT_STATE *state)
typedef XINPUT_GET_STATE(xinput_get_state);

XINPUT_GET_STATE(xinputGetStateNull)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}

#define XINPUT_SET_STATE(name) DWORD name(DWORD userID, XINPUT_VIBRATION *vibration)
typedef XINPUT_SET_STATE(xinput_set_state);
XINPUT_SET_STATE(xinputSetStateNull)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
struct win32_xinput
{
	xinput_get_state *xinputGetState;
	xinput_set_state *xinputSetState;
};

internal void
Win32LoadXInput(win32_xinput *xinput)
{
	HMODULE xinputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!xinputLibrary)
    {
        xinputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
    
    if(!xinputLibrary)
    {
        // TODO(casey): Diagnostic
        xinputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if(xinputLibrary)
    {
    	xinput->xinputGetState = (xinput_get_state *)GetProcAddress(xinputLibrary, "XInputGetState");
    	xinput->xinputSetState = (xinput_set_state *)GetProcAddress(xinputLibrary, "XInputSetState");
    }

    if(!xinput->xinputGetState)
    {
    	xinput->xinputGetState = xinputGetStateNull;
    	xinput->xinputSetState = xinputSetStateNull;
    }
}


internal win32_client_rect
Win32GetClientRect(HWND windowHandle)
{
	win32_client_rect result = {};

	RECT clientRect = {};
	GetClientRect(windowHandle, &clientRect);

	result.width = clientRect.right - clientRect.left;
	result.height = clientRect.bottom - clientRect.top;

	return result;
}

internal FILETIME
Win32GetLastAccessTime(char *path)
{
	FILETIME result = {};
	WIN32_FILE_ATTRIBUTE_DATA attribute;

	if(GetFileAttributesExA(path, GetFileExInfoStandard, &attribute))
	{
		result = attribute.ftLastWriteTime;
	}	

	return result;
}


internal void
Win32LoadGameCodes(win32_game_code *win32GameCode, char *path)
{
	if(win32GameCode->module)
	{
		FreeModule(win32GameCode->module);
	}

	win32GameCode->module = LoadLibrary(path);

	if(win32GameCode->module)
	{
		win32GameCode->GameFixedUpdate = (game_fixed_update *)GetProcAddress(win32GameCode->module, "GameFixedUpdate");
		win32GameCode->GameNonfixedUpdate = (game_nonfixed_update *)GetProcAddress(win32GameCode->module, "GameNonfixedUpdate");
		win32GameCode->lastAccessFileTime = Win32GetLastAccessTime(path);
	}
	else
	{
		win32GameCode->GameFixedUpdate = 0;
		win32GameCode->GameNonfixedUpdate = 0;
	}
}

internal void
Win32ResizeBuffer(win32_screen_buffer *screenBuffer, i32 screenWidth, i32 screenHeight)
{
	if(screenBuffer->memory)
	{
		VirtualFree(screenBuffer->memory, screenBuffer->size, MEM_RELEASE);
	}

	screenBuffer->bitmapInfo.bmiHeader.biSize = sizeof(screenBuffer->bitmapInfo);
	screenBuffer->bitmapInfo.bmiHeader.biWidth = screenWidth;
	screenBuffer->bitmapInfo.bmiHeader.biHeight = screenHeight;
	screenBuffer->bitmapInfo.bmiHeader.biPlanes = 1;
	screenBuffer->bitmapInfo.bmiHeader.biBitCount = 32; 
	screenBuffer->bitmapInfo.bmiHeader.biCompression = BI_RGB; // Uncompressed
	screenBuffer->bitmapInfo.bmiHeader.biSizeImage = 0;

	screenBuffer->width = screenWidth;
	screenBuffer->height = screenHeight;
	screenBuffer->pitch = screenBuffer->width * Bytes_Per_Pixel;
	screenBuffer->size = screenBuffer->pitch * screenBuffer->height;
	screenBuffer->memory = VirtualAlloc(0, screenBuffer->size, MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32DrawBuffer(HWND windowHandle, win32_screen_buffer *buffer)
{
	HDC deviceContext = GetDC(windowHandle);

	if(deviceContext)
	{
		win32_client_rect clientRect = Win32GetClientRect(windowHandle);

		StretchDIBits(deviceContext,
					0,
					0,
					clientRect.width,
					clientRect.height,
					0, 0,
					buffer->width,
					buffer->height,
					buffer->memory,
					&buffer->bitmapInfo,
					DIB_RGB_COLORS,
					SRCCOPY);	

		ReleaseDC(windowHandle, deviceContext);
	}
}

internal LARGE_INTEGER
Win32GetPerformanceCount(void)
{
	LARGE_INTEGER performanceCount;
	QueryPerformanceCounter(&performanceCount);

	return performanceCount;
}

internal r32
Win32GetSecondsElasped(LARGE_INTEGER start, LARGE_INTEGER end)
{
	return (r32)(end.QuadPart - start.QuadPart) / (r32)performanceFrequency.QuadPart;
}

LRESULT CALLBACK 
MainWindowProcedure(HWND windowHandle,
				UINT message,
				WPARAM wParam,
				LPARAM lParam)
{
	LRESULT result = {};

	switch(message)
	{
		case WM_CREATE:
		{
		}break;

		case WM_PAINT:
		{
			HDC deviceContext = GetDC(windowHandle);

			if(deviceContext)
			{
				RECT clientRect = {};
				GetClientRect(windowHandle, &clientRect);

				PAINTSTRUCT paintStruct = {};
				paintStruct.hdc = deviceContext;
				// TODO : Find out whether this matters
				paintStruct.fErase = false;
				paintStruct.rcPaint = clientRect;

				BeginPaint(windowHandle, &paintStruct);
				EndPaint(windowHandle, &paintStruct);

				ReleaseDC(windowHandle, deviceContext);
			}
		}break;
		case WM_DESTROY:
		case WM_QUIT:
		{
			isGameRunning = false;
		}break;

		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		{
			InvalidCodePath;
		}break;

		default:
		{
			result = DefWindowProc(windowHandle, message, wParam, lParam);
		};
	}

	return result;
}

internal void
ProcessGameControllerButton(game_controller *controller, b32 isDown, b32 wasDown, u8 keyCode)
{
	game_button *button = controller->buttons + keyCode;
	button->isDown = isDown;
	button->wasDown = wasDown;
}

enum input_record_status
{
	input_record_status_idle,
	input_record_status_recording,
	input_record_status_playing,
};
struct win32_input_record
{
	HANDLE handle;

	input_record_status status;

	size_t totalSize;
	size_t currentIndex;
};
internal void
Win32BeginInputRecording(win32_input_record *inputRecord, char *filePath)
{
	inputRecord->handle = CreateFileA(filePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	inputRecord->status = input_record_status_recording;
	inputRecord->currentIndex = 0;
	inputRecord->totalSize = 0;
}
internal void
Win32RecordInput(win32_input_record *inputRecord, game_input *gameInput)
{
	if(WriteFile(inputRecord->handle, gameInput, (DWORD)sizeof(game_input), 0, 0))
	{
		inputRecord->totalSize += sizeof(game_input);
	}
}
internal void
Win32EndInputRecording(win32_input_record *inputRecord)
{
	inputRecord->status = input_record_status_idle;
	CloseHandle(inputRecord->handle);
	inputRecord->handle = 0;
}

internal void
Win32BeginInputPlaying(win32_input_record *inputRecord, char *filePath)
{
	inputRecord->handle = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	inputRecord->status = input_record_status_playing;
	inputRecord->currentIndex = 0;
}
internal void
Win32PlayInput(win32_input_record *inputRecord, game_input *gameInput)
{
	ReadFile(inputRecord->handle, (LPVOID)gameInput, (DWORD)sizeof(game_input), 0, 0);
	inputRecord->currentIndex += sizeof(game_input);

	if(inputRecord->currentIndex == inputRecord->totalSize)
	{
		inputRecord->currentIndex = 0;
		SetFilePointer(inputRecord->handle, 0, 0, FILE_BEGIN);
	}
}
internal void
Win32EndInputPlaying(win32_input_record *inputRecord)
{
	CloseHandle(inputRecord->handle);
	inputRecord->status = input_record_status_idle;
	inputRecord->currentIndex = 0;
	inputRecord->handle = 0;
}

// IMPORTANT : Unlike other functions inside the main loop, this function is NOT GUARENTEED to be called once per loop.
LRESULT CALLBACK
KeyboardMouseInputProcedure(HWND windowHandle, game_controller *newController, win32_input_record *inputRecord)
{
	LRESULT result = {};

	MSG message;
	while(PeekMessageA(&message, windowHandle, 0, 0, PM_REMOVE))
	{
		switch(message.message)
		{
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				WPARAM keyCode = message.wParam;

				b32 wasDown = (((message.lParam >> 30) & 1) == 1);
				b32 isDown = ((message.lParam >> 31) == 0);

				if(wasDown != isDown)
				{
					if(keyCode == 'W')
					{
						// TODO : Because this is keyboard event, 
						// this only occurs once -> which makes it impossible
						// to move the player constantly
                		newController->leftStick.y = 1.0f;
					}
					else if(keyCode == 'A')
					{
                		newController->leftStick.x = -1.0f;
					}
					else if(keyCode == 'S')
					{
                		newController->leftStick.y = -1.0f;
					}
					else if(keyCode == 'D')
					{
                		newController->leftStick.x = 1.0f;
					}
					else if(keyCode == 'j')
					{
					}
					else if(keyCode == 'i')
					{
						if(inputRecord->status == input_record_status_idle)
						{
							Win32BeginInputRecording(inputRecord, "../../build/win32_input_record.wir");
						}
					}
					else if(keyCode == 'p')
					{
						if(inputRecord->status == input_record_status_idle)
						{
							Win32BeginInputPlaying(inputRecord, "../../build/win32_input_record.wir");
						}
					}
					else if(keyCode == 'o')
					{
						if(inputRecord->status == input_record_status_recording)
						{
							Win32EndInputRecording(inputRecord);
						}
						else if(inputRecord->status == input_record_status_playing)
						{
							Win32EndInputPlaying(inputRecord);
							*newController = {}; // For now, always remember to do this as the last input might be still inside the input.
						}
					}
					
					// else if(keyCode == )
					// {
					// }
					// else if(keyCode == )
					// {
					// }
					// else if(keyCode == )
					// {
					// }
					// else if(keyCode == )
					// {
					// }
					// else if(keyCode == )
					// {
					// }
					// else if(keyCode == )
					// {
					// }
					// else if(keyCode == )
					// {
					// }
				}

				if (keyCode == VK_ESCAPE)
				{
					isGameRunning = false;
				}
			}break;

			default:
			{
				TranslateMessage(&message);
				DispatchMessageA(&message);
			};
		}
	}

	return result;
}

DEBUG_PLATFORM_READ_FILE(Win32ReadFile)
{
	void *result = 0;
	HANDLE fileHandle = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if(fileHandle)
	{
		LARGE_INTEGER fileSize;
		if(GetFileSizeEx(fileHandle, &fileSize))
		{
			// void *buffer = malloc(fileSize.QuadPart);
			void *buffer = VirtualAlloc(0, fileSize.QuadPart, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
			
			if(ReadFile(fileHandle, (LPVOID)buffer, (DWORD)fileSize.QuadPart, 0, 0))
			{
				result = buffer;
			}
			else
			{
				free(buffer);
			}
		}

		CloseHandle(fileHandle);
	}

	return result;
}

// TODO : Win32WriteFile function is not tested as this function is not used by anyone. Test this.
DEBUG_PLATFORM_WRITE_FILE(Win32WriteFile)
{
	b32 result = 0;

	HANDLE fileHandle = CreateFileA(filePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

	if(fileHandle)
	{
		if(WriteFile(fileHandle, source, (DWORD)sourceSize, 0, 0))
		{
			result = true;
		}
	}

	return result;
}

DEBUG_PLATFORM_FREE_FILE(Win32FreeFile)
{
	free(pointer);
}

int CALLBACK 
WinMain(HINSTANCE instanceHandle,
		HINSTANCE prevInstanceHandle, 
		LPSTR cmd, 
		int cmdShowMethod)
{
	win32_xinput win32Xinput = {};
	Win32LoadXInput(&win32Xinput);
	// TODO : Any way to change monitor refresh rate?
	// Or let the user change this value?
	i32 monitorRefreshRate = 60;	
	i32 targetFramePerSec = monitorRefreshRate/2;
	r32 targetSecondsPerFrame = 1.0f/targetFramePerSec;

	QueryPerformanceFrequency(&performanceFrequency);
	b32 isSleepGranular = (timeBeginPeriod(1) == TIMERR_NOERROR);

	char foxLibraryFullPath[MAX_PATH] = "../../build/fox.dll";
	char foxUsingLibraryFullPath[MAX_PATH] = "../../build/fox_using.dll";

	i32 screenWidth = 960;
	i32 screenHeight = 540;
	Win32ResizeBuffer(&globalScreenBuffer, screenWidth, screenHeight);

    WNDCLASSEXA windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = MainWindowProcedure;
    windowClass.hInstance = instanceHandle;
    windowClass.lpszClassName = "FoxWindowClass";
    windowClass.hCursor = LoadCursor(0, IDC_ARROW);
	if(RegisterClassExA(&windowClass))
	{
		HWND windowHandle = CreateWindowExA(0,
										windowClass.lpszClassName,
								  		"orbits",
							            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
							            CW_USEDEFAULT, CW_USEDEFAULT,
								  		screenWidth, screenHeight,
										0, 0,
										instanceHandle,
										0);
		if(windowHandle)
		{
			game_input gameInputs[2] = {};
			game_input *newInput = gameInputs;
			game_input *oldInput = gameInputs + 1;

			game_memory gameMemory = {};
			gameMemory.permanentStorageSize = Megabytes(64);
			gameMemory.transientStorageSize = Megabytes(128);
			gameMemory.permanentStorage = VirtualAlloc(0, (size_t)gameMemory.permanentStorageSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
			gameMemory.transientStorage = VirtualAlloc(0, (size_t)gameMemory.transientStorageSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

			platform_api platformAPI = {};
			platformAPI.PlatformReadFile = Win32ReadFile;
			platformAPI.PlatformWriteFile = Win32WriteFile;
			platformAPI.PlatformFreeFile = Win32FreeFile;

			game_time_manager gameTimeManager = {};
			gameTimeManager.fixeddt = targetSecondsPerFrame;

			// NOTE : Copy the original library so that we can modify(compile) the original one as much as we want
			CopyFile(foxLibraryFullPath, foxUsingLibraryFullPath, false);

			win32_game_code win32GameCode = {};
			Win32LoadGameCodes(&win32GameCode, foxUsingLibraryFullPath);

			win32_input_record inputRecord = {};

			if(gameMemory.permanentStorage && gameMemory.transientStorage &&
				win32GameCode.GameFixedUpdate && win32GameCode.GameNonfixedUpdate)
			{
				isGameRunning = true;
			}

			LARGE_INTEGER lastPerformanceCount = Win32GetPerformanceCount();
			while(isGameRunning)
			{
				// NOTE : Hot loading code.
				{
					FILETIME foxFileTime = Win32GetLastAccessTime(foxLibraryFullPath);
					if(CompareFileTime(&foxFileTime, &win32GameCode.lastAccessFileTime) == 1)
					{
						// NOTE : We have to do this first because otherwise, the copyfile will always fail
						FreeModule(win32GameCode.module);
						CopyFile(foxLibraryFullPath, foxUsingLibraryFullPath, false);
						Win32LoadGameCodes(&win32GameCode, foxUsingLibraryFullPath);
					}
				}

				win32_client_rect clientRect = Win32GetClientRect(windowHandle);
				v2 clientRectHalfDim = V2(clientRect.width/2.0f, clientRect.height/2.0f);
				POINT cursorP = {};
				GetCursorPos(&cursorP);
				ScreenToClient(windowHandle, &cursorP); // NOTE : Based on upper left corner
				newInput->controllers[0].mouseP = 
					V2(((r32)cursorP.x - clientRectHalfDim.x)/clientRectHalfDim.x, ((r32)-cursorP.y + clientRectHalfDim.y)/clientRectHalfDim.y);

				char buffer[256];
                sprintf_s(buffer, "%0.05f, %0.05f\n", newInput->controllers[0].mouseP.x, newInput->controllers[0].mouseP.y);
                OutputDebugStringA(buffer);	

                XINPUT_STATE xinputState;
                if(win32Xinput.xinputGetState(0, &xinputState) == ERROR_SUCCESS)
                {
                	XINPUT_GAMEPAD *gamepad = &xinputState.Gamepad;
                	game_controller *controller = newInput->controllers + 0;
                	if(gamepad->sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                	{
                		controller->leftStick.x = gamepad->sThumbLX/32767.0f;
                	}
                	else if(gamepad->sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                	{
                		controller->leftStick.x = gamepad->sThumbLX/32768.0f;
                	}
                	if(gamepad->sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                	{
                		controller->leftStick.y = gamepad->sThumbLY/32767.0f;
                	}
                	else if(gamepad->sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                	{
                		controller->leftStick.y = gamepad->sThumbLY/32768.0f;
                	}
                	
                	if(gamepad->sThumbRX > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
                	{
                		controller->rightStick.x = gamepad->sThumbRX/32767.0f;
                	}
                	else if(gamepad->sThumbRX < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
                	{
                		controller->rightStick.x = gamepad->sThumbRX/32768.0f; 
                	}
                	if(gamepad->sThumbRY > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
                	{
                		controller->rightStick.y = gamepad->sThumbRY/32767.0f;
                	}
                	else if(gamepad->sThumbRY < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
                	{
                		controller->rightStick.y = gamepad->sThumbRY/32768.0f;
                	}

                	if(gamepad->bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
                	{
	                	controller->leftTrigger= gamepad->bLeftTrigger/255.0f;
                	}
                	if(gamepad->bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
                	{
	                	controller->rightTrigger= gamepad->bRightTrigger/255.0f;
                	}

                	if(gamepad->wButtons & XINPUT_GAMEPAD_B)
                	{
                		ProcessGameControllerButton(controller, 1, 0, key_action_right);
                	}
                }

				// NOTE : Process keyboard based messages first!
				KeyboardMouseInputProcedure(windowHandle, 
										newInput->controllers + 0, &inputRecord);

				if(inputRecord.status == input_record_status_recording)
				{
					Win32RecordInput(&inputRecord, newInput);
				}
				else if(inputRecord.status == input_record_status_playing)
				{
					Win32PlayInput(&inputRecord, newInput);
				}

				game_screen_buffer gameBuffer = {};
				gameBuffer.width = globalScreenBuffer.width;
				gameBuffer.height = globalScreenBuffer.height;
				gameBuffer.center.x = gameBuffer.width/2.0f;
				gameBuffer.center.y = gameBuffer.height/2.0f;
				gameBuffer.pitch = globalScreenBuffer.pitch;
				gameBuffer.memory = globalScreenBuffer.memory;


				win32GameCode.GameFixedUpdate(&gameBuffer, newInput, oldInput, &gameMemory, &platformAPI, &gameTimeManager);
				win32GameCode.GameNonfixedUpdate(&gameBuffer, newInput, oldInput, &gameMemory, &platformAPI, &gameTimeManager);

				if(gameTimeManager.effectTime > 0.0f)
				{
					gameTimeManager.effectTime -= targetSecondsPerFrame;
				}
				else
				{
					gameTimeManager.dtScale = 1.0f;
				}

				LARGE_INTEGER currentPerformanceCount = Win32GetPerformanceCount();
				r32 elapsedSec = Win32GetSecondsElasped(lastPerformanceCount, currentPerformanceCount);

				if(elapsedSec < targetSecondsPerFrame)
				{
					if(isSleepGranular)
					{
						DWORD sleepMS = (DWORD)(1000.0f*(targetSecondsPerFrame-elapsedSec));
						if(sleepMS > 0)
						{
							Sleep(sleepMS);
						}
					}

					// NOTE : Just in case!
					r32 testElapsedSec = Win32GetSecondsElasped(lastPerformanceCount, Win32GetPerformanceCount());
					while(testElapsedSec < targetSecondsPerFrame)
					{
						testElapsedSec = Win32GetSecondsElasped(lastPerformanceCount, Win32GetPerformanceCount());
					}
				}

				LARGE_INTEGER endPerformanceCount = Win32GetPerformanceCount();
				r32 countElapsed = (r32)(endPerformanceCount.QuadPart - lastPerformanceCount.QuadPart);
				lastPerformanceCount = endPerformanceCount;

				Win32DrawBuffer(windowHandle, &globalScreenBuffer);



#if 0
				char buffer[256];
                sprintf_s(buffer, "%.06f\n", RandomBetween(0.0f, 360.0f));
                OutputDebugStringA(buffer);	

				char buffer[256];
				r32 fps = (r32)performanceFrequency.QuadPart/countElapsed;
                sprintf_s(buffer, "%.02fFPS\n", fps);
                OutputDebugStringA(buffer);	
#endif
	            // TODO : Copying the whole input is pretty expensive - should be a beter way to handle this.
				game_input *temp = newInput;
				newInput = oldInput;
				oldInput = temp;
				*newInput = {};
				// // NOTE : Swap controllers

				// game_controller *temp = oldController;
				// oldController = newController;
				// newController = temp;
				// *newController = *oldController;
			}
		}
	}

	return 0;
}