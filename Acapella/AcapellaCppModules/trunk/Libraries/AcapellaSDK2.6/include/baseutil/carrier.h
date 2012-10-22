#ifndef x_PKI_CT_BASEUTIL_CARRIER_H_INCLUDED_
#define x_PKI_CT_BASEUTIL_CARRIER_H_INCLUDED_

#include <stddef.h> // for size_t

#ifndef NULL
#define NULL 0
#endif
#include "safestring.h"

namespace Nbaseutil {

/// A small std::auto_ptr-like class for transferring memory blocks in exception-safe way, in the baseutil library.
/**
* The Carrier class is used as the return value of InputStream::GetContent(). NIMacro::Vector cannot be used 
* for that as it is unavailable in the baseutil library. However, a Carrier can be easily transformed
* into a Vector by the Vector::CreateFrom(const Nbaseutil::Carrier&) static member function.
*/
class Carrier {
public:
	/// A typedef for simple memory release function, like free().
	typedef void (*ReleaseFunc1)(void* block);

	/// A typedef for a memory release function with extra data.
	typedef void (*ReleaseFunc2)(void* block, void* callback_data);

	/// Default constructor, encapsulates a zero-size memory block.
	Carrier()
		: block_(NULL)
		, size_(0)
		, releasefunc1_(NULL)
		, releasefunc2_(NULL) 
		, callback_data_(NULL)
		, zero_terminated_(false)
		, readonly_(false)
	{}

	/// Encapsulate a block of memory which will be released by simple release function.
	/**
	 * @param block Pointer to the block
	 * @param size Size of the block in bytes
	 * @param releasefunc If not NULL, then (*releasefunc)(block) will be called for releasing the memory.
	 * @param zero_terminated If true, then there must be an extra zero byte after the memory block. 
	 * @param readonly If true, then the memory block might not be writable (e.g. a readonly memory-mapped file).
	 * This will mean better performance for the c_str() member function.
	 * The zero byte is not counted in the size parameter.
	 */
	Carrier(void* block, size_t size, ReleaseFunc1 releasefunc, bool zero_terminated=false, bool readonly=false)
		: block_(block)
		, size_(size)
		, releasefunc1_(releasefunc)
		, releasefunc2_(NULL) 
		, callback_data_(NULL)
		, zero_terminated_(zero_terminated)
		, readonly_(readonly)
	{}

	/// Same as the other ctor, but the memory will be released by calling (*releasefunc)(block, callback_data) instead (if releasefunc is not NULL). 
	Carrier(void* block, size_t size, ReleaseFunc2 releasefunc, void* callback_data, bool zero_terminated=false, bool readonly=false)
		: block_(block)
		, size_(size)
		, releasefunc1_(NULL)
		, releasefunc2_(releasefunc)
		, callback_data_(callback_data)
		, zero_terminated_(zero_terminated)
		, readonly_(readonly)
	{}

	/// Swaps a safestring into the Carrier object.
	DI_baseutil Carrier(safestring& buffer);

	/// Dtor, calls the release function unless the ownership of the memory has been taken away from this Carrier.
	~Carrier() {
		if (releasefunc1_) {
			(*releasefunc1_)(block_);
		} else if (releasefunc2_) {
			(*releasefunc2_)(block_, callback_data_);
		}
	}

	/// Transfers block ownerhsip a la std::auto_ptr
	Carrier(const Carrier& b)
		: block_(b.block_)
		, size_(b.size_)
		, releasefunc1_(b.releasefunc1_)
		, releasefunc2_(b.releasefunc2_)
		, callback_data_(b.callback_data_)
		, zero_terminated_(b.zero_terminated_)
		, readonly_(b.readonly_)
	{
		b.Dismiss();
	}

	/**
	* The assignment transfers block ownerhsip a la std::auto_ptr, i.e. memory block ownership is transfered to assigned object.
	* The b object is emptied (mutated despite of 'const' qualifier). 
	*/
	void operator=(const Carrier& b) {
		if (block_) {
			if (releasefunc1_) {
				(*releasefunc1_)(block_);
			} else if (releasefunc2_) {
				(*releasefunc2_)(block_, callback_data_);
			}
		}
		block_ = b.block_;
		size_ = b.size_;
		releasefunc1_ = b.releasefunc1_;
		releasefunc2_ = b.releasefunc2_;
		callback_data_ = b.callback_data_;
		zero_terminated_ = b.zero_terminated_;
		readonly_ = b.readonly_;
		b.Dismiss();
	}

	/**
	* Return raw pointer to the memory block. 
	* The Carrier retains the memory ownership, i.e. the caller must not attempt to free() or delete[] the pointer.
	* If the block size is zero, NULL may be returned.
	* The memory block may not be modified (by casting away the constness of the returned pointer).
	* Use GetMutablePointer() instead for getting read-write access to the block.
	* The returned pointer may be invalidated by a later GetMutablePointer() call.
	*/
	const void* GetPointer() const {
		return block_;
	}

	/**
	* Return a mutable raw pointer to the memory block. 
	* The Carrier retains the memory ownership, i.e. the caller must not attempt to free() or delete[] the pointer.
	* If the block size is zero, NULL may be returned.
	* The returned pointer may differ from an earlier GetPointer() call.
	*/
	DI_baseutil void* GetMutablePointer();

	/// Return the memory block size in bytes. The possible zero-termination byte is not included in the size.
	size_t GetSize() const {
		return size_;
	}

	/**
	 * Ensures that there is a zero byte after the block and returns a pointer to the beginning of the block.
	 * If the block size is zero, then returns a pointer to a static empty string.
	 *
	 * If the block is not zero-terminated, this may involve a copy of the memory block.
	 * One can call IsZeroTerminated() to find out if this is a case. 
	 * Note that InputStream::GetContent() always prepares zero-terminated blocks so
	 * if the Carrier originates from InputStream::GetContent() then it is guaranteed that there will be no unnecessary overhead.
	 */
	DI_baseutil char* c_str() const;

	/// Reports if the block is followed by a zero byte after the official size. If yes, then the c_str() member function has no overhead.
	bool IsZeroTerminated() const {return zero_terminated_;}

	/// Reports if the block cannot be physically changed. If this returns true, a memory copy must be made in order to make changes.
	bool IsReadOnly() const {return readonly_;}

	/**
	 * Takes out the Carrier data so it can be encapsulated elsewhere. 
	 * Be sure to call Dismiss() if/when the data has been assigned to a new owner.
	 * Dismiss() is not called automatically in this function as this would make it hard to write exception-safe code.
	 * If IsReadOnly() returns true, the memory block may not be modified (by casting away the constness of the pointer).
	 * The returned memory block pointer may be invalidated by a later GetMutablePointer() call.
	 */
	bool GetData(const void*& block, size_t& size, ReleaseFunc1& releasefunc1, ReleaseFunc2& releasefunc2, void*& callback_data) const {
		block = block_;
		size = size_;
		releasefunc1 = releasefunc1_;
		releasefunc2 = releasefunc2_;
		callback_data = callback_data_;
		return block_!=NULL;
	}

	/**
	* Gives up the ownership of the block, the block will not be released in the dtor any more. 
	* This is marked const, because it's called from copy ctor and assignment operator on the const argument object.
	* The memory is not accessible through this Carrier after Dismiss(), GetPointer() will return NULL and GetSize() will return 0.
	*/
	void Dismiss() const {
		// The ownership is maintained by the presence of releasefunc1_/releasefunc2_.
		releasefunc1_ = NULL;
		releasefunc2_ = NULL;
		// From now on we don't have a control over memory lifetime, so clear the fields potentially becoming invalid:
		block_ = NULL;
		size_ = 0;
		// callback_data makes sense only together with releasefunc2_, so it will be unusable anyway and does not need to be cleared.
	}
private: // data
	mutable void* block_; // mutable in order to be accessible in Dismiss().
	mutable size_t size_; // mutable in order to be accessible in Dismiss().
	mutable ReleaseFunc1 releasefunc1_;
	mutable ReleaseFunc2 releasefunc2_;
	void* callback_data_;
	bool zero_terminated_, readonly_;
};

} // namespace

// this is the last exported header, pop the warning level pushed in heap_dbg_start.h
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
