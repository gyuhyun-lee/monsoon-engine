struct macos_opengl_info
{
    GLuint TextureID;
    NSOpenGLContext *OpenGLContext;
};

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

static CVReturn 
DisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime,
                CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    // CVReturn Result = [(__bridge fox_openGL_view*)displayLinkContext getFrameForTime:outputTime];
    // return Result;
    return kCVReturnSuccess;
}

static macos_opengl_info
PrepareDisplayingWithOpenGL(NSWindow *Window, NSRect *WindowFrameRect,
                    int BufferWidth, int BufferHeight, void *GameBufferMemory)
{
    macos_opengl_info Result = {};
    NSOpenGLPixelFormatAttribute OpenGLPixelFormatAttributes[] = 
        {NSOpenGLPFADoubleBuffer, 
        NSOpenGLPFAAccelerated,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        0};
    
    NSOpenGLPixelFormat *OpenGLPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes : OpenGLPixelFormatAttributes];
    Result.OpenGLContext = [[NSOpenGLContext alloc] initWithFormat : OpenGLPixelFormat shareContext : 0];
    NSOpenGLView *OpenGLView = [[NSOpenGLView alloc] initWithFrame : *WindowFrameRect 
                                                    pixelFormat : OpenGLPixelFormat];

    CVDisplayLinkRef DisplayLink;
    // kCVReturnSuccess has constant value of 0..
    if(CVDisplayLinkCreateWithActiveCGDisplays(&DisplayLink)== kCVReturnSuccess)
    {
        // TODO : Double-check whether setting a output callback function even matters.
        CVDisplayLinkSetOutputCallback(DisplayLink, DisplayLinkCallback, 0); 
        CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(DisplayLink, [Result.OpenGLContext CGLContextObj], [OpenGLPixelFormat CGLPixelFormatObj]);
        CVDisplayLinkStart(DisplayLink);
    }
    [Result.OpenGLContext makeCurrentContext];
	[Window setContentView:OpenGLView];
	[OpenGLView setOpenGLContext : Result.OpenGLContext];
	[Result.OpenGLContext setView:OpenGLView];
    
    /*
        Set texture info
    */
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    // GL_UNPACK_ALIGNMENT affects how to unpack the given texture(in out case, our output buffer)
    // TODO : why 1 byte alignment?
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &Result.TextureID);

    glBindTexture(GL_TEXTURE_2D, Result.TextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, 
                GL_RGBA8, 
                BufferWidth, BufferHeight,
                0, 
                GL_BGRA, 
                GL_UNSIGNED_INT_8_8_8_8_REV, 
                GameBufferMemory);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return Result;
}
