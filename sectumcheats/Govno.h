#pragma once
#include "SDK\SDK.h"
class IBaseClientDLL
{
public:
	ClientClass * GetAllClasses()
	{
		typedef ClientClass* (*oGetAllClasses)(void*);
		return getvfunc<oGetAllClasses>(this, 8)(this);
	}
};