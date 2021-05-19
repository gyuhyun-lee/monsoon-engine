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

DEBUG_PLATFORM_READ_FILE(DEBUGMacOSReadFile)
{
    debug_platform_read_file_result Result = {};

    int File = open(FileName, O_RDONLY);
    int Error = errno;
    if(File >= 0) // NOTE : If the open() succeded, the return value is non-negative value.
    {
        struct stat FileStat;
        fstat(File , &FileStat); 
        off_t FileSize = FileStat.st_size;

        if(FileSize > 0)
        {
            // TODO : NO MORE OS LEVEL ALLOCATION!
            // Pass this function a memory that it can use so that the function can only
            // read from it.
            Result.Size = FileSize;
            Result.Memory = malloc(Result.Size);
            if(read(File, Result.Memory, FileSize) == -1)
            {
                free(Result.Memory);
                Result.Size = 0;
            }
        }

        close(File);
    }

    return Result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGMacOSWriteEntireFile)
{
    // NOTE : This call will fail if the file already exists.
    int File = open(FileNameToCreate, O_WRONLY|O_CREAT|O_EXCL, S_IRWXU);
    if(File >= 0) // NOTE : If the open() succeded, the return value is non-negative value.
    {
        if(write(File, MemoryToWrite, FileSize) == -1)
        {
            // TODO : LOG here
        }

        close(File);
    }
    else
    {
        // TODO : File already exists. LOG here.
    }
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGMacOSFreeFileMemory)
{
    free(MemoryToFree);
}

MACOS_CALLBACK void
MacOSDualSenseInputDetected(void *Context, IOReturn Result, void *Sender, IOHIDValueRef Value)
{}
MACOS_CALLBACK void
MacOSDualShockInputDetected(void *Context, IOReturn Result, void *Sender, IOHIDValueRef Value)
{}
MACOS_CALLBACK void
MacOSXBOXInputDetected(void *Context, IOReturn Result, void *Sender, IOHIDValueRef Value)
{}

internal r32 
MacOSGetProconStickValue(i32 StickValue, i32 StickMin, i32 StickAverage, i32 StickMax)
{
    r32 Result = 0.0f;
    Result = 2.0f*((r32)(StickValue - StickAverage)/(StickMax - StickMin));
    // TODO : Not a particularly good deadzone handling, but as long as I don't get the
    // official document from nintendo, this will  stay this way.
    if(Result < -1.0f)
    {
        Result = -1.0f;
    }
    else if(Result > 1.0f)
    {
        Result = 1.0f;
    }
    else if(Result < 0.1f && Result > -0.1f)
    {
        Result = 0.0f;
    }

    return Result;
}

MACOS_CALLBACK void
MacOSProConInputDetected(void *Context, IOReturn Result, void *Sender, IOHIDValueRef Value)
{
    game_controller *Controller = (game_controller *)Context;
    IOHIDElementRef Element = IOHIDValueGetElement(Value);

    const uint8_t *ValueInByte = IOHIDValueGetBytePtr(Value);

    /*
        NOTE : 
        Memory Order
        Value[3] Value[2] Value[1] Value[0]

        ABXY Button -> Value[2]

        _Each values are 12bit_
        LEFT X - Value6 Bottom Half << 8 | Value5 
        LEFT Y - Value 7 << 4 | Value6 Top Half >> 4
        RIGHT X - Value9 Bottom Half << 8 |  Value8
        RIGHT Y -  Value 10 << 4 | Value9 Top Half >> 4

        Although the Byte Length is 48bytes, only the first 10 bytes are used.
    */

    u8 Value0 = ValueInByte[0];
    u8 Value1 = ValueInByte[1];
    u8 Value2 = ValueInByte[2];
    u8 Value3 = ValueInByte[3];
    u8 Value4 = ValueInByte[4];
    u8 Value5 = ValueInByte[5];
    u8 Value6 = ValueInByte[6];
    u8 Value7 = ValueInByte[7];
    u8 Value8 = ValueInByte[8];
    u8 Value9 = ValueInByte[9];
    u8 Value10 = ValueInByte[10];
    int ValueInInt = IOHIDValueGetIntegerValue(Value);

    #define LeftStickXMax	3860
    #define LeftStickXMin	605 
    #define LeftStickXAverage 2232
    #define LeftStickYMax	3590
    #define LeftStickYMin	377
    #define LeftStickYAverage 1983
    #define RightStickXMax	3764 
    #define RightStickXMin	642 
    #define RightStickXAverage 2203
    #define RightStickYMax	3776
    #define RightStickYMin	432 
    #define RightStickYAverage 2104

    // TODO : Maybe the game can offer a 'calibrate deadzone yourself' kind of option to the user?
    // For example, by making them to fully spin both sticks so that we can know what's the 
    // MIN/MAX values are.
    i32 RightStickX = (((u16)(Value9 & 0b00001111) << 8) | (u16)Value8);
    i32 RightStickY = (((u16)Value10) << 4) | ((Value6 >> 9) & 0b00001111);
    i32 LeftStickX = (((u16)(Value6 & 0b00001111) << 8) | (u16)Value5);
    i32 LeftStickY = (((u16)Value7) << 4) | ((Value6 >> 4) & 0b00001111);
    //printf("RightStickX : %u, RightStickY : %u\n", RightStickX, RightStickY);
    //printf("LeftStickX : %u, LeftStickY : %u\n", LeftStickX, LeftStickY);

    Controller->LeftStickX = MacOSGetProconStickValue(LeftStickX, LeftStickXMin, LeftStickXAverage, LeftStickXMax);
    Controller->LeftStickY = MacOSGetProconStickValue(LeftStickY, LeftStickYMin, LeftStickYAverage, LeftStickYMax);
    Controller->RightStickX = MacOSGetProconStickValue(RightStickX, RightStickXMin, RightStickXAverage, RightStickXMax);
    Controller->RightStickY = MacOSGetProconStickValue(RightStickY, RightStickYMin, RightStickYAverage, RightStickYMax);

    printf("LeftStickX : %.6f, LeftStickY : %.6f, RightStickX : %.6f, RightStickY : %.6f\n", 
            Controller->LeftStickX, Controller->LeftStickY, Controller->RightStickX, Controller->RightStickY);

    // NOTE : controller min/max getter
#if 0
    local_variable u16 RightXMin = 40000;
    local_variable u16 RightXMax = 0;
    local_variable u16 RightYMin = 40000;
    local_variable u16 RightYMax = 0;

    local_variable u16 LeftXMin = 40000;
    local_variable u16 LeftXMax = 0;
    local_variable u16 LeftYMin = 40000;
    local_variable u16 LeftYMax = 0;

    if(RightStickX > RightXMax)
    {
        RightXMax = RightStickX;
    }
    if(RightStickX < RightXMin)
    {
        RightXMin = RightStickX;
    }

    if(RightStickY > RightYMax)
    {
        RightYMax = RightStickY;
    }
    if(RightStickY < RightYMin)
    {
        RightYMin = RightStickY;
    }

    if(LeftStickX > LeftXMax)
    {
        LeftXMax = LeftStickX;
    }
    if(LeftStickX < LeftXMin)
    {
        LeftXMin = LeftStickX;
    }

    if(LeftStickY > LeftYMax)
    {
        LeftYMax = LeftStickY;
    }
    if(LeftStickY < LeftYMin)
    {
        LeftYMin = LeftStickY;
    }
#endif

    // TODO : Make all these bit shift as a seperate function 
    // so that anyone can easily understand what's going on here?
    Controller->AButton = (((ValueInInt << 12) >> 31) & 1);
    if(Controller->AButton)
        printf("AButton\n");
    Controller->BButton = (((ValueInInt << 13) >> 31) & 1);
    if(Controller->BButton)
        printf("BButton\n");
    Controller->XButton = (((ValueInInt << 14) >> 31) & 1);
    if(Controller->XButton)
        printf("XButton\n");
    Controller->YButton = (((ValueInInt << 15) >> 31) & 1);
    if(Controller->YButton)
        printf("YButton\n");
    Controller->MinusButton = (((ValueInInt << 7) >> 31) & 1);
    if(Controller->MinusButton)
        printf("MinusButton\n");
    Controller->PlusButton = (((ValueInInt << 6) >> 31) & 1);
    if(Controller->PlusButton)
        printf("PlusButton\n");
    Controller->HomeButton = (((ValueInInt << 3) >> 31) & 1);
    if(Controller->HomeButton)
        printf("HomeButton\n");
    Controller->CaptureButton = (((ValueInInt << 2) >> 31) & 1);
    if(Controller->CaptureButton)
        printf("CaptureButton\n");

    Controller->LeftShoulder = (((Value4 << 1) >> 7) & 1);
    if(Controller->LeftShoulder)
        printf("LButton\n");

    Controller->LeftTrigger = (((Value4 << 0) >> 7) & 1);
    if(Controller->LeftTrigger)
        printf("ZLButton\n");

    Controller->RightTrigger = (((ValueInInt << 8) >> 31) & 1);
    if(Controller->RightTrigger)
        printf("ZRButton\n");

    Controller->RightShoulder = (((ValueInInt << 9) >> 31) & 1);
    if(Controller->RightShoulder)
        printf("RButton\n");


    Controller->DPadUp = (((Value4 << 6) >> 7) & 1);
    if(Controller->DPadUp)
        printf("DPadUp\n");
    Controller->DPadDown = (((Value4 << 7) >> 7) & 1);
    if(Controller->DPadDown)
        printf("DPadDown\n");
    Controller->DPadLeft = (((Value4 << 4) >> 7) & 1);
    if(Controller->DPadLeft)
        printf("DPadLeft\n");
    Controller->DPadRight = (((Value4 << 5) >> 7) & 1);
    
}

MACOS_CALLBACK void
DeviceRemovalCallback(void *context, IOReturn result, void *sender)
{
    int a = 1;
}

MACOS_CALLBACK void 
ControllerConnectionDetected(void *Context, IOReturn Result, void *Sender, IOHIDDeviceRef Device)
{
    game_input *Input = (game_input *)Context;
    game_controller *Controller = Input->Controllers + Input->ControllerCount++;

    /*
       https://opensource.apple.com/source/IOHIDFamily/IOHIDFamily-315.7.16/IOHIDFamily/IOHIDUsageTables.h 
        NOTE : 
            kHIDPage_GenericDesktop = 0x01
            kHIDUsage_GD_Joystick = 0x04
            kHIDUsage_GD_GamePad = 0x05
    */
    CFTypeRef UsagePage = IOHIDDeviceGetProperty(Device, CFSTR(kIOHIDPrimaryUsagePageKey));
    CFTypeRef Usage = IOHIDDeviceGetProperty(Device, CFSTR(kIOHIDPrimaryUsageKey));

    if(IOHIDDeviceConformsTo(Device, kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick) == TRUE ||
        IOHIDDeviceConformsTo(Device, kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad) == TRUE)
    {
        CFStringRef ProductKey = (CFStringRef)IOHIDDeviceGetProperty(Device, CFSTR(kIOHIDProductKey));
        const char *ProducKeyInChar = CFStringGetCStringPtr(ProductKey, kCFStringEncodingUTF8);
        CFTypeRef Manufacturer = IOHIDDeviceGetProperty(Device, CFSTR(kIOHIDManufacturerKey));
        CFTypeRef SerialNumber = IOHIDDeviceGetProperty(Device, CFSTR(kIOHIDSerialNumberKey));

        if(strcmp(ProducKeyInChar, "Pro Controller") == 0)
        {
            IOHIDDeviceRegisterInputValueCallback(Device, MacOSProConInputDetected, Controller);  
            Controller->Connected = 1;
        }
        // TODO : These names are totally random for now, as I don't have the controllers..
        else if(strcmp(ProducKeyInChar, "Dual Shock") == 0)
        {
            IOHIDDeviceRegisterInputValueCallback(Device, MacOSDualShockInputDetected, Controller);  
            Controller->Connected = 1;
        }
        else if(strcmp(ProducKeyInChar, "Dual Sense") == 0)
        {
            IOHIDDeviceRegisterInputValueCallback(Device, MacOSDualSenseInputDetected, Controller);  
            Controller->Connected = 1;
        }
        else if(strcmp(ProducKeyInChar, "XBOX") == 0)
        {
            IOHIDDeviceRegisterInputValueCallback(Device, MacOSXBOXInputDetected, Controller);  
            Controller->Connected = 1;
        }


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
}

MACOS_CALLBACK void 
ControllerRemovalDetected(void *context, IOReturn result, void *sender, IOHIDDeviceRef device)
{
    // TODO : Maybe unschedule device from run loop here?
}

// TODO : Support for multiple controllers?
internal void
MacOSInitController(game_input *Input)
{
    HIDManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);

    // NOTE : ProCon falls inside the 'kHIDUsage_GD_Joystick' catergory.
    // TODO : For now, it does not detect xbox controller. Handle this case!
    IOHIDManagerSetDeviceMatchingMultiple(HIDManager,(__bridge CFArrayRef)
    @[
        @{@(kIOHIDDeviceUsagePageKey): @(kHIDPage_GenericDesktop), @(kIOHIDDeviceUsageKey): @(kHIDUsage_GD_GamePad)},
        @{@(kIOHIDDeviceUsagePageKey): @(kHIDPage_GenericDesktop), @(kIOHIDDeviceUsageKey): @(kHIDUsage_GD_Joystick)},
    ]);

    IOHIDManagerRegisterDeviceMatchingCallback(HIDManager, ControllerConnectionDetected, Input);
    IOHIDManagerRegisterDeviceRemovalCallback(HIDManager, ControllerRemovalDetected, Input);

    IOHIDManagerScheduleWithRunLoop(HIDManager,
                                   CFRunLoopGetCurrent(), // TODO : CFRunLoopGetCurrent vs CFRunLoopGetMain ?
                                    kCFRunLoopDefaultMode);
    IOHIDManagerOpen(HIDManager, kIOHIDOptionsTypeNone);

    CFSetRef Devices = IOHIDManagerCopyDevices(HIDManager);
}

internal void
ZeroAudio(game_audio_buffer *AudioBuffer)
{
    for(u32 SampleIndex = 0;
        SampleIndex < AudioBuffer->SampleCount;
        SampleIndex += AudioBuffer->ChannelCount)
    {
        AudioBuffer->Samples[SampleIndex] = 0;
        AudioBuffer->Samples[SampleIndex+1] = 0;
    }
}

MACOS_CALLBACK OSStatus
MacOSRenderAudio(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, 
                const AudioTimeStamp *inTimeStamp, 
                UInt32 inBusNumber, 
                UInt32 FrameCount, 
                AudioBufferList *BufferToFill)
{
    game_audio_buffer *SourceBuffer = (game_audio_buffer *)inRefCon;
    i16 *DestBuffer = (i16 *)BufferToFill->mBuffers[0].mData;
    u32 SamplesToFillCount = FrameCount*SourceBuffer->ChannelCount;

    // TODO : Still a bug at the start?
    if(SourceBuffer->IsSoundReady)
    {
        u32 FutureConsumedSampleIndex = SourceBuffer->ConsumedSampleIndex + SamplesToFillCount;
        u32 RunningSampleIndexInBuffer = SourceBuffer->RunningSampleIndex%SourceBuffer->SampleCount;

        if(SourceBuffer->ConsumedSampleIndex < RunningSampleIndexInBuffer)
        {
            if(FutureConsumedSampleIndex > RunningSampleIndexInBuffer)
            {
                SamplesToFillCount = RunningSampleIndexInBuffer - SourceBuffer->ConsumedSampleIndex;
            }
        }
        else if(SourceBuffer->ConsumedSampleIndex > RunningSampleIndexInBuffer)
        {
            if(FutureConsumedSampleIndex > RunningSampleIndexInBuffer + SourceBuffer->SampleCount)
            {
                SamplesToFillCount = RunningSampleIndexInBuffer + SourceBuffer->SampleCount - SourceBuffer->ConsumedSampleIndex;
            }
        }
        else
        {
            // TODO : If ConsumnedSampleIndex == RunningSampleIndex, what should we do here?
            SamplesToFillCount = 0;
            Assert("Invalid case");
        }

        for(u32 SampleIndex = 0;
            SampleIndex < SamplesToFillCount;
            SampleIndex += SourceBuffer->ChannelCount)
        {
            i16 SourceLeft = SourceBuffer->Samples[SourceBuffer->ConsumedSampleIndex++];
            i16 SourceRight = SourceBuffer->Samples[SourceBuffer->ConsumedSampleIndex++];
            
            DestBuffer[SampleIndex] = SourceLeft;
            DestBuffer[SampleIndex+1] = SourceRight;

            SourceBuffer->ConsumedSampleIndex %= SourceBuffer->SampleCount;
        }
    }
    else
    {
        for(u32 SampleIndex = 0;
            SampleIndex < SamplesToFillCount;
            SampleIndex += SourceBuffer->ChannelCount)
        {
            DestBuffer[SampleIndex] = 0;
            DestBuffer[SampleIndex+1] = 0;
        }   
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
        GameCode->FillAudioBuffer = (game_fill_audio_buffer *)dlsym(GameCode->Handle, "GameFillAudioBuffer");
        //Error = dlerror();
    }
    else
    {
        GameCode->UpdateAndRender = GameUpdateAndRenderStub;
        GameCode->FillAudioBuffer = GameFillAudioBufferStub;
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

    game_memory GameMemory = {};
    GameMemory.PermanentStorageSize = Megabytes(4);
    GameMemory.TransientStorageSize = Gigabytes(1);
    u64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize; 
    vm_allocate(mach_task_self(), 
                (vm_address_t *)&GameMemory.PermanentStorage,
                TotalSize, 
                VM_FLAGS_ANYWHERE);
    GameMemory.TransientStorage = (u8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;

    if(GameMemory.PermanentStorage && GameMemory.TransientStorage)
    {
        game_platform_api GamePlatformAPI = {};
        GamePlatformAPI.DEBUGReadEntireFile = DEBUGMacOSReadFile;
        GamePlatformAPI.DEBUGWriteEntireFile = DEBUGMacOSWriteEntireFile;
        GamePlatformAPI.DEBUGFreeFileMemory = DEBUGMacOSFreeFileMemory;
        // TODO : There are many allocation methods. Make sure this one is the fastest - or does it matter?

        game_audio_buffer GameAudioBuffer = {};
        GameAudioBuffer.SamplesPerSecond = 48000;
        GameAudioBuffer.ChannelCount = 2;
        // NOTE : Audio Buffer is 3 seconds worth for now. 
        GameAudioBuffer.SampleCount = 1 * GameAudioBuffer.SamplesPerSecond * GameAudioBuffer.ChannelCount;
        GameAudioBuffer.Samples = (i16 *)malloc(sizeof(i16) * GameAudioBuffer.SampleCount);

        ZeroAudio(&GameAudioBuffer);
        MacOSInitAudio(&GameAudioBuffer);

        // TODO : Unlike Win32, I cannot use GetState() to Get the state of controller - macos controlls these stuff.
        // Therefore, I cannot use the oldInput - newInput swap scheme like handmade hero. Maybe put a indicator inside
        // the game_input struct so that I can know which game_input to use inside the controller input callback?
        game_input Input[2] = {};
        game_input *NewInput = Input;
        game_input *OldInput = Input + 1;

        MacOSInitController(Input);

        struct mach_timebase_info MachTimeInfo;
        mach_timebase_info(&MachTimeInfo);
        r32 NanoSecondPerTick = ((r32)MachTimeInfo.numer/(r32)MachTimeInfo.denom);

        r32 TargetSecondsPerFrame = 1/30.0f;
        u32 TargetNanoSecondsPerFrame = (u32)(TargetSecondsPerFrame*SecToNanoSec);

        macos_offscreen_buffer MacOSBuffer = {};
        MacOSBuffer.Width = 1920;
        MacOSBuffer.Height = 1080;
        MacOSBuffer.BytesPerPixel = 4;
        MacOSBuffer.Pitch = MacOSBuffer.Width * MacOSBuffer.BytesPerPixel;
        MacOSBuffer.Memory = malloc(MacOSBuffer.Pitch * MacOSBuffer.Height); // TODO : Put this inside the unified game memory?

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

        IsGameRunning = true;

        u64 BeginTime = 0;

        [App activateIgnoringOtherApps:YES];

        // NOTE : Here's the game loop
        while(IsGameRunning)
        {
            int DynamicLibraryLock = open("DynamicLibraryLockPath", O_RDONLY);
            if(DynamicLibraryLock != 0)
            {
                MacOSGetGameCode(&GameCode, DynamicLibraryPath);
            }

            MacOSHandleEvents(App);

            game_offscreen_buffer GameOffscreenBuffer = {};
            GameOffscreenBuffer.Width = MacOSBuffer.Width;
            GameOffscreenBuffer.Height = MacOSBuffer.Height;
            GameOffscreenBuffer.BytesPerPixel = MacOSBuffer.BytesPerPixel;
            GameOffscreenBuffer.Pitch = MacOSBuffer.Pitch;
            GameOffscreenBuffer.Memory = MacOSBuffer.Memory;

            GameCode.UpdateAndRender(&GameOffscreenBuffer, &GameMemory, &GamePlatformAPI, TargetSecondsPerFrame);
            GameCode.FillAudioBuffer(&GameAudioBuffer, TargetSecondsPerFrame);

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
            // TODO : More precise frame rate
            printf("%f\n", MachTimeDifferenceToNanoSecond(BeginTime, EndTime, NanoSecondPerTick)/SecToNanoSec);

            BeginTime = EndTime;
        }
    }

    return 0;
}
