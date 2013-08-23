
#ifndef _SERIALIZER_H_INCLUDED_
#define _SERIALIZER_H_INCLUDED_

#include <set>
#include <map>
#include <string>
#include <assert.h>
#include <stdio.h>
#include "refcounted.h"
#include "memblock.h"

#ifdef _MSC_VER
#pragma warning(disable: 4251)
#endif

namespace NIMacro {
const int ExcReasonLength=128;

class DI_MemBlock Vector;
typedef DerivedPointer<Vector> PVector;
class ThreadSharable;

struct FormatTag {
     unsigned int uFlag;     // numeric flag corresponding to this format tag.
     const char* pszTag;           // a case-sensitive tag, can't be NULL.
     const char* pszSynonym;       // optional synonym (may be NULL).
     int iTagLen;            // length of tag and synonum.
     int iSynLen;
};

class ObjectRegistry: public Nbaseutil::mb_malloced {
public:
	virtual ~ObjectRegistry() {}
	virtual void Put(const void* id)=0;
	virtual void Put(const void* id, const void* pAddress)=0;
	virtual bool Present(const void* id)=0;
	virtual const void* Address(const void* id)=0;
};

class ObjectRegistrySerializer: public ObjectRegistry {
	std::set<const void*, std::less<const void*>, Nbaseutil::mb_allocator_typedef<const void*>::allocator > x_;
public:
	virtual ~ObjectRegistrySerializer() {}
	virtual void Put(const void* id) { x_.insert(id); }
	virtual void Put(const void* id, const void* pAddress) { assert(false); }
	virtual bool Present(const void* id) { return x_.find(id) != x_.end(); }
	virtual const void* Address(const void* id) { return NULL; }
};

class ObjectRegistryUnserializer: public ObjectRegistry {
	typedef std::map<const void*,const void*, std::less<const void*>, Nbaseutil::mb_allocator_typedef< std::map<const void*, const void*>::value_type >::allocator > map_t;
	map_t x_;
public:
	virtual ~ObjectRegistryUnserializer() {}
	virtual void Put(const void* id) { assert(false); }
	virtual void Put(const void* id, const void* pAddress) { x_.insert( std::pair<const void*,const void*>(id, pAddress)); }
	virtual bool Present(const void* id) { return x_.find(id) != x_.end(); }
	virtual const void* Address(const void* id) { 
		map_t::iterator p = x_.find(id); 
		if (p==x_.end()) return NULL;
		return p->second;
	}
};

/// A singleton class for holding the data about registered MemBlock hierarchy classes.
/**
* All MemBlock hierarchy classes should be registered by the MemBlockFactory() 
* singleton returned by MemBlock::Factory(). The MemBlock hierarchy classes 
* in external module libraries are automatically registered by exporting a 
* ExportedClasses() function from the library (see NIMacro::ExportedClassesFunc).
*/
class DI_MemBlock MemBlockFactory: public Nbaseutil::mb_malloced {
public: // typedefs
	typedef bool (*AuxLoaderFunc)(const Nbaseutil::safestring& classname);
public:
	/// Return a pointer to a dummy object of the registered class, or NULL.
	/**
	* It is not allowed to modify the returned object, as it's still held in the MemBlockFactory.
	* Only const interface can be used.
	* @param class_name Name of the class as registered.
	* @param try_to_load If class is not yet registered, tries to locate and load the appropriate module library which would register the class. This flag can be passed only if no ResolvePoint objects are currently locked in this thread.
	* @return A exemplary object of the class, or NULL.
	*/
	PMemBlock GetInstance(const Nbaseutil::safestring& class_name, bool try_to_load=true);
	/// Register a MemBlock hierarchy class list. 
	/** 
	* @param dll_filename The filename of the DLL providing the class definition.
	* @param pClasses Array of classes to register. If NULL, locates loaded DLL in the current process and calles ExportedClasses() in this DLL.
	* @param iCount Number of elements in the pClasses array.
	* Throws in case of errors.
	*/
	void RegisterMemBlockLibrary(const Nbaseutil::safestring& dll_filename, const ClassInfo* pClasses, int iCount);
	void RegisterMemBlockLibrary(const Nbaseutil::safestring& pszDllName, const ClassInfo* pClasses, int iCount, Nbaseutil::ThrowHandlerFunc th);
	unsigned int LocateAndLoadClass(const Nbaseutil::safestring& pszClassName);	// return value essentially unused
	Nbaseutil::safestring ListClasses(char Separator);	// return the list of registered classes.
	~MemBlockFactory();
	/// Obsolete, do not use
	void InstantiateAllClasses();
	/// Obsolete, do not use
	void ClearAllClasses();
	/// Registers an extra loader for classes not yet registered in the map.
	void SetAuxLoader(AuxLoaderFunc aux_loader);
private: // implementation
	friend class MemBlock;
	MemBlockFactory();
	typedef std::map<Nbaseutil::safestring, MemBlockCreatorFunc, std::less<Nbaseutil::safestring>, Nbaseutil::mb_allocator_typedef< std::pair <const Nbaseutil::safestring, MemBlockCreatorFunc> >::allocator > CLASSMAP;
	CLASSMAP ClassMap;
	// Bugfix 27.09.2004: protect ClassMap in multithreaded execution (random errors in Olavi's long scripts?)
	Mutex ClassMap_mutex;
	AuxLoaderFunc aux_loader_;
};

//#ifndef IMACRO_STATIC_LINKED
DI_MemBlock void RegisterMemBlockLibrary(const char* pszDllName, const ClassInfo* pClasses, int iCount);
/*#else
// unsigned int (*ExportedClassesFunc)(const ClassInfo*& pClasses, int& iCount);
 DI_MemBlock void RegisterMemBlockLibrary(ExportedClassesFunc f);
#endif
*/

class DI_MemBlock Formatter: public OptionsBase {
public:
	/// Main serialization formats:
	enum format_mode_t {
		/// Unknown serialization format.
		ser_unknown,
		ser_binary,		// raw binary
		ser_ascii,		// ascii text
		ser_html,
		ser_xml,		// extended markup language
		ser_hdf5,		// hierarchical data format 
		ser_bmp,		// windows bitmap image
		ser_acapella,
		ser_exclude,

		ser_format_mode_end,	// marks the last used id in this class. this must be the last entry here.
	};

	enum encoding_format_t {
		ser_gzip = ser_format_mode_end+1,
		ser_base64,
		ser_gzip_base64,
		ser_hexadecimal,
	};

	// format layout modes
	enum options_t {
		opt_plot2d,		// 2-dimensional plot or graph
		opt_plot3d,		// 3-dimensional plot
		opt_line,		// items separated by newlines
		opt_skipnulls,	// do not output zero data values.
		opt_rowindices,	// when outputting table, output also row indices as the first column.
		opt_data_in_end,	// when outputting table, output the main data vector in the end. This is used for gnuplot.
		opt_uselocale,		// use current locale-s decimal point
		opt_usecomma,		// use comma as a decimal point
		opt_shortmemblockdescr, // when serializing a memblock item, output description only.
		opt_detect_numerics, // when serializing strings, detect apparent numeric values and convert the decimal point if needed.

		opt_end_formatter	// marks the last used id in this class. this must be the last entry here.
	};

public: // static interface
	static Nbaseutil::safestring Opt2String(options_t opt);

protected:
	//@{
	// Find a single tag from FormatTag array.
	// 
	// Returns numeric tag.
	// Deletes matched tag from the sl list.
	// If more than 1 tag matches, throws an Exception, unless returnearly==true.
	// Matching is case insensitive.
	// @param sl list of string tags to search for
	// @param p array of tag definitions
	// @param n length of p array
	// @param defaultvalue value to return, if no tag matches.
	// @param returnearly do not check for multiple matches, return the first match.
	//@}
	unsigned int FindTag(Nbaseutil::safestringlist& sl, FormatTag* p, unsigned int n, unsigned int defaultvalue, bool returnearly=false);

	// A defaultvalue for FindTag, which can be checked upon return.
	enum { tag_not_found = (unsigned int) -1 };

	//@{
	// Similar to FindTag() with 4 arguments, but throws an exception, if no tag matches.
	//@}
	unsigned int FindTag(Nbaseutil::safestringlist& sl, FormatTag* p, unsigned int n);

//	format_mode_t mainformat_;	// main file format
//	typedef std::set<options_t, std::less<options_t>, Nbaseutil::mb_allocator_typedef<options_t>::allocator > options_set_t;
//	options_set_t* options_;	// data layout option in the file
//	DataType	  datatype_;	// pixel or data element type in the file
//	int	precision;				// floating point precision for %g format.
public:
	Formatter();
	bool Init(const Nbaseutil::safestringlist& sl, Nbaseutil::ThrowHandlerFunc throwhandler);
	virtual ~Formatter();
	format_mode_t GetBinaryFormat() const {return format_mode_t(GetValue("bin_fmt", ser_ascii).GetInt());}
	DataTypeBase::DataType GetDataType() const {return DataTypeBase::DataType(GetValue("datatype", DataTypeBase::Void).GetInt());}
	bool HasOption(options_t opt) const;
	void SetPrecision(int precision_setting) {SetValue("precision", precision_setting);}
	int GetPrecision() const {return GetValue("precision", 6).GetInt();}
};

#ifdef _MSC_VER
#pragma warning(disable: 4251) // needs to have dll-interface to be used by clients of class
#endif

} // namespace NIMacro
#endif
