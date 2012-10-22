#ifndef x_ACAPELLA_MEMBLOCK_MEMCHUNK__H_INCLUDED
#define x_ACAPELLA_MEMBLOCK_MEMCHUNK__H_INCLUDED

#ifdef _MSC_VER
typedef volatile long atomic_integer_t;
#else
#include <tr1/memory>
typedef _Atomic_word atomic_integer_t;
#endif

namespace NIMacro {

	class DI_MemBlock AtomicCounter: public Nbaseutil::mb_malloced {
	public:
		AtomicCounter(): x_(0) {}
		void Inc();
		int Dec();
		int GetValue_unsafe() {return int(x_);} // Unsafe unless 0 or 1 is returned, otherwise may return incorrect value in multiprocessor environment.
	private:
		atomic_integer_t x_;
	};


	/// A class encapsulating a raw memory block. This class provides safe deallocation mechanisms and reference-counting for shared use of the memory.
	class DI_MemBlock MemChunk: public Nbaseutil::mb_malloced {
	public:

		/// A convenience typedef for Nbaseutil::Carrier::ReleaseFunc1.
		typedef Nbaseutil::Carrier::ReleaseFunc1 ReleaseFunc1;

		/// A convenience typedef for Nbaseutil::Carrier::ReleaseFunc2.
		typedef Nbaseutil::Carrier::ReleaseFunc2 ReleaseFunc2;

		/// Constructs an empty dummy MemChunk.
		MemChunk();

		/**
		* Allocate memory and encapsulate in the constructed MemBlock. The memory is zero-initialized.
		* @param n The memory size in bytes to allocate.
		*/
		MemChunk(size_t n);

		/**
		* Encapsulate an existing memory block in the constructed MemBlock. The ownership of memory is taken over by the MemBlock object.
		* 
		* @param p Pointer to the allocated memory block. This will be returned by the MallocedPointer().
		* @param p_free Pointer to the deallocation function. If not NULL, then this function will be eventually called with the value of p.
		* @param n The size of memory block in bytes.
		*/
		MemChunk(void *p, ReleaseFunc1 p_free, size_t n);

		/**
		* Encapsulate an existing memory block in the constructed MemBlock. The ownership of memory is taken over by the MemBlock object.
		* 
		* @param p Pointer to the allocated memory block. This will be returned by the MallocedPointer().
		* @param p_free_ex Pointer to the deallocation function. If not NULL, then this function will be eventually called with p and callback_data.
		* @param callback_data A value to be passed back to the deallocation function. It must be either NULL or point to an existing object in accessible memory.
		* @param n The size of the allocated memory block in bytes.
		*/
		MemChunk(void *p, MemChunk::ReleaseFunc2 p_free_ex, void* callback_data, size_t n);

		/// Construct a MemChunk from a Carrier object. The memory ownership is passed over to the MemChunk. The Carrier object will be empty afterwards.
		MemChunk(const Nbaseutil::Carrier& carrier);

		/// Copy ctor. This is quite cheap operation. The physical memory block remains in shared use.
		MemChunk(const MemChunk& b);


		/// Comparison operator. Compares data content byte-by-byte.
		bool operator==(const MemChunk& b) const;

		/// Fast no-throw swap.
		void Swap(MemChunk& b) throw() {
			std::swap(releasefunc1_, b.releasefunc1_);
			std::swap(releasefunc2_, b.releasefunc2_);
			std::swap(callback_data_, b.callback_data_);
			std::swap(clonecount_, b.clonecount_);
			std::swap(physical_ptr_, b.physical_ptr_);
			std::swap(physical_size_, b.physical_size_);
			std::swap(readonly_, b.readonly_);
		}

		~MemChunk() throw();

		/// Return the pointer that would be passed to the deallocation function.
		const void* GetPhysicalPointer() const {return physical_ptr_;}

		/// Return the initally declared MemChunk size.
		size_t GetPhysicalSize() const {return physical_size_;}

		/// Return true if the managed memory block is in shared use with another MemChunk.
		bool IsDataShared() const {return clonecount_ && clonecount_->GetValue_unsafe()>1;}

		/// If the managed memory block is in shared use, return its sharedness refcount, otherwise 0. There are two counters: for the memory block and for the construction of typed array inside it.
		int GetCloneCount(bool typed) const {return clonecount_? clonecount_[typed?1:0].GetValue_unsafe(): 0;}

		/// Return true if the encapsulated memory may be not writable.
		bool IsReadOnly() const {return readonly_;}

		/**
		* Check the internal class invariants. Returns true if OK.
		* @param msg In case of problems error messages are appended here.
		*/
		bool Consistent(Nbaseutil::safestring& msg) const;

		/**
		* Resize the managed memory block. This may involve a new allocation and a copy. 
		* This function may not be called if the buffer is currently holding non-POD objects.
		* @param n New size in bytes. This may be larger or smaller than before.
		* @param copy_content Copy the old content over in any case. The copy is done by memcpy().
		* @param init_to_zero When enlarging the buffer, init the new buffer part to zero.
		*/
		void Resize(size_t n, bool copy_content=true, bool init_to_zero=true);

		/// Assignment operator. This is quite cheap operation. The physical memory block remains in shared use.
		MemChunk& operator=(const MemChunk& b);

		/**
		* This function is meant for callers who manage C++ objects in the MemChunk. 
		* In this case the caller has to call this function to indicate that it manages the C++ objects in the raw array.
		* If raw array is in shared use, this function increments the typecount field.
		*/
		void IncrTypeCount() const;

		/**
		* This function is meant for callers who manage C++ objects in the MemChunk. 
		* In this case the caller has to call this function to indicate that it is giving up managament of C++ objects in the raw array.
		* If raw array is in shared use, this function decrements the typecount field and returns the resulting typecount.
		* If zero is returned, this means that no other client is managing the C++ objects and the caller has to destroy the objects.
		*/
		int DecrTypeCount() const;

		/// Return true if the data is in shared use with some client who is managing the C++ objects in the MemChunk.
		bool IsDataTyped() const;

		/**
		* Transfers memory ownership to a carrier which is returned. 
		* The object will be empty after that operation. 
		* If the data is in shared use, a copy of the memory is made. 
		* Otherwise this is a fast operation.
		*/
		Nbaseutil::Carrier TransferToCarrier();

	private: // implementation
		void InitCloneCount();
		void Init(const MemChunk& b);
		void Cleanup() throw();
		void Dismiss(); // abandon the ownership of memory.

	private: // data
		friend void test_memchunk(MemChunk& mc, int mode);
		ReleaseFunc1 releasefunc1_;
		ReleaseFunc2 releasefunc2_;
		void* callback_data_;
		mutable AtomicCounter* clonecount_;
		void* physical_ptr_;
		size_t physical_size_;
		bool readonly_;
	};

	/// Back-compatibility typedef, use MemChunk::ReleaseFunc1 instead.
	typedef Nbaseutil::Carrier::ReleaseFunc1 freeProc;

#ifdef _MSC_VER
#ifdef _WINBASE_
	// <windows.h> has been included, the Interlocked functions are available inline
	// Because of winsock/windows includes order and other dependencies
	// cannot #include <windows.h> directly here.
	inline void AtomicCounter::Inc() {
		::InterlockedIncrement(&x_);
	}
	inline int AtomicCounter::Dec() {
		return ::InterlockedDecrement(&x_);
	}
#endif
#else
	inline void AtomicCounter::Inc() {
        __gnu_cxx::__atomic_add(&x_, 1);
    }

	inline int AtomicCounter::Dec() {
        return __gnu_cxx::__exchange_and_add(&x_, -1) - 1;
    }
#endif

} // namespace
#endif
