#ifndef x_ACAPELLA_MEMBLOCK_VECTOR_H_INCLUDED_
#define x_ACAPELLA_MEMBLOCK_VECTOR_H_INCLUDED_

#include "memblock.h"
#include "error.h"
#include "typedchunk.h"

// Windows evil min/max macro guards
#ifdef min
#define ACAPELLA_EVIL_MIN_WAS_DEFINED
#undef min
#endif
#ifdef max
#define ACAPELLA_EVIL_MAX_WAS_DEFINED
#undef max
#endif

namespace NIMacro {

/// Possible rounding modes for Vector::RealToPixel().
enum rounding_t {
	round_none=0,
	round_down=1,
	round_up=2,
	round_nearest=3,
};

class DI_MemBlock Vector;
typedef DerivedPointer<Vector> PVector;

class DI_MemBlock Table;
typedef DerivedPointer<Table> PTable;

/// @cond Implementation_details
template<typename STDVECTOR>
inline void StdVectorDeleter_internal(void* p, void* callback_data) {
	delete static_cast<STDVECTOR*>(callback_data);
}
/// @endcond

/// Class for encapsulating uniform data arrays.
/** The objects of Vector class encapsulate a 1D array of elements. The element type may be numeric, string,
* PMemBlock or DataItem (polytype). The usage of polytype Vector's is not encouraged as very few modules can 
* cope with them, and it somewhat goes against the Vector ideology of having a plain array of similar type items.
*
* The derived classes Image and DataCube maintain their own structure of the data, but they can be always viewed 
* as straight 1D Vectors.
*
* The Vector class introduces the 'raw factor' concept. This is a floating-point scaling factor applying for all of the data.
* 
*
* For setting and extracting single elements one can use the SetElem(), GetXXX(), ElemXXX() methods and operator[].
* These are typesafe and convert the elements as needed. SetElem() and ElemXXX() methods also perform bounds check for the element
* index. 
*
* When working with the whole array these methods are generally too slow. Instead one takes out the raw pointer 
* to the data array with a proper XXXPointer() method and works directly with that. Note that the raw pointer may
* get invalidated when the Vector size is changed. Also note that the const raw pointer may get invalid if later a non-const 
* pointer is obtained (see MemBlock::Pointer()). The drawback is that the raw pointers may be of multiple different types.
*
* The XXXPointer() methods are typesafe and throw if the data cannot be cast to the requested type. They allow only
* signed-unsigned casting by historic reasons.
* Instead of the several XXXPointer() methods you can call base class MemBlock::Pointer() and cast the resulting void* pointer to the 
* correct type. This is faster, but not typesafe and therefore not recommended, unless you are using the DISPATCH macros - see below.
*
* Usually the module must support incoming vectors of different data types. This is naturally done by templates. 
* However, as the vector type is not known until run-time the dispatch to templates must be also done at run-time.
* The dispatch is usually made by the switch on the vector type returned by the ElemType() method. In order to simplify 
* this task there are several DISPATCH macros defined, like DISPATCH_ALL, DISPATCH_NUMERIC, etc., which encode the whole 
* switch with a single line. The drawback is that the data pointers going into the template function are void*, and the template
* must cast them to the correct type (template parameter) itself.
*
*/
class DI_MemBlock Vector: public MemBlock, public DataTypeBase {
	typedef MemBlock super;
public: // enums and typedefs

	/// Key constants for using in SecondaryItem mechanism.
	enum MapKey {
		/// Maximum raw pixel value in data array (element type).
		key_max = 100,			
		/// Minimum raw pixel value in data array (element type). 
		key_min = 101,
		/// Mean raw pixel value (double).
		key_mean = 102,
		/// Median raw pixel value (element type).
		key_median = 103,
		/// Value equal or not very much larger than key_max. Deprecated, do not use in new code. (Element type.)
		key_rough_max = 104,
		/// Sum of all elements of the vector (suitable type).
		key_sum = 105,
		/// Width of the image, if vector may represent an image. is not calculated automatically. see read_hdf() and toimage() modules.
		key_width = 106,
		/// Standard deviation (double).
		key_stddev = 107,
		// Sample histogram of the data (Table).
		key_histogram = 108,	
		/// Square of key_stddev (double).
		key_variance = 109,
		/// Same as key_median except in case of even number of elements returns the average of two middle elements.
		key_fmedian=110,
		/// Index of the first element not less than any other element.
		key_maxat=111,
		/// Index of the first element not larger than any other element.
		key_minat=112,
		/// Same as key_min except for floating-point types infinities are ignored.
		key_finite_min = 113,
		/// Same as key_max except for floating-point types infinities are ignored.
		key_finite_max = 114,
		/// Keeps MD5 of pixel data only, for faster recomputing of MD5 hashes.
		key_md5_pixeldata = 115,
		/// Key range 11000..11256 reserved for storing dimension sizes for multidimensional objects.
		key_dimsize=11000,		
	};

public: // static interface
	/**
	* Creates a Vector of indicated type elements and allocates memory. 
	* The memory is default initialized (to zero or empty objects).
	* @param length				Number of elements in the array.
	* @param type				Type of the elements.
	* @param init_to_zero		Obsolete, not used.
	* @param first_elem_index	Deprecated, do not use in new code (pass zero). Starting index of the data in the allocated array. If larger than 0, 
	*                           the beginning elements of the buffer remain unused. 
	* @param factor				Raw factor associated with the data. The factor can be later changed by SetFactor().
	*/
	static PVector Create(size_t length, DataType type, bool init_to_zero=true, int first_elem_index=0, double factor=1.0);	

	/**
	* @brief Creates a Vector by encapsulating an existing memory chunk.
	* @param length				Number of elements in the array.
	* @param type				Type of the elements.
	* @param p_data				Pointer to the memory chunk. If the type is an object type, then the memory has to contain initialized C++ objects.
	* @param p_free				Pointer to a function that can release the memory chunk. If memory has been allocated by malloc(), then just pass &free.
	*                           If the memory does not need to be released, pass NULL.
	* @param first_elem_index	Starting index of the data in the p_data array. If larger than 0, 
	*                           then it means that the beginning elements of the buffer are unused.  
	* @param factor				Raw factor associated with the data. The factor can be later changed by SetFactor().
	*/
	static PVector Create(size_t length, DataType type, void* p_data, MemChunk::ReleaseFunc1 p_free, int first_elem_index=0, double factor=1.0);

	/**
	* Same as another Create() except that the release function takes an additional callback_data parameter.
	* @param length				Number of elements in the array.
	* @param type				Type of the elements.
	* @param p_data				Pointer to the memory chunk. If the type is an object type, then the memory has to contain initialized C++ objects.
	* @param p_free				Pointer to a function that can release the memory chunk. Both p_data and callback_data values will be passed back to it.
	* @param callback_data      This value is passed back to the p_free function.
	* @param first_elem_index	Starting index of the data in the p_data array. If larger than 0, 
	*                           then it means that the beginning elements of the buffer are unused.  
	* @param factor				Raw factor associated with the data. The factor can be later changed by SetFactor().
	*/
	static PVector Create(size_t length, DataType type, void* p_data, MemChunk::ReleaseFunc2 p_free, void* callback_data, int first_elem_index=0, double factor=1.0);

	/**
	* Create a Vector from a TypedChunk object. This is the most general creation mechanism of Vector objects.
	* @param typedchunk A TypedChunk object specifying the data array and type information. 
	* The data array will remain in shared use by Vector and argument TypedChunk, until
	* the data array is modifed through the Vector interface or the non-const pointer to it is obtained.
	* For better performance, do not keep typedchunk argument object around without a reason.
	* @param factor	Raw factor associated with the data. The factor can be later changed by SetFactor().
	*/
	static PVector Create(const TypedChunk& typedchunk, double factor=1.0);


	/**
	* Create a Vector or higher dimension object (Image, DataCube, GeomBlock<N>) from a TypedChunk object.
	* @param rank The object rank (number of dimensions).
	* @param dimsizes An array specifying dimension sizes for each dimension.
	* @param elem_type Type of vector elements.
	* @param factor The raw factor.
	* @return Pointer to the created N-cube. The data is initialized to zero.
	*/
	static PVector CreateNCube(int rank, const unsigned int dimsizes[], DataType elem_type, double factor=1.0);

	/// Encapsulates a single number into a 1-element Vector of type Vector::Int.
	static PVector CreateFrom(int i); 

	/// Encapsulates a single number into a 1-element Vector of type Vector::Double.
	static PVector CreateFrom(double f);

	/// Encapsulates a single string into a 1-element Vector of type Vector::String.
	static PVector CreateFrom(const Nbaseutil::safestring& s); 

	/// Encapsulates a single dataitem into a 1-element Vector of type Vector::PolyType. Note: PolyType vectors should be avoided on script level.
	static PVector CreateFrom(const DataItem& s);

	/// Transfers a memory block from Carrier into a Vector of type Vector::Byte. This is a fast operation. Carrier will be left empty.
	static PVector CreateFrom(const Nbaseutil::Carrier& x);

	/// Encapsulates a MemBlock into a single-element PMemory Vector.
	static PVector CreateFrom(const PMemBlock& x);

	/** Create a Vector by a std::vector. The content will be swapped into the created Vector. 
	* @param stdvector A non-const reference to a std::vector of any element type accepted by Vector. The stdvector object will be empty after the call.
	*/
	template<typename STDVECTOR>
	static PVector CreateFromStdVector(STDVECTOR& stdvector) {
		STDVECTOR* holder = new STDVECTOR();
		holder->swap(stdvector);
		PVector v = Create(holder->size(), CType2ElemType(typename STDVECTOR::value_type()), holder->empty()? 0: &(*holder)[0], &StdVectorDeleter_internal<STDVECTOR>, holder);
		// std::vector will destroy the objects in the buffer by itself, turn off destroy in Vector
		v->typedchunk_.SetNoDestroy(); 
		return v;
	}

	/** Create a Vector of Byte type from a safestring. The content will be swapped into the created Vector. 
	* @param buffer A non-const reference to a safestring. The buffer object will be empty after the call.
	*/
	static PVector CreateFromSafeString(Nbaseutil::safestring& buffer);

	/// Create an empty vector of type Vector::Byte.
	static PVector Create();

	// Addition 15.08.2003: take over MakeSequence functionality from Container.
	/// Create a vector containing a sequence from a to b.
	static PVector MakeSequence(const Nbaseutil::safestring& a, const Nbaseutil::safestring& b);

	/// Return the script-level attribute name corresponding to a key_* constant.
	static const char* Key2string(int key);

	/// Dynamic vector result type: find a suitable type for data range [resmin, resmax].
	static Vector::DataType MinMax2Type(double resmin, double resmax);

	/// Static vector result type: find a suitable type which can accomodate both types a and b.
	static Vector::DataType ChooseCoverType(Vector::DataType a, Vector::DataType b);

	/// Parse a type in string representation, e.g. "signed int" or "uint16". Words are separated by spaces, commas, semicolons or tabs.
	static DataType String2Type(const Nbaseutil::safestring& stype, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed);

	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(unsigned char dummy) {return Byte;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(char dummy) {return Char;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(unsigned short dummy) {return UnsignedShort;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(short dummy) {return Short;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(triple dummy) {return Triple;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(unsigned int dummy) {return UnsignedInt;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(int dummy) {return Int;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(Nbaseutil::uint64 dummy) {return UnsignedInt64;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(Nbaseutil::int64 dummy) {return Int64;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(float dummy) {return Float;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(double dummy) {return Double;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(const PMemBlock& dummy) {return PMemory;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(const void* dummy) {return VoidStar;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(const Nbaseutil::safestring& dummy) {return String;}
	/// Useful in templates. Converts C++ type into Vector::DataType.
	static DataType CType2ElemType(const DataItem& dummy) {return PolyType;}

public: // interface

	/// Return element size in bytes.
	unsigned int ElemSize() const {return typedchunk_.ElemSize();}

	/// Return element type.
	DataType ElemType() const {return DataType(typedchunk_.ElemType());}

	/// Return the number of elements in the Vector.
	unsigned int Length() const {return static_cast<unsigned int>(typedchunk_.Length());}

	/// Return the size of data array in bytes.
	size_t Size() const {return Length()*ElemSize();}

	/**
	* @brief Return the size of the managed memory block, in bytes. 
	* This may be larger then Length()*Size(), in this case there is unused room at the end of the buffer.
	* The offset value is not included in the returned size.
	*/
//	size_t Size() const {return memchunk_.GetSize();}

	/** 
	* @brief Change the array element type and related members. 
	* Data chunk is not converted. In case of object types the buffer is cleared.
	*
	* @param t   The new element type.
	* @param ResizeBuffer  The buffer is resized appropriately and the number of elements in the vector remains the same as before.
	*                     If ResizeBuffer==false, buffer remains the same and number of elements changes (ResizeVector() is called).
	* @param yCopyContent Copy/retain the array content in any case. No type conversions are done by this copying,
	*                    therefore the content preserving seems usually to have no meaning and is switched off by default.
	*/
	virtual void SetElemType(DataType t, bool ResizeBuffer=false, bool yCopyContent=false); 

	/// Return const reference to the encapsulated TypedChunk member.
	const TypedChunk& GetTypedChunk() const {return typedchunk_;}

	/// Replace the managed data chunk with a new one. The previous data chunk is released. If the new data array length is different than the previous length, then ResizeVector() will be called from that function.
	void SetTypedChunk(const TypedChunk& typedchunk);

	/** @brief Set the logical start of the array to begin at current logical index k. Adjust the vector length accordingly.
	* After this op. logical index 0 will map to previous logical index k.
	* The range of legal values is -FirstIndex() .. Length()
	* SetFirstIndex(0) is zero op.
	*/
	void SetFirstIndex(int k);

	/** @brief Return a slice of itself corresponding to elements k to l (excl.) 
	*
	* If l==0, return a slice containing of one element, k.
	*/
	PVector CloneSlice(unsigned int k, unsigned int l=0) const; 

	/// Calculate and return the distribution of vector data. The returned Table has 2 columns: x and data.
	/**
	* This calls the Acapella distribution() module, i.e. IMacro DLL must be loaded in the current process space. 
	* An Exception is thrown in case of errors, as always.
	* @param steps Number of bins for the distribution.
	* @param averaging If packing raw distribution into a smaller array, average the values instead of just summing. Note that summing is usually statistically more correct. If averaging is used, the factor of output array will be set to the mean averaging factor.
	*/
	PTable GetHistogram(int steps=64, bool averaging=false) const;

	/// Erase up to n elements starting at pos. This reorganizes the buffer and finally calls ResizeVector() to set the new length.
	void Erase(int pos, int n);	

	/**
	* Take the data  array of brother into shared use. The data sharing is ended by the first non-const Pointer() method called from either object. See also: TransferOwnerShip().
	* This method is used in some kind of type conversions. Use with care, this is a low-level function. The new owners' other
	* attributes may need to be adjusted separately beforhand for the new size and content of the memory block.
	* The raw memory is passed over (type information not retained), so this method can be used only for non-object element types.
	*
	* @param brother A vector whose data array is taken into shared use.
	* @return True, if the length of the new data array in elements is the same as of the old array.
	*/
	bool UseMemoryOf(const PVector& brother);

	/**
	* Transfers the managed memory block to another MemBlock.  
	* The managed memory block (MemChunk) is detached from this object and assigned to the NewOwner. See also UseMemoryOf().
	* This object data array will be of zero size afterwards.
	*
	* This method is used in some kind of type conversions. Use with care, this is a low-level function. The new owners' other
	* attributes may need to be adjusted separately for the new size and content of the memory block. 
	*
	* @param NewOwner The memblock who will become the new owner of the managed memory block. Its previous memblock is deallocated. 
	*/
	void TransferOwnerShip(PVector NewOwner);

	/**
	* @brief Access the managed memory block read-only. 
	*
	* The function returns a const pointer to the (start of the data in) managed memory block. Typically one uses
	* Vector class ...Pointer(), like Vector::IntPointer() methods instead, which are more typesafe.
	*
	* It is not allowed to cast the constness away and modify the managed memory block,as it may be in shared use with another MemBlock. Call the 
	* non-const Pointer() method instead.
	*/
	const void*	Pointer() const {return typedchunk_.GetPointer();}	

	/**
	* @brief Access the managed memory block. 
	*
	* The function returns a pointer to the (start of the data in) managed memory block. Typically one uses
	* Vector class ...Pointer(), like Vector::IntPointer() methods instead, which are more typesafe.
	*
	* If the managed memory block is in shared use, then a physical copy of it is made and the pointer to the new block is returned. Thus the 
	* address returned by the non-const Pointer() may differ from the one returned earlier by the const Pointer() method.
	*
	* If the MemBlock object is announced const via the SetConst() method, this method throws, at least in DEBUG builds. The MemBlock objects are automatically
	* announced const after inserting them in the Acapella datablock after module run. Call PMemBlock::MakeClone() method in such situation.
	*/
	void* Pointer() {return typedchunk_.GetPointer();}

	/// A template raw array access function. Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer.
	template<typename T>
	T* TypedPointer() {return Pointer_tmpl<T>();}

	/// A template raw array access function. Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer.
	template<typename T>
	const T* TypedPointer() const {return Pointer_tmpl<T>();}

	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	unsigned char* BytePointer(int base=0) {return Pointer_tmpl<unsigned char>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const unsigned char* BytePointer(int base=0) const {return Pointer_tmpl<unsigned char>(base);}
	/// Same as BytePointer()
	unsigned char *UnsignedCharPointer(int base=0)  {return BytePointer();} 
	/// Same as BytePointer()
	const unsigned char *UnsignedCharPointer(int base=0) const {return BytePointer();} 
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	char* CharPointer(int base=0) {return Pointer_tmpl<char>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const char* CharPointer(int base=0) const {return Pointer_tmpl<char>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	short* ShortPointer(int base=0) {return Pointer_tmpl<short>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const short* ShortPointer(int base=0) const {return Pointer_tmpl<short>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	unsigned short* UnsignedShortPointer(int base=0) {return Pointer_tmpl<unsigned short>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const unsigned short* UnsignedShortPointer(int base=0) const {return Pointer_tmpl<unsigned short>(base);}
	/// Checks that the vector data type is Triple, and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	triple* TriplePointer(int base=0) {return Pointer_tmpl<triple>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const triple* TriplePointer(int base=0) const {return Pointer_tmpl<triple>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	Nbaseutil::int64* Int64Pointer(int base=0) {return Pointer_tmpl<Nbaseutil::int64>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const Nbaseutil::int64* Int64Pointer(int base=0) const {return Pointer_tmpl<Nbaseutil::int64>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	int* IntPointer(int base=0) {return Pointer_tmpl<int>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const int* IntPointer(int base=0) const {return Pointer_tmpl<int>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	Nbaseutil::uint64* UnsignedInt64Pointer(int base=0) {return Pointer_tmpl<Nbaseutil::uint64>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const Nbaseutil::uint64* UnsignedInt64Pointer(int base=0) const {return Pointer_tmpl<Nbaseutil::uint64>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	unsigned int* UnsignedIntPointer(int base=0) {return Pointer_tmpl<unsigned int>(base);}
	/// Checks that the vector data can be safely (only signedness change) cast to the requested type and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const unsigned int* UnsignedIntPointer(int base=0) const {return Pointer_tmpl<unsigned int>(base);}
	/// Checks that the vector data type is Float, and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	float* FloatPointer(int base=0) {return Pointer_tmpl<float>(base);}
	/// Checks that the vector data type is Float, and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const float* FloatPointer(int base=0) const {return Pointer_tmpl<float>(base);}
	/// Checks that the vector data type is Double, and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	double* DoublePointer(int base=0) {return Pointer_tmpl<double>(base);}
	/// Checks that the vector data type is Double, and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const double* DoublePointer(int base=0) const {return Pointer_tmpl<double>(base);}
	/// Checks that the vector data type is void*, and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	void** VoidStarPointer(int base=0) {return Pointer_tmpl<void*>(base);} 
	/// Checks that the vector data type is void*, and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	void* const* VoidStarPointer(int base=0) const {return Pointer_tmpl<void*>(base);} 
	/// Checks that the vector data type is PMemory, and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	PMemBlock* PMemoryPointer(int base=0) {return Pointer_tmpl<PMemBlock>(base);}
	/// Checks that the vector data type is PMemory, and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const PMemBlock* PMemoryPointer(int base=0) const {return Pointer_tmpl<PMemBlock>(base);}
	/// Checks that the vector data type is String, and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	Nbaseutil::safestring* StringPointer(int base=0) {return Pointer_tmpl<Nbaseutil::safestring>(base);}
	/// Checks that the vector data type is String, and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const Nbaseutil::safestring* StringPointer(int base=0) const {return Pointer_tmpl<Nbaseutil::safestring>(base);}
	/// Checks that the vector data type is PolyType, and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	DataItem* PolyTypePointer(int base=0) {return Pointer_tmpl<DataItem>(base);}
	/// Checks that the vector data type is PolyType, and returns pointer to the data buffer. If base>0, then the starting element lies at index base in the returned array. The client code may not access elements less than base.
	const DataItem* PolyTypePointer(int base=0) const {return Pointer_tmpl<DataItem>(base);}

	/// Helper function: returns true if data can be safely cast to iIntoFormat without any physical conversion. Only signed-unsigned casting is allowed.
	bool CanBeCast(const DataType iIntoFormat) const;


	/// Return element converted to int, or throw. This is a bit slow, bounds are checked. Raw factor is not included in the return value.
	virtual int operator[](unsigned int iIndex) const;		

	/// Same as operator[], formed as an ordinary function for convenience. Raw factor is not included in the return value.
	int ElemInt(unsigned int iIndex) const {return (*this)[iIndex];}

	/// Use instead of ElemInt()/operator[] if the datatype is unsigned int. Raw factor is not included in the return value.
	unsigned int ElemUInt(unsigned int iIndex) const;

	/// Return element converted to double, or throw. This is a bit slow, bounds are checked. Raw factor is not included in the return value.
	virtual double Elem(unsigned int iIndex) const;

	/// Return element converted to memblock, or throw. This is a bit slow, bounds are checked. Raw factor is not included in the return value.
	PMemBlock ElemMemory(unsigned int iIndex) const;	

	/// Return element without bounds and conversion checks. Raw factor is not included in the return value.
	inline int GetInt(int index) const;

	/// Return element without bounds and conversion checks. Use in case of UnsignedInt/UnsignedInt datatype. Raw factor is not included in the return value.
	inline unsigned int GetUInt(int index) const;

	/// Return element without bounds check. Raw factor is not included in the return value.
	inline double GetDouble(int index) const;
	
	/// Return element as a string without bounds check, or an empty string if data cannot be converted to a string. Raw factor is not included in the return value.
	Nbaseutil::safestring GetString(int index) const;
	
	/// Return element as a DataItem without bounds check.  Raw factor is not included in the return value.
	DataItem GetDataItem(int index) const;

	/// Return element as a DataItem without bounds check, with raw factor applied.
	DataItem GetDataItemWithFactor(int index) const;
	
	/// Return element without bounds check, if ElemType()==PMemory.
	PMemBlock GetMemory(int index) const;

public: // virtual interface
	
	/// Return the rank (number of dimensions) of this object. This depends on the dynamic type of the object.
	virtual int GetRank() const {return 1;}
	
	/// Return dimension size, if argument in correct range (0..Rank()-1). Otherwise returns 0.
	virtual unsigned int GetDimSize(int dimension) const {return (dimension==0)? Length(): 0;}

	/// Return a vector of type UnsignedInt containing dimension sizes. See also GeomBlock::GetDimSizeArray().
	virtual PVector GetDimSizes() const;

	/**
	* Change dimension sizes. If the product of the dimension sizes differs from the previous vector length, then ResizeVector() is called by that function.
	* @param dimsizes Array specifying the new dimension sizes.
	* @param ndimsizes Length of the dimsizes array. If it does not equal to the rank of the object, an exception is thrown.
	*/
	virtual void ChangeDimSizes(const unsigned int* dimsizes, int ndimsizes) {
		if (1!=ndimsizes) {
			throw Nbaseutil::Exception(Nbaseutil::ERR_PROGRAM_ERROR, Nbaseutil::Printf("Vector::ChangeDimSizes() is called with an invalid number of dimensions %d for: %s")(ndimsizes)(GetDescription()));
		}
		if (Length()!=dimsizes[0]) {
			ResizeVector(dimsizes[0]);
		}
	}

	/**
	* Change the size of the vector. The base class method will resize the contained TypedChunk object and call the virtual ChangeDimSizes() function
	* for updating the dimension sizes in the derived object parts. Only the last dimension size is changed, if this appears to be not possible,
	* an exception is thrown. Override only in case if virtual ChangeDimSizes() is not able to cope with the resizing.
	* @param n New number of elements. Preconditions: n % (Length()/GetDimSize(GetRank()-1)) == 0.
	* @param yCopyContent Copy the array buffer over, if a new buffer gets allocated.
	* @param yInitToZero Initialize added elements to zero when growing the vector. Has no effect on PMemory type vector.
	*/
	virtual void ResizeVector(size_t n, bool yCopyContent=true, bool yInitToZero=true);

	/// Nullify (clear) the data chunk.
	void Clear();

	/// Fill the vector with the specified value. The value must be convertible to the element type, otherwise an exception occurs.
	void Fill(const DataItem& value);

	/**
	* Delete elements marked 0 in code array. Otherwise, move element i (1-based) to the position code[i] (1-based).
	* code itself is also 1-based and has to contain codelen+1 elements. First element (code[0]) is unused and unaccessed.
	* codelen must be less or equal to Length(). The newlen parameter gives the new length 
	* of the vector. code[i] may not exceed newlen (not checked in Release build).
	*/
	virtual void Rearrange(const int code[], unsigned int codelen, unsigned int newlen, Nbaseutil::ThrowHandlerFunc th=ThrowIndeed);

	/**
	* @brief The safest and most flexible way to convert vector into another type. See also: SetElemType().
	* Note that this method does not always clone the object. If a clone is 
	* needed, call PMemBlock::MakeClone() method on the returned smartpointer.
	*
	* @return Converted vector (or this, if vector already in correct type). 
	* @param t   New element type.
	* @param copycontent Copy the array content over, if new buffer gets allocated.
	* @param preservefactor Keep the raw factor unchanged, or throw. If false, raw factor gets modified when squeezing larger datatype into a smaller one.
	*/
	PVector ConvertElemsToType(Vector::DataType t, bool copycontent=true, bool preservefactor=false) const;

	/**
	* Convert the vector to a type which can accommodate \a x without data or significant precision loss.
	* Note that this method only makes a new object if necessary, use PMemBlock::MakeClone() 
	* method on the returned smartpointer if cloning is needed.
	* @param x The element which potentially will be inserted in the vector.
	* @param preservefactor Try to keep raw factor unchanged, or throw. If false, raw factor gets modified when squeezing larger datatype into a smaller one.
	*/
	PVector ConvertElemsToFittingType(const DataItem& x, bool preservefactor=false) const;


	/// Without changing data representation, reorganize it so that the factor is f, or throw.
	void EnsureFactor(double f); 

	/// Set an element, converted from an int.
	void SetElem(unsigned int iIndex, int iValue);	  
	/// Set an element, converted from an unsigned int.
	void SetElem(unsigned int iIndex, unsigned int iValue);	  
	/// Set an element, converted from an int64.
	void SetElem(unsigned int iIndex, Nbaseutil::int64 iValue);	  
	/// Set an element, converted from an uint64.
	void SetElem(unsigned int iIndex, Nbaseutil::uint64 iValue);	  
	/// Set an element, converted from a double.
	void SetElem(unsigned int iIndex, double dValue); 
	/// Set an element, converted from a memblock, or throw.
	void SetElem(unsigned int iIndex, PMemBlock poValue);	
	/// Set an element, converted from a string, or throw.
	void SetElem(unsigned int iIndex, const Nbaseutil::safestring& s);	
	/// Set an element, converted from a string, or throw.
	void SetElem(unsigned int iIndex, const char* s);	
	/// Set an element, converted from a dataitem, or throw.
	void SetElem(unsigned int iIndex, const DataItem& x);	

	// Addition 15.08.2003: why there is no push_back?
	/// Append an element to the end of vector, in amortized constant time. T must be a type (convertible to) a type of SetElem() functions' second parameter.
	template<typename T>
	void push_back(T value) {
		unsigned int n = Length();
		ResizeVector(n+1);
		SetElem(n, value);
	}

	/// Remove the element in the end of the vector. 
	void pop_back() {
		if (Length()>0) ResizeVector(Length()-1);
	}

	/// Insert an element into the vector at position pos (pos<=Length()).
	void Insert(int pos, const DataItem& value);

	/// Return raw factor of the data. In script level the data appears as multiplied by the raw factor, by default.
	double Factor() const {return factor_;} 

	/// Set the raw factor. Precondition: f may not be <=0 or nan/inf.
	void SetFactor(double f);

	// See the top of this file for dicussion about integer-float conversions:
	/// For integer vectors: convert an actual real value to an integer which can be stored in the vector buffer.
	unsigned int RealToPixel(double x) const {return iround(x/factor_);}

	/// For integer vectors: convert an actual real value to an integer which can be stored in the vector buffer.
	unsigned int RealToPixel(double x, rounding_t roundingmode) const;

	/// Convert physical data buffer value to a real value by multiplying by the raw factor.
	double PixelToReal(unsigned int x) const { return x*factor_;}

	/// Convert physical data buffer value to a real value by multiplying by the raw factor.
	double PixelToReal(int x) const { return double(x)*factor_;}

	/// Convert physical data buffer value to a real value by multiplying by the raw factor.
	double PixelToReal(double x) const { return x*factor_;}	

	/**
	* Shift the vector by k elements. 
	* If k>0, appends new zero elements in the beginning of vector.
	* If k<0, deletes first elements and shrinks the vector.
	*/
	virtual void ShiftVector(int k);		

	/// Append another vector at the end of this vector.
	virtual void Append(PVector b);	
	
	/// Return the size of element type x in bytes.
	static unsigned int SizeOf(Vector::DataType x);	

	/// Check if type x is integral and unsigned.
	static bool IsUnsignedIntegral(Vector::DataType x);	

	/// Check if type x is integral.
	static bool IsIntegral(Vector::DataType x);	

	/// Check if type x is double or float.
	static bool IsFloatingPoint(Vector::DataType x);

	/// Check if type x is of a numeric type (supports arithmetic operations).
	static bool IsNumeric(Vector::DataType x) {return IsIntegral(x) || IsFloatingPoint(x);}

	/// Return true if the vector elements are C++ objects having nontrivial ctors/dtors.
	static bool IsObject(Vector::DataType x) {return TypedChunk::IsObject(TypedChunk::DataType(x));}

	/// Useful in serialization. Converts datatype to string.
	static const char* TypeToString(DataType t);

	/// For integer datatypes: returns the maximum representable number in the datatype.
	static Nbaseutil::uint64 limits_max(DataType t);

	/// For integer datatypes: returns the minimum representable number in the datatype.
	static Nbaseutil::int64 limits_min(DataType t);

protected: // implementation

	/// Construct object and allocate a memory chunk.
	Vector(size_t length, DataType elem_type, double factor):
		factor_(factor),
		typedchunk_(length, TypedChunk::DataType(elem_type))
	{}

	Vector(size_t length, DataType elem_type, double factor, void *p, MemChunk::ReleaseFunc1 p_free, size_t offset=0):
		factor_(factor),
		typedchunk_(length, TypedChunk::DataType(elem_type), p, p_free, offset)
	{}

	Vector(size_t length, DataType elem_type, double factor, void *p, MemChunk::ReleaseFunc2 p_free, void* callback_data, size_t offset=0):
		factor_(factor),
		typedchunk_(length, TypedChunk::DataType(elem_type), p, p_free, callback_data, offset)
	{}

	Vector(const TypedChunk& chunk, double factor):
		factor_(factor),
		typedchunk_(chunk)
	{}

	/// Construct object, keep memory chunk in shared use.
	Vector(const Vector& b);

	/// Dtor performs all cleanup
	~Vector();

	PVector ClearBorders() const;


protected: // virtual interface.

	/// Return a slice of itself corresponding to elements k to l (excl.) 
	virtual PVector DoCloneSlice(unsigned int k, unsigned int l) const; 

	//DataType Rep2ElemType_internal(bool yUnsigned, unsigned int uArgs) const; // used only internally from Rep2ElemType().

public: // virtual overrides
	virtual const char*		Class() const {return "vector";}
	virtual PMemBlock		DoClone(copymode_t copy_mode) const;
	virtual bool			Conforms(const char *szClassName) const;
	virtual bool			Consistent(Nbaseutil::safestring& msg, bool CheckContent=true) const;	
	virtual bool			AddConvertArg(const Nbaseutil::safestring& option, const DataItem& value, ConvertOptions& opt, Nbaseutil::ThrowHandlerFunc throwhandler=LogWarnings()) const;
	virtual PMemBlock		ConvertFrom(const PMemBlock source, const ConvertOptions& opt) const;
//	virtual PMemBlock		ConvertTo(const Nbaseutil::safestring& classname) const;
	virtual SafeValue		DoVerb(const char* pszCommand, int iArg, SafeValue vArg[]);
	virtual bool			Entertain(MemBlockVisitor& visitor, const Nbaseutil::safestring& name, entertainmode_t mode=enter_deep);	
	virtual void			IterateChildren(NIMacro::AcaVisitor& visitor, const NIMacro::TraverseNode& node);
	virtual bool			Equals(const MemBlock& b) const;
	virtual	MemBlockImpl*	DoPimpl() const;
	virtual SafeValue		ResolveSubItem(const Nbaseutil::safestring& subitemname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed) const;
	virtual const char*		ClassDescription() const;
	virtual PMemBlock		op(char opcode, const PMemBlock y, const op_options& options) const;
	virtual bool			DoSetSubItem(const Nbaseutil::safestring& subitemname, const SafeValue& item, const Nbaseutil::safestring& fullpathname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed);

protected: // virtual overrides
	virtual Nbaseutil::safestring DoGetDescription() const;
	virtual void DoSerialize(int ser_format, SerializeVisitor& visitor, const Nbaseutil::safestring& itemname, const Formatter& formatter, Nbaseutil::safestring& buffer) const;
	//virtual PMemBlock DoUnSerialize(int ser_format, const Formatter& formatter, const char* buffer, const char** p_buff_end) const;

private: // implementation

	template<typename T>
	T* Pointer_tmpl(int base=0) {
		if (CanBeCast(CType2ElemType(T()))) {
			return static_cast<T*>(Pointer())-base;
		} else {
			throw Nbaseutil::Exception(Nbaseutil::ERR_PROGRAM_ERROR, Nbaseutil::Printf("Cannot cast data array to type %s: %s")(TypeToString(CType2ElemType(T())))(GetDescription()));
		}
	}

	template<typename T>
	const T* Pointer_tmpl(int base=0) const {
		if (CanBeCast(CType2ElemType(T()))) {
			return static_cast<const T*>(Pointer())-base;
		} else {
			throw Nbaseutil::Exception(Nbaseutil::ERR_PROGRAM_ERROR, Nbaseutil::Printf("Cannot cast data array to type %s: %s")(TypeToString(CType2ElemType(T())))(GetDescription()));
		}
	}


	void SerializeAcapella(SerializeVisitor& visitor, const Nbaseutil::safestring& itemname, const Formatter& formatter, Nbaseutil::safestring& buffer) const;

private: // data
	/// Raw factor of the data.
	double factor_;			
	/// Memory chunk
	TypedChunk typedchunk_;

};


#ifndef ACAPELLA_SUPPRESS_DISPATCH_MACROS
/// Dispatch to template function FUNCTION according to ELEMTYPE value. Dispatches for all types.
/**
Usage example:

@code
template<typename T>
int uniq_tmpl(const T* src, T* dst0, int n);

PVector uniq(PVector item) 
	int n = item->Length(); // 
	result = Vector::Create(n, item->ElemType());
	result->SetFactor(item->Factor());
	int m;
	if (!sorted) {
		DISPATCH_ALL(item->ElemType(), m = uniq_tmpl, (item->TypedPointer<dispatch_t>(), result->TypedPointer<dispatch_t>(), n));
	} else {
		DISPATCH_ALL(item->ElemType(), m = uniq_sorted_tmpl, (item->TypedPointer<dispatch_t>(), result->TypedPointer<dispatch_t>(), n));
	}
	result->ResizeVector(m);
}
@endcode
*/
#define DISPATCH_ALL(ELEMTYPE,FUNCTION,ARGS) \
switch(ELEMTYPE) {\
case Vector::Byte: {typedef unsigned char dispatch_t; FUNCTION <unsigned char>ARGS; break;}\
case Vector::UnsignedShort: {typedef unsigned short dispatch_t; FUNCTION <unsigned short>ARGS; break;}\
case Vector::UnsignedInt: {typedef unsigned int dispatch_t; FUNCTION <unsigned int>ARGS; break;}\
case Vector::Char: {typedef char dispatch_t; FUNCTION<char>ARGS; break;}\
case Vector::Triple: {typedef triple dispatch_t; FUNCTION<triple>ARGS; break;}\
case Vector::Short: {typedef short dispatch_t; FUNCTION<short>ARGS; break;}\
case Vector::Int: {typedef int dispatch_t; FUNCTION<int>ARGS; break;}\
case Vector::PMemory: {typedef PMemBlock dispatch_t; FUNCTION<PMemBlock>ARGS; break;}\
case Vector::Float: {typedef float dispatch_t; FUNCTION<float>ARGS; break;}\
case Vector::Double: {typedef double dispatch_t; FUNCTION<double>ARGS; break;}\
case Vector::String: {typedef Nbaseutil::safestring dispatch_t; FUNCTION<Nbaseutil::safestring>ARGS; break;}\
case Vector::PolyType: {typedef DataItem dispatch_t; FUNCTION<DataItem>ARGS; break;}\
case Vector::Int64: {typedef Nbaseutil::int64 dispatch_t; FUNCTION<Nbaseutil::int64>ARGS; break;}\
case Vector::UnsignedInt64: {typedef Nbaseutil::uint64 dispatch_t; FUNCTION<Nbaseutil::uint64>ARGS; break;}\
default: throw Nbaseutil::Exception(Nbaseutil::ERR_NOTIMPLEMENTED, Nbaseutil::Printf("Function " #FUNCTION " not implemented for data type '%s'")(Vector::TypeToString(ELEMTYPE)));\
};

/// Dispatch to template function FUNCTION according to ELEMTYPE value. Dispatches for integer and floating-point types, otherwise throws. 64-bit types are not included.
#define DISPATCH_NUMERIC(ELEMTYPE,FUNCTION,ARGS) \
switch(ELEMTYPE) {\
case Vector::Byte: {typedef unsigned char dispatch_t; FUNCTION <unsigned char>ARGS; break;}\
case Vector::UnsignedShort: {typedef unsigned short dispatch_t; FUNCTION <unsigned short>ARGS; break;}\
case Vector::UnsignedInt: {typedef unsigned int dispatch_t; FUNCTION <unsigned int>ARGS; break;}\
case Vector::Char: {typedef char dispatch_t; FUNCTION<char>ARGS; break;}\
case Vector::Short: {typedef short dispatch_t; FUNCTION<short>ARGS; break;}\
case Vector::Int: {typedef int dispatch_t; FUNCTION<int>ARGS; break;}\
case Vector::Float: {typedef float dispatch_t; FUNCTION<float>ARGS; break;}\
case Vector::Double: {typedef double dispatch_t; FUNCTION<double>ARGS; break;}\
case Vector::Triple: {typedef triple dispatch_t; FUNCTION<triple>ARGS; break;}\
case Vector::Int64: {typedef Nbaseutil::int64 dispatch_t; FUNCTION<Nbaseutil::int64>ARGS; break;}\
case Vector::UnsignedInt64: {typedef Nbaseutil::uint64 dispatch_t; FUNCTION<Nbaseutil::uint64>ARGS; break;}\
default: throw Nbaseutil::Exception(Nbaseutil::ERR_NOTIMPLEMENTED, Nbaseutil::Printf("Function " #FUNCTION " not implemented for data type '%s'")(Vector::TypeToString(ELEMTYPE)));\
};

/// Dispatch to template function FUNCTION according to ELEMTYPE value. Dispatches for integer types, otherwise throws. 64-bit types are not included.
#define DISPATCH_INTEGRAL(ELEMTYPE,FUNCTION,ARGS) \
switch(ELEMTYPE) {\
case Vector::Byte: {typedef unsigned char dispatch_t; FUNCTION <unsigned char>ARGS; break;}\
case Vector::UnsignedShort: {typedef unsigned short dispatch_t; FUNCTION <unsigned short>ARGS; break;}\
case Vector::UnsignedInt: {typedef unsigned int dispatch_t; FUNCTION <unsigned int>ARGS; break;}\
case Vector::Char: {typedef char dispatch_t; FUNCTION<char>ARGS; break;}\
case Vector::Short: {typedef short dispatch_t; FUNCTION<short>ARGS; break;}\
case Vector::Int: {typedef int dispatch_t; FUNCTION<int>ARGS; break;}\
case Vector::Triple: {typedef triple dispatch_t; FUNCTION<triple>ARGS; break;}\
case Vector::Int64: {typedef Nbaseutil::int64 dispatch_t; FUNCTION<Nbaseutil::int64>ARGS; break;}\
case Vector::UnsignedInt64: {typedef Nbaseutil::uint64 dispatch_t; FUNCTION<Nbaseutil::uint64>ARGS; break;}\
default: throw Nbaseutil::Exception(Nbaseutil::ERR_NOTIMPLEMENTED, Nbaseutil::Printf("Function " #FUNCTION " not implemented for data type '%s'")(Vector::TypeToString(ELEMTYPE)));\
};

/// Dispatch to template function FUNCTION according to ELEMTYPE value. Dispatches for floating-point types, otherwise does nothing.
#define DISPATCH_FLOATING_SILENT(ELEMTYPE,FUNCTION,ARGS) \
switch(ELEMTYPE) {\
case Vector::Float: {typedef float dispatch_t; FUNCTION<float>ARGS; break;}\
case Vector::Double: {typedef double dispatch_t; FUNCTION<double>ARGS; break;}\
default: break;\
};

/// Dispatch to template function FUNCTION according to ELEMTYPE value. Dispatches for C++ object types (String, PMemory, PolyType), otherwise throws.
#define DISPATCH_OBJECT(ELEMTYPE,FUNCTION,ARGS) \
switch(ELEMTYPE) {\
case Vector::PMemory: {typedef PMemBlock dispatch_t; FUNCTION<PMemBlock>ARGS; break;}\
case Vector::String: {typedef Nbaseutil::safestring dispatch_t; FUNCTION<Nbaseutil::safestring>ARGS; break;}\
case Vector::PolyType: {typedef DataItem dispatch_t; FUNCTION<DataItem>ARGS; break;}\
default: throw Nbaseutil::Exception(Nbaseutil::ERR_NOTIMPLEMENTED, Nbaseutil::Printf("Function " #FUNCTION " not implemented for data type '%s'")(Vector::TypeToString(ELEMTYPE)));\
};

/// Dispatch to template function FUNCTION according to ELEMTYPE value. Dispatches for numeric and String types, otherwise throws.
#define DISPATCH_NUMERIC_AND_STRING(ELEMTYPE,FUNCTION,ARGS) \
switch(ELEMTYPE) {\
case Vector::Byte: {typedef unsigned char dispatch_t; FUNCTION <unsigned char>ARGS; break;}\
case Vector::UnsignedShort: {typedef unsigned short dispatch_t; FUNCTION <unsigned short>ARGS; break;}\
case Vector::UnsignedInt: {typedef unsigned int dispatch_t; FUNCTION <unsigned int>ARGS; break;}\
case Vector::Char: {typedef char dispatch_t; FUNCTION<char>ARGS; break;}\
case Vector::Short: {typedef short dispatch_t; FUNCTION<short>ARGS; break;}\
case Vector::Int: {typedef int dispatch_t; FUNCTION<int>ARGS; break;}\
case Vector::Float: {typedef float dispatch_t; FUNCTION<float>ARGS; break;}\
case Vector::Double: {typedef double dispatch_t; FUNCTION<double>ARGS; break;}\
case Vector::Triple: {typedef triple dispatch_t; FUNCTION<triple>ARGS; break;}\
case Vector::Int64: {typedef Nbaseutil::int64 dispatch_t; FUNCTION<Nbaseutil::int64>ARGS; break;}\
case Vector::UnsignedInt64: {typedef Nbaseutil::uint64 dispatch_t; FUNCTION<Nbaseutil::uint64>ARGS; break;}\
case Vector::String: {typedef Nbaseutil::safestring dispatch_t; FUNCTION<Nbaseutil::safestring>ARGS; break;}\
default: throw Nbaseutil::Exception(Nbaseutil::ERR_NOTIMPLEMENTED, Nbaseutil::Printf("Function " #FUNCTION " not implemented for data type '%s'")(Vector::TypeToString(ELEMTYPE)));\
};

#endif // ACAPELLA_SUPPRESS_DISPATCH_MACROS

/**
* Reverse conversion to Vector::CType2ElemType() functions.
* Usage example: some_tmpl_class< TypeToCType<Vector::Byte>::type> x;
*/
template<enum Vector::DataType>
struct TypeToCType {
};

// Do not put in Doxygen documentation, WISE does not cope with such long filenames
/// @cond ALLOW_LONG_DOXYGEN_FILENAMES
template<> struct TypeToCType<Vector::Char> {typedef char type;};
template<> struct TypeToCType<Vector::Short> {typedef short type;};
template<> struct TypeToCType<Vector::Int> {typedef int type;};
template<> struct TypeToCType<Vector::Byte> {typedef unsigned char type;};
template<> struct TypeToCType<Vector::UnsignedShort> {typedef unsigned short type;};
template<> struct TypeToCType<Vector::Triple> {typedef triple type;};
template<> struct TypeToCType<Vector::UnsignedInt> {typedef unsigned int type;};
template<> struct TypeToCType<Vector::Float> {typedef float type;};
template<> struct TypeToCType<Vector::Double> {typedef double type;};
template<> struct TypeToCType<Vector::VoidStar> {typedef void* type;};
template<> struct TypeToCType<Vector::PMemory> {typedef PMemBlock type;};
template<> struct TypeToCType<Vector::String> {typedef Nbaseutil::safestring type;};
template<> struct TypeToCType<Vector::PolyType> {typedef DataItem type;};
template<> struct TypeToCType<Vector::UnsignedInt64> {typedef Nbaseutil::uint64 type;};
template<> struct TypeToCType<Vector::Int64> {typedef Nbaseutil::int64 type;};
/// @endcond


/**
* A helper function template for rounding a number to the destination type.
* @tparam T Template parameter, specify a numeric type here.
* @param x The double number to round.
* @return If T is an integer type, then rounded x value is returned. No overflow/signedness check is made. If T is a floating-point type, x is returned unmodified.
*/
template<typename T> inline T round_to_type(double x) {return x>=0.0? T(x+0.5): T(x-0.5);}

/// @cond implementation_details
template<> inline double round_to_type<double>(double x) {return x;}
template<> inline float round_to_type<float>(double x) {return float(x);}
/// @endcond

/**
* A helper function template for rounding a number to the destination type, performing additional checks.
* @tparam T Template parameter, specify a numeric type here.
* @param x The double number to round.
* @return If T is an integer type, then rounded x value is returned. 
*     If the rounded result is not representable in the target type, an exception is thrown. 
*     If T is a floating-point type, x is returned unmodified.
*/
template<typename T> inline T round_to_type_checked(double x) {
	if (x>std::numeric_limits<T>::max() || x<std::numeric_limits<T>::min()) {
		throw Nbaseutil::Exception(Nbaseutil::ERR_BADDATA, Nbaseutil::Printf("Value %g does not fit into destination type range %d..%u")(x)(std::numeric_limits<T>::min())(std::numeric_limits<T>::max()));
	}
	return x>=0.0? T(x+0.5): T(x-0.5);
}

/// @cond implementation_details
template<> inline double round_to_type_checked<double>(double x) {return x;}
template<> inline float round_to_type_checked<float>(double x) {return float(x);}
/// @endcond


// IMPLEMENTATION DETAILS

//inline unsigned int Vector::SizeOf(Vector::DataType x) {
//	return TypedChunk::SizeOf(TypedChunk::DataType(x));
//}

inline const char* Vector::TypeToString(DataType t) {
	switch(t) {
	case Vector::Int: return "signed int";
	case Vector::UnsignedInt: return "unsigned int";
	case Vector::Int64: return "signed int64";
	case Vector::UnsignedInt64: return "unsigned int64";
	case Vector::Short: return "signed short";
	case Vector::UnsignedShort: return "unsigned short";
	case Vector::Char: return "signed char";
	case Vector::Byte: return "byte";
	case Vector::Float: return "float";
	case Vector::Double: return "double";
	case Vector::VoidStar: return "voidstar";
	case Vector::PMemory: return "memblock";
	case Vector::Triple: return "triple";
	case Vector::PolyType: return "polytype";
	case Vector::String: return "string";
	default: return "";
	}
}

inline bool Vector::IsUnsignedIntegral(Vector::DataType x) {
	switch(x) {
	case UnsignedInt: case UnsignedInt64: case UnsignedShort: case Byte: case Triple: return true;
	default: return false;
	}
}

inline bool Vector::IsIntegral(Vector::DataType x) {
	switch(x) {
	case UnsignedInt: case UnsignedShort: case Byte: case Triple: 
	case Int: case Short: case Char: case UnsignedInt64: case Int64:
		return true;
	default: return false;
	}
}

inline bool Vector::IsFloatingPoint(Vector::DataType x) {
	return x==Float || x==Double;
}

inline Nbaseutil::uint64 Vector::limits_max(DataType t) {
	DISPATCH_INTEGRAL(t, return std::numeric_limits, ::max());
	return 0;
}

inline Nbaseutil::int64 Vector::limits_min(DataType t) {
	DISPATCH_INTEGRAL(t, return std::numeric_limits, ::min());
	return 0;
}


template<typename T>
inline T vector_get_element_tmpl_internal_no_bounds_check(const void* p, int index, T* dummy=NULL) {
	return ((T*)p)[index];
}

/// Return element without bounds check
inline int Vector::GetInt(int index) const {
	DISPATCH_NUMERIC(ElemType(), return (int) vector_get_element_tmpl_internal_no_bounds_check, (Pointer(), index));
}

inline unsigned int Vector::GetUInt(int index) const {
	DISPATCH_NUMERIC(ElemType(), return (unsigned int) vector_get_element_tmpl_internal_no_bounds_check, (Pointer(), index));
}


/// Return element without bounds check
inline double Vector::GetDouble(int index) const {
	DISPATCH_NUMERIC(ElemType(), return (double) vector_get_element_tmpl_internal_no_bounds_check, (Pointer(), index));
}

/// Return element without bounds check, if ElemType()==PMemory
inline PMemBlock Vector::GetMemory(int index) const {
	return ElemType()==PMemory ? ((PMemBlock*)Pointer())[index]: NULL;
}

/// Construct an empty vector. The template parameter (element type) must be specified explicitly.
template<typename T> PVector Vec() {PVector v = Vector::CreateFrom(T()); v->pop_back(); return v;}

/// Construct a vector having 1 element.
template<typename T> PVector Vec(const T& x1) {return Vector::CreateFrom(x1);}

/// Construct a vector having 2 elements.
template<typename T> PVector Vec(const T& x1, const T& x2) {PVector v = Vector::CreateFrom(x1); v->push_back(x2); return v;}

/// Construct a vector having 3 elements.
template<typename T> PVector Vec(const T& x1, const T& x2, const T& x3) {PVector v = Vector::CreateFrom(x1); v->push_back(x2); v->push_back(x3); return v;}

/// Construct a vector having 4 elements.
template<typename T> PVector Vec(const T& x1, const T& x2, const T& x3, const T& x4) {PVector v = Vector::CreateFrom(x1); v->push_back(x2); v->push_back(x3); v->push_back(x4); return v;}

/// Construct a vector having 5 elements.
template<typename T> PVector Vec(const T& x1, const T& x2, const T& x3, const T& x4, const T& x5) {PVector v = Vector::CreateFrom(x1); v->push_back(x2); v->push_back(x3); v->push_back(x4); v->push_back(x5); return v;}


/// A structure for holding a node chain during MemBlock::Traverse().
struct TraverseNode {
	/// Name for the current node.
	const char* name_;
	/// Pointer to the parent node, or NULL.
	const TraverseNode* parent_;
	/// This node is an "attribute", not a "subobject".
	bool isattr_;
	/// Convenience constructor.
	TraverseNode(const char* name, const TraverseNode* parent=NULL, bool isattr=false): name_(name), parent_(parent), isattr_(isattr) {}
};

class TraverseVisitor: public MemBlockVisitor {
	typedef MemBlockVisitor super;
public: // virtual interface

	virtual void VisitStart(const TraverseNode& node, DataItem& item) {}
	virtual void VisitStart(const TraverseNode& node, MemBlock& item) {}
	virtual void VisitStart(const TraverseNode& node, ThreadSharable& item) {}

	virtual void VisitVData(const TraverseNode& node, Vector::DataType t, const void* vdata, int count, Vector* ownervector=NULL) {}

	virtual void VisitEnd(const TraverseNode& node, DataItem& item) {}
	virtual void VisitEnd(const TraverseNode& node, MemBlock& item) {}
	virtual void VisitEnd(const TraverseNode& node, ThreadSharable& item) {}

private: 
	/// This function is not used, do not call or override this!
	virtual order_t Visit(const Nbaseutil::safestring&, PMemBlock&) {return stop_scan;}
	/// This function is not used, do not call or override this!
	virtual order_t Visit(const Nbaseutil::safestring&, NIMacro::DataItem&) {return stop_scan;}
};

class AcaVisitor: public Nbaseutil::mb_malloced {
public:
	AcaVisitor(MemBlock::iteratemode_t mode): mode_(mode) {}
	virtual ~AcaVisitor() {}

public: // virtual interface
	virtual void Visit(const TraverseNode& node, DataItem& item) {}
	virtual void Visit(const TraverseNode& node, PMemBlock& item) {}
	virtual void Visit(const TraverseNode& node, PSharable& item) {}

public: // data
	MemBlock::iteratemode_t mode_;
};

/// For a type in ItemType enum, return a suitable type to store in a Vector.
inline Vector::DataType ItemType2DataType(ItemType t) {
	switch(t) {
	case Integer: return Vector::Int;
	case Integer64: return Vector::Int64;
	case Floating: return Vector::Double;
	case Pointer: return Vector::VoidStar;
	case Memory: return Vector::PMemory;
	case Asciiz: return Vector::String;
	//case SoftLink: return Vector::String;
	default: return Vector::PolyType;
	}
}

} // namespace NIMacro

#ifdef ACAPELLA_EVIL_MIN_WAS_DEFINED
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#undef ACAPELLA_EVIL_MIN_WAS_DEFINED
#endif
#ifdef ACAPELLA_EVIL_MAX_WAS_DEFINED
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#undef ACAPELLA_EVIL_MAX_WAS_DEFINED
#endif

#endif
