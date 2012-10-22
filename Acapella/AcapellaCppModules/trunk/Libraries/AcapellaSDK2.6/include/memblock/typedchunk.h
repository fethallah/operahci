#ifndef x_ACAPELLA_MEMBLOCK_TYPEDCHUNK_H_INCLUDED
#define x_ACAPELLA_MEMBLOCK_TYPEDCHUNK_H_INCLUDED

#include <limits>
#include "memchunk.h"
#include "datatype.h"
#include "triple.h"
#include "pmemblock.h"
#include "dataitem.h"
#include "refcounted.h"

namespace NIMacro {

	// Forward declarations
	class DI_MemBlock Vector;
	typedef DerivedPointer<Vector> PVector;

	class TypedChunk;

	class DelayedInitializer: public ThreadSafeRefcountable {
	public:
		virtual void DelayedInit(TypedChunk& chunk)=0;
		virtual ~DelayedInitializer() {}
		virtual DelayedInitializer* Clone() {return this;}
	};
	typedef RefcountedPointer<DelayedInitializer> PDelayedInitializer;

	/// A class encapsulating a MemChunk and maintaining type information of the MemChunk content.
	class DI_MemBlock TypedChunk: public Nbaseutil::mb_malloced, public DataTypeBase {
	public: // static interface

		/// Useful in templates. Converts C++ type into TypedChunk::DataType.
		template<typename T> static TypedChunk::DataType CType2ElemType(T x) {return TypedChunk::DataType(TypeOf(x));}

		/// Return the size of element type x in bytes.
		static unsigned int SizeOf(DataType x);	

		/// Return true if the elements of specified datatype are C++ objects having nontrivial ctors/dtors.
		static bool IsObject(DataType x) {return x==String||x==PMemory||x==PolyType;}

		static bool IsIntegral(DataType x) {return x==Int||x==UnsignedInt||x==Short||x==UnsignedShort||x==Char||x==Byte||x==Triple||x==Int64||x==UnsignedInt64;}


		/**
		* Initialize raw memory to contain default-constructed objects of type t.
		* @param buff Pointer to the buffer
		* @param n Number of elements of type t to be initialized in the buffer.
		* @param t Type of the elements.
		*/
		static void Initialize_Buffer(void* buff, size_t n, TypedChunk::DataType t);

		/**
		* Initialize raw memory to contain copies of objects from an existing array.
		* @param dst Pointer to the raw memory buffer to fill.
		* @param src Pointer to the existing array of objects to be copied into dst.
		* @param n Number of elements of type t to be copied.
		* @param t Type of the elements.
		*/
		static void Init_Copy_Buffer(void* dst, const void* src, size_t n, TypedChunk::DataType t);

		/**
		* Initialize raw memory to contain objects from an existing array.
		* @param dst Pointer to the raw memory buffer to fill.
		* @param src Pointer to the existing array of objects to be moved into dst. 
		*            The src array will contain initialized empty objects of type t after the call.
		* @param n Number of elements of type t to be moved into the dst array.
		* @param t Type of the elements.
		*/
		static void Init_Move_Buffer(void* dst, void* src, size_t n, TypedChunk::DataType t);

		/**
		* Copy an array of objects.
		* @param dst Pointer to the existing array to copy over.
		* @param src Pointer to the existing array of objects to be copied into dst.
		* @param n Number of elements of type t to be copied.
		* @param t Type of the elements.
		*/
		static void Copy_Buffer(void* dst, const void* src, size_t n, TypedChunk::DataType t);

		/**
		* Call destructors for objects of type t.
		* @param buff Pointer to the buffer
		* @param n Number of elements of type t to be destroyed.
		* @param t Type of the elements.
		*/
		static void Destroy_Buffer(void* buff, size_t n, TypedChunk::DataType t);

	public: // interface
		/**
		* Construct a TypedChunk and allocate memory for the data.
		* @param length Number of existing elements of given type in the memory area.
		* @param type The type of the elements.
		*/
		TypedChunk(size_t length, DataType type)
			: memchunk_(length*SizeOf(type))
			, logical_ptr_(memchunk_.GetPhysicalPointer())
			, vector_length_(length)
			, elem_type_(type)
			, size_of_elem_(SizeOf(type))
			, nodestroy_(false)
			, delayed_initializer_(NULL)
		{
			Init();
		}

		TypedChunk(size_t length, DataType type, DelayedInitializer* delayed_initializer)
			: memchunk_(0)
			, logical_ptr_(NULL)
			, vector_length_(length)
			, elem_type_(type)
			, size_of_elem_(SizeOf(type))
			, nodestroy_(false)
			, delayed_initializer_(delayed_initializer)
		{
			memchunk_.IncrTypeCount();
		}

		/**
		* Construct a TypedChunk from an existing memory area.
		* @param length Number of existing elements of given type in the memory area.
		* @param type The type of the elements.
		* @param p Pointer to the existing memory area. If type is an object type, then the array must contain corresponding initialized C++ objects.
		* @param p_free Pointer to the deallocation function. Pass NULL if deallocation is not needed. The deallocation function will be called with p as the argument.
		* @param offset Offset in bytes in the memory area where the typed array begins.
		*/
		TypedChunk(size_t length, DataType type, void *p, MemChunk::ReleaseFunc1 p_free, size_t offset=0)
			: memchunk_(p, p_free, length*SizeOf(type) + offset)
			, logical_ptr_(static_cast<const char*>(memchunk_.GetPhysicalPointer())+offset)
			, vector_length_(length)
			, elem_type_(type)
			, size_of_elem_(SizeOf(type))
			, nodestroy_(false)
			, delayed_initializer_(NULL)
		{
			memchunk_.IncrTypeCount();
		}

		/**
		* Construct a TypedChunk from an existing memory area, using a deallocation function with an extra callback value.
		* @param length Number of existing elements of given type in the memory area.
		* @param type The type of the elements.
		* @param p Pointer to the existing memory area. If type is an object type, then the array must contain corresponding initialized C++ objects.
		* @param p_free Pointer to the deallocation function. Pass NULL if deallocation is not needed. The deallocation function will be called with p and callback_data as arguments.
		* @param callback_data Extra pointer value to be passed back to the deallocation function.
		* @param offset Offset in bytes in the memory area where the typed array begins.
		*/
		TypedChunk(size_t length, DataType type, void *p, MemChunk::ReleaseFunc2 p_free, void* callback_data, size_t offset=0)
			: memchunk_(p, p_free, callback_data, length*SizeOf(type))
			, logical_ptr_(static_cast<const char*>(memchunk_.GetPhysicalPointer())+offset)
			, vector_length_(length)
			, elem_type_(type)
			, size_of_elem_(SizeOf(type))
			, nodestroy_(false)
			, delayed_initializer_(NULL)
		{
			memchunk_.IncrTypeCount();
		}

		/**
		* Construct a TypedChunk from an existing MemChunk. This is a more general method than the one provided by other constructors.
		* @param length Number of existing elements of given type in the memory area. This must fit into the MemChunk size, together with offset.
		* @param type The type of the elements.
		* @param chunk The memory chunk. If type is an object type, then the array must contain corresponding initialized C++ objects. 
		*		Managament of the objects is taken over by TypedChunk. It is not allowed to pass such MemChunk to more than one TypedChunk.
		* @param offset Offset in bytes in the memory chunk where the typed array begins.
		*/
		TypedChunk(size_t length, DataType type, const MemChunk& chunk, size_t offset=0);

		TypedChunk(const TypedChunk& b)
			: memchunk_(b.memchunk_)
			, logical_ptr_(b.logical_ptr_) 
			, vector_length_(b.vector_length_)
			, elem_type_(b.elem_type_)
			, size_of_elem_(b.size_of_elem_) 
			, nodestroy_(b.nodestroy_)
			, delayed_initializer_(b.delayed_initializer_?b.delayed_initializer_->Clone(): NULL)
		{
			memchunk_.IncrTypeCount();
		}

		void operator=(const TypedChunk& b);

		~TypedChunk() {
			Cleanup();
		}

		/// Return the number of typed elements in the buffer.
		size_t Length() const {return vector_length_;}

		/// Return the type of elements.
		DataType ElemType() const {return elem_type_;}

		/**
		* Return const pointer to the typed data if the correct template parameter is used, otherwise throw.
		* The returned array may be in shared use with other TypedChunk or MemChunk objects, modification
		* of the array (by casting away the constness) is prohibited.
		*/
		template<typename T> const T* TypedPtr() const {
			const void* p = GetPointer();
			if (CType2ElemType(T())==ElemType()) {
				return static_cast<const T*>(p);
			} else {
				throw Nbaseutil::Exception(Nbaseutil::ERR_PROGRAM_ERROR, "Invalid type cast of vector data.");
			}
		}

		/**
		* Return pointer to the typed data if the correct template parameter is used, otherwise throw. 
		* If the internal array is currently in shared use, it is detached (a copy is made), thus
		* the returned array will not be in shared use with any other MemChunk/TypedChunk.
		*/
		template<typename T> T* TypedPtr() {
			GetPointer();
			return const_cast<T*>(const_cast<const TypedChunk*>(this)->TypedPtr<T>());
		}

		/// Return const pointer to the typed data, if the template parameter is the same as the actual type, modulo signedness, otherwise throw.
		template<typename T> const T* CompatPtr() const {
			const void* p = GetPointer();
			if (CType2ElemType(T())==ElemType() || (sizeof(T)==SizeOf(ElemType()) && std::numeric_limits<T>::is_integer && IsIntegral(ElemType()))) {
				return static_cast<T*>(p);
			} else {
				throw Nbaseutil::Exception(Nbaseutil::ERR_PROGRAM_ERROR, "Invalid type cast of vector data.");
			}
		}

		/**
		* Return pointer to the typed data, if the template parameter is of the same as the actual type, module signedness, otherwise throw.
		* The returned array will not be in shared use.
		*/
		template<typename T> const T* CompatPtr() {
			GetPointer();
			return const_cast<const TypedChunk*>(this)->CompatPtr<T>();
		}

		/// Return untyped pointer to the typed data array. The array may remain in shared use.
		const void* GetPointer() const {
			if (delayed_initializer_) {
				DelayedInitialize();
			}
			return logical_ptr_;
		}

		/// Return untyped pointer to the typed data array. The returned array will not be in shared use (is detached when necessary).
		void* GetPointer() {
			if (delayed_initializer_) {
				DelayedInitialize();
			}
			if (memchunk_.IsDataShared() || memchunk_.IsReadOnly()) {
				DetachSharedData();
			}
			return const_cast<void*>(logical_ptr_);
		}

		/**
		* Replace the encapsulated MemChunk. The previous MemChunk is released. 
		* The size of the new MemChunk must be at least Length()*ElemSize()+offset, otherwise an Exception is thrown.
		* @param new_chunk The new chunk to use. The data area will remain in shared use with the new_chunk argument. If type is an object type, then the array must contain corresponding initialized C++ objects.
		*		Managament of the objects is taken over by TypedChunk. It is not allowed to pass such MemChunk to more than one TypedChunk.
		* @param offset Offset in bytes in the new MemChunk memory block where the typed array begins.
		*/
		void SetMemChunk(const MemChunk& new_chunk, unsigned int offset=0);

		/**
		* Replace the encapsulated MemChunk and return the previous MemChunk. 
		* The size of the new MemChunk must be at least Length()*ElemSize()+offset, otherwise an Exception is thrown.
		* @param new_chunk The new chunk to use. It will be swapped into the object. If type is an object type, then the array must contain corresponding initialized C++ objects.
		*		Managament of the objects is taken over by TypedChunk. It is not allowed to pass such MemChunk to more than one TypedChunk.
		*		After return the new_chunk object contains the previous chunk data.
		*		If the element type is an object type, then the returned array contains living C++ objects which must be destroyed properly by the caller.
		* @param offset Offset in bytes in the new MemChunk memory block where the typed array begins.
		*/
		void SwapMemChunk(MemChunk& new_chunk, unsigned int offset=0);

		/**
		* Resize the array. This may involve calling MemChunk::Resize().
		* @param new_length New vector length, in elements. This may be larger or smaller than before.
		* @param copy_content Copy the old content over into new array.
		* @param init_to_zero When enlarging the buffer, init the new buffer part to zero/empty objects. For object types this is always done.
		*/
		void ResizeVector(size_t new_length, bool copy_content=true, bool init_to_zero=true);

		/// Return element size in bytes.
		unsigned int ElemSize() const {return size_of_elem_;}

		/// Return const reference to the encapsulated MemChunk.
		const MemChunk& GetMemChunk() const {return memchunk_;}

		/// Return the offset value specified by ctor or later.
		unsigned int GetOffset() const;

		/// Content comparison
		bool operator==(const TypedChunk& b) const;

		/// Check the internal consistency of the object. In case of failure appends error messages to msg parameter and returns false.
		bool Consistent(Nbaseutil::safestring& msg) const;

		/// This has to be called if the memory release function will destroy C++ objects in the buffer by itself.
		void SetNoDestroy() {nodestroy_=true;}

		/**
		* Compresses the encapsulated array to reduce the memory footprint. 
		* The array is automatically uncompressed later when accessed.
		* This method is marked 'const' as the object state does not change logically.
		* @return The number of bytes saved by the compression. 
		*		Zero is returned if compression is not possible for any reason
		*		(array is in shared use, already compressed, or delay-initialised).
		*/
		size_t Compress() const;

	private: // implementation

		/// Calls default ctors for objects in the data buffer.
		void Init();

		/// Calls dtors for objects in the data buffer.
		void Cleanup();

		/// Ensure that the data buffer would not be in shared use.
		void DetachSharedData();

		/// Initialize by delayed initializer.
		void DelayedInitialize() const;

	private:  // data
		MemChunk memchunk_;
		/// Pointer to the beginning of typed data in the memory chunk.
		const void* logical_ptr_; 
		/// Size of the typed data, in elements
		size_t vector_length_;
		/// Element type.
		DataType elem_type_;
		/// Size of the element
		unsigned char size_of_elem_;
		/// flag to not destroy C++ objects before deallocation
		bool nodestroy_;
		/// Pointer to the delayed initializer, or NULL
		PDelayedInitializer delayed_initializer_;
	};


// IMPLEMENTATION DETAILS
/// @cond Implementation_details

inline unsigned int TypedChunk::SizeOf(DataType x) {
	switch(x) {
	case TypedChunk::Int: case TypedChunk::UnsignedInt: return sizeof(int);
	case TypedChunk::Short: case TypedChunk::UnsignedShort: return sizeof(short);
	case TypedChunk::Char: case TypedChunk::Byte: return sizeof(char);
	case TypedChunk::Float: return sizeof(float);
	case TypedChunk::Double: return sizeof(double);
	case TypedChunk::VoidStar: return sizeof(void*);
	case TypedChunk::PMemory: return sizeof(PMemBlock);
	case TypedChunk::Triple: return sizeof(triple);
	case TypedChunk::PolyType: return sizeof(DataItem);
	case TypedChunk::String: return sizeof(Nbaseutil::safestring);
	case TypedChunk::Int64: case TypedChunk::UnsignedInt64: return sizeof(Nbaseutil::int64);
	default: throw "TypedChunk::SizeOf(): unknown vector type";
	}
}

/// @endcond

} // namespace


#endif
