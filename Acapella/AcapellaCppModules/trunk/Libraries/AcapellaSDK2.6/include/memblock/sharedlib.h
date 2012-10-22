#ifndef x_ACAPELLA_MEMBLOCK_SHAREDLIB_H_INCLUDED
#define x_ACAPELLA_MEMBLOCK_SHAREDLIB_H_INCLUDED

namespace NIMacro {

/// A typedef for shared library handles. This can be cast directly from/to the OS-specific shared library handle (HMODULE in Windows, void* in POSIX).
typedef void* hmodule_t; 

/// Constants to use in LoadSharedLib() flags.
enum LoadLibraryFlags {
	/// On Windows, specifies the LOAD_WITH_ALTERED_SEARCH_PATH flag; on Linux does nothing.
	load_with_altered_search_path = 1,
	/// On Windows, specifies the DONT_RESOLVE_DLL_REFERENCES flag; on Linux does nothing.
	dont_resolve_dll_references = 2,
};

/**
* Load a shared library from disk. Throws an exception in case of an error.
* @param filename The library name. This can be:
*     - Filename with path (absolute or relative). This is assumed to be exact filename. This call is platform-specific.
*     - Filename without path, but with an extension. The file is searched by OS rules, depending on the platform. This call is platform-specific.
*     - Library name without extension. Default platform prefixes (e.g. "lib") and suffixes (e.g. "dll") are added.
*            This includes also build type R/D sufix on Windows platform.
* @param flags Additional flags composed of enum LoadLibraryFlags bit values.
*/
DI_MemBlock hmodule_t LoadSharedLib(const Nbaseutil::safestring& filename, unsigned int flags=0);

/**
* Get the handle to the shared library if it has been loaded in the process space.
* @param modulename Name of the shared library, without path and extension.
*      Platform-specific prefixes and suffixes are added, plus build type R/D sufix and Acapella version numbering sufix 
*	   and .AML module library exension. If the file with fully specified suffices has not
*      been loaded, then the filename without suffixes is tried as well.
* @return The shared library handle or NULL in case of failure.
*/
DI_MemBlock hmodule_t GetSharedLibHandle(const Nbaseutil::safestring& modulename);


/// Unload a library loaded by a LoadSharedLib() call. 
DI_MemBlock void FreeSharedLib(hmodule_t lib_handle);

/// Return handle for IMacro shared library if loaded in the current process, or NULL.
inline hmodule_t GetIMacroHandle() {
	return GetSharedLibHandle("imacro");
}

/**
* Find the addres of an exported symbol in the specified shared library.
* This forwards to ::GetProcAddress() or dlsym() depending on platform.
* @param lib_handle Library handle obtained by LoadSharedLib() or GetSharedLibHandle().
* @param funcname Function name as exported from the library. 
*      It is suggested to load only extern "C" functions by this facility here as the decoration rules for C++ function names are implementation specific.
*/
DI_MemBlock void* GetProcAddress(hmodule_t lib_handle, const Nbaseutil::safestring& funcname);

/**
* Return full shared library pathname by the library handle.
* @param lib_handle Library handle. Pass NULL to obtain the pathname of the main executable.
*/
DI_MemBlock Nbaseutil::safestring GetSharedLibFileName(hmodule_t lib_handle);

/// Constants to be used with LibVersion() function.
enum LibVersionWhat {
	/// First version number
	VERS_MAJOR=0,
	/// Second version number
	VERS_MINOR=1,
	/// Third version number
	VERS_BUILD1=2,
	/// Fourth version number
	VERS_BUILD2=3,
	/// All four version numbers together placed in an unsigned int of 32 bits. Note that this does not work if any of the numbers exceeds 255.
	VERS_LONG=4,
};

/**
* Return version information about the shared library loaded in the current process space. 
* If the shared library does not have version information, 0 is returned.
* In case of other errors exceptions are thrown.
@param lib_handle Handle of the shared library loaded in the current process.
@param what Which version number to return.
*/
DI_MemBlock unsigned int LibVersion(hmodule_t lib_handle, LibVersionWhat what);

/// A back-compatibility version of LibVersion() taking the library name as the argument.
inline unsigned int LibVersion(const char* libname, LibVersionWhat what) {
	return LibVersion(GetSharedLibHandle(libname), what);
}

/**
* Finds out the library build date. This has to be encoded in the Comment field of the DLL version resource on Windows. 
* On Linux, the special acaversion.cpp file is used instead.
* @return The build date of the shared library, if available, in format "dd.mm.yyyy". Otherwise returns an empty string.
* @param lib_handle Handle of the shared library loaded in the current process.
*/
DI_MemBlock Nbaseutil::safestring LibBuildDate(hmodule_t lib_handle);

/// A back-compatibility version of LibBuildDate() taking the library name as the argument.
inline Nbaseutil::safestring LibBuildDate(const char* libname) {
	return LibBuildDate(GetSharedLibHandle(libname));
}

/**
* Returns descriptive string for a DLL loaded in current process space. 
* The string includes full path, version info and comment.
@param lib_handle Handle of the shared library loaded in the current process.
*/
DI_MemBlock Nbaseutil::safestring LibInfo(hmodule_t lib_handle);

/// A back-compatibility version of LibBuildDate() taking the library name as the argument.
inline Nbaseutil::safestring LibInfo(const char* libname) {
	return LibInfo(GetSharedLibHandle(libname));
}

} // namespace
#endif
