#include "macos_fox.h"

static CVReturn 
DisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime,
                CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
// CVReturn result = [(__bridge fox_openGL_view*)displayLinkContext getFrameForTime:outputTime];
// return result;
return kCVReturnSuccess;
}

macos_opengl_info 
MacOSInitializeOpenGL(game_screen_buffer *gameBuffer)
{
macos_opengl_info result = {};

NSOpenGLPixelFormatAttribute attributes[] = 
{
    NSOpenGLPFADoubleBuffer,
    // NSOpenGLPFAClosestPolicy,	
    NSOpenGLPFAAccelerated,
    NSOpenGLPFAColorSize, 24,
    NSOpenGLPFAAlphaSize, 8,
    // NSOpenGLPFASampleBuffers, 0,
    0,
};
result.pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
result.context = [[NSOpenGLContext alloc] initWithFormat:result.pixelFormat shareContext:0];

// Create the display link that can link with all the possible displays
CVDisplayLinkCreateWithActiveCGDisplays(&result.displayLink);
// Set renderer output callback function
CVDisplayLinkSetOutputCallback(result.displayLink, &DisplayLinkCallback, 0);
// Select the display link most optimal for the current context(renderer)
CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(result.displayLink, [result.context CGLContextObj], [result.pixelFormat CGLPixelFormatObj]);
// NOTE : Or use the main display
// CVDisplayLinkSetCurrentCGDisplay(result.displayLink, CVDMainDisplay());
CVDisplayLinkStart(result.displayLink);

// GLint swapInt = 1;
// [result.context setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

[result.context makeCurrentContext];

glEnable(GL_TEXTURE_2D);
glDisable(GL_DEPTH_TEST);
glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
glActiveTexture(GL_TEXTURE0);
glGenTextures(1, &result.textureID);
glBindTexture(GL_TEXTURE_2D, result.textureID);

// glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, gameBuffer->width, gameBuffer->height,
// 			0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, gameBuffer->memory);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gameBuffer->width, gameBuffer->height,
            0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, gameBuffer->memory);

GLenum error = glGetError();
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

return result;
}

internal long
MacOSGetFileTime(const char *filePath)
{
long result;

struct stat fileStat;
stat(filePath, &fileStat);
result = fileStat.st_mtime;

return result;
}

internal off_t
MacOSGetFileSize(const char *filePath)
{
    off_t result = 0;

    struct stat fileStat;
    stat(filePath, &fileStat);
    result = fileStat.st_size;

    return result;
}

internal void
MacOSGetGameCodes(macos_game_code *gameCode, const char *filePath)
{
if(gameCode->handle)
{
    dlclose(gameCode->handle);
	}

	// TODO : Find out what mode to use for dlopen
	gameCode->handle = dlopen(filePath, RTLD_LAZY);
	gameCode->lastFileTime = MacOSGetFileTime(filePath);
	if(gameCode->handle)
	{
		// void *dlsym(void *restrict handle, const char *restrict name);
		gameCode->GameUpdateAndRender = (game_update_and_render *)dlsym(gameCode->handle, "GameUpdateAndRender");
        gameCode->GameUpdateAudioBuffer = (game_update_audio_buffer *)dlsym(gameCode->handle, "GameUpdateAudioBuffer");
	}
	else
	{
		// TODO : Logging
	}
}
