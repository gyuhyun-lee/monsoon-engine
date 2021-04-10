#include <Cocoa/Cocoa.h> 
#include <OpenGL/gl.h>

#include "macos_soma.cpp"

@interface fox_app_delegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

// NOTE : Whenever the application steps into [NSApp Run] method, 
// we are unable to control the application ourselves. Therfore,
// we need our version of app delegate so that the OS can call this function
// to 'stop' the application from running.
@implementation fox_app_delegate : NSObject
// -(void)applicationWillFinishLaunching: (NSNotification *)notification
- (void)applicationDidFinishLaunching: (NSNotification *)notification 
{
    // This 'stop' function should only be called here - because the application
    // cannot be manipulated by ourseleves when the [NSApp Run] function is called.
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
}

- (void)dealloc 
{
    [super dealloc];
}
@end

int main(int argc, char **argv)
{
    NSApplication *App = [NSApplication sharedApplication]; // Your program should invoke this method as one of the first statements in main();
    [App setActivationPolicy :NSApplicationActivationPolicyRegular]; // The application is an ordinary app that appears in the Dock and may have a user interface.

    NSFileManager *FileManager = [NSFileManager defaultManager];
    NSString *BundlePath = [[NSBundle mainBundle] bundlePath];
	NSString *ResourcePath = [NSString stringWithFormat:@"%@/Contents/Resources", BundlePath];

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


    //- (instancetype)initWithTitle:(NSString *)title;
    //- (void)addItem:(NSMenuItem *)newItem;

	int BufferWidth = 960;
	int BufferHeight = 540;
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
            *Pixel++ = (0xff00ffff);
        }

        FirstPixelOfRow += BufferPitch;
    }

	NSRect WindowFrameRect = NSMakeRect(100.0f, 100.0f, (float)BufferWidth, (float)BufferHeight);

	NSWindow *window = [[NSWindow alloc] initWithContentRect : WindowFrameRect
											// Apple window styles : https://developer.apple.com/documentation/appkit/nswindow/stylemask
											styleMask : NSTitledWindowMask|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable
											backing : NSBackingStoreBuffered 
											defer : NO];

	NSString *AppName = [[NSProcessInfo processInfo]processName];
	[window setTitle:AppName];
	[window makeKeyAndOrderFront:0];
	fox_app_delegate *appDelegate = [[fox_app_delegate alloc] init];
	[App setDelegate:appDelegate];
	[window setDelegate:appDelegate];

    GLuint TextureID = PrepareDisplayingWithOpenGL(Window, &WindowFrameRect, BufferWidth, BufferHeight, GameBufferMemory); 

	[App activateIgnoringOtherApps:YES];
	[App run];

    while(1)
    {
			// CGLLockContext(macosOpenGLInfo.cglContext);
            {
		        glClear(GL_COLOR_BUFFER_BIT);
	            glViewport(0, 0, BufferWidth, BufferHeight);
		        glBindTexture(GL_TEXTURE_2D, TextureID);
			    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, BufferWidth, BufferHeight,
						0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, GameBufferMemory);
		        glBegin(GL_QUADS); 
		        {
		            glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
		            glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, 1.0f);
		            glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  -1.0f);
		            glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  -1.0f);
		        } glEnd();
		        glBindTexture(GL_TEXTURE_2D, 0);

			    [OpenGLContext flushBuffer];
            }
		    // CGLUnlockContext(macosOpenGLInfo.cglContext);
    }

    return 0;
}
