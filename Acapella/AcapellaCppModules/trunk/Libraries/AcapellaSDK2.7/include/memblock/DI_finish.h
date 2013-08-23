#ifndef x_IMACRO_MEMBLOCK_DI_FINISH_H_INCLUDED_
#define x_IMACRO_MEMBLOCK_DI_FINISH_H_INCLUDED_

#ifndef x_IMACRO_MEMBLOCK_DI_START_H_INCLUDED_
#error DI_start.h must be included before DI_finish.h!
#endif
#undef x_IMACRO_MEMBLOCK_DI_START_H_INCLUDED_

#ifdef _MSC_VER
#	pragma warning(pop)
#endif


// make available the NBaseutil namespace in the same way as NIMacro.
// cannot put it in DI_start.h because of MSVC bug (enum defs leak the using directive out of the namespace!)
namespace NIMacro {
	using namespace Nbaseutil;
}

#ifdef ACAPELLA_EVIL_MIN_WAS_DEFINED
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#undef ACAPELLA_EVIL_MIN_WAS_DEFINED
#endif
#ifdef ACAPELLA_EVIL_MAX_WAS_DEFINED
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#undef ACAPELLA_EVIL_MAX_WAS_DEFINED
#endif

#endif
