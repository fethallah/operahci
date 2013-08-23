#ifndef x_PKI_CT_BASEUTIL_GCC_H_INCLUDED
#define x_PKI_CT_BASEUTIL_GCC_H_INCLUDED
#include "./config.h"

// ScopeGuard warning elimination support.
//#include <ScopeGuard/ScopeGuard.h>
namespace Nbaseutil {
	/// This function can be used for silencing warnings about apparantly unused local variables. It does nothing.
	template <typename T> inline void MarkUsed(const T& x) {}
}
#ifndef __GNUC__
#ifndef MARK_UNUSED_VAR
/// This macro can be used for silencing warnings about apparantly unused local variables. It does nothing.
#define MARK_UNUSED_VAR(x) x
#endif
#ifndef ON_BLOCK_EXIT1
/// Same as ScopeGuard facility ON_BLOCK_EXIT macro, restricted to exactly 1 arguments for avoiding spurious compiler warnings
#define ON_BLOCK_EXIT1(fn) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) = MakeGuard(fn); MARK_UNUSED_VAR(ANONYMOUS_VARIABLE(scopeGuard));
/// Same as ScopeGuard facility ON_BLOCK_EXIT macro, restricted to exactly 2 arguments for avoiding spurious compiler warnings
#define ON_BLOCK_EXIT2(fn,arg1) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) = MakeGuard(fn,arg1); MARK_UNUSED_VAR(ANONYMOUS_VARIABLE(scopeGuard));
/// Same as ScopeGuard facility ON_BLOCK_EXIT macro, restricted to exactly 3 arguments for avoiding spurious compiler warnings
#define ON_BLOCK_EXIT3(fn,arg1,arg2) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) = MakeGuard(fn,arg1,arg2); MARK_UNUSED_VAR(ANONYMOUS_VARIABLE(scopeGuard));
/// Same as ScopeGuard facility ON_BLOCK_EXIT macro, restricted to exactly 4 arguments for avoiding spurious compiler warnings
#define ON_BLOCK_EXIT4(fn,arg1,arg2,arg3) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) = MakeGuard(fn,arg1,arg2,arg3); MARK_UNUSED_VAR(ANONYMOUS_VARIABLE(scopeGuard));
/// Same as ScopeGuard facility ON_BLOCK_EXIT_OBJ macro, restricted to exactly 2 arguments for avoiding spurious compiler warnings
#define ON_BLOCK_EXIT_OBJ2(obj,fn) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) = MakeObjGuard(obj,fn); MARK_UNUSED_VAR(ANONYMOUS_VARIABLE(scopeGuard));
/// Same as ScopeGuard facility ON_BLOCK_EXIT_OBJ macro, restricted to exactly 3 arguments for avoiding spurious compiler warnings
#define ON_BLOCK_EXIT_OBJ3(obj,fn,arg1) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) = MakeObjGuard(obj,fn,arg1); MARK_UNUSED_VAR(ANONYMOUS_VARIABLE(scopeGuard));
/// Same as ScopeGuard facility ON_BLOCK_EXIT_OBJ macro, restricted to exactly 4 arguments for avoiding spurious compiler warnings
#define ON_BLOCK_EXIT_OBJ4(obj,fn,arg1,arg2) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) = MakeObjGuard(obj,fn,arg1,arg2); MARK_UNUSED_VAR(ANONYMOUS_VARIABLE(scopeGuard));
/// Same as ScopeGuard facility ON_BLOCK_EXIT_OBJ macro, restricted to exactly 5 arguments for avoiding spurious compiler warnings
#define ON_BLOCK_EXIT_OBJ5(obj,fn,arg1,arg2,arg3) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) = MakeObjGuard(obj,fn,arg1,arg2,arg3); MARK_UNUSED_VAR(ANONYMOUS_VARIABLE(scopeGuard));
#endif

#else
// gcc branch
#ifndef MARK_UNUSED_VAR
#define MARK_UNUSED_VAR(x) MarkUsed(x)
#endif
#ifndef ON_BLOCK_EXIT1
#define ON_BLOCK_EXIT1(fn) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) __attribute__ ((unused)) = MakeGuard(fn); 
#define ON_BLOCK_EXIT2(fn,arg1) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) __attribute__ ((unused)) = MakeGuard(fn,arg1); 
#define ON_BLOCK_EXIT3(fn,arg1,arg2) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) __attribute__ ((unused)) = MakeGuard(fn,arg1,arg2); 
#define ON_BLOCK_EXIT4(fn,arg1,arg2,arg3) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) __attribute__ ((unused)) = MakeGuard(fn,arg1,arg2,arg3); 
#define ON_BLOCK_EXIT_OBJ2(obj,fn) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) __attribute__ ((unused)) = MakeObjGuard(obj,fn); 
#define ON_BLOCK_EXIT_OBJ3(obj,fn,arg1) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) __attribute__ ((unused)) = MakeObjGuard(obj,fn,arg1); 
#define ON_BLOCK_EXIT_OBJ4(obj,fn,arg1,arg2) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) __attribute__ ((unused)) = MakeObjGuard(obj,fn,arg1,arg2); 
#define ON_BLOCK_EXIT_OBJ5(obj,fn,arg1,arg2,arg3) ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) __attribute__ ((unused)) = MakeObjGuard(obj,fn,arg1,arg2,arg3);
#endif
#endif

#include "heap_dbg_start.h"

// Define NULL in any case, other headers use it
#ifndef NULL
/// Defines NULL constant (C++ style) for any case, if not yet defined.
#	define NULL    0
#endif

// provide isinf/isnan/isfinite in Nbaseutil namespace
#if defined(ACAPELLA_HAS_C99)
#include <cmath>
namespace Nbaseutil {
	namespace detail {
		template<typename T> int capture_isinf(T x) {return std::isinf(x);}
		template<typename T> int capture_isnan(T x) {return std::isnan(x);}
		template<typename T> int capture_isfinite(T x) {return std::isfinite(x);}
	}
#undef isinf
#undef isnan
#undef isfinite
	template<typename T> int isinf(T x) {return detail::capture_isinf(x);}
	template<typename T> int isnan(T x) {return detail::capture_isnan(x);}
	template<typename T> int isfinite(T x) {return detail::capture_isfinite(x);}
} // namespace

#elif defined(ACAPELLA_RTL_MSVC)

#include <float.h>
namespace Nbaseutil {
	template<typename T> int isinf(T x) {return !_finite(x) && !_isnan(x);}
	template<typename T> int isnan(T x) {return _isnan(x);}
	template<typename T> int isfinite(T x) {return _finite(x);}
} // namespace

#endif

#endif
