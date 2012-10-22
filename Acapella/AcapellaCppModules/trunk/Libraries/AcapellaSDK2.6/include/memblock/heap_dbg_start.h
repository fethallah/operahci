#ifndef x_IMACRO_MEMBLOCK_HEAP_DEBUG_STRAT_H_INCLUDED
#define x_IMACRO_MEMBLOCK_HEAP_DEBUG_STRAT_H_INCLUDED

#if defined(_MSC_VER) && defined(_DEBUG) && !defined(UIMACRO_EXPORTS) && !defined(USE_DEBUG_NEW)
#	define USE_DEBUG_NEW
#endif

#include "memblock_export.h"

#ifdef USE_DEBUG_NEW
#	define _CRTDBG_MAP_ALLOC
#	define _NORMAL_BLOCK 1
#endif

#endif
