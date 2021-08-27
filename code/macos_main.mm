/*
    Here are the most basic things to do inside the MACOS platform layer.
    Framerate control(MORE PRECISE ONE!!)

    Sound

    more robust input handling. keyboard, mouse..

    input recording
*/

#include "monsoon_platform_independent.h"
#include "monsoon.cpp"
#include "macos_keycode.h"

// NOTE : Cocoa.h is already using internal.. so I need to do this :(
#include <mach/thread_info.h>
#undef internal 
#include <Cocoa/Cocoa.h> // APPKIT
#include <mach/mach_time.h> // mach_absolute_time
#include <stdio.h> // printf for debugging purpose
#include <sys/stat.h>
#include <OpenGL/gl.h> // Every opengl related functions
#include <dlfcn.h>  // dlopen, dlsym for loading dylibs
#include <IOKit/hid/IOHIDLib.h> //IOHIDManager, IOHIDElement
#include <AudioUnit/AudioUnit.h> 


#define internal static
#define MACOS_CALLBACK static

#include "macos_support.cpp"

global_variable b32 isGamerunning;
global_variable IOHIDManagerRef HIDManager; // TODO : Any way to get rid of this global variable?
global_variable b32 isFullScreen;

internal u64 
MachTimeDifferenceToNanoSecond(u64 BeginMachTime, u64 EndMachTime, r32 NanoSecondPerTick)
{
    return (u64)(((EndMachTime - BeginMachTime)*NanoSecondPerTick));
}
internal u64 
MachTimeDifferenceInMicroSecond(u64 BeginMachTime, u64 EndMachTime, r32 NanoSecondPerTick)
{
    return (u64)(((EndMachTime - BeginMachTime)*NanoSecondPerTick));
}

internal void
CopyPermanentStorage(memory_arena *dest, memory_arena *source)
{
    // NOTE : We also need to copy the world arena,
    // If we don't, we will lose all the entity information
    dest->used = source->used;
    dest->totalSize = source->totalSize;
    dest->temporaryMemoryCount = source->temporaryMemoryCount;
    memcpy((void *)dest->base, 
            (void *)source->base,
            source->used);
}

internal void
MacOShandleEvents(NSApplication *App, game_input_manager *inputManager, debug_game_input_record *DEBUGInputRecord, 
                game_state *gameState)
{
    game_input_raw *newInput = inputManager->rawInputs + inputManager->newInputIndex;
    game_controller *keyboardController = newInput->controllers + 0;

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
                case NSEventTypeKeyUp:
                case NSEventTypeKeyDown:
                {
                    b32 WasDown = Event.ARepeat;
                    b32 isDown = ([Event type] == NSEventTypeKeyDown);

                    if((isDown != WasDown) || !isDown)
                    {
                        //printf("isDown : %d, WasDown : %d", isDown, WasDown);
                        u16 KeyCode = [Event keyCode];
                        if(KeyCode == MACOS_Escape)
                        {
                            isGamerunning = false;
                        }
                        else if(KeyCode == MACOS_W)
                        {
                            keyboardController->moveUp = isDown;
                        }
                        else if(KeyCode == MACOS_A)
                        {
                            keyboardController->moveLeft = isDown;
                        }
                        else if(KeyCode == MACOS_S)
                        {
                            keyboardController->moveDown = isDown;
                        }
                        else if(KeyCode == MACOS_D)
                        {
                            keyboardController->moveRight = isDown;
                        }
                        else if(KeyCode == MACOS_Space)
                        {
                            keyboardController->AButton = isDown;
                        }
                        else if(KeyCode == MACOS_B)
                        {
                            keyboardController->BButton = isDown;
                        }
                        else if(KeyCode == MACOS_N)
                        {
                            keyboardController->YButton = isDown;
                        }
                        else if(KeyCode == MACOS_F10)
                        {
                            if(isDown)
                            {
                                // NOTE : Start input recording

                                DEBUGInputRecord->playIndex = 0;
                                DEBUGInputRecord->playIndexCount = 0;

                                DEBUGInputRecord->isRecording = true;
                            }
                        }
                        else if(KeyCode == MACOS_F11)
                        {
                            // NOTE : End input recording
                            DEBUGInputRecord->isRecording = false;
                        }
                        else if(KeyCode == MACOS_F12)
                        {
                            if(isDown)
                            {
                                if(DEBUGInputRecord->isPlaying == false)
                                {
                                    Assert(DEBUGInputRecord->inputMemory && DEBUGInputRecord->permanentStorage);

                                    // NOTE : play & Stop input recording
                                    DEBUGInputRecord->playIndexCount = DEBUGInputRecord->playIndex;
                                    DEBUGInputRecord->playIndex = 0;

                                    DEBUGInputRecord->isPlaying = true;

                                }
                                else
                                {
                                    DEBUGInputRecord->isPlaying = false;
                                    DEBUGInputRecord->playIndex = 0;
                                }
                            }
                        }
                        b32 AltWasDown = false;
                        if(KeyCode == MACOS_Option)
                        {
                            AltWasDown = true;
                        }

                        if(KeyCode == MACOS_Return)
                        {
                            if(isDown)
                            {
                                NSWindow *Window = [Event window];
                                // TODO : proper buffer resize here!
                                [Window toggleFullScreen:0];
                                isFullScreen = !isFullScreen;
                            }
                        }
                    }
                }break;

                default:
                {
                    [App sendEvent : Event];
                }
            }
        }
        else
        {
            break;
        }
    }
}

DEBUG_PLATFORM_READ_FILE(DEBUGMacOSReadFile)
{
    debug_platform_read_file_result Result = {};

    int File = open(fileName, O_RDONLY);
    int Error = errno;
    if(File >= 0) // NOTE : If the open() succeded, the return value is non-negative value.
    {
        struct stat FileStat;
        fstat(File , &FileStat); 
        off_t fileSize = FileStat.st_size;

        if(fileSize > 0)
        {
            // TODO : NO MORE OS LEVEL ALLOCATION!
            // Pass this function a memory that it can use so that the function can only
            // read from it.
            Result.size = fileSize;
            Result.memory = malloc(Result.size);
            if(read(File, Result.memory, fileSize) == -1)
            {
                free(Result.memory);
                Result.size = 0;
            }
        }

        close(File);
    }

    return Result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGMacOSWriteEntireFile)
{
    // NOTE : This call will fail if the file already exists.
    int File = open(fileNameToCreate, O_WRONLY|O_CREAT|O_EXCL, S_IRWXU);
    if(File >= 0) // NOTE : If the open() succeded, the return value is non-negative value.
    {
        if(write(File, memoryToWrite, fileSize) == -1)
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
    free(memoryToFree);
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
    // TODO : Not a particularly good deadzone handling, but as long as I don't get the
    // official document from nintendo, this will  stay this way.
#define  PROCON_DEADZONE 0.1f

    r32 Result = 0.0f;
    Result = 2.0f*((r32)(StickValue - StickAverage)/(StickMax - StickMin));

    if(Result < -1.0f)
    {
        Result = -1.0f;
    }
    else if(Result > 1.0f)
    {
        Result = 1.0f;
    }
    else if(Result < PROCON_DEADZONE && Result > -PROCON_DEADZONE)
    {
        Result = 0.0f;
    }

    return Result;
}

MACOS_CALLBACK void
MacOSProConInputDetected(void *Context, IOReturn Result, void *Sender, IOHIDValueRef Value)
{
    game_input_manager *inputManager = (game_input_manager *)Context;
    game_input_raw *newInput = inputManager->rawInputs + inputManager->newInputIndex;
    game_controller *controller = newInput->controllers + (newInput->controllerCount - 1);

    IOHIDElementRef Element = IOHIDValueGetElement(Value);

    const uint8_t *ValueInByte = IOHIDValueGetBytePtr(Value);

    /*
        NOTE : 
       .memory Order
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

    #define leftStickXMax	3860
    #define leftStickXMin	605 
    #define leftStickXAverage 2232
    #define leftStickYMax	3590
    #define leftStickYMin	377
    #define leftStickYAverage 1983
    #define rightStickXMax	3764 
    #define rightStickXMin	642 
    #define rightStickXAverage 2203
    #define rightStickYMax	3776
    #define rightStickYMin	432 
    #define rightStickYAverage 2104

    // TODO : Maybe the game can offer a 'calibrate deadzone yourself' kind of option to the user?
    // For example, by making them to fully spin both sticks so that we can know what's the 
    // MIN/MAX values are.
    i32 rightStickX = (((u16)(Value9 & 0b00001111) << 8) | (u16)Value8);
    i32 rightStickY = (((u16)Value10) << 4) | ((Value6 >> 9) & 0b00001111);
    i32 leftStickX = (((u16)(Value6 & 0b00001111) << 8) | (u16)Value5);
    i32 leftStickY = (((u16)Value7) << 4) | ((Value6 >> 4) & 0b00001111);

    controller->leftStickX = MacOSGetProconStickValue(leftStickX, leftStickXMin, leftStickXAverage, leftStickXMax);
    controller->leftStickY = MacOSGetProconStickValue(leftStickY, leftStickYMin, leftStickYAverage, leftStickYMax);
    controller->rightStickX = MacOSGetProconStickValue(rightStickX, rightStickXMin, rightStickXAverage, rightStickXMax);
    controller->rightStickY = MacOSGetProconStickValue(rightStickY, rightStickYMin, rightStickYAverage, rightStickYMax);

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

    if(rightStickX > RightXMax)
    {
        RightXMax = rightStickX;
    }
    if(rightStickX < RightXMin)
    {
        RightXMin = rightStickX;
    }

    if(rightStickY > RightYMax)
    {
        RightYMax = rightStickY;
    }
    if(rightStickY < RightYMin)
    {
        RightYMin = rightStickY;
    }

    if(leftStickX > LeftXMax)
    {
        LeftXMax = leftStickX;
    }
    if(leftStickX < LeftXMin)
    {
        LeftXMin = leftStickX;
    }

    if(leftStickY > LeftYMax)
    {
        LeftYMax = leftStickY;
    }
    if(leftStickY < LeftYMin)
    {
        LeftYMin = leftStickY;
    }
#endif

    // TODO : Make all these bit shift as a seperate function 
    // so that anyone can easily understand what's going on here?
    controller->AButton = (((ValueInInt << 12) >> 31) & 1);
    if(controller->AButton)
        printf("AButton\n");
    controller->BButton = (((ValueInInt << 13) >> 31) & 1);
    if(controller->BButton)
        printf("BButton\n");
    controller->XButton = (((ValueInInt << 14) >> 31) & 1);
    if(controller->XButton)
        printf("XButton\n");
    controller->YButton = (((ValueInInt << 15) >> 31) & 1);
    if(controller->YButton)
        printf("YButton\n");
    controller->minusButton = (((ValueInInt << 7) >> 31) & 1);
    if(controller->minusButton)
        printf("minusButton\n");
    controller->plusButton = (((ValueInInt << 6) >> 31) & 1);
    if(controller->plusButton)
        printf("plusButton\n");
    controller->homeButton = (((ValueInInt << 3) >> 31) & 1);
    if(controller->homeButton)
        printf("homeButton\n");
    controller->captureButton = (((ValueInInt << 2) >> 31) & 1);
    if(controller->captureButton)
        printf("captureButton\n");

    controller->leftShoulder = (((Value4 << 1) >> 7) & 1);
    if(controller->leftShoulder)
        printf("LButton\n");

    controller->leftTrigger = (((Value4 << 0) >> 7) & 1);
    if(controller->leftTrigger)
        printf("ZLButton\n");

    controller->rightTrigger = (((ValueInInt << 8) >> 31) & 1);
    if(controller->rightTrigger)
        printf("ZRButton\n");

    controller->rightShoulder = (((ValueInInt << 9) >> 31) & 1);
    if(controller->rightShoulder)
        printf("RButton\n");


    controller->DPadUp = (((Value4 << 6) >> 7) & 1);
    if(controller->DPadUp)
        printf("DPadUp\n");
    controller->DPadDown = (((Value4 << 7) >> 7) & 1);
    if(controller->DPadDown)
        printf("DPadDown\n");
    controller->DPadLeft = (((Value4 << 4) >> 7) & 1);
    if(controller->DPadLeft)
        printf("DPadLeft\n");
    controller->DPadRight = (((Value4 << 5) >> 7) & 1);
    
}

MACOS_CALLBACK void
DeviceRemovalCallback(void *context, IOReturn result, void *sender)
{
    int a = 1;
}

MACOS_CALLBACK void 
ControllerConnected(void *Context, IOReturn Result, void *Sender, IOHIDDeviceRef Device)
{
    game_input_manager *inputManager = (game_input_manager *)Context;
    game_input_raw *newInput = inputManager->rawInputs + inputManager->newInputIndex;
    game_controller *controller = newInput->controllers + newInput->controllerCount++;

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
            IOHIDDeviceRegisterInputValueCallback(Device, MacOSProConInputDetected, inputManager);  
            controller->isAnalog = 1;
        }
        // TODO : These names are totally random for now, as I don't have the controllers..
        else if(strcmp(ProducKeyInChar, "Dual Shock") == 0)
        {
            IOHIDDeviceRegisterInputValueCallback(Device, MacOSDualShockInputDetected, inputManager);  
            controller->isAnalog = 1;
        }
        else if(strcmp(ProducKeyInChar, "Dual Sense") == 0)
        {
            IOHIDDeviceRegisterInputValueCallback(Device, MacOSDualSenseInputDetected, inputManager);  
            controller->isAnalog = 1;
        }
        else if(strcmp(ProducKeyInChar, "XBOX") == 0)
        {
            IOHIDDeviceRegisterInputValueCallback(Device, MacOSXBOXInputDetected, inputManager);  
            controller->isAnalog = 1;
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
MacOSInitController(game_input_manager *inputManager)
{
    HIDManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);

    // NOTE : ProCon falls inside the 'kHIDUsage_GD_Joystick' catergory.
    // TODO : For now, it does not detect xbox controller. handle this case!
    IOHIDManagerSetDeviceMatchingMultiple(HIDManager,(__bridge CFArrayRef)
    @[
        @{@(kIOHIDDeviceUsagePageKey): @(kHIDPage_GenericDesktop), @(kIOHIDDeviceUsageKey): @(kHIDUsage_GD_GamePad)},
        @{@(kIOHIDDeviceUsagePageKey): @(kHIDPage_GenericDesktop), @(kIOHIDDeviceUsageKey): @(kHIDUsage_GD_Joystick)},
    ]);

    IOHIDManagerRegisterDeviceMatchingCallback(HIDManager, ControllerConnected, inputManager);
    IOHIDManagerRegisterDeviceRemovalCallback(HIDManager, ControllerRemovalDetected, inputManager);

    IOHIDManagerScheduleWithRunLoop(HIDManager,
                                   CFRunLoopGetCurrent(), // TODO : CFRunLoopGetCurrent vs CFRunLoopGetMain ?
                                    kCFRunLoopDefaultMode);
    IOHIDManagerOpen(HIDManager, kIOHIDOptionsTypeNone);

    CFSetRef Devices = IOHIDManagerCopyDevices(HIDManager);
}

internal void
ZeroAudio(game_audio_buffer *AudioBuffer)
{
    for(u32 sampleIndex = 0;
        sampleIndex < AudioBuffer->sampleCount;
        sampleIndex += AudioBuffer->channelCount)
    {
        AudioBuffer->samples[sampleIndex] = 0;
        AudioBuffer->samples[sampleIndex+1] = 0;
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
    u32 samplesToFillCount = FrameCount*SourceBuffer->channelCount;

    // TODO : Still a bug at the start?
    if(SourceBuffer->isSoundReady)
    {
        u32 FutureconsumedSampleIndex = SourceBuffer->consumedSampleIndex + samplesToFillCount;
        u32 runningSampleIndexInBuffer = SourceBuffer->runningSampleIndex%SourceBuffer->sampleCount;

        if(SourceBuffer->consumedSampleIndex < runningSampleIndexInBuffer)
        {
            if(FutureconsumedSampleIndex > runningSampleIndexInBuffer)
            {
                samplesToFillCount = runningSampleIndexInBuffer - SourceBuffer->consumedSampleIndex;
            }
        }
        else if(SourceBuffer->consumedSampleIndex > runningSampleIndexInBuffer)
        {
            if(FutureconsumedSampleIndex > runningSampleIndexInBuffer + SourceBuffer->sampleCount)
            {
                samplesToFillCount = runningSampleIndexInBuffer + SourceBuffer->sampleCount - SourceBuffer->consumedSampleIndex;
            }
        }
        else
        {
            // TODO : If ConsumnedsampleIndex == runningSampleIndex, what should we do here?
            samplesToFillCount = 0;
            Assert("Invalid case");
        }

        for(u32 sampleIndex = 0;
            sampleIndex < samplesToFillCount;
            sampleIndex += SourceBuffer->channelCount)
        {
            i16 SourceLeft = SourceBuffer->samples[SourceBuffer->consumedSampleIndex++];
            i16 SourceRight = SourceBuffer->samples[SourceBuffer->consumedSampleIndex++];
            
            DestBuffer[sampleIndex] = SourceLeft;
            DestBuffer[sampleIndex+1] = SourceRight;

            SourceBuffer->consumedSampleIndex %= SourceBuffer->sampleCount;
        }
    }
    else
    {
        for(u32 sampleIndex = 0;
            sampleIndex < samplesToFillCount;
            sampleIndex += SourceBuffer->channelCount)
        {
            DestBuffer[sampleIndex] = 0;
            DestBuffer[sampleIndex+1] = 0;
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
    AudioStreamDescription.mBitsPerChannel = 8*sizeof(AudioBuffer->samples[0]);
    Assert(AudioStreamDescription.mBitsPerChannel == 16);
    AudioStreamDescription.mChannelsPerFrame = AudioBuffer->channelCount;
    AudioStreamDescription.mFramesPerPacket = 1;
    AudioStreamDescription.mBytesPerFrame = AudioBuffer->channelCount * sizeof(AudioBuffer->samples[0]);
    AudioStreamDescription.mSampleRate = AudioBuffer->samplesPerSecond;
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
MacOSGetFileModifiedTime(char *fileName)
{
    u64 Result = 0;

    int File = open(fileName, O_RDONLY);

    struct stat FileStat;
    fstat(File , &FileStat); 
    Result = FileStat.st_mtime;

    close(File);

    return Result;
}


internal void
MacOSGetGameCode(game_code *GameCode, char *fileName)
{
    if(GameCode->handle)
    {
        u64 lastModifiedTime = MacOSGetFileModifiedTime(fileName);
        if(lastModifiedTime > GameCode->lastModifiedTime)
        {
            GameCode->lastModifiedTime = lastModifiedTime;
            dlclose(GameCode->handle);
        }
        else
        {
            return;
        }
    }

    GameCode->handle = dlopen(fileName, RTLD_LAZY);

    //char *Error = dlerror();
    if(GameCode->handle)
    {
        GameCode->UpdateAndRender = (game_update_and_render *)dlsym(GameCode->handle, "GameUpdateAndRender");
        GameCode->FillAudioBuffer = (game_fill_audio_buffer *)dlsym(GameCode->handle, "GameFillAudioBuffer");
        //Error = dlerror();
    }
    else
    {
        GameCode->UpdateAndRender = GameUpdateAndRenderStub;
        GameCode->FillAudioBuffer = GameFillAudioBufferStub;
        GameCode->lastModifiedTime = 0;
    }
}


int main(int argc, char **argv)
{
    // TODO : How to 'build' a path that works no matter where the library is.
    char *DynamicLibraryPath = "/Volumes/work/build/fox.app/Contents/Resources/monsoon.dylib";
    char *DynamicLibraryLockPath = "/Volumes/work/build/fox.app/Contents/Resources/monsoon.lock";
    game_code GameCode = {};
    MacOSGetGameCode(&GameCode, DynamicLibraryPath);

    game_memory GameMemory = {};
    GameMemory.permanentStorageSize = Megabytes(512);
    GameMemory.transientStorageSize = Gigabytes(1);
    u64 totalSize = GameMemory.permanentStorageSize + GameMemory.transientStorageSize; 
    // NOTE : vm_allocate will always clear the memory to zero. Great!
    vm_allocate(mach_task_self(), 
                (vm_address_t *)&GameMemory.permanentStorage,
                totalSize, 
                VM_FLAGS_ANYWHERE);
    GameMemory.transientStorage = (u8 *)GameMemory.permanentStorage + GameMemory.permanentStorageSize;

    if(GameCode.UpdateAndRender && GameCode.FillAudioBuffer &&
        GameMemory.permanentStorage && GameMemory.transientStorage)
    {
        game_platform_api GameplatformAPI = {};
        GameplatformAPI.DEBUGReadEntireFile = DEBUGMacOSReadFile;
        GameplatformAPI.DEBUGWriteEntireFile = DEBUGMacOSWriteEntireFile;
        GameplatformAPI.DEBUGFreeFileMemory = DEBUGMacOSFreeFileMemory;
        // TODO : There are many allocation methods. Make sure this one is the fastest - or does it matter?

        game_audio_buffer GameAudioBuffer = {};
        GameAudioBuffer.samplesPerSecond = 48000;
        GameAudioBuffer.channelCount = 2;
        // NOTE : Audio Buffer is 3 seconds worth for now. 
        GameAudioBuffer.sampleCount = 1 * GameAudioBuffer.samplesPerSecond * GameAudioBuffer.channelCount;
        GameAudioBuffer.samples = (i16 *)malloc(sizeof(i16) * GameAudioBuffer.sampleCount);

        ZeroAudio(&GameAudioBuffer);
        MacOSInitAudio(&GameAudioBuffer);

        // TODO : Unlike Win32, I cannot use GetState() to Get the state of controller - macos controlls these stuff.
        // Therefore, I cannot use the oldInput - newInput swap scheme like handmade hero. Maybe put a indicator inside
        // the game_input struct so that I can know which game_input to use inside the controller input callback?

        game_input_manager inputManager = {};
        inputManager.newInputIndex = 0;
        // NOTE : 0th controller is keyboard controller
        inputManager.rawInputs[0].controllerCount = 1;
        inputManager.rawInputs[1].controllerCount = 1;
        MacOSInitController(&inputManager);

        struct mach_timebase_info MachTimeInfo;
        mach_timebase_info(&MachTimeInfo);
        r32 NanoSecondPerTick = ((r32)MachTimeInfo.numer/(r32)MachTimeInfo.denom);

        u32 TargetFramesPerSecond = 30;
        r32 TargetSecondsPerFrame = 1.0f/(r32)TargetFramesPerSecond;
        u32 TargetNanoSecondsPerFrame = (u32)(TargetSecondsPerFrame*SecToNanoSec);

        debug_game_input_record DEBUGInputRecord = {};
        DEBUGInputRecord.maxPlayIndexCount = TargetFramesPerSecond * 20;
        DEBUGInputRecord.inputMemory = malloc(sizeof(game_input_raw)*DEBUGInputRecord.maxPlayIndexCount);
        DEBUGInputRecord.permanentStorage = malloc(GameMemory.permanentStorageSize);

        macos_offscreen_buffer MacOSBuffer = {};
        MacOSBuffer.width = 1920;
        MacOSBuffer.height = 1080;
        MacOSBuffer.bytesPerPixel = 4;
        MacOSBuffer.pitch = MacOSBuffer.width * MacOSBuffer.bytesPerPixel;
        MacOSBuffer.memory = malloc(MacOSBuffer.pitch * MacOSBuffer.height); // TODO : Put this inside the unified game memory?

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
        // be half the size at the first playe. Maybe this is because of the high resolution monitor?
        NSRect WindowFrameRect = NSMakeRect(100.0f, 100.0f, (float)MacOSBuffer.width/2, (float)MacOSBuffer.height/2);

        NSWindow *Window = [[NSWindow alloc] initWithContentRect : WindowFrameRect
                                                // Apple window styles : https://developer.apple.com/documentation/appkit/nswindow/stylemask
                                                styleMask : NSTitledWindowMask|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable
                                                backing : NSBackingStoreBuffered 
                                                defer : NO];

        NSString *AppName = [[NSProcessInfo processInfo]processName];
        [Window setTitle:AppName];
        [Window makeKeyAndOrderFront:0];

        macos_opengl_info MacOSOpenGLInfo = {};
        MacOSOpenGLInfo = PrepareDisplayingWithOpenGL(Window, &WindowFrameRect, MacOSBuffer.width, MacOSBuffer.height, MacOSBuffer.memory); 

        [App activateIgnoringOtherApps:YES];

        u64 beginTime = mach_absolute_time();
        isGamerunning = true;
        // NOTE : Here's the game loop
        while(isGamerunning)
        {
            game_state *gameState = (game_state *)(GameMemory.permanentStorage);
            game_input_raw *newInput = inputManager.rawInputs + inputManager.newInputIndex;

            // TODO : Use copy function instead!
            int DynamicLibraryLock = open("DynamicLibraryLockPath", O_RDONLY);
            if(DynamicLibraryLock != 0)
            {
                MacOSGetGameCode(&GameCode, DynamicLibraryPath);
                close(DynamicLibraryLock);
            }

            MacOShandleEvents(App, &inputManager, &DEBUGInputRecord, gameState);

            if(DEBUGInputRecord.isRecording)
            {
                // NOTE : Just started recording
                if(DEBUGInputRecord.playIndex == 0)
                {
                    memcpy(DEBUGInputRecord.permanentStorage, 
                            GameMemory.permanentStorage, 
                            GameMemory.permanentStorageSize);
                }
                game_input_raw *RecordedInputs = (game_input_raw *)DEBUGInputRecord.inputMemory;
                                                                        
                game_input_raw *CurrentInputRecord = RecordedInputs + DEBUGInputRecord.playIndex++;
                *CurrentInputRecord = *(inputManager.rawInputs + inputManager.newInputIndex);

                if(DEBUGInputRecord.playIndex == DEBUGInputRecord.maxPlayIndexCount)
                {
                    Assert(0);
                }
            }

            game_offscreen_buffer GameOffscreenBuffer = {};
            GameOffscreenBuffer.width = MacOSBuffer.width;
            GameOffscreenBuffer.height = MacOSBuffer.height;
            GameOffscreenBuffer.bytesPerPixel = MacOSBuffer.bytesPerPixel;
            GameOffscreenBuffer.pitch = MacOSBuffer.pitch;
            GameOffscreenBuffer.memory = MacOSBuffer.memory;

            if(DEBUGInputRecord.isPlaying)
            {
                // NOTE : Record playing just started
                if(DEBUGInputRecord.playIndex == 0)
                {
                    memcpy(GameMemory.permanentStorage, 
                            DEBUGInputRecord.permanentStorage, 
                            GameMemory.permanentStorageSize);
                }
                
                game_input_raw *RecordedInputs = (game_input_raw *)DEBUGInputRecord.inputMemory;
                game_input_raw *CurrentInputRecord = RecordedInputs + DEBUGInputRecord.playIndex++; 
                *newInput = *CurrentInputRecord;

                if(DEBUGInputRecord.playIndex == DEBUGInputRecord.playIndexCount)
                {
                    DEBUGInputRecord.playIndex = 0;
                }
            }
            if(GameCode.UpdateAndRender)
            {
                GameCode.UpdateAndRender(&GameOffscreenBuffer, &GameMemory, newInput, &GameplatformAPI, TargetSecondsPerFrame);
                for(u32 cycleCounterIndex = 0;
                    cycleCounterIndex < DEBUGCycleCounter_Count;
                    ++cycleCounterIndex)
                {
                    debug_game_cycle_counter *cycleCounter = GameMemory.DEBUGCycleCounters + cycleCounterIndex;
                    printf("%u, cycles : %llu, hits : %u\n", cycleCounterIndex, cycleCounter->cycleCount, cycleCounter->hitCount);

                    cycleCounter->cycleCount = 0;
                    cycleCounter->hitCount = 0;
                }
                
                printf("\n");
            }
            if(GameCode.FillAudioBuffer)
            {
                GameCode.FillAudioBuffer(&GameAudioBuffer, TargetSecondsPerFrame);
            }

            // Rendering loop with OpenGL.
            {
                glClear(GL_COLOR_BUFFER_BIT);
                glBindTexture(GL_TEXTURE_2D, MacOSOpenGLInfo.TextureID);
                glTexImage2D(GL_TEXTURE_2D, 0, 
                            GL_RGBA8, 
                            MacOSBuffer.width, MacOSBuffer.height,
                            0, 
                            GL_BGRA, 
                            GL_UNSIGNED_INT_8_8_8_8_REV, 
                            MacOSBuffer.memory);
                if(!isFullScreen)
                {
                    glViewport(0, 0, MacOSBuffer.width, MacOSBuffer.height);
                }
                else
                {
                    // TODO : This does not give proper display size?
                    CGDirectDisplayID DisplayID = CGMainDisplayID();
                    u32 Displaywidth = CGDisplayPixelsWide(DisplayID);
                    u32 Displayheight = CGDisplayPixelsHigh(DisplayID);

                    glViewport(0, 0, Displaywidth, Displayheight);
                }


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

            // NOTE : because nanosleep is so accurate, we have to UNDERSLEEP by some amount,
            // and then spin inside the while loop to get the accurate page sleep
            // TODO : How to decide this offset?
            u64 endTime = mach_absolute_time();
            u64 TimeDifferenceInNanoSecond = MachTimeDifferenceToNanoSecond(beginTime, endTime, NanoSecondPerTick);
            u64 UnderSleepOffset = 1100000;
#if 1
            if(TimeDifferenceInNanoSecond < TargetNanoSecondsPerFrame)
            {
                timespec TimeSpec = {};
                TimeSpec.tv_nsec = TargetNanoSecondsPerFrame - TimeDifferenceInNanoSecond - UnderSleepOffset;

                nanosleep(&TimeSpec, 0);
            }
            else
            {
                // TODO : Missed Frame!
            }

            while(TimeDifferenceInNanoSecond < TargetNanoSecondsPerFrame)
            {
                endTime = mach_absolute_time();
                TimeDifferenceInNanoSecond = MachTimeDifferenceToNanoSecond(beginTime, endTime, NanoSecondPerTick);
            }
#else
            if(TimeDifferenceInNanoSecond)
            {
            }
#endif

            //inputManager.newInputIndex = inputManager.newInputIndex == 0? 1 : 0;

            endTime = mach_absolute_time();
            // TODO : More precise frame rate
            //printf("%f\n", (r32)(MachTimeDifferenceToNanoSecond(beginTime, endTime, NanoSecondPerTick))/SecToNanoSec);
            beginTime = endTime;

        }
    }

    return 0;
}
