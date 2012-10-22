#ifndef x_IMACRO_MEMBLOCK_DI_START_H_INCLUDED_
#define x_IMACRO_MEMBLOCK_DI_START_H_INCLUDED_


// For including DI_MemBlock.h in the projects compiled at MSVC++ 6.0 warning level 4
// This header is the first one in DI_MemBlock.h
// The DI_finish header must be the last one.


// include baseutil library (formerly part of memblock.dll).
#define DEFINE_BASEUTIL_ASSERT
#include <baseutil/DI_baseutil.h>

#ifdef _MSC_VER
#	include <disable4786/disable4786.h>	// $/EvoPro/_CPP/Disable4786
#	pragma warning(disable: 4514 4503 4251)
//  remember other warning settings and go to the warning level 3
#	pragma warning(push, 3)
#endif

#undef x_IMACRO_MEMBLOCK_DI_FINISH_H_INCLUDED_

#ifndef DI_MemBlock
#	if defined(_MSC_VER)
#		define DI_MemBlock __declspec(dllimport)
#	else
#		define DI_MemBlock
#	endif
#endif

#ifndef IMACRO
#define IMACRO "Acapella"
#endif

// Windows evil min/max macro guards
#ifdef min
#define ACAPELLA_EVIL_MIN_WAS_DEFINED
#undef min
#endif
#ifdef max
#define ACAPELLA_EVIL_MAX_WAS_DEFINED
#undef max
#endif

#endif
