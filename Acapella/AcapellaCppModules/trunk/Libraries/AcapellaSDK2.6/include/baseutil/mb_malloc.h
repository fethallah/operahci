#ifndef x_PKI_CT_BASEUTIL_MB_MALLOC_H_INCLUDED_
#define x_PKI_CT_BASEUTIL_MB_MALLOC_H_INCLUDED_

namespace Nbaseutil {

/// Allocate memory (n bytes) in the heap of first loaded baseutil DLL.
DI_baseutil void* mb_malloc(size_t n);

/// Allocate memory in the heap of of first loaded baseutil DLL, with debugging support (MSVC Debug build only; pass 1 for blocktype). 
DI_baseutil void* mb_malloc_dbg(size_t n, int blocktype, const char* filename, int lineno);

/// Allocate zero-initialized memory (n*m bytes) in the heap of first loaded baseutil DLL.
DI_baseutil void* mb_calloc(size_t n, size_t m);

/// Allocate zero-initialized memory in the heap of first loaded baseutil DLL, with debugging support (MSVC Debug build only; pass 1 for blocktype). 
DI_baseutil void* mb_calloc_dbg(size_t n, size_t m, int blocktype, const char* filename, int lineno);

/// Free the memory allocated with mb_malloc() or mb_calloc().
DI_baseutil void mb_free(void *p);

/// Free the memory allocated with mb_malloc_dbg() or mb_calloc_dbg() (MSVC Debug build only; pass 1 for blocktype). 
DI_baseutil void mb_free_dbg(void *p, int blocktype);

/// Reallocate the memory allocated with mb_malloc() or mb_calloc().
DI_baseutil void* mb_realloc(void *p, size_t n);

/// Reallocate the memory allocated with mb_malloc_dbg() or mb_calloc_dbg() (MSVC Debug build only; pass 1 for blocktype). 
DI_baseutil void* mb_realloc_dbg(void *p, size_t n, int blocktype, const char* filename, int lineno);

/**
* Allocate the memory as with mb_malloc()/mb_malloc_dbg() 
* except that the allocation is not registered by the leakage
* detection system in the MSVC Debug environment 
* and will therefore not reported as a leak in the end of the program.
* Use this function to allocate "static" memory which will not be 
* released before the end of the program. 
*/
DI_baseutil void* malloc_once(size_t n);

/// Release the memory allocated with malloc_once (this is optional).
DI_baseutil void free_once(void* p);

/// Reallocate the memory allocated by malloc_once().
DI_baseutil void* realloc_once(void* p, size_t n);

/**
* A base class for classes whose objects can 
* transfer border of DLL-s of different C/C++ runtime libraries.
* This works also in case if there are two different
* versions of baseutil DLL loaded in the same process (Debug and Release, for example).
* One can create a derived-class object e.g. in a Debug-build DLL
* and delete it later from a Release-build DLL linked to another
* baseutil DLL version. The mechanism to achieve this involves 
* cooperation between different loaded baseutil DLL versions;
* the first loaded DLL will take care of all allocations and deallocations,
* any later loaded baseutil DLL will just forward all requests.
* The same mechanism is used for all mb_* free functions.
*/
class mb_malloced {
public:
	/// Common new operator, allocates memory from the heap of first loaded baseutil DLL.
	void* operator	new(size_t n) { return mb_malloc(n); }

	/// Common delete operator, frees memory in the heap of first loaded baseutil DLL.
	void operator delete(void *p) { mb_free(p); }

	/// Placement new operator, does nothing, returns placement value.
	void* operator new(size_t n, void* placement) {return placement;}

	/// Placement delete operator, does nothing.
	void operator delete(void *p, void* placement) {} 

	/// Array new operator, allocates memory from the heap of first loaded baseutil DLL.
	void* operator new[] (size_t n) { return mb_malloc(n); }

	/// Array delete operator, frees memory in the heap of first loaded baseutil DLL.
	void  operator delete[] (void* p) { mb_free(p);}

// _CRTDBG_MAP_ALLOC is defined in heap_dbg_start.h, included in gcc.h
// In every .cpp file you will need to include heap_dbg.h *as the last header* - this will redefine "new".
#ifdef _CRTDBG_MAP_ALLOC

	/// MSVC++ Debug-build new, defined only if _CRTDBG_MAP_ALLOC macro is defined earlier.
	void* operator new(size_t n, const char* filename, int lineno) {return mb_malloc_dbg(n, 1, filename, lineno);}

	/// MSVC++ Debug-build delete, defined only if _CRTDBG_MAP_ALLOC macro is defined earlier.
	void operator delete(void* p, const char* filename, int lineno) {mb_free_dbg(p, 1);}

	/// MSVC++ Debug-build new, defined only if _CRTDBG_MAP_ALLOC macro is defined earlier.
	void* operator new(size_t n, int blocktype, const char* filename, int lineno) {return mb_malloc_dbg(n, blocktype, filename, lineno);}

	/// MSVC++ Debug-build delete, defined only if _CRTDBG_MAP_ALLOC macro is defined earlier.
	void operator delete(void *p, int blocktype, const char* filename, int lineno) { mb_free_dbg(p, 1); }

	/// MSVC++ Debug-build new, defined only if _CRTDBG_MAP_ALLOC macro is defined earlier.
	void* operator new[](size_t n, const char* filename, int lineno) {return mb_malloc_dbg(n, 1, filename, lineno);}

	/// MSVC++ Debug-build delete, defined only if _CRTDBG_MAP_ALLOC macro is defined earlier.
	void operator delete[](void* p, const char* filename, int lineno) {mb_free_dbg(p, 1);}

	/// MSVC++ Debug-build new, defined only if _CRTDBG_MAP_ALLOC macro is defined earlier.
	void* operator new[](size_t n, int blocktype, const char* filename, int lineno) {return mb_malloc_dbg(n, blocktype, filename, lineno);}

	/// MSVC++ Debug-build delete, defined only if _CRTDBG_MAP_ALLOC macro is defined earlier.
	void operator delete[](void *p, int blocktype, const char* filename, int lineno) { mb_free_dbg(p, 1); }
#endif
};

/**
* Works in Windows DEBUG builds. Enables the application to execute
* a user breakpoint interrupt if the specified memory block is allocated
* by mb_malloc() or mb_calloc(). See the -bpaddr option of Acapella
* command-line interface. This function is near to useless in multithreaded
* code because the allocation addresses will vary randomly from run to run.
* @param bpaddr The address of memory block to watch out for.
* @param bpaddr_size If not 0, then the block size has to match it.
* @param lineno Allocation source code line number for further filtering.
*/
DI_baseutil void SetBpAddr(void* bpaddr, size_t bpaddr_size=0, int lineno=0);


typedef int (*OutOfMemoryHandlerFunc)(size_t amount);

/**
* Set or clear the out-of-memory handler for mb_malloc et al. Returns the previous handler or NULL. 
* This feature is most probably not used in Windows because of _set_new_mode(1) present in IMacro library init.
*/
DI_baseutil OutOfMemoryHandlerFunc SetMbOutOfMemoryHandler(OutOfMemoryHandlerFunc handler);


} //namespace

#endif
