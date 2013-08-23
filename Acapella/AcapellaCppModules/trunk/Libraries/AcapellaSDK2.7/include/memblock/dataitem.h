#ifndef _DATAITEM_H_INCLUDED_
#define _DATAITEM_H_INCLUDED_

#include <map>

#include "datatype.h"
#include "threadsharablepointer.h"
#include "pmemblock.h"

namespace NIMacro {

// forward declarations
class DI_MemBlock MemBlock;
struct DI_MemBlock PMemBlock;
class ThreadSharable;
typedef ThreadSharablePointer<ThreadSharable> PSharable;
//template class DI_MemBlock ThreadSharablePointer<ThreadSharable>;		// This trick is needed for avoiding MSVC++ warning C4251.

class SerializeVisitor;
class Formatter;
class TraverseVisitor;
struct TraverseNode;

/// Softlinks currently not yet implemented.
//enum softlink_t {
//	softlink_rel,
//	softlink_abs,
//};

struct triple;

struct Missing{};

class DataItem;

/// UtfPosCache maps UTF8 character positions to byte positions in large strings.
class DI_MemBlock UtfPosCache: public Nbaseutil::mb_malloced {
public:
	int& operator[](int charpos) {
		return map_[charpos];
	}
	int operator[](int charpos) const {
		map_t::const_iterator p=map_.find(charpos);
		return p==map_.end()? -1: p->second;
	}
private:
	friend class DataItem;
	typedef std::map<int,int,std::less<int>,Nbaseutil::mb_allocator_typedef<std::map<int, int>::value_type>::allocator> map_t;
	map_t map_;
};

/// A class for providing an opaque buffer for storing a safestring.
class opaquestring: private Nbaseutil::safestring {
	Nbaseutil::safestring& Ref() {return *this;}
	friend class DataItem;
};

/** Class for holding different type of values. In content queries, if the content is of wrong type, 0 or empty result is returned.
*
* The possible types are given by NIMacro::DataType enumeration.
*
* This class contains no virtual methods.
* Note that SafeValue is derived from DataItem and overrides some functions.
* DataItem and SafeValue can be cast freely into each other. SafeValue is
* more pickier and throws more excpetions if the data is not convertible to
* the requested type.
*/
class DI_MemBlock DataItem: public Nbaseutil::mb_malloced {
public: // typdefs

public: // interface

	/// Constructs an Undefined DataItem.
	DataItem() {
		Init();
		type_=Undefined;
		x.k = 0;
	}

	/// Contructs a Missing dataitem, to be deleted from datablock upon save.
	DataItem(Missing) {
		Init();
		type_=Undefined;
		x.k = 1;
	}

	/// Constructs an empty DataItem of type t.
	DataItem(ItemType t) {
		Init();
		switch(t) {
			case Integer: x.k = 0; break;
			case Integer64: x.k64 = 0; break;
			case Floating: x.f = 0; break;
			case Pointer: case Memory: case Sharable: x.p = 0; break;
			case Asciiz: x.buf_[0] = 0; type_ = 0; return;
			case Undefined: x.k=0; break;
			case PolyType: t=Integer; x.k = 0; break;
		}
		type_ = t;
	}

	/// Constructs an integer DataItem.
	DataItem(int k) {
		Init();
		type_=Integer;
		x.k = k;
	}

	/// Constructs an integer or floating-point dataitem, depending on the value of k.
	DataItem(unsigned int k);

	/// Encapsulates a 64-bit integer.
	DataItem(Nbaseutil::int64 k) {
		Init();
		type_=Integer64;
		x.k64 = k;
	}

	/// Constructs an integer or floating-point dataitem, depending on the value of k.
	DataItem(Nbaseutil::uint64 k);

	/// Constructs an integer dataitem by casting triple to int.
	DataItem(const triple& k);

	/// Constructs a floating-point dataitem.
	DataItem(double f) {
		Init();
		type_=Floating;
		x.f = f;
	}
	/// Constructs a naked pointer dataitem. This is explicit in order to avoid surprises when converting Sharable or string pointers.
	explicit DataItem(const void *p) {
		// no memory management is done for void pointers
		Init();
		type_=Pointer;
		x.p = p;
	}

	/// Constructs a Memory type DataItem by encapsulating a smartpointer to m.
	DataItem(MemBlock &m);

	/// Constructs a Memory type DataItem by encapsulating a smartpointer to *m.
	DataItem(const PMemBlock &m);

	/// Constructs a Sharable type DataItem by encapsulating a smartpointer to m.
	DataItem(ThreadSharable &m);

	/// Constructs a Sharable type DataItem by encapsulating a smartpointer to *p.
	DataItem(PSharable p);

	/// Constructs a string DataItem. The passed string must be in UTF-8 encoding, it is copied into the DataItem.
	DataItem(const char* s);

	/// Constructs a string DataItem.
	DataItem(const Nbaseutil::safestring& s);

	/// A special value to pass to the DataItem(safestring, UtfPosCache*) constructor.
	static UtfPosCache* const c_IsAscii;

	/**
	* Constructs a string DataItem, with known UTF-8 codepoint locations in the string.
	* This constructor can be used for better performance if the codepoint locations are
	* known before DataItem construction.
	* @param s The string in UTF-8 encoding. The string contents are swapped over to the DataItem internals.
	* @param utfposcache Information about multibyte character positions in the string.
	*		The ownership of utfposcache is passed over to the
	*		function. Pass special constant c_IsAscii if it is known that the string does not contain
	*		multibyte characters. Pass NULL if the presence of absence of multibyte characters is not known.
	*/
	DataItem(Nbaseutil::safestring& s, UtfPosCache* utfposcache);

	/// Copy ctor.
	DataItem(const DataItem& a);

	/// Assignment op.
	DataItem& operator=(const DataItem& a);

	/// Comparison operator for sorting. If both items numeric, sorts numerically, otherwise alphabetically by GetString() results.
	bool operator<(const DataItem &a) const;

	/**
	* Value comparison operator. Returns false immediately if items are of different types.
	* Ascii characters in strings are compared case-insensitive.
	* For MemBlock types calls MemBlock::operator==().
	* For naked pointers and Sharable types only pointers are compared.
	*
	* See also Compare().
	*/
	bool operator==(const DataItem &a) const;

	/// Comparison operator, exact negation of operator==().
	bool operator!=(const DataItem &a) const {return !(*this==a);}

	/**
	* More tolerant comparison, makes some data conversions if items are of different type:
	*    - integers are converted to double for comparison with double.
	*    - numeric strings are converted to int64 or double for comparing with numbers.
	*    - other things are converted to string representation for comparing with strings and with each other.
	* This forwards to string_compare() for numeric string handling.
	* @return Comparison result:
	*    - 0: equal
	*    - -1: this is less than a.
	*    - +1: this is greater than a.
	*    - -2: One of the operands is NaN.
	*/
	int Compare(const DataItem& b) const;

	/**
	* Makes a deep physical copy of any contained MemBlock/Sharable hierachies.
	* The resulting DataItem can be safely passed over thread boundaries.
	* The contained items are either not accessible from anywhere else, or they
	* have internal locking which allows multithread access (the latter is the case
	* for some Sharable types).
	*/
	void MakeCopy();

	~DataItem();

	// type_ queries:

	/// Return DataItem type.
	ItemType GetType() const {return type_? ItemType(type_): Asciiz;}

	/// Check if item is not Undefined. This is not 'operator bool()' in order to avoid unwanted integral conversions.
	operator void* () const {
		return type_==Undefined? NULL: (void*) this;
	}

	bool IsMissing() const {
#ifdef DEBUG_ASSERT
		DEBUG_ASSERT(type_ != Undefined || (x.k==0 || x.k==1));
#endif
		return type_==Undefined && x.k==1;
	}

	// Value returning:

	/**
	* Return the value converted to integer, or 0 if conversion not possible.
	*/
	int	GetInt() const;

	/**
	* Return the value converted to 64-bit integer, or 0 if conversion not possible.
	*/
	Nbaseutil::int64 GetInt64() const;

	/// Return the value in double form, or 0.0.
	double GetDouble() const;

	/// Return the value in double form, or 0.0.
	double GetFloating() const {return GetDouble();}

	/**
	* Return the pointer to the value in case of type_==Memory and type_==Pointer DataItems.
	* In case of Memory the pointer to the actual malloced memory block is returned.
	* One should not use this pointer for modification the contained value.
	* If the contained value is MemBlock, it lives independently of DataItem.
	* In case of other types NULL is returned.
	*/
	const void*	GetPointer() const;

	/// Return the content in string repr. For numeric types the number is printed in decimal format.
	Nbaseutil::safestring GetString() const;

	/**
	* Return const reference to the encapsulated string, if the DataItem holds a single safestring.
	* Otherwise, the string representation of the item is stored in the buffer and reference to the buffer is returned.
	* Pass in a local variable of type opaquestring as the buffer argument; it cannot be accessed directly, instead one should use
	* the returned reference only.
	*/
	const Nbaseutil::safestring& GetString(opaquestring& buffer) const;

	/// Return the content in string repr, possibly more effective than GetString(). For numeric types the number is printed in decimal format.
	void GetStringTo(Nbaseutil::safestring& buffer) const;

	/** Return pointer to the internal UtfPosCache object, if the contained object is a large string.
	* If the string is pure ASCII, the constant DataItem::c_IsAscii is returned.
	* If the string is of unknown content, NULL is returned.
	*/
	UtfPosCache* GetStringUtfPosCache() const {
		return type_==Stringy? x.s.utfposcache_: NULL;
	}

	/// Converts UTF-8 character position to the byte position in the contained string.
	int CharPosToBytePos(int charpos) const;

	/// Converts byte position to the UTF-8 character position in the contained string.
	int BytePosToCharPos(int bytepos) const;

	/// Return the content in string repr. This should be used only if the item is a string or a 1-element stringvector. In case of other types a static buffer is used, which makes the function thread-unsafe.
	const char* GetStringPointer() const;

	/// Return the string length for Asciiz/Stringy or 1-element stringvector type DataItem, otherwise 0.
	int GetStringLength() const {
		return type_==Asciiz? int(x.s.p_->length()): (type_==0? small_str_len(): (IsSingleStringVector()? (int) SingleStringRef().length(): 0));
	}

	/// Return an IMacro constant expression which would evaluate to this data item.
	Nbaseutil::safestring GetExpression() const;

	/// Return an IMacro constant expression which would evaluate to this data item. If it is a floating-point numeric, then the specified precision is used with the %.XXXg printf format.
	Nbaseutil::safestring GetExpression(int precision) const;

	/// Return an informal description of the data item.
	Nbaseutil::safestring GetDescription() const;

	/**
	* Return the pointer to the holded MemBlock object for Memory type items.
	* Also, if the DataItem is a number or string, returns 1-element vector containing that.
	* Returns NULL otherwise.
	*/
	PMemBlock GetMemory() const;

	/// Return the pointer to the holded ThreadSharable object, or NULL.
	PSharable GetSharable() const;

	/** Return the pointer to the holded MemBlock or derived class object,
	* if the holded object class corresponds to the argument.
	* Supported values for parameter are: "memblock", "image", ...
	* If the type is wrong or other error appears, NULL is returned.
	* If the return value is not NULL, then it is safe to cast the result to the corresponding class pointer.
	* @param classname This is passed to MemBlock::Conforms() for checking the dynamic type.
	* @param throwing If true, then Exception is thrown instead of NULL return.
	*/
	PMemBlock GetMemoryOfClass(const char* classname, bool throwing=false) const;

	/// Deprecated, use GetMemoryOfClass() instead.
	PMemBlock GetMemoryClass(const char* Class, bool throwing=false) const;

	/// Back-compatibility function.
	const DataItem& GetSafeValue() const {return *this;}

	/// Resolve the subkey. If subkey is empty, returns itself. If subkey resolving fails, throws an Exception.
	DataItem ResolveSubItem(const Nbaseutil::safestring& subkey) const;

	/// Same as the other version of ResolveSubItem(), but uses throwhandler for reporting exceptions, and returns Undefined item in such cases.
	DataItem ResolveSubItem(const Nbaseutil::safestring& subkey, Nbaseutil::ThrowHandlerFunc throwhandler) const;

	/**
	* Gets a subitem, if it exists and is convertible to the desired type,
	* otherwise uses the default and returns false.
	*/
	bool Fetch(const Nbaseutil::safestring& itempath, int& value, int defaultvalue) const;
	bool Fetch(const Nbaseutil::safestring& itempath, Nbaseutil::int64& value, Nbaseutil::int64 defaultvalue) const;
	bool Fetch(const Nbaseutil::safestring& itempath, double& value, double defaultvalue) const;
	bool Fetch(const Nbaseutil::safestring& itempath, Nbaseutil::safestring& value, const Nbaseutil::safestring& defaultvalue) const;
	bool Fetch(const Nbaseutil::safestring& itempath, PMemBlock& value, const PMemBlock& defaultvalue) const;
	bool Fetch(const Nbaseutil::safestring& itempath, DataItem& value, const DataItem& defaultvalue) const;

	/// Get a subitem, if exists and convertible, otherwise throw
    void Fetch(const Nbaseutil::safestring& itempath, int& value) const;
    void Fetch(const Nbaseutil::safestring& itempath, Nbaseutil::int64& value) const;
    void Fetch(const Nbaseutil::safestring& itempath, double& value) const;
    void Fetch(const Nbaseutil::safestring& itempath, Nbaseutil::safestring& value) const;
    void Fetch(const Nbaseutil::safestring& itempath, PMemBlock& value) const;
    void Fetch(const Nbaseutil::safestring& itempath, DataItem& value) const;

	/// Remember the index of the module call in the Script which created this dataitem. Return the previous value.
	// Enh 03.03.2004: check that parameter fits in the iCreator member.
	// Change 03.03.2004: moved SetCreator offline because of conflict with Windows' min/max macros.
	int SetCreator(int k);

	/**
	* Return the index of the module call in the Script which created this dataitem.
	* If the dataitem was passed in as a procedure input argument, then the procheader_signature constant (a negative value) is returned.
	* If the dataitem origin is unknown, -1 is returned.
	*/
	int GetCreator() const {return iCreator;}

	 /// Return the internal pointer to the value.
	 const void* GetPrivatePointer() const;

	 /// Return the size of the memory pointed to by GetPrivatePointer() (Acapella 2.5).
	 size_t GetPrivateLength() const;

	 /// Verify the inherent consistency of data item; add any error messages to msg.
	 bool Consistent(Nbaseutil::safestring& msg);

	/// Return true if this is a string dataitem in any representation.
	bool IsString() const;

	/// Fast no-throw swap operation
	void swap(DataItem& b) throw() {
		std::swap(x, b.x);
		// std::swap does not like arrays, do it manually.
		// cannot cast to an int32 because of fear of type-punning optimizations.
		char buff[4];
		for (int i=0; i<4; ++i) buff[i] = buf_continued_[i];
		for (int i=0; i<4; ++i) buf_continued_[i] = b.buf_continued_[i];
		for (int i=0; i<4; ++i) b.buf_continued_[i] = buff[i];
		std::swap(iCreator, b.iCreator);
		std::swap(type_, b.type_);
	}

	/// If the DataItem points to a MemBlock, swaps the MemBlock pointers with the argument.
	void Swap(PMemBlock& m) throw() {
		if (type_==Memory) {
			std::swap(x.m, m.p);
		}
	}

	/**
	* Swaps the content of s into the DataItem.
	* @param s New string in UTF-8. If the previous type of the DataItem was string, the old string is returned in the s parameter after the call.
	* @param utfposcache Pointer to the correct utf positions cache object for the new string, or NULL. The ownership of the object is passed over to the DataItem.
	*/
	void Swap(Nbaseutil::safestring& s, UtfPosCache* utfposcache);

	/// Convert scalar to 1-element vector, if this is a scalar.
	void ScalarToVector();

	/// Convert 1-element vector to scalar, if this is a vector
	void VectorToScalar();

	/// Serialize the item. See MemBlock::Serialize() for description of the parameters. Calls Serialize() for non-NULL MemBlock and Sharable objects, does nothing otherwise.
	bool Serialize(int ser_format, SerializeVisitor& visitor, const Nbaseutil::safestring& itemname, const Formatter& formatter, Nbaseutil::safestring& buffer) const;

	/// Traverse the item and call visitor for itself and contained items.
	void Traverse(TraverseVisitor& visitor, const TraverseNode& node);

	/// Calculate the total amount of memory taken up by this DataItem, in deep recursion.
	Nbaseutil::uint64 CalcMemUsage() const;

private: // implementation

	/// Initialization; this is called from all constructors.
	inline void Init() {
		iCreator = -1;
	}

	bool IsSingleStringVector() const;

	const Nbaseutil::safestring& SingleStringRef() const;

	int small_str_len() const {
		for (int i=0; i<c_bufsize; ++i) {
			if (x.buf_[i]==0) return i;
		}
		return c_bufsize;
	}
	void BuildUtfPosCache() const;
	void Assign(const Nbaseutil::safestring& s);
	void AssignOrSwap(Nbaseutil::safestring& s);

private: // data
	// MSVC wants to align DataItem to 24 bytes instead of 20 bytes anyway.
	// In this case we can make small string internal buffer to 20 bytes instead of 16.
	// MSVC insists of making the union 24 bytes if we define char buf[20].
	// Make it char buf[20] + filler char buf1[4].
	static const int c_bufsize=20;
	union xunion {
		int k;			// Value for type_==Integer
		Nbaseutil::int64 k64; // Value for type_==Int64
		double f;		// Value for type_==Double
		const void *p;	// Value for type_==Pointer; pointer to the value for other types.
						// const attribute is used as data pointed from DataItems is generally considered constant.
		MemBlock* m;    // for studying in debugger
		struct str_struc {
			Nbaseutil::safestring* p_;	// pointer to the string object, for large strings.
			mutable UtfPosCache* utfposcache_;		// pointer to the object caching known UTF-8 character locations in the string.
		} s;
		char buf_[c_bufsize-4];  // internal small string buffer.
	} x;
	char buf_continued_[4];
	char type_; // Either ItemType constant or 0 for small string buffer termination.
	short iCreator;	// The modulecall index in the current Script which created or modified this data item in the current datablock.
};

/// A base class for scanning DataItems.
class ItemVisitor: public Nbaseutil::mb_malloced {
public:
	/** Visit will be called for each scanned item.
	* In sName the item name will be passed, if the item has a name in the given context.
	* Otherwise, an empty string will be passed.
	* The item itself will be passed as the second parameter.
	*/
	virtual void Visit(const Nbaseutil::safestring& sName, const DataItem& oItem) = 0;
	virtual ~ItemVisitor() {}
};

/**
* @brief A convenience helper class for forwarding the ItemVisitor callback to a class method instead of a free function.

See the NIMacro::ItemVisitor class.
Class T must contain method:
@code
void VisitItem(const PString& sName, const DataItem& oItem);
@endcode
This will be called for each item encountered in the relevant ScanItems() operation.
*/
template<class T>
class CallBackItemVisitor: public ItemVisitor {
	T& t_;
public:
	CallBackItemVisitor(T& t): t_(t) {}
	virtual void Visit(const Nbaseutil::safestring& sName, const DataItem& oItem) {
		t_.VisitItem(sName, oItem);
	}
};

struct DataItemMemBlockSwapper {
	PMemBlock mb_;
	DataItem& di_;
	DataItemMemBlockSwapper(DataItem& di): di_(di) {di.Swap(mb_);}
	~DataItemMemBlockSwapper() {di_.Swap(mb_);}
};


/// Constructs a numeric DataItem from a numeric string, choosing the appropriate type.
DI_MemBlock DataItem Numeric(const Nbaseutil::safestring& s);

/**
* Compares strings alphabetically or numerically. 
* If both strings are numeric (including Acapella predefined numeric pseudoconstants),
* then the comparison is done numerically (in int64 if both strings are integral and fit in
* the signed int64 range, in double otherwise).
* @return Comparison result:
*    - 0: equal
*    - -1: s1 is less than s2.
*    - +1: s1 is greater than s2.
*    - -2: One of the operands is NaN.
*/
DI_MemBlock int Compare(const Nbaseutil::safestring& s1, const Nbaseutil::safestring& s2);

} // namespace

namespace std {
	/// Overload of std::swap for DataItems, slightly faster.
	inline void swap(::NIMacro::DataItem& a, ::NIMacro::DataItem& b) {
		a.swap(b);
	}
} // namespace std


#endif
