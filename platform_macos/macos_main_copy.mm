#include <Cocoa/Cocoa.h> 
#include <AudioUnit/AudioUnit.h>
#include <OpenGL/gl.h> // TODO : Change this to use METAL?
#include <dlfcn.h> // UNIX file handle functions - dlopen, dlclose

// File Stat
#include <sys/stat.h> // stat() function for retrieving file size

// Framerate control
#include <mach/mach_time.h>

#include "../code/fox_platform_independent.h"
#include "../code/fox.cpp"
#include "macos_fox.cpp"

global_variable b32 isGameRunning;
global_variable b32 isGamePausing;
global_variable game_screen_buffer gameBuffer;
global_variable game_memory gameMemory;

@implementation fox_app_delegate : NSObject
// -(void)applicationWillFinishLaunching: (NSNotification *)notification
- (void)applicationDidFinishLaunching: (NSNotification *)notification 
{
    [NSApp stop: nil];
    // Post empty event: without it we can't put application to front
    // for some reason (I get this technique from GLFW source).
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    NSEvent* event =
        [NSEvent otherEventWithType: NSApplicationDefined
                 location: NSMakePoint(0, 0)
                 modifierFlags: 0
                 timestamp: 0
                 windowNumber: 0
                 context: nil
                 subtype: 0
                 data1: 0
                 data2: 0];
    [NSApp postEvent: event atStart: YES];
    [pool drain];

	isGameRunning = true;
}

- (void)dealloc 
{
    [super dealloc];
}
@end

@implementation fox_openGL_view
- (void)reshape 
{
    glViewport(0, 0, gameBuffer.width, gameBuffer.height);
}
@end

struct macos_input_record
{
	int fileDescriptor;

	size_t totalSize;

	size_t currentPlayingIndex;

	b32 isNewlyRecorded;

	b32 isRecording;
	b32 isPlaying;
};

internal void
MacOSBeginInputRecording(macos_input_record *inputRecord, char *filePath)
{
	if(!inputRecord->isPlaying)
	{
		inputRecord->fileDescriptor = open(filePath, O_WRONLY|O_CREAT|O_TRUNC, 
											S_IWUSR|S_IRUSR|S_IWOTH|S_IROTH);

		if(inputRecord->fileDescriptor != -1)
		{
			inputRecord->isRecording = true;
			inputRecord->isNewlyRecorded = true;
			inputRecord->totalSize = 0;
			inputRecord->currentPlayingIndex = 0;
		}
	}
}

internal void
MacOSInputRecording(macos_input_record *inputRecord, game_input *gameInput)
{
	Assert(inputRecord->fileDescriptor != -1);

	write(inputRecord->fileDescriptor, (void *)gameInput, sizeof(game_input));
	inputRecord->totalSize += sizeof(game_input);
}

internal void
MacOSEndInputRecording(macos_input_record *inputRecord)
{
	Assert(inputRecord->fileDescriptor != -1);

	int result = close(inputRecord->fileDescriptor);
	inputRecord->isRecording = false;
}

internal void
MacOSStartPlayingInputRecording(macos_input_record *inputRecord, char *filePath)
{
	if(!inputRecord->isRecording)
	{
		inputRecord->fileDescriptor = open(filePath, O_RDONLY);

		if(inputRecord->fileDescriptor != -1)
		{
			inputRecord->isPlaying = true;
			inputRecord->currentPlayingIndex = 0;
			if(!inputRecord->isNewlyRecorded)
			{
				inputRecord->totalSize = (size_t)MacOSGetFileSize(filePath);
			}
		}
	}
}

internal void
MacOSPlayInputRecording(macos_input_record *inputRecord, game_input *gameInput)
{
	read(inputRecord->fileDescriptor, (void *)gameInput, sizeof(game_input));
	inputRecord->currentPlayingIndex += sizeof(game_input);

	if(inputRecord->currentPlayingIndex == inputRecord->totalSize)
	{
		inputRecord->currentPlayingIndex = 0;
		lseek(inputRecord->fileDescriptor, (off_t)0, 0);
	}
}

internal void
MacOSStopPlayingInputRecording(macos_input_record *inputRecord, game_input *newInput)
{
	close(inputRecord->fileDescriptor);
	inputRecord->isPlaying = false;
	*newInput = {};
}

internal void
ProcessKeyboardButton(game_input *gameInput, b32 isDown, u8 keyCode)
{
	game_controller *controller = gameInput->controllers + 0;
	game_button *button = controller->buttons + keyCode;
	button->isDown = isDown;
}

void 
MacOSHandleEvents(NSApplication *application, game_input *newInput, game_input *oldInput, macos_input_record *inputRecord)
{
    NSAutoreleasePool *eventsAutoreleasePool = [[NSAutoreleasePool alloc] init];
    while(true) 
    {
        NSEvent* event =
            [application nextEventMatchingMask: NSAnyEventMask
                         untilDate: [NSDate distantPast]
                         inMode: NSDefaultRunLoopMode
                         dequeue: YES];
        if (event) 
        {
	        switch ([event type]) 
	        {	
	        	case NSKeyUp:
	        	case NSKeyDown:
	        	{
	        		// TODO : Might wanna do this as character? 
	        		u16 keyCode = [event keyCode];
	        		NSLog(@"%d", keyCode);
	        		b32 isDown = ([event type] == NSKeyDown);
                    char character = 'D';
	        		switch(keyCode)
	        		{
#if 0
	        			case MACOSKEY_A:
						{
							ProcessKeyboardButton(newInput, isDown, KEY_A);
						}break;
						case MACOSKEY_D:
						{
							ProcessKeyboardButton(newInput, isDown, KEY_D);
						}break;
						case MACOSKEY_C:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_C);
	        			}break;
	        			case MACOSKEY_B:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_B);
	        			}break;
	        			case MACOSKEY_E:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_E);
	        			}break;
	        			case MACOSKEY_F:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_F);
	        			}break;
	        			case MACOSKEY_H:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_H);
	        			}break;
	        			case MACOSKEY_I:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_I);
	        			}break;
	        			case MACOSKEY_J:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_J);
	        			}break;
	        			case MACOSKEY_K:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_K);
	        			}break;
	        			case MACOSKEY_L:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_L);

	        				if(GetButtonTriggered(newInput, oldInput, 0, KEY_L))
	        				{
		        				if(!inputRecord->isRecording)
		        				{
			        				MacOSBeginInputRecording(inputRecord, "fox.fir");
		        				}
		        				else
		        				{
		        					MacOSEndInputRecording(inputRecord);
		        					// start
		        				}
		        			}
	        			}break;
	        			case MACOSKEY_M:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_M);
	        			}break;
	        			case MACOSKEY_N:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_N);
	        			}break;
	        			case MACOSKEY_O: // O
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_O);

	        				if(GetButtonTriggered(newInput, oldInput, 0, KEY_O))
	        				{
		        				if(!inputRecord->isPlaying)
		        				{
		        					MacOSStartPlayingInputRecording(inputRecord, "fox.fir");
		        				}
		        				else
		        				{
		        					MacOSStopPlayingInputRecording(inputRecord, newInput);
		        				}
		        			}
	        			}break;
	        			case MACOSKEY_P: // P
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_P);
	        				// isGamePausing = !isGamePausing;
	        			}break;
	        			case MACOSKEY_Q:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_Q);
	        			}break;
	        			case MACOSKEY_R:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_R);
	        			}break;
	        			case MACOSKEY_S:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_S);
	        			}break;
	        			case MACOSKEY_T:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_T);
	        			}break;
	        			case MACOSKEY_U:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_U);
	        			}break;
	        			case MACOSKEY_V:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_V);
	        			}break;
	        			case MACOSKEY_W:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_W);
	        			}break;
	        			case MACOSKEY_X:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_X);
	        			}break;
	        			case MACOSKEY_Y:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_Y);
	        			}break;
        				case MACOSKEY_Z:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_Z);
	        			}break;

	        			case MACOSKEY_0:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_0);
	        			}break;
	        			case MACOSKEY_1:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_1);
	        			}break;
	        			case MACOSKEY_2:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_2);
	        			}break;
	        			case MACOSKEY_3:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_3);
	        			}break;
	        			case MACOSKEY_4:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_4);
	        			}break;
	        			case MACOSKEY_5:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_5);
	        			}break;
	        			case MACOSKEY_6:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_6);
	        			}break;
	        			case MACOSKEY_7:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_7);
	        			}break;
	        			case MACOSKEY_8:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_8);
	        			}break;
	        			case MACOSKEY_9:
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_9);
	        			}break;
	        			
	        			case MACOSKEY_ENTER: // enter
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_ENTER);
	        			}break;

        				case MACOSKEY_SPACE: // space
	        			{
	        				ProcessKeyboardButton(newInput, isDown, KEY_SPACE);
	        			}break;
	        			
	        			case MACOSKEY_ESC: // esc
	        			{
	        				int reuslt = close(inputRecord->fileDescriptor);
	        				int a = 1;

	        				[application terminate:0];
	        				isGameRunning = false;
	        			}break;
#endif

	        			default :
	        			{

	        			}break;
	        		}
	        	}break;
	        	
	        	default:
	        	{
	        		// Handle basic events such as autofocus, autoresize
	        		[application sendEvent:event];
	        	}break;
	        }
        }
        else
        {
        	// *oldInput = {};
        	break;
        }
   	}


   [eventsAutoreleasePool drain]; 
}

// TODO : Any reason to use filemanager? ex, it's faster?
#if 0
internal void *
MacOSReadFile(NSFileManager *fileManager, char *filePath)
{
	void *result = 0;

	NSFileHandle *handle;
	handle = [NSFileHandle fileHandleForReadingAtPath:[NSString stringWithUTF8String: filePath]];
	if(handle)
	{
		NSData *nsdata = [handle readDataToEndOfFile];
		int length = [nsdata length];
		result = malloc(length);
		if(result)
		{
			[nsdata getBytes: result length: length];
		}
	}

	return result;
}

// void *
// MacOSWriteFile()
// {

// }
#endif

DEBUG_PLATFORM_READ_FILE(MacOSPlatformReadFile)
{
	void *result = 0;

	int fileDescriptor = open(filePath, O_RDONLY);
	if(fileDescriptor != -1)
	{
		size_t fileSize = (size_t)MacOSGetFileSize(filePath);
		// TODO : Find out which allocation function to use
		result = malloc(fileSize);

		if(result)
		{
			read(fileDescriptor, result, fileSize);
		}

		close(fileDescriptor);
	}

	return result;
}

DEBUG_PLATFORM_WRITE_FILE(MacOSPlatformWriteFile)
{
	bool result = false;

	int fileDescriptor = open(filePath, O_WRONLY|O_CREAT|O_TRUNC, 
										S_IWUSR|S_IRUSR|S_IWOTH|S_IROTH);
	if(fileDescriptor != -1)
	{
		lseek(fileDescriptor, 0, SEEK_SET);
		write(fileDescriptor, source, sourceSize);
		close(fileDescriptor);

		result = true;
	}

	return result;
}

DEBUG_PLATFORM_FREE_FILE(MacOSPlatformFreeFile)
{
	free(pointer);
}

void CopyToMacosAudioBuffer(macos_audio_buffer *macosAudioBuffer, game_audio_buffer *gameAudioBuffer)
{
    i16 *destLeft = macosAudioBuffer->samples[0] + macosAudioBuffer->writeCursor;
    i16 *destRight = macosAudioBuffer->samples[1] + macosAudioBuffer->writeCursor;

    i16 *sourceLeft = gameAudioBuffer->samples[0];
    i16 *sourceRight = gameAudioBuffer->samples[1];

    u32 addedWriteCursor = macosAudioBuffer->writeCursor + gameAudioBuffer->sampleCount;
    i32 overflowedWriteCursor = addedWriteCursor - macosAudioBuffer->samplesPerSecond;
    u32 remainingSamplesUntilEnd = macosAudioBuffer->samplesPerSecond - macosAudioBuffer->writeCursor;

#if 1
    if(overflowedWriteCursor <= 0)
    {
        // TODO : Find out which one is faster 
        // memcpy vs copying one sample each
        memcpy((void *)(destLeft), (void *)(sourceLeft), gameAudioBuffer->totalBytesPerChannel);
        memcpy((void *)(destRight), (void *)(sourceRight), gameAudioBuffer->totalBytesPerChannel);
        macosAudioBuffer->writeCursor += gameAudioBuffer->sampleCount;
        macosAudioBuffer->writeCursor %= macosAudioBuffer->samplesPerSecond;
    }
    else
    {
        memcpy((void *)(destLeft), (void *)(sourceLeft), sizeof(i16)*remainingSamplesUntilEnd);
        memcpy((void *)(destRight), (void *)(sourceRight), sizeof(i16)*remainingSamplesUntilEnd);

        destLeft = macosAudioBuffer->samples[0];
        destRight = macosAudioBuffer->samples[1];
        sourceLeft += remainingSamplesUntilEnd;
        sourceRight += remainingSamplesUntilEnd;

        memcpy((void *)(destLeft), (void *)(sourceLeft), sizeof(i16)*overflowedWriteCursor);
        memcpy((void *)(destRight), (void *)(sourceRight), sizeof(i16)*overflowedWriteCursor);

        macosAudioBuffer->writeCursor = overflowedWriteCursor;
    }

#endif
}


int main(int argc, char **argv)
{
    // TODO : 
	gameMemory.permanentStorageSize = Megabytes(64);
	gameMemory.transientStorageSize = Megabytes(256);

	kern_return_t result = vm_allocate((vm_map_t)mach_task_self(),
										(vm_address_t *)&gameMemory.permanentStorage,
										gameMemory.permanentStorageSize+gameMemory.transientStorageSize,
										VM_FLAGS_ANYWHERE);
	gameMemory.transientStorage = (u8 *)gameMemory.permanentStorage + gameMemory.permanentStorageSize;

	int bufferWidth = 960;
	int bufferHeight = 540;
	gameBuffer.width = bufferWidth; 
	gameBuffer.height = bufferHeight;
	gameBuffer.pitch = gameBuffer.width*4;
    gameBuffer.size = gameBuffer.width * gameBuffer.height;
	gameBuffer.memory = (u8 *)malloc(gameBuffer.pitch*gameBuffer.height);

	int targetFramesPerSec = 30;
	float targetSecondsPerFrame = 1.0f/targetFramesPerSec;
    mach_timebase_info_data_t timeBase;
    mach_timebase_info(&timeBase);
    float ticksPerSecond = 1.0e9*((float)timeBase.denom/(float)timeBase.numer);

    macos_audio_buffer macosAudioBuffer = {};
    macosAudioBuffer.samplesPerSecond = 48000;
    macosAudioBuffer.bytesPerSample = sizeof(i16);
    macosAudioBuffer.channelCount = 2;
    int bytesPerSecond = macosAudioBuffer.bytesPerSample * macosAudioBuffer.samplesPerSecond;
    macosAudioBuffer.samples[0] = (i16 *)malloc(macosAudioBuffer.channelCount*bytesPerSecond);
    macosAudioBuffer.samples[1] = macosAudioBuffer.samples[0] + macosAudioBuffer.samplesPerSecond;
    macosAudioBuffer.writeCursor = 0;
    macosAudioBuffer.playCursor = 0;
    ClearAudioBuffer(macosAudioBuffer.samples[0], macosAudioBuffer.samples[1], macosAudioBuffer.samplesPerSecond);

    u32 safeSampleCount = (macosAudioBuffer.samplesPerSecond / targetFramesPerSec);
    game_audio_buffer gameAudioBuffer = {};
    gameAudioBuffer.totalBytesPerChannel = sizeof(i16) * safeSampleCount;
    gameAudioBuffer.channelCount = macosAudioBuffer.channelCount;
    gameAudioBuffer.sampleCount = safeSampleCount;
    gameAudioBuffer.samplesPerSecond = macosAudioBuffer.samplesPerSecond;
    // NOTE : Need to allocate gmaeAudioBuffer seperated from the platform audio buffer 
    // because platform audio buffer can be called during the GameUpdateAudioBuffer
    gameAudioBuffer.samples[0] = (i16 *)malloc(gameAudioBuffer.channelCount * gameAudioBuffer.totalBytesPerChannel);
    gameAudioBuffer.samples[1] = gameAudioBuffer.samples[0] + safeSampleCount;

    //InitializeAudio(&macosAudioBuffer);

	if(gameMemory.permanentStorage && gameMemory.transientStorage &&
		gameBuffer.memory)
	{
		NSApplication *application = [NSApplication sharedApplication];
		// Make the window to be able to brought in front.
		[NSApp setActivationPolicy : NSApplicationActivationPolicyRegular];

		// don't need to give the first item in the menubar a name (it will get the application's name automatically)
		NSMenu *mainMenu = [NSMenu alloc];
		// Menu Item with the app's name - which is the very first menu item.
		NSMenuItem *appNameMenuItem = [NSMenuItem new];
		[mainMenu addItem:appNameMenuItem];
		[NSApp setMainMenu:mainMenu];

		NSMenu *appNameMenuItemSubMenu = [NSMenu alloc];
		NSString *appName = [[NSProcessInfo processInfo]processName];
		NSString *quitTitle = [@"Quit " stringByAppendingString:appName];
		NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"];
		[appNameMenuItemSubMenu addItem:quitMenuItem];
		[appNameMenuItem setSubmenu:appNameMenuItemSubMenu];

		// NOTE : Set working dicectory
		NSFileManager *fileManager = [NSFileManager defaultManager];
		NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
		NSString *resourcePath = [NSString stringWithFormat:@"%@/Contents/Resources", bundlePath];

		NSRect windowFrameRect = NSMakeRect(100.0f, 100.0f, (float)bufferWidth, (float)bufferHeight);

		NSWindow *window = [[NSWindow alloc] initWithContentRect : windowFrameRect
											// Apple window styles : https://developer.apple.com/documentation/appkit/nswindow/stylemask
											styleMask : NSTitledWindowMask|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable
											backing : NSBackingStoreBuffered 
											defer : NO];
		[window setTitle:appName];
		[window makeKeyAndOrderFront:0];
		//fox_app_delegate *appDelegate = [[fox_app_delegate alloc] init];
		// NOTE : Need to set both delegate because we are inheriting both the NSWindowDelegate & NSApplicationDelegate
		//[application setDelegate:appDelegate];
		//[window setDelegate:appDelegate];

		macos_opengl_info macosOpenGLInfo = MacOSInitializeOpenGL(&gameBuffer);
		fox_openGL_view *foxView = [[fox_openGL_view alloc] initWithFrame:windowFrameRect];
		[window setContentView:foxView];
		[foxView setOpenGLContext:macosOpenGLInfo.context];
		[foxView setPixelFormat:macosOpenGLInfo.pixelFormat];
		[macosOpenGLInfo.context setView:foxView];
		glViewport(0, 0, gameBuffer.width, gameBuffer.height);

		game_input gameInputs[2] = {};
		game_input *newInput = gameInputs;
		game_input *oldInput = gameInputs + 1;

		macos_game_code macosGameCode = {};
		// TODO : Pass the filename to here so that we can process dylib with any name
		const char *dylibPath = [[NSString stringWithFormat:@"%@/fox.dylib", resourcePath] UTF8String];
		const char *copydylibPath = [[NSString stringWithFormat:@"%@/fox_copy.dylib", resourcePath] UTF8String];

		MacOSGetGameCodes(&macosGameCode, dylibPath);

		macos_input_record inputRecord = {};

		platform_api platformapi = {};

		platformapi.PlatformReadFile = MacOSPlatformReadFile;
		platformapi.PlatformWriteFile = MacOSPlatformWriteFile;
		platformapi.PlatformFreeFile = MacOSPlatformFreeFile;

		[NSApp activateIgnoringOtherApps:YES];
		[NSApp run];

		// Because we wrote [NSApp stop] in the applicationDidLaunch, we can get inside this loop.
		while(isGameRunning)
		{ 
			u64 beginTimer = mach_absolute_time();

			{
				long currentFileTime = MacOSGetFileTime(dylibPath);
				if(currentFileTime > macosGameCode.lastFileTime)
				{
					// TODO : Find more about copyfile flags
					// TODO : Find out why we need to do this?
					// copyfile(dylibPath, copydylibPath, 0, COPYFILE_DATA);	
					MacOSGetGameCodes(&macosGameCode, dylibPath);
				}
			}

			MacOSHandleEvents(application, newInput, oldInput, &inputRecord);

			if(inputRecord.isRecording)
			{
				MacOSInputRecording(&inputRecord, newInput);
			}
			if(inputRecord.isPlaying)
			{
				MacOSPlayInputRecording(&inputRecord, newInput);
			}

            if(!isGamePausing)
            {
                if(macosGameCode.GameUpdateAndRender)
                {
                    macosGameCode.GameUpdateAndRender(&gameBuffer, newInput, oldInput, &gameMemory, &platformapi, targetSecondsPerFrame);
                }

                if(macosGameCode.GameUpdateAudioBuffer)
                {
                    macosGameCode.GameUpdateAudioBuffer(&gameAudioBuffer, &gameMemory, targetSecondsPerFrame);
                    CopyToMacosAudioBuffer(&macosAudioBuffer, &gameAudioBuffer);
                }
            }

			// CGLLockContext(macosOpenGLInfo.cglContext);
		    glClear(GL_COLOR_BUFFER_BIT);
		    glBindTexture(GL_TEXTURE_2D, macosOpenGLInfo.textureID);
		    // Upload new video frame to the GPU.
			// glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, gameBuffer.width, gameBuffer.height,
			// 			0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, gameBuffer.memory);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gameBuffer.width, gameBuffer.height,
						0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, gameBuffer.memory);

		    // Draw textured full-viewport quad.
		    glBegin(GL_QUADS); 
		    {
		        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
		        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, 1.0f);
		        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  -1.0f);
		        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  -1.0f);
		    } glEnd();

			[macosOpenGLInfo.context flushBuffer];
		    // CGLUnlockContext(macosOpenGLInfo.cglContext);

		    game_input *temp = newInput;
		    newInput = oldInput;
		    oldInput = temp;
		    *newInput = *oldInput;

		    u64 endTimer = mach_absolute_time();
		    float passedSeconds = (float)(endTimer-beginTimer)/ticksPerSecond;
		    if(passedSeconds < targetSecondsPerFrame)
		    {
		    	// NOTE : 1000000 to get to "second" space
		    	// usleep(1) means sleep for 1 microsecond(0.001 milliseconds)
			    usleep((targetSecondsPerFrame-passedSeconds) * 1000000);
		    }
		    else
		    {
		    	// TODO : Missed frame!
                // TODO : Logging
		    }
		}
	}

	return 0;
}

