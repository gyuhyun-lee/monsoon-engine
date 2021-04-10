/* 
 
Author : Gyuhyeon, Lee 
Email : weanother@gmail.com 
Copyright by GyuHyeon, Lee. All Rights Reserved. 
 
*/

typedef LARGE_INTEGER wi64;

struct win32_screen_buffer
{
	i32 width;
	i32 height;
	i32 pitch;
	i32 size;

	BITMAPINFO bitmapInfo;
	// TODO : Check whether storing the client rect will help
	// RECT clientRect;

	void *memory;
};

struct win32_client_rect
{
	i32 width;
	i32 height;
};

struct win32_game_code
{
	HMODULE module;
	game_fixed_update *GameFixedUpdate;
	game_nonfixed_update *GameNonfixedUpdate;
	FILETIME lastAccessFileTime;
};
