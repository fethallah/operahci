#ifndef x_PKI_CT_BASEUTIL_HEAP_DEBUG_STRAT_H_INCLUDED
#define x_PKI_CT_BASEUTIL_HEAP_DEBUG_STRAT_H_INCLUDED

#if defined(_MSC_VER) && defined(_DEBUG) && !defined(UIMACRO_EXPORTS) && !defined(USE_DEBUG_NEW)
#	define USE_DEBUG_NEW
#endif

#ifdef USE_DEBUG_NEW
#	define _CRTDBG_MAP_ALLOC
#	define _NORMAL_BLOCK 1
#endif

// This is the first exported header; disable some annoying warnings
// The warning level is restored in the last exported header (carrier.h)
#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable: 4100 4275)
#endif

#endif
