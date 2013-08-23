// define dllexport macro.
// if this is not yet done, then this header is included 
// from some other project than MemBlock.
// All IMacro project files include "precompiled.h" first.

#ifndef DI_MemBlock
#	ifdef __WIN32__
#		define DI_MemBlock __declspec(dllimport) 
#	else
#		define DI_MemBlock
#	endif
#endif

