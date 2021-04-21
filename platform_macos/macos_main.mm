/*
    OKAY, here are the most basic things to do inside the MACOS platform layer.

    1. Gamepad input
    2. Sound
    3. more robust input handling. keyboard, mouse..
    4. unified game memory
    5. DEBUG file I/O
    6. Framerate control
    7. live code editing
    8. input recording

*/

#include <Cocoa/Cocoa.h> 
#include <OpenGL/gl.h>
#include <stdint.h>

#define i8 int8_t 
#define i16 int16_t
#define i32 int32_t 
#define i64 int64_t 

#define b32 int32_t

#define u8 uint8_t 
#define u16 uint16_t 
#define u32 uint32_t 
#define u64 uint64_t 

#define r32 float
#define r64 double

#define global_variable static
#define internal static
#define local_function static

global_variable b32 IsGameRunning;

#include "macos_keycode.h"
#include "macos_soma.cpp"

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

int main(int argc, char **argv)
{
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

	int BufferWidth = 960*2;
	int BufferHeight = 540*2;
    int BytesPerPixel = 4;
    int BufferPitch = BufferWidth * BytesPerPixel;
    void *GameBufferMemory = malloc(BufferPitch * BufferHeight);

    uint8 *FirstPixelOfRow = (uint8 *)GameBufferMemory;
    for(int Row = 0;
        Row < BufferHeight;
        ++Row)
    {
        uint32 *Pixel = (uint32 *)(FirstPixelOfRow);
        for(int Column = 0;
            Column < BufferWidth;
            ++Column)
        {
            if(Column == BufferWidth/2 ||
            Row == BufferHeight/2)
            {
                *Pixel++ = (0xffff00ff);
            }
            else
            {
                *Pixel++ = (0xff00ffff);
            }
        }

        FirstPixelOfRow += BufferPitch;
    }

    // TODO : Don't know why, but MacOS creates a window with doubled size, so request the window to
    // be half the size at the first place. Maybe there's a proper way to fix this?
	NSRect WindowFrameRect = NSMakeRect(100.0f, 100.0f, (float)BufferWidth/2, (float)BufferHeight/2);

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
	//[Window setDelegate:appDelegate]; // TODO : Do we really need this?

    NSWindow *MainWindow = [App mainWindow];

    macos_opengl_info MacOSOpenGLInfo = {};
    MacOSOpenGLInfo = PrepareDisplayingWithOpenGL(Window, &WindowFrameRect, BufferWidth, BufferHeight, GameBufferMemory); 

    IsGameRunning = true; // TODO : Put this inside the event handling loop

	[App activateIgnoringOtherApps:YES];
	[App run];

    while(IsGameRunning)
    {
        MacOSHandleEvents(App);

        // Rendering loop with OpenGL.
        {
            glClear(GL_COLOR_BUFFER_BIT);
            glBindTexture(GL_TEXTURE_2D, MacOSOpenGLInfo.TextureID);
            glTexImage2D(GL_TEXTURE_2D, 0, 
                        GL_RGBA8, 
                        BufferWidth, BufferHeight,
                        0, 
                        GL_BGRA, 
                        GL_UNSIGNED_INT_8_8_8_8_REV, 
                        GameBufferMemory);
            glViewport(0, 0, BufferWidth, BufferHeight);

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
    }

    return 0;
}
