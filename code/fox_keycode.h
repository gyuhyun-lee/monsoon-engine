/* 
 
Author : Gyuhyeon, Lee 
Email : weanother@gmail.com 
Copyright by GyuHyeon, Lee. All Rights Reserved. 
 
*/

#ifndef FOX_KEYCODE_H
#define key_action_down 1
#define key_action_up 2
#define key_action_right 3
#define key_action_left 4

#define key_dpad_up 5
#define key_dpad_right 6
#define key_dpad_left 7
#define key_dpad_down 8

#define key_left_bumper 9
#define key_right_bumper 10
#define key_back 11
#define key_start 12

inline b32
GetButtonDown(game_input *gameInput, u32 controllerIndex, u8 keyCode)
{
	return gameInput->controllers[controllerIndex].buttons[keyCode].isDown;
}

// NOTE : Use this when you want to know whether the button is pressed only once.
inline b32
GetButtonTriggered(game_input *gameInput, game_input *oldInput, u32 controllerIndex, u8 keyCode)
{
	return gameInput->controllers[controllerIndex].buttons[keyCode].isDown && 
			!oldInput->controllers[controllerIndex].buttons[keyCode].isDown;
}

inline b32
GetButtonPressed(game_input *gameInput, game_input *oldInput, u32 controllerIndex, u8 keyCode)
{
	// TODO : This will have one frame delay, as it takes 1 frame for wasDown to be 1. Fix this Delay.
	return gameInput->controllers[controllerIndex].buttons[keyCode].isDown && 
			oldInput->controllers[controllerIndex].buttons[keyCode].isDown;
}

inline b32
GetLeftTriggerDown(game_input *gameInput, u32 controllerIndex)
{
	b32 result = false;

	if(gameInput->controllers[controllerIndex].leftTrigger > 0.1f)
	{
		result = true;
	}

	return result;
}

inline b32
GetRightTriggerDown(game_input *gameInput, u32 controllerIndex)
{
	b32 result = false;

	if(gameInput->controllers[controllerIndex].rightTrigger > 0.1f)
	{
		result = true;
	}

	return result;
}

inline v2
GetLeftStickValue(game_input *gameInput, u32 controllerIndex)
{
	return gameInput->controllers[controllerIndex].leftStick;
}

inline v2
GetRightStickValue(game_input *gameInput, u32 controllerIndex)
{
	return gameInput->controllers[controllerIndex].rightStick;
}

#define FOX_KEYCODE_H
#endif
