#ifndef MACOS_SUPPORT_H
#define MACOS_SUPPORT_H

struct macos_offscreen_buffer
{
    u32 width;
    u32 height;
    u32 pitch;

    u32 bytesPerPixel;
    void *memory;
};

struct macos_opengl_info
{
    GLuint TextureID;
    NSOpenGLContext *OpenGLContext;
};

// TODO : I can make the game to resize when the user clicks the edge of the window,
// but it requires whole window delegate kind of thing that I really don't want to deal with.
// Maybe make the game to only support certain resolutions which can be selected by the user?

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

#endif
