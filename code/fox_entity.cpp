/* 
 
Author : Gyuhyeon, Lee 
Email : weanother@gmail.com 
Copyright by GyuHyeon, Lee. All Rights Reserved. 
 
*/

#include "fox_entity.h"

inline void
SetEntityFlag(u8 *flags, u8 flag)
{
	*flags |= (flag);
}

inline void
RemoveEntityFlag(u8 *flags, u8 flag)
{
	*flags &= ~flag;
}

inline b32
IsEntityFlagSet(u8 flags, u8 flag)
{
	return flags & flag;
}

