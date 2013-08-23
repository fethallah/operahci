#ifndef x_IMACRO_IMACRO_DI_START_H_INCLUDED_
#define x_IMACRO_IMACRO_DI_START_H_INCLUDED_


// For including DI_IMacro.h in the projects compiled at MSVC++ 6.0 warning level 4
// This header is the first one in DI_IMacro.h
// The DI_finish header must be the last one.

#ifdef _MSC_VER
#	include <disable4786/disable4786.h>	// $/EvoPro/_CPP/Disable4786
#	pragma warning(disable: 4514 4503)
//  remember other warning settings and go to the warning level 3
#	pragma warning(push, 3)
#	pragma warning(disable: 4251 4275)
#endif

#undef x_IMACRO_IMACRO_DI_FINISH_H_INCLUDED_

#include <memblock/DI_memblock.h>

#ifndef ACAPELLA_PLATFORM_WINDOWS
// On Linux, all this dllexport stuff is not needed:
#define DI_IMacro
#endif

#endif
