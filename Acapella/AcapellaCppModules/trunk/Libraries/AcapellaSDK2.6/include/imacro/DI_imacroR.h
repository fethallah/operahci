// For force-including Release version of IMacro and MemBlock libraries.

#ifdef _DEBUG
#	define SAVE_DEBUG _DEBUG
#	undef _DEBUG
#	include "DI_IMacro.h"
#	define _DEBUG SAVE_DEBUG
#	undef SAVE_DEBUG
#else
#	include "DI_IMacro.h"
#endif
