/*
    Here are the most basic things to do inside the MACOS platform layer.

    Framerate control

    Gamepad input -> IOHIDManager & IOHIDElement

    unified game memory

    Sound

    more robust input handling. keyboard, mouse..

    live code editing
    input recording
*/

#include <stdio.h> // printf for debugging purpose

#include <sys/stat.h>

#include <Cocoa/Cocoa.h> // APPKIT
#include <mach/mach_time.h> // mach_absolute_time
#include <OpenGL/gl.h> // Every opengl related functions
#include <stdint.h> // int32_t, uint32_t...
#include <dlfcn.h>  // dlopen, dlsym for loading dylibs

#include <IOKit/hid/IOHIDLib.h> //IOHIDManager, IOHIDElement
#include <AudioUnit/AudioUnit.h>

#include "soma_platform_independent.h"
#include "macos_keycode.h"
#include "macos_support.cpp"

#include "soma.cpp"


#define MACOS_CALLBACK static

global_variable b32 IsGameRunning;
global_variable IOHIDDeviceRef GlobalDevice;

global_variable IOHIDManagerRef HIDManager;

u64 MachTimeDifferenceToNanoSecond(u64 BeginMachTime, u64 EndMachTime, r32 NanoSecondPerTick)
{
    return (u64)(((EndMachTime - BeginMachTime)*NanoSecondPerTick));
}

void
MacOSHandleEvents(NSApplication *App)
{
    // TODO : Check if this loop has memory leak.
    while(1)
    {
        NSEvent *Event = [App nextEventMatchingMask:NSAnyEventMask
                         untilDate:nil
                            inMode:NSDefaultRunLoopMode
                           dequeue:YES];
        if(Event)
        {
            switch([Event type])
            {
                case NSEventTypeKeyUp :
                case NSEventTypeKeyDown :
                {
                    u16 KeyCode = [Event keyCode];
                    if(KeyCode == MACOS_Escape)
                    {
                        IsGameRunning = false;
                    }
                }break;

                default :
                {
                    [App sendEvent : Event];
                }
            }
        }
        else
        {
            // TODO : Make sure this does not 'block' the code.
            break;
        }
    }
}

internal void *
DEBUGMacOSPlatformReadFile(char *FileName)
{
    void *result = 0;

    int File = open(FileName, O_RDONLY);
    if(File)
    {
        struct stat FileStat;
        fstat(File , &FileStat); 
        off_t FileSize = FileStat.st_size;

        if(FileSize > 0)
        {
            result = malloc(FileSize);
            if(read(File, result, FileSize) == -1)
            {
                free(result);
            }
        }
    }

    close(File);

    return result;
}

internal void
DEBUGMacOSPlatformWriteFile(char *FileNameToCreate, void *MemoryToWrite, u64 TotalSize)
{
    int File = open(FileNameToCreate, O_RDONLY | O_CREAT);
    if(write(File, MemoryToWrite, TotalSize) == -1)
    {
        // TODO : LOG here
    }
}

internal void
DEBUGMacOSPlatformFreeFileMemory(void *MemoryToFree)
{
    free(MemoryToFree);
}

MACOS_CALLBACK void
ControllerInputDetected(void *context, IOReturn result, void *sender, IOHIDValueRef value)
{
    // TODO : Is there any way to know what controller is the input from?
    macos_controller *Controllers = (macos_controller *)context;
    Controllers = 0;

    Controllers->AButton = 1; 

    // TODO : Press some buttons from the procon and check what the values are.
    IOHIDElementRef Element = IOHIDValueGetElement(value);

    printf("asdfasdfasd\n");

    //// TODO : Can I check this value to know whether I should pull the value more frequently?
    // IOHIDValueGetTimeStamp(value) 
}

MACOS_CALLBACK void
DeviceRemovalCallback(void *context, IOReturn result, void *sender)
{
    int a = 1;
}

MACOS_CALLBACK void 
ControllerConnectionDetected(void *Context, IOReturn Result, void *Sender, IOHIDDeviceRef Device)
{
    CFTypeRef Product = IOHIDDeviceGetProperty(Device, CFSTR(kIOHIDProductKey));
    CFTypeRef Manufacturer = IOHIDDeviceGetProperty(Device, CFSTR(kIOHIDManufacturerKey));
    CFTypeRef SerialNumber = IOHIDDeviceGetProperty(Device, CFSTR(kIOHIDSerialNumberKey));
    CFTypeRef Usage = IOHIDDeviceGetProperty(Device, CFSTR(kIOHIDPrimaryUsageKey));
    CFTypeRef UsagePage = IOHIDDeviceGetProperty(Device, CFSTR(kIOHIDPrimaryUsagePageKey));

    if(IOHIDDeviceConformsTo(Device, kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick) == TRUE)
    {
        printf("Wha??\n");
    }


    GlobalDevice = Device;


    // TODO : Whether this does not work, or the controller input is not detected properly.
    //IOHIDDeviceRegisterInputValueCallback(Device, ControllerInputDetected, 0);  
    IOHIDDeviceRegisterRemovalCallback(Device, DeviceRemovalCallback, 0);

    IOHIDDeviceScheduleWithRunLoop(Device, 
                                CFRunLoopGetCurrent(), 
                                kCFRunLoopDefaultMode);

    if(IOHIDDeviceOpen(Device, kIOHIDOptionsTypeNone) == kIOReturnSuccess)
    {
        printf("Device Opened\n");
    }
    printf("Controller Detected\n");
}
MACOS_CALLBACK void 
ControllerRemovalDetected(void *context, IOReturn result, void *sender, IOHIDDeviceRef device)
{
    // TODO : Maybe unschedule device from run loop here?
}

// TODO : Support for multiple controllers?
internal void
MacOSInitController(macos_controller *Controllers)
{
    HIDManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);

    // NOTE : ProCon falls inside the 'kHIDUsage_GD_Joystick' catergory.
    // TODO : For now, it does not detect xbox controller. Handle this case!
    IOHIDManagerSetDeviceMatchingMultiple(HIDManager,(__bridge CFArrayRef)
    @[
        @{@(kIOHIDDeviceUsagePageKey): @(kHIDPage_GenericDesktop), @(kIOHIDDeviceUsageKey): @(kHIDUsage_GD_GamePad)},
        @{@(kIOHIDDeviceUsagePageKey): @(kHIDPage_GenericDesktop), @(kIOHIDDeviceUsageKey): @(kHIDUsage_GD_Joystick)},
        @{@(kIOHIDDeviceUsagePageKey): @(kHIDPage_GenericDesktop), @(kIOHIDDeviceUsageKey): @(kHIDUsage_GD_Keypad)},
    ]);

    IOHIDManagerRegisterDeviceMatchingCallback(HIDManager, ControllerConnectionDetected, 0);
    IOHIDManagerRegisterInputValueCallback(HIDManager, ControllerInputDetected, 0); // NOTE : This does not work too...
    IOHIDManagerRegisterDeviceRemovalCallback(HIDManager, ControllerRemovalDetected, 0);

    IOHIDManagerScheduleWithRunLoop(HIDManager,
                                   CFRunLoopGetCurrent(), // TODO : CFRunLoopGetCurrent vs CFRunLoopGetMain ?
                                    kCFRunLoopDefaultMode);
    IOHIDManagerOpen(HIDManager, kIOHIDOptionsTypeNone);
}

MACOS_CALLBACK OSStatus
MacOSRenderAudio(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, 
                const AudioTimeStamp *inTimeStamp, 
                UInt32 inBusNumber, 
                UInt32 FrameCount, 
                AudioBufferList *BufferToFill)
{
    // TODO : Actually use this audio buffer as source buffer
    game_audio_buffer *SourceBuffer = (game_audio_buffer *)inRefCon;

    i16 *DestBuffer = (i16 *)BufferToFill->mBuffers[0].mData;
    u32 ConsumedSampleIndex = SourceBuffer->ConsumedSampleIndex;
    for(u32 FrameIndex = 0;
        FrameIndex < FrameCount;
        ++FrameIndex)
    {
        DestBuffer[FrameIndex] = SourceBuffer->Samples[ConsumedSampleIndex++];
        DestBuffer[FrameIndex+1] = SourceBuffer->Samples[ConsumedSampleIndex++];
        ConsumedSampleIndex %= SourceBuffer->SampleCount;
    }

    OSStatus Error = noErr;
    return Error;
}

internal void
MacOSInitAudio(game_audio_buffer *AudioBuffer)
{
    AudioUnit AudioUnit = {};
    AudioComponentDescription AudioComponentDescription = {};
    AudioComponentDescription.componentType = kAudioUnitType_Output;
    AudioComponentDescription.componentSubType = kAudioUnitSubType_DefaultOutput;
    AudioComponentDescription.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent AudioComponent = AudioComponentFindNext(0, &AudioComponentDescription);
    
    AudioComponentInstanceNew(AudioComponent, &AudioUnit);

    AudioStreamBasicDescription AudioStreamDescription;
    AudioStreamDescription.mBitsPerChannel = 8*sizeof(AudioBuffer->Samples[0]);
    Assert(AudioStreamDescription.mBitsPerChannel == 16);
    AudioStreamDescription.mChannelsPerFrame = AudioBuffer->ChannelCount;
    AudioStreamDescription.mFramesPerPacket = 1;
    AudioStreamDescription.mBytesPerFrame = AudioBuffer->ChannelCount * sizeof(AudioBuffer->Samples[0]);
    AudioStreamDescription.mSampleRate = AudioBuffer->SamplesPerSecond;
    AudioStreamDescription.mBytesPerPacket = AudioStreamDescription.mBytesPerFrame *
                                            AudioStreamDescription.mFramesPerPacket;
    AudioStreamDescription.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    AudioStreamDescription.mFormatID = kAudioFormatLinearPCM;

    if(AudioUnitSetProperty(AudioUnit, kAudioUnitProperty_StreamFormat, 
                        kAudioUnitScope_Input, 0, 
                        &AudioStreamDescription, sizeof(AudioStreamDescription)) == noErr)
    {
        AURenderCallbackStruct AudioUnitRenderCallback = {};
        AudioUnitRenderCallback.inputProcRefCon = (void *)AudioBuffer;
        AudioUnitRenderCallback.inputProc = MacOSRenderAudio;
        AudioUnitSetProperty(AudioUnit, kAudioUnitProperty_SetRenderCallback, 
                        kAudioUnitScope_Global, 0, 
                        &AudioUnitRenderCallback, sizeof(AudioUnitRenderCallback));

        printf("Audio Unit property set\n");
        if(AudioUnitInitialize(AudioUnit) == noErr)
        {
            printf("Audio Unit initialized\n");
            //if(AudioOutputUnitStart(AudioUnit) == noErr)
            {
                printf("Audio Unit started\n");
            }
        }
    }
}

internal u64
MacOSGetFileModifiedTime(char *FileName)
{
    u64 Result = 0;

    int File = open(FileName, O_RDONLY);

    struct stat FileStat;
    fstat(File , &FileStat); 
    Result = FileStat.st_mtime;

    close(File);

    return Result;
}

/*
    NOTE : Here's how dynamic library loading works.
        1. The 'copied' version of the library will be used, not the original one that is being built.
        2. However, we will compare the 
*/
internal void
MacOSGetGameCode(game_code *GameCode, char *FileName)
{
    if(GameCode->Handle)
    {
        u64 LastModifiedTime = MacOSGetFileModifiedTime(FileName);
        if(LastModifiedTime > GameCode->LastModifiedTime)
        {
            GameCode->LastModifiedTime = LastModifiedTime;
            dlclose(GameCode->Handle);
        }
        else
        {
            return;
        }
    }

    GameCode->Handle = dlopen(FileName, RTLD_LAZY);

    //char *Error = dlerror();
    if(GameCode->Handle)
    {
        GameCode->UpdateAndRender = (game_update_and_render *)dlsym(GameCode->Handle, "GameUpdateAndRender");
        //Error = dlerror();
    }
    else
    {
        GameCode->UpdateAndRender = GameUpdateAndRenderStub;
        GameCode->LastModifiedTime = 0;
    }
}

int main(int argc, char **argv)
{
    // TODO : How to 'build' a path that works no matter where the library is.
    char *DynamicLibraryPath = "/Volumes/work/soma/build/fox.app/Contents/Resources/soma.dylib";
    char *DynamicLibraryLockPath = "/Volumes/work/soma/build/fox.app/Contents/Resources/soma.lock";
    game_code GameCode = {};
    MacOSGetGameCode(&GameCode, DynamicLibraryPath);

    void *FileMemory = DEBUGMacOSPlatformReadFile("/Volumes/work/soma/data/sample.bmp");

    game_audio_buffer GameAudioBuffer = {};
    GameAudioBuffer.SamplesPerSecond = 48000;
    GameAudioBuffer.ChannelCount = 2;
    // NOTE : Audio Buffer is 3 seconds worth for now. 
    GameAudioBuffer.SampleCount = 3 * GameAudioBuffer.SamplesPerSecond * GameAudioBuffer.ChannelCount;
    GameAudioBuffer.Samples = (i16 *)malloc(sizeof(i16) * GameAudioBuffer.SampleCount);
    MacOSInitAudio(&GameAudioBuffer);

    // TODO : Support multiple controllers?
    macos_controller Controllers[4];
    MacOSInitController(Controllers);

    struct mach_timebase_info MachTimeInfo;
    mach_timebase_info(&MachTimeInfo);
    r32 NanoSecondPerTick = ((r32)MachTimeInfo.numer/(r32)MachTimeInfo.denom);

    r32 TargetSecondsPerFrame = 1/30.0f;
    u32 TargetNanoSecondsPerFrame = (u32)(TargetSecondsPerFrame*SecToNanoSec);

    NSApplication *App = [NSApplication sharedApplication]; // Your program should invoke this method as one of the first statements in main();
    [App setActivationPolicy :NSApplicationActivationPolicyRegular]; // The application is an ordinary app that appears in the Dock and may have a user interface.

    NSMenu *AppMainMenu = [NSMenu alloc];
    NSMenuItem *MenuItemWithAppName = [NSMenuItem new];
    [AppMainMenu addItem : MenuItemWithAppName];
	[NSApp setMainMenu:AppMainMenu];
    
    
	NSMenu *SubMenuOfMenuItemWithAppName = [NSMenu alloc];
	NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" 
                    action:@selector(terminate:)  // Decides what will happen when the menu is clicked or selected
                    keyEquivalent:@"q"];
	[SubMenuOfMenuItemWithAppName addItem:quitMenuItem];
	[MenuItemWithAppName setSubmenu:SubMenuOfMenuItemWithAppName];

    macos_offscreen_buffer MacOSBuffer = {};
	MacOSBuffer.Width = 1920;
	MacOSBuffer.Height = 1080;
    MacOSBuffer.BytesPerPixel = 4;
    MacOSBuffer.Pitch = MacOSBuffer.Width * MacOSBuffer.BytesPerPixel;
    MacOSBuffer.Memory = malloc(MacOSBuffer.Pitch * MacOSBuffer.Height); // TODO : Put this inside the unified game memory?

    // TODO : Don't know why, but MacOS creates a window with doubled size, so request the window to
    // be half the size at the first place. Maybe this is because of the high resolution monitor?
	NSRect WindowFrameRect = NSMakeRect(100.0f, 100.0f, (float)MacOSBuffer.Width/2, (float)MacOSBuffer.Height/2);

	NSWindow *Window = [[NSWindow alloc] initWithContentRect : WindowFrameRect
											// Apple window styles : https://developer.apple.com/documentation/appkit/nswindow/stylemask
											styleMask : NSTitledWindowMask|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable
											backing : NSBackingStoreBuffered 
											defer : NO];

	NSString *AppName = [[NSProcessInfo processInfo]processName];
	[Window setTitle:AppName];
	[Window makeKeyAndOrderFront:0];
    // [Window toggleFullScreen:0]; // TODO : Fullscreen support?

	fox_app_delegate *appDelegate = [[fox_app_delegate alloc] init];
	[App setDelegate:appDelegate];

    NSWindow *MainWindow = [App mainWindow];

    macos_opengl_info MacOSOpenGLInfo = {};
    MacOSOpenGLInfo = PrepareDisplayingWithOpenGL(Window, &WindowFrameRect, MacOSBuffer.Width, MacOSBuffer.Height, MacOSBuffer.Memory); 

    // NOTE : vm_allocate is preferred to be multiple of 4096 bytes

    u8 *GameMemory = 0;
    u64 GameMemorySize = 4096;
    // TODO : There are many allocation methods. Make sure this one is the fastest - or does it matter?
    vm_allocate(mach_task_self(), (vm_address_t *)&GameMemory, GameMemorySize, VM_FLAGS_ANYWHERE); 

    if(GameMemory)
    {
        IsGameRunning = true; // TODO : Put this inside the event handling loop

        u64 BeginTime = 0;

        [App activateIgnoringOtherApps:YES];

        while(IsGameRunning)
        {
#if 1
            int DynamicLibraryLock = open("DynamicLibraryLockPath", O_RDONLY);
            if(DynamicLibraryLock != 0)
            {
                MacOSGetGameCode(&GameCode, DynamicLibraryPath);
            }
#endif
            CFSetRef EnumeratedDevices = IOHIDManagerCopyDevices(HIDManager); 
            MacOSHandleEvents(App);

            // GCController *Controllers = (GCController *)[GCController controllers]; // TODO : Does this only support MFi controllers?

            game_offscreen_buffer GameOffscreenBuffer = {};
            GameOffscreenBuffer.Width = MacOSBuffer.Width;
            GameOffscreenBuffer.Height = MacOSBuffer.Height;
            GameOffscreenBuffer.BytesPerPixel = MacOSBuffer.BytesPerPixel;
            GameOffscreenBuffer.Pitch = MacOSBuffer.Pitch;
            GameOffscreenBuffer.Memory = MacOSBuffer.Memory;

            GameCode.UpdateAndRender(&GameOffscreenBuffer, &GameAudioBuffer);

            // Rendering loop with OpenGL.
            {
                glClear(GL_COLOR_BUFFER_BIT);
                glBindTexture(GL_TEXTURE_2D, MacOSOpenGLInfo.TextureID);
                glTexImage2D(GL_TEXTURE_2D, 0, 
                            GL_RGBA8, 
                            MacOSBuffer.Width, MacOSBuffer.Height,
                            0, 
                            GL_BGRA, 
                            GL_UNSIGNED_INT_8_8_8_8_REV, 
                            MacOSBuffer.Memory);
                glViewport(0, 0, MacOSBuffer.Width, MacOSBuffer.Height);

                glBegin(GL_QUADS); 
                {
                    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
                    glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, 1.0f);
                    glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  -1.0f);
                    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  -1.0f);
                } glEnd();
                glBindTexture(GL_TEXTURE_2D, 0);

                [MacOSOpenGLInfo.OpenGLContext flushBuffer]; // This will call glFlush() internally
            }

            u64 EndTime = mach_absolute_time();
            u32 TimeDifferenceInNanoSecond = (u32)MachTimeDifferenceToNanoSecond(BeginTime, EndTime, NanoSecondPerTick);

            if(TimeDifferenceInNanoSecond < TargetNanoSecondsPerFrame)
            {
                timespec TimeSpec = {};
                TimeSpec.tv_nsec = TargetNanoSecondsPerFrame - TimeDifferenceInNanoSecond;

                nanosleep(&TimeSpec, 0);
            }
            else
            {
                // TODO : Missed Frame!
            }

            EndTime = mach_absolute_time();
            // TODO : More precise frame rate?
            printf("%f\n", MachTimeDifferenceToNanoSecond(BeginTime, EndTime, NanoSecondPerTick)/SecToNanoSec);

            BeginTime = EndTime;
        }
    }

    return 0;
}
