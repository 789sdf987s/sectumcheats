#pragma once
#include "stdafx.h"

void DoAutoStrafe()
{
	if (!Hacks.LocalPlayer->GetFlags() & FL_ONGROUND)
	{
		Hacks.CurrentCmd->forwardmove = 450;
		if (Hacks.CurrentCmd->mousedx < 0)
			Hacks.CurrentCmd->sidemove -= 800;
		else if (Hacks.CurrentCmd->mousedx > 0)
			Hacks.CurrentCmd->sidemove += 800;
	}
}