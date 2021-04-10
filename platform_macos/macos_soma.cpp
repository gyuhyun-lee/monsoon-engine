static CVReturn 
DisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime,
                CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    // CVReturn result = [(__bridge fox_openGL_view*)displayLinkContext getFrameForTime:outputTime];
    // return result;
    return kCVReturnSuccess;
}

static void
SetDisplayWithOpenGL(NSWindow *Window, NSRect *WindowFrameRect,
                    int BufferWidth, int BufferHeight, int GameBufferMemory)
{
    NSOpenGLPixelFormatAttribute OpenGLPixelFormatAttributes[] = 
        {NSOpenGLPFADoubleBuffer, 
        NSOpenGLPFAAccelerated,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        0};
    
    NSOpenGLPixelFormat *OpenGLPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes : OpenGLPixelFormatAttributes];
    NSOpenGLContext *OpenGLContext = [[NSOpenGLContext alloc] initWithFormat : OpenGLPixelFormat shareContext : 0];
    NSOpenGLView *OpenGLView = [[NSOpenGLView alloc] initWithFrame : *WindowFrameRect 
                                                    pixelFormat : OpenGLPixelFormat];

    CVDisplayLinkRef DisplayLink;
    // kCVReturnSuccess has constant value of 0..
    if(CVDisplayLinkCreateWithActiveCGDisplays(&DisplayLink)== kCVReturnSuccess)
    {
        // TODO : Double-check whether setting a output callback function even matters.
        CVDisplayLinkSetOutputCallback(DisplayLink, DisplayLinkCallback, 0); 
        CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(DisplayLink, [OpenGLContext CGLContextObj], [OpenGLPixelFormat CGLPixelFormatObj]);
        CVDisplayLinkStart(DisplayLink);
    }
    [OpenGLContext makeCurrentContext];
	[Window setContentView:OpenGLView];
	[OpenGLView setOpenGLContext:OpenGLContext];
	[OpenGLContext setView:OpenGLView];
    
	GLuint TextureID;
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    // GL_UNPACK_ALIGNMENT affects how to unpack the given texture(in out case, our output buffer)
    // TODO : why 1 byte alignment?
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &TextureID);
    glBindTexture(GL_TEXTURE_2D, TextureID);

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

    GLenum error = glGetError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);



}


