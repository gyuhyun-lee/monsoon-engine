#ifndef MACOS_FOX_H
#define MACOS_FOX_H

struct macos_opengl_info
{
	NSOpenGLPixelFormat *pixelFormat;
	NSOpenGLContext *context;

	GLuint textureID;
	CVDisplayLinkRef displayLink;
};

struct macos_audio_buffer
{
    // TODO : Maybe make this just to i16 *samples
    int16 *samples[2];
    i32 bytesPerSample;

    i32 samplesPerSecond;
    //u32 sampleCount;
    u32 channelCount;

    u32 playCursor; // where to play next
    u32 writeCursor; // where the last write was.
};

struct macos_game_code
{
	void *handle;
	long lastFileTime;
	game_update_and_render *GameUpdateAndRender;
    game_update_audio_buffer *GameUpdateAudioBuffer;
};


// Let's seperate these abominations 
// so that we don't have to look at these ever again.
@interface fox_app_delegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end
@interface fox_openGL_view : NSOpenGLView
@end

#endif
