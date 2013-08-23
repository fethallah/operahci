#ifndef _IMACRO_PMEMBLOCK_H_INCLUDED_
#define _IMACRO_PMEMBLOCK_H_INCLUDED_

#include <math.h>
#include "memblock_export.h"	
#include "datatype.h"
#include <typeinfo>
#include <stddef.h> // for size_t

// Icc thinks that const and non-const assignment operators are different.
// If only const version is declared, icc would generate its own non-const version,
// which would work incorrectly. Thus we have to generate the non-const version as well.
// Msvc and gcc think that const and non-const assignment ops are the same. 
// If const version is provided, they will not generate their own.
// If both versions are given, msvc will produce a warning.
#ifdef __INTEL_COMPILER
/// Generate additional smartpointer assignment op with regular signature (needed by icc for resolving ambiguties).
#define ACAPELLA_GENERATE_REGULAR_SMARTPOINTER_ASSIGNMENT_OPERATOR
#else
/// Generate additional smartpointer assignment op from raw pointers (needed by msvc and gcc for resolving ambiguties).
#define ACAPELLA_GENERATE_RAWPOINTER_ASSIGNMENT_OPERATOR
#endif

#ifndef __GNUC__
#define ACAPELLA_GENERATE_INT_NULL_ASSIGNMENT
#endif


namespace NIMacro {


// Undefine min/max and restore in the end the header.
#if defined(max) || defined(min)
#	define max_was_defined
#	undef min
#	undef max
#endif

#ifndef _DEBUG
	inline int iround(double x) { return (x>0.0) ? int(x+0.5) : int(x-0.5); }
	inline unsigned int uiround(double x) { return static_cast<unsigned int>(x); }
#else
	inline int iround(double x) { 
#ifdef DEBUG_ASSERT
	  DEBUG_ASSERT((x>std::numeric_limits<int>::min()-0.5 && x<std::numeric_limits<int>::max()+0.5) || Nbaseutil::isnan(x));
#endif
		return (x>0.0) ? int(x+0.5) : int(x-0.5); 
	}
	inline unsigned int uiround(double x) { 
#ifdef DEBUG_ASSERT
		DEBUG_ASSERT(x>std::numeric_limits<unsigned int>::min()-0.5 && x<std::numeric_limits<unsigned int>::max()+0.5);
#endif
		return static_cast<unsigned int>(x); 
	}
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)	// unreferenced formal parameter
#endif

class DI_MemBlock MemBlock;
class DI_MemBlock DataItem;

/**
* MemBlock hierarchy smart pointer base class, uses intrusive refcounting.
* The creatable smartpointers are defined in the derived DerivedPointer template class.
* The refcount update is not thread-safe, so take care to access PMemBlocks only in
* one thread at a time (this holds for MemBlocks themselves as well).
*
* The MemBlock base class has methods Capture() and Release() for incrementing and decrementing
* the refcount; the operation itself is called aither capturing or releasing the object.
*
* The smartpointer can contain NULL. Assigning NULL to the smartpointer will release
* the pointer and break the connection to the object.
*
* As the refcounting is intrusive, one can extract a raw pointer from the smartpointer, pass 
* it around and assign to another smartpointer. One just needs to be sure that the object is alive
* during that operation, i.e. there is always at least one smartpointer to it present.
*
*/
struct DI_MemBlock PMemBlock {
public: // typedefs
	typedef MemBlock element_type;
public:
	/// Default ctor, initializes smartpointer to NULL.
	PMemBlock(): p(NULL) {}

	/// Construct from a raw pointer. Increments the object's refcount.
	PMemBlock(const MemBlock* x);

	/// Copy ctor. Increments the object's refcount.
	PMemBlock(const PMemBlock& b);

	/// Copy constructor with dynamic type checking. Throws an exception in case of conform test failure. Increments the object's refcount.
	PMemBlock(const PMemBlock& b, const char* ConformClass);

	/**
	* A version for optimizing cloning in style: x = x->Clone().
	* If a module needs to clone it's input parameter and is not interested in the old memblock,
	* it should call x.MakeClone() instead for better performance. 
	* Note that x = x->Clone() is usually only slightly worse because the encapsulated raw memory is in shared use
	* during the assignment and is not duplicated anyway.
	*/
	void MakeClone();


	/// Assignment op from a pointer. Assign NULL to release the pointed MemBlock. Releases the current object and captures the new one. Assigning a pointer to the same object is OK.
	void Assign(const MemBlock* x) const;

	/// Const assignment op (constness is associated with the pointed object, so const assignment op makes sense). Assigning a smartpointer to the same object is OK.
	const PMemBlock& operator=(const PMemBlock& b) const {Assign(b.Pointer()); return *this;}

#ifdef ACAPELLA_GENERATE_REGULAR_SMARTPOINTER_ASSIGNMENT_OPERATOR
	/// Assignment with the regular signature, for any case (icc next version?), just forward to the const op.
	PMemBlock& operator=(const PMemBlock& x) {Assign(x.Pointer()); return *this;}
#endif

#ifdef ACAPELLA_GENERATE_RAWPOINTER_ASSIGNMENT_OPERATOR
	PMemBlock& operator=(const MemBlock* x) {Assign(x); return *this;}
	const PMemBlock& operator=(const MemBlock* x) const {Assign(x); return *this;}
#endif

	/// Dtor, releases the pointed object when refcount drops to zero.
	~PMemBlock();

public: // pointer operations

	/// Implement the arrow operator. Transfer pointer constness to the object.
	const MemBlock* operator->() const { return p; }

	/// Implement the arrow operator. Transfer pointer non-constness to the object.
	MemBlock* operator->() { return p; }

	/// Implement the star operator. Transfer pointer constness to the object.
	const MemBlock& operator*() const { return *p;}

	/// Implement the star operator. Transfer pointer non-constness to the object.
	MemBlock& operator*() { return *p;}

public:
	/// Checks that the smartpointer is not NULL. See also operator void*().
	bool InUse() const { return p!=NULL; }

	/// Return reference to the pointed object, throw if NULL. Transfer pointer constness to the object.
	const MemBlock& Ref() const { if (!p) throw ::Nbaseutil::Exception(::Nbaseutil::ERR_PROGRAM_ERROR, "Dereferencing NULL PMemBlock pointer"); return *p; }				// Returns the content as a reference.

	/// Return reference to the pointed object, throw if NULL. Transfer pointer non-constness to the object.
	MemBlock& Ref() { if (!p) throw ::Nbaseutil::Exception(::Nbaseutil::ERR_PROGRAM_ERROR, "Dereferencing NULL PMemBlock pointer"); return *p; }				// Returns the content as a reference.

	/// Extract the raw pointer. Transfer pointer constness to the object. One can use the raw pointer only while it is sure that the object is alive. You can assign the raw pointer back to another smart-pointer with no problems.
	const MemBlock* Pointer() const { return p; }

	/// Extract the raw pointer. Transfer pointer constness to the object. One can use the raw pointer only while it is sure that the object is alive. You can assign the raw pointer back to another smart-pointer with no problems.
	MemBlock* Pointer() { return p; }

	/// Compare raw pointers.
	bool operator==(const void* x) const { return p==x; }

	/// Compare raw pointers.
	bool operator==(const PMemBlock& b) const { return p == b.p; }

	/// Compare raw pointers.
	bool operator!=(const void* x) const { return p!=x; }				

	/// Compare raw pointers.
	bool operator!=(const PMemBlock& b) const { return p!=b.p; }

#ifdef ACAPELLA_PTR64
	/// An unsigned integer type which can hold the pointer value.
	typedef Nbaseutil::uint64 uint_t;
#else
	/// An unsigned integer type which can hold the pointer value.
	typedef unsigned int uint_t;
#endif

	/// Convert the pointer into an unsigned integer. The return type size is guaranteed to be of sufficient size for holding the pointer. 
	size_t Int() const { return (size_t) p; }

	/**
	* This operation is similar to dynamic_cast<>.
	*
	* Checks if the pointed memblock is not NULL and conforms to the specified class. 
	* This function fowards to MemBlock::Conforms(). It is present in the PMemBlock
	* class in order to avoid debug-build assertion when dereferencing a non-conforming pointer.
	*
	* The Conforms() mechanism also accepts some pseudo class Ts like "stencil" and "mask".
	* It also works in the case where the actual class definition is in another library
	* and not visible to the compiler.
	*
	* If pointer is NULL, Conforms() returns false.
	*/
	bool Conforms(const char* ClassT) const;


	/**
	* Checks if the object is of the specified class or another representation of the specified class.
	* In the latter case it is converted to the specified class and *this is replaced to point to the converted object.
	* This function is marked const as constness is associated with the pointed object and pointer reseating does not change the object.
	* @return True if the smartpointer points to the object of specified type or its derived type after the call.
	*/
	bool TryConform(const char* classname) const;

	/**
	* For safety, PMemBlock does not define a direct conversion method
	* to MemBlock*, the raw pointer can be obtained via the Pointer()
	* member function if needed.
	* 
	* However, PMemBlock defines an 'operator void*' conversion. This serves
	* for two purposes: first, enables the "if(ptr)" syntax, and second,
	* allows one to store PMemBlocks to use as keys in STL containers, which
	* then use std::less<void*, void*> for ordering.
	*/
	operator void*() const {return p;}	// Equivalent to InUse() method.		

	/// Fast nothrow swap operation
	void swap(PMemBlock& b) throw() {std::swap(p, b.p);}

	/**
	* Check if the smartpointer points to a relevant object
	* and cast it to the proper reference, or throw.
	*/
	template<class T> T& As();

	/// Obsolete, kept for binary back-compatibility.
	static bool do_dereference_check();
 
private: // implementation
	bool MakeBackwardCompatAutoConversionsToConform(const char* classname) const;

protected: // data
	friend class DataItem;
	mutable MemBlock* p;	// The managed pointer.

};

inline DataTypeBase::DataType TypeOf(const PMemBlock& dummy) {return DataTypeBase::PMemory;}

/// @cond implementation_details
// Internal debugging support functions.
DI_MemBlock bool MemBlockConformsInternal(const PMemBlock& m, const char* cls);
DI_MemBlock void IllegalMemBlockCast(const MemBlock* p, PMemBlock factory_obj);
DI_MemBlock void Dereference_check_dbg(const MemBlock* p);
/// @endcond

template<class T> 
struct DerivedPointer;

template<typename Dst>
bool MemBlockConforms(const MemBlock* p, const DerivedPointer<Dst>& dummy) {
	if (dynamic_cast<const Dst*>(p)==NULL) {
		IllegalMemBlockCast(p, Dst::Create());
		return false;
	}
	return true;
}

class DI_MemBlock Image;

// Specialization for image class; the image class is not yet defined everywhere where this is needed.
template<>
inline bool MemBlockConforms<Image>(const MemBlock* src, const DerivedPointer<Image>& dummy) {
	return MemBlockConformsInternal(src, "image");
}

/// Actual template class for producing PMemBlock hierarchy smartpointers.
template<class T> 
struct DerivedPointer: public PMemBlock {
public: // typedefs
	typedef T element_type;
public: // interface
	DerivedPointer() {}
	/// The ctor accepts any kind MemBlock hierarchy object pointer.
	DerivedPointer(const MemBlock* x) : PMemBlock(x) {}
	/// The ctor accepts any kind MemBlock hierarchy object pointer.
	DerivedPointer(const PMemBlock& b) : PMemBlock(b) {}
	/// The ctor accepts any kind MemBlock hierarchy object pointer.
	DerivedPointer(const PMemBlock& b, const char* ConformClass): PMemBlock(b, ConformClass) {}

#ifdef ACAPELLA_GENERATE_INT_NULL_ASSIGNMENT

	/// Assignment of NULL. Any other value than 0 is invalid here.
	const DerivedPointer& operator=(int) const {Assign(0); return *this;}

	/// Assignment of NULL. Any other value than 0 is invalid here.
	DerivedPointer& operator=(int) {Assign(0); return *this;}
#endif

	/// The assignment accepts any kind MemBlock hierarchy object pointer.
//	const DerivedPointer& operator=(const PMemBlock& b) const {Assign(b.Pointer()); return *this; }

	/// Const assignment operator
	const DerivedPointer& operator=(const DerivedPointer& b) const {Assign(static_cast<const PMemBlock&>(b).Pointer()); return *this; }

#ifdef ACAPELLA_GENERATE_REGULAR_SMARTPOINTER_ASSIGNMENT_OPERATOR
	/// Assignment with the regular signature, for any case (icc next version?), just forward to the const op.
	DerivedPointer& operator=(const DerivedPointer& b) {Assign(static_cast<const PMemBlock&>(b).Pointer()); return *this; }
#endif

#ifdef ACAPELLA_GENERATE_RAWPOINTER_ASSIGNMENT_OPERATOR
	DerivedPointer& operator=(const MemBlock* x) {Assign(x); return *this;}
	const DerivedPointer& operator=(const MemBlock* x) const {Assign(x); return *this;}
#endif

	/// Perform conformance check on the pointer before dereferencing. Throw if NULL or the dynamic type does not conform to the static type.
	void Dereference_check() const {
#ifdef _DEBUG
		Dereference_check_dbg(p);
		MemBlockConforms<T>(p, *this);
#endif
	}
	
	T* operator->() { 
		Dereference_check();
		return (T*) p; 
	}
	T& operator*() { 
		Dereference_check();
		return (T&) *p;
	}
	T& Ref() {
		Dereference_check();
		return (T&) *p; 
	}
	T* Pointer() {return (T*) p; }
	const T* operator->() const { 
		Dereference_check();
		return (T*) p; 
	}
	const T& operator*() const { 
		Dereference_check();
		return (T&) *p;
	}
	const T& Ref() const {
		Dereference_check();
		return (T&) *p; 
	}
	const T* Pointer() const {return (T*) p; }
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

} // namespace

namespace std {
	/// Overload of std::swap() for PMemBlocks, slightly faster.
	inline void swap(::NIMacro::PMemBlock& a, ::NIMacro::PMemBlock& b) {
		a.swap(b);
	}
} // namespace std

#ifdef max_was_defined
#	define max(a,b) ((a)>(b) ? (a): (b))
#	define min(a,b) ((a)<(b) ? (a): (b))
#	undef max_was_defined
#endif

#endif
