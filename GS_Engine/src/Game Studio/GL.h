#pragma once

#include "Logger.h"

#ifdef GS_DEBUG
	#define GS_GL_CALL(func)	func;\
								Logger::GetglGetError(__FILE__, __LINE__);
#else
	#define GS_GL_CALL(func)	func;
#endif