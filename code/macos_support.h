#ifndef MACOS_SUPPORT_H
#define MACOS_SUPPORT_H

struct macos_offscreen_buffer
{
    u32 Width;
    u32 Height;
    u32 Pitch;

    u32 BytesPerPixel;
    void *Memory;
};

struct macos_opengl_info
{
    GLuint TextureID;
    NSOpenGLContext *OpenGLContext;
};

/*
 * Here is how we manage the game controllers and game pads
 * 1. 'Create' IOHIDManager
 * 2. To find the devices we want to support, we create a dictionary and set the values to 'gamepad' and 'joystick'
 *      procon - joystick
 *      xbox controller - ?
 *      dualshock - ?
 *      dualsense - ?
 * 3. Set callback functions which will be called when the devices get connected or disconnected
 * 4. Schedule the IOHIDManager with our run loop
 *      CFRUnLoopGetCurrent() or CFRunLoopGetMain?
 *  5. Open the IOHIDManager, which will get the current & future HIDevices(in our case, the gmaepad and joystick)
 * */

// NOTE : app delegate is mianly for applicationDidFinishLaunching function,
// which we are using to go inside our game loop even after the [App Run] call.
@interface fox_app_delegate : NSObject <NSApplicationDelegate>
//[Window setDelegate:appDelegate]; // TODO : Do we really need this?
@end

#endif
