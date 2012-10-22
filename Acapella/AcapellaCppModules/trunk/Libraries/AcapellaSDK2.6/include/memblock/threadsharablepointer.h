#ifndef x_IMACRO_MEMBLOCK_THREADSHARABLEPOINTER_H_INCLUDED_
#define x_IMACRO_MEMBLOCK_THREADSHARABLEPOINTER_H_INCLUDED_

namespace NIMacro {

class ThreadSharable;


template <class T>
class ThreadSharablePointer {
public:
	typedef T element_type;
	mutable T* p_;
public:
	ThreadSharablePointer(const T* a=NULL);
	ThreadSharablePointer(const ThreadSharablePointer<T>& b);
	template<class U>
		ThreadSharablePointer(const ThreadSharablePointer<U>& b);
	~ThreadSharablePointer();

	const ThreadSharablePointer<T>& operator=(const ThreadSharablePointer<T>& b) const ;
	const ThreadSharablePointer& operator=(const T* p) const ;

	template<class U>
		const ThreadSharablePointer<T>& operator=(const ThreadSharablePointer<U>& b) const;

	// return the pointed object.
	// reflect smart-pointer constness and ness to the pointed object.

	T& operator*() {return *p_;}  // never throws
	T* operator->() {return p_;}  // never throws
	T* Pointer() {return p_;}  // never throws

	const T& operator*() const {return *p_;}  // never throws
	const T* operator->() const {return p_;}  // never throws
	const T* Pointer() const {return p_;}  // never throws

	void swap(ThreadSharablePointer<T>& other) {T* tmp=other.p_; other.p_=p_; p_=tmp;} // never throws

	/// For testing in boolean expressions; operator bool() would have nasty side-effects.
	operator void*() const  {return p_;} // should be quite safe even if p_ reading is not atomic.

	/**
	* Make a physical deep copy of the pointee for thread boundary passing. 
	* If the pointee's  refcount is 1, then its copy and copy of
	* subobjects which are accessible only through this object may be skipped.
	*/
	void MakeCopy() {
		if (p_) {
#if defined(_MSC_VER) && defined(_DEBUG) && defined(DEFINE_BASEUTIL_ASSERT)
			PSharable x = p_->DoClone();
			DEBUG_ASSERT( typeid(*p_)==typeid(*x));
			*this = x;
#else
			*this = p_->DoClone();
#endif
		}
	}

};

} // namespace
#endif
