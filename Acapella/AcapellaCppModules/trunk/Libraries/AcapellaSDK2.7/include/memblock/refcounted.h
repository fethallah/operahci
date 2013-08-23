#ifndef x_ACAPELLA_MEMBLOCK_REFCOUNTED_H_INCLUDED
#define x_ACAPELLA_MEMBLOCK_REFCOUNTED_H_INCLUDED

#ifdef MEMBLOCK_HALFSTABLE
#include "refcounted_stable.h"
#else

#ifdef _MSC_VER
#pragma warning(disable: 4275)
#endif

#include <string>
#include <exception>
#include <typeinfo>

// Avoid Boost preprocessor errors when including <acapella/DI_acapellaR.h>
#ifndef _DEBUG
#undef __MSVC_RUNTIME_CHECKS
#endif
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>

#ifdef ACAPELLA_PLATFORM_POSIX
#	include <typeinfo>
#endif
#include "safevalue.h"

// Change 10.11.2004: use boost mutex both on Windows and Linux
namespace NIMacro {

#ifdef ACAPELLA_PLATFORM_WINDOWS
	typedef unsigned long thread_id_t; // DWORD
#else
	typedef Nbaseutil::iptr_t thread_id_t;
#endif

class SafeValue;
class Lock;
class SimpleLock;
class SerializeVisitor;
class AcaVisitor;
class Formatter;
using Nbaseutil::boost_mutex;
using Nbaseutil::boost_recursive_mutex;
using Nbaseutil::boost_condition;

/**
* A Mutex class. This wraps boost::recursive_timed_mutex on both Windows and Linux.
* Use together with the Lock class.
*
* The Mutex class distinguishes inbetween GUI thread and non-GUI threads.
* For setting the GUI thread the client code must call Mutex::SetGuiThreadID() in the correct thread,
* before any other operations.
* In GUI threads the mutex timeout is always 0.5 seconds. In other threads the mutex timeout is 5 seconds in Debug
* and 3 seconds in Release builds, but can be changed individually by Mutex::SetTimeout() member function.
* The GUI thread timeout is 0.5 seconds, in anticipation that the GUI response should be that quick,
* and assuming that losing a GUI event is not disastrous - the user can try to click once more.
*/
class DI_MemBlock Mutex: private boost::noncopyable {
public:
	Mutex();
	~Mutex();

	/// Set the timeout for mutex, in units of 0.01 seconds. This timeout is used only in non-GUI threads.
	int SetTimeout(int centiseconds);

	/**
	* Return true if the mutex might be currently locked by this or by another thread.
	* Return false if the mutex is probably currently unlocked.
	* Note that this is only informational, and the mutex status may be changed
	* by another thread immediately at any time before or after returning from this function.
	* Additionally, such a change is not guaranteed to be visible by this function,
	* and different threads are not guaranteed to get consistent results when calling this function.
	* So the result of this function call may be used only for tentative optimization
	* of some functionality; the results should not depend on the return value of this function.
	*/
	bool IsLocked() const;

	/**
	* Return true if the mutex is currently locked by the calling thread.
	* This function return value is stable, opposite to the IsLocked() function.
	* See also TryLock() class.
	*/
	bool IsLockedByCurrentThread() const;


	/// Announce that current thread is GUI thread; previously used GUI thread ID is returned. Pass false to indicate that no thread should be handled as GUI thread.
	static void SetGuiThread(bool is_gui_thread_id=true);

	/// Return true if the current thread is considered the GUI thread.
	static bool IsGuiThread();

	/// Return the current thread ID as reported by relevant OS SDK function.
	static thread_id_t GetCurrentThreadId();

	/**
	* Return the number showing how many times the thread has recursively locked
	* the mutex. Returned result is "stable" only in the thread which helds the lock.
	*/
	int GetLockCount() const;

	/// Return ID of the thread currently holding the lock, or 0. Returned value is "stable" only in the thread actually holding the lock.
	thread_id_t GetLockingThreadId() const;

	/**
	* If the mutex is locked, returns the filename parameter announced in the Acquire call.
	* If the thread has called Acquire recursively multiple times, then the filename of the first call is returned.
	* Returned value is "stable" only in the thread actually holding the lock.
	* If mutex is not locked, then a pointer to an empty string is returned.
	*/
	const char* GetLockingFileName() const {return filename_? filename_: "";}

	/// Similar to GetLockingFileName()
	int GetLockingLine() const {return line_;}
private:
	mutable boost::recursive_timed_mutex mx_;
	mutable const char* filename_;
	mutable unsigned short line_;	// use two shorts instead of single int, in order to keep layout and size of the object the same.
	mutable short lockcount_;	// locking count
    mutable unsigned int thread_id_;  // thread id which has locked the mutex, or NULL.
	int timeout_;	// the timeout for this mutex in centiseconds.
	friend class Lock;
};

/// A lock for locking a Mutex object until local block exit.
class DI_MemBlock Lock: private boost::noncopyable {
public:
	Lock(Mutex& mutex, const char* filename, int line);
	~Lock();
private: // data
	boost::recursive_timed_mutex::scoped_lock lk_;
	Mutex& mutex_;
};

#ifdef DEFINE_IMACRO_SCOPELOCK_MACRO
#	undef SCOPELOCK
#	define IMACRO_CONCATENATE_DIRECT(s1, s2) s1##s2
#	define IMACRO_CONCATENATE(s1, s2) IMACRO_CONCATENATE_DIRECT(s1, s2)
#	define IMACRO_ANONYMOUS_VARIABLE(str) IMACRO_CONCATENATE(str, __LINE__)
/// A macro to simplify creating an Lock object.
#	define SCOPELOCK(mutex) NIMacro::Lock IMACRO_ANONYMOUS_VARIABLE(imacro_lock)(mutex,__FILE__,__LINE__)
#	define SIMPLELOCK(mx) BOOST_RECURSIVE_MUTEX_SCOPED_LOCK(mx)
#endif



// Cannot be instantiated
class DI_MemBlock WeakThreadSharablePointerBase {
protected:
	WeakThreadSharablePointerBase(const ThreadSharable* obj);
	WeakThreadSharablePointerBase(const WeakThreadSharablePointerBase& b);
	WeakThreadSharablePointerBase& operator=(const WeakThreadSharablePointerBase& b);
	virtual ~WeakThreadSharablePointerBase();
	PSharable Get() const;
	virtual void NotifyWeakPointerReleased()=0;
private: // data
	// The data, especially the next_ field, is also protected by the corresponding ThreadSharable::mutex_,
	// which acts like a master lock for all weakpointers to the object. Inner mutex is needed as well because
	// of protecting the obj_ member.
	mutable Nbaseutil::boost_recursive_mutex mx_;
	ThreadSharable* obj_; // pointer to a live object, or NULL.
	WeakThreadSharablePointerBase* next_; // next weakpointer to the same object, or NULL.
private: // implementation
	friend class ThreadSharable;
	/// Nullify this and all linked weakpointers. Return true if succeeded.
	bool Release();
};

/** A base class for observing weak pointer deletion. For using, derive your own class from that and override
* the NotifyWeakPointerReleased() function. Create the weak pointer by specifying your class as the second template
* parameter, and your object as the second ctor parameter.
*/
class WeakPointerObserver {
public:
	/**
	* Called when a weak pointer is released, which has this object as an observer. The base class implementation does nothing.
	* @param weak_ptr_addr Pointer to the weak pointer. The weak pointer value itself is already NULL by the time of call.
	*           The pointed ThreadSharable object may be alive, but it should not be accessed in any way.
	*/
	virtual void NotifyWeakPointerReleased(WeakThreadSharablePointerBase* weak_ptr_addr) {}

	virtual ~WeakPointerObserver() {}
};


/**
* A class for holding a "weak" pointer to a ThreadSharable object.
* When the ThreadSharable object is destroyed, all weak pointers
* referring to it are nullified automatically. This is done thread-safely, no external
* synchronisation needed.
*/
template <class T, class U=WeakPointerObserver>
class WeakThreadSharablePointer: private WeakThreadSharablePointerBase {
	typedef WeakThreadSharablePointerBase super;
public:
	/**
	* Construct by an existing ThreadSharablePointer only.
	* @param obj Pointer to an existing ThreadSharable object.
	* @param observer Optional observer to be notfied when the weak pointer is released. If present, then observer
	*                must outlive the weak pointer.
	*/
	WeakThreadSharablePointer(const ThreadSharablePointer<T>& obj, WeakPointerObserver* observer=NULL)
		: WeakThreadSharablePointerBase(obj.Pointer())
		, observer_(observer)
	{}

	/**
	* Return a new smartpointer to the object. If object has been deleted, return NULL.
	* Naturally, the object will be alive while this pointer exists.
	* Note that implementing bool or void* operator is not possible by thread-safety reasons.
	*/
	ThreadSharablePointer<T> Get() const {
		PSharable p = super::Get();
		DEBUG_ASSERT(!p || dynamic_cast<T*>(p.Pointer()));
		return reinterpret_cast<T*>(p.Pointer());
	}
private: //implementation
	U* observer_;
	void NotifyWeakPointerReleased() {
		if (observer_) {
			observer_->NotifyWeakPointerReleased(this);
		}
	}
};

/**
* A base class for objects which are accessed intermittently
* from different threads. It contains a mutex for locking
* the whole object.
*
* Conventions are as follows:
* - A corresponding ThreadSharablePointer<...> smartpointer
*		will be defined.
* - All objects are allocated on heap and the pointer is
*		assigned to a smartpointer immediately, or at least
*		before the pointer becomes known to other threads.
*		The object will be alive as long as there is at least
*		one smartpointer to it alive.
* - The object can be passed around and handed over to another
*		threads by copying the smartpointers. During creation/destruction
*		of smartpointers the object is locked for a very short time.
*		Therefore it is suggested that smartpointers passed by
*		(const) reference instead of value, when possible, to enhance
*		performance.
* - For accessing object data and methods the object must be locked.
*		For that purpose one may use corresponding LockingThreadSharablePointer
*		smartpointers. One or more LockingThreadSharablePointers to an object
*		can exist only in a single thread, other threads' access is blocked
*		while any LockingThreadSharablePointer is alive.
* - If a method is documented as needing no
*		external locking, then it is not needed to
*		use LockingThreadSharablePointers. In this case the method
*		performs (possibly more granular) locking inside the object itself,
*		if needed.
*/
class DI_MemBlock ThreadSharable: public Nbaseutil::mb_malloced {
public: // static interface

	typedef PSharable (*factory_func_t)();
	
	/// Register a ThreadSharable-derived class along with a factory function which is able to create dummy objects of this class.
	static void RegisterClass(const Nbaseutil::safestring& classname, factory_func_t factory_func);

	/// Create a dummy object of a registered ThreadShrable-derived class. An exception is thrown if class is not registered.
	static PSharable CreateDummy(const Nbaseutil::safestring& classname);

public: // interface
	/// Increase refcount; this is called by smartpointers.
	void Capture() const;

	/// Decrease refcount; this is called by smartpointers. If refcount drops to zero, object is deleted.
	void Release() const;

	/**
	* Return current refcount of the object. For ensuring stability of
	* the result one should lock the object during this call. If return value
	* is 1 and the smartpointer you use to access the object is visible
	* to only one thread, then this thread is the single (remaining) owner
	* of the object, i.e. return value 1 remains stable also after unlocking the
	* object.
	*/
	int GetRefCount() const {return refcount_;}

	/// Return the verbal description of the object; derived classes should override this function.
	virtual Nbaseutil::safestring GetDescription() const {return "ThreadSharable object";}

	/// Serialize the object in specified format, if possible, otherwise throw. See MemBlock::Serialize() for parameters documentation.
	void Serialize(int ser_format, SerializeVisitor& visitor, const Nbaseutil::safestring& itemname, const Formatter& formatter, Nbaseutil::safestring& buffer) const;

	/// Unserialize an object, if possible, otherwise throw. See MemBlock::UnSerialize() for parameters documentation.
	PSharable UnSerialize(int ser_format, const Formatter& formatter, const char* buffer, const char** p_buff_end=NULL) const;

	/// Traverse the item and call visitor for itself and contained items.
	virtual void Traverse(TraverseVisitor& visitor, const TraverseNode& node);

	/// Call visitor for all child items.
	virtual void IterateChildren(AcaVisitor& visitor, const TraverseNode& node) {}

protected:
	ThreadSharable()
		: ACAPELLA_MUTEX_INIT(refcount_mx_, Nbaseutil::locklevel_trivial)
		, refcount_(0)
		, ACAPELLA_MUTEX_INIT(weak_pointers_mx_, Nbaseutil::locklevel_service5+5)
		, weak_pointers_(NULL) 
	{}
	ThreadSharable(const ThreadSharable& b)
		: ACAPELLA_MUTEX_INIT(refcount_mx_, Nbaseutil::locklevel_trivial)
		, refcount_(0) // refcounts do not copy
		, ACAPELLA_MUTEX_INIT(weak_pointers_mx_, Nbaseutil::locklevel_service5+5)
		, weak_pointers_(NULL) 
	{}
	ThreadSharable& operator=(const ThreadSharable& b)
	{
		return *this; // refcounts do not copy
	}
protected:
	virtual ~ThreadSharable() {}

	/**
	* Make a deep physical copy of the object for thread boundary passing, if possible, otherwise throws;
	* derived classes must override this method.
	* If current object refcount is 1, then it is allowed to skip copy of this object and those subobjects
	* which are accessible only through this object, in order to achive better performance.
	*/
	virtual PSharable DoClone() const=0;

	/// Called before deletion. Return true if there is a chance that refcount has grown meanwhile. If the refcount after return from this function is more than zero, the object won't be deleted.
	virtual bool NotifyBeforeDelete() const {return false;}

	virtual void DoSerialize(int ser_format, SerializeVisitor& visitor, const Nbaseutil::safestring& itemname, const Formatter& formatter, Nbaseutil::safestring& buffer) const {
		throw Nbaseutil::Exception(Nbaseutil::ERR_NOTIMPLEMENTED, Nbaseutil::Printf("Serialization format %d not supported for: %s")(ser_format)(GetDescription()));
	}

	virtual PSharable DoUnSerialize(int ser_format, const Formatter& formatter, const char* buffer, const char** p_buff_end) const {
		throw Nbaseutil::Exception(Nbaseutil::ERR_NOTIMPLEMENTED, Nbaseutil::Printf("Unserialization not supported by: %s")(GetDescription()));
	}

public:
	// Enh 29.03.2004: provide virtual Clone and DoVerb methods in this base class
	/**
	* Generic interface for adding functionality; derived classes will override it to process specific verbs;
	* The meaning of arguments depends on the verb.
	* If the object doesn't know how to handle the verb, it should call base class implementation.
	*/
	virtual SafeValue DoVerb(const char* verb, const SafeValue& arg1, const SafeValue& arg2) {return Undefined;}

	/**
	* Makes a copy of the object, if possible, otherwise throws.
	* The copy is made so that it will be safe to pass it into another thread.
	* This method always makes a physical copy, use PSharable::MakeCopy() for enhancing performance.
	*/
	PSharable Clone() const {
		PSharable dummy(this); // increase refcount in order to force physical copy always.
		PSharable s = DoClone();
#ifdef DEBUG_ASSERT
		DEBUG_ASSERT( typeid(*this)==typeid(*s));
#endif
		return s;
	}
private: // friend interface for WeakThreadSharablePointerBase
	void AddWeakPointer(WeakThreadSharablePointerBase& ptr);
	void RemoveWeakPointer(WeakThreadSharablePointerBase& ptr);
private: // data protected by refcount_mx_
	mutable Nbaseutil::boost_mutex refcount_mx_;
	mutable int refcount_;
private: // data protected by weak_pointers_mx_
	mutable Nbaseutil::boost_mutex weak_pointers_mx_;
	WeakThreadSharablePointerBase* weak_pointers_;
private: // friends
	friend class Lock;
	friend class TryLock;
	friend class WeakThreadSharablePointerBase;
	friend class ThreadSharablePointer<ThreadSharable>;
	friend class DataItem;
};


template<class T>
 inline ThreadSharablePointer<T>::ThreadSharablePointer(const T* a): p_(const_cast<T*>(a)) {
	 if (a) ((ThreadSharable*)a)->Capture();
 }
template<class T>
 inline ThreadSharablePointer<T>::ThreadSharablePointer(const ThreadSharablePointer<T>& b): p_(const_cast<T*>(b.p_)) {
	 if (p_) ((ThreadSharable*)p_)->Capture();
 }
template<class T>
template<class U>
 inline ThreadSharablePointer<T>::ThreadSharablePointer(const ThreadSharablePointer<U>& b): p_(const_cast<U*>(b.p_)) {
	 if (p_) p_->Capture();
 }

 template<class T>
 inline ThreadSharablePointer<T>::~ThreadSharablePointer() {
	 if (p_) ((ThreadSharable*)p_)->Release();
 }
template<class T>
 inline const ThreadSharablePointer<T>& ThreadSharablePointer<T>::operator=(const ThreadSharablePointer<T>& b) const {
     T* bp = b.p_;
	 if (bp) ((ThreadSharable*)bp)->Capture();
	 T* p = p_;
	 p_ = bp; // must be atomic!
	 if (p) ((ThreadSharable*)p)->Release();
	 return (ThreadSharablePointer<T>&) (*this);
 }
template<class T>
 inline const ThreadSharablePointer<T>& ThreadSharablePointer<T>::operator=(const T* p) const {
	 if (p) ((ThreadSharable*)p)->Capture();
	 T* p1(p_); 
	 p_ = const_cast<T*>(p); // must be atomic!
	 if (p1) ((ThreadSharable*)p1)->Release();
	 return (const ThreadSharablePointer<T>&) (*this);
 }

template<class T>
template<class U>
inline const ThreadSharablePointer<T>& ThreadSharablePointer<T>::operator=(const ThreadSharablePointer<U>& b) const {
    U* bp = b.p_;
    if (bp) reinterpret_cast<ThreadSharable*>(bp)->Capture();
    T* p(p_);
	p_ = bp; // must be atomic!
	if (p) reinterpret_cast<ThreadSharable*>(p)->Release();
	return *this;
}


template<typename T>
class LockingPointer {
	// this assumes only that T has an accessible member Mutex called mx_.
	T* p_;
	Lock lck_;
public:
	LockingPointer(const T* p, const char* filename, int line): p_((T*)p), lck_(((T*)VerifyNonNull(p))->mx_, filename, line) {}
	~LockingPointer() {}

	T& operator*() {return *p_;}
	T* operator->() {return p_;}
	T* Pointer() {return p_;}

	const T& operator*() const {return *p_;}
	const T* operator->() const {return p_;}
	const T* Pointer() const {return p_;}

	operator void*() const {return p_;}
private:
#ifdef _DEBUG
	static const T* VerifyNonNull(const T* p) {
		DEBUG_ASSERT(p);
		return p;
	}
#else
	static const T* VerifyNonNull(const T* p) {return p;}
#endif
};


template<typename T>
class RefcountedPointer {
	// This assumes only that T has const methods Capture() and Release().
	// This is const-transparent, i.e. no const-correctness is maintained.
	mutable T* p_;
public:
	RefcountedPointer(const T* p=NULL): p_(const_cast<T*>(p)) {
#ifdef _DEBUG
		ASSERT(dynamic_cast<T*>(p_)==p_);
#endif
		if (p_) p_->Capture();
	}
	RefcountedPointer(const RefcountedPointer<T>& b): p_(b.p_) {
#ifdef _DEBUG
		ASSERT(dynamic_cast<T*>(p_)==p_);
#endif
		if (p_) p_->Capture();
	}
	~RefcountedPointer() {
#ifdef _DEBUG
		ASSERT(dynamic_cast<T*>(p_)==p_);
#endif
		if (p_) p_->Release();
	}
//	const RefcountedPointer& operator=(const RefcountedPointer<T>& b) const {
	const RefcountedPointer& operator=(const RefcountedPointer<T>& b) {
#ifdef _DEBUG
		ASSERT(dynamic_cast<T*>(p_)==p_);
		ASSERT(dynamic_cast<T*>(b.p_)==b.p_);
#endif
		T* bp = b.p_;
		if (bp) bp->Capture();
		T* p = p_;
		p_ = bp;
		if (p) p->Release();
		return *this;
	}
	T& operator*() const {
#ifdef _DEBUG
		ASSERT(p_ && dynamic_cast<T*>(p_)==p_);
#endif
		return *p_;
	}
	T* operator->() const {
#ifdef _DEBUG
		ASSERT(p_ && dynamic_cast<T*>(p_)==p_);
#endif
		return p_;
	}
	T* Pointer() const {return p_;}
	/// Bugfix 14.11.2011 (fb #34070): do not define 'operator bool', in order to avoid unexpected integral conversions
	operator void*() const {return p_;}
};

/**
* A base class for simple non-locked refcounted classes.
* Use a RefcountedPointer template specialization for defining smartpointers
* for derived classes.
*/
class DI_MemBlock Refcountable: public Nbaseutil::mb_malloced {
	mutable int refcount_;
public:
	/// Default ctor
	Refcountable(): refcount_(0) {}

	/// Copy ctor, to avoid copying the refcount.
	Refcountable(const Refcountable& b): refcount_(0) {}

	/// Copy assignment op, to avoid copying the refcount.
	Refcountable& operator=(const Refcountable& b) {return *this;}

	/// Virtual dtor, to make the RefcountedPointer mechanism working properly.
	virtual ~Refcountable() {}

	/// Increase the refcount. This is called automatically by RefcountedPointer.
	void Capture() const {++refcount_;}

	/// Decrease the refcount. This is called automatically by RefcountedPointer.
	void Release() const {if (--refcount_<=0) delete const_cast<Refcountable*>(this);}
};


/**
* A base class for a threadsafe refcounted classes. The reference counter is
* protected by a mutex so one can copy and pass the smartpointers to other threads.
* Use a RefcountedPointer template specialization for defining smartpointers
* for derived classes.
*/
class DI_MemBlock ThreadSafeRefcountable: public Nbaseutil::mb_malloced {
	mutable int refcount_;
	mutable boost_mutex mx_;
public:
	/// Default ctor
	ThreadSafeRefcountable(): refcount_(0), ACAPELLA_MUTEX_INIT(mx_, Nbaseutil::locklevel_refcount) {}

	/// Copy ctor, to avoid copying the refcount.
	ThreadSafeRefcountable(const ThreadSafeRefcountable& b): refcount_(0), ACAPELLA_MUTEX_INIT(mx_, Nbaseutil::locklevel_refcount) {}

	/// Copy assignment op, to avoid copying the refcount.
	ThreadSafeRefcountable& operator=(const ThreadSafeRefcountable& b) {return *this;}

	/// Virtual dtor, to make the RefcountedPointer mechanism working properly.
	virtual ~ThreadSafeRefcountable() {
#ifdef DEBUG_ASSERT
		DEBUG_ASSERT(refcount_==0);
#endif
	}

	/// Increase the refcount. This is called automatically by RefcountedPointer.
	void Capture() const {
		BOOST_MUTEX_SCOPED_LOCK(mx_);
		++refcount_;
	}

	/// Decrease the refcount. This is called automatically by RefcountedPointer.
	void Release() const {
		int refcount;
		{
			BOOST_MUTEX_SCOPED_LOCK(mx_);
			refcount = --refcount_;
		}
		if (refcount==0) {
			delete const_cast<ThreadSafeRefcountable*>(this);
		}
	}
	int GetRefCount() const {
		BOOST_MUTEX_SCOPED_LOCK(mx_);
		return refcount_;
	}
};

/*

template <class T, class U>
class NNlink;

template <class T, class U>
class NNlink_node {
	friend class NNlink<T,U>;
	friend class NNlink<U,T>;
	RefCountedPointer<T> t_;
	RefCountedPointer<U> u_;

	NNlink_node(const T* t, const U* u): t_(t), u_(u) {}
	NNlink_node(const RefCountedPointer<T>& t, const RefCountedPointer<U>& u): t_(t), u_(u) {}
}

template <class T, class U>
class NNlink_ {

};
*/

/** A special exception to throw in GUI thread in case of locked mutexes.
* The GUI thread should catch this exception, Sleep for a moment and try again.
* This assumes that the operation can be redone without ill side effects.
* This is usually the case with GUI thread (window refresh, etc.)
*/
class MutexTimeOutException: public std::runtime_error {
public:
	MutexTimeOutException(const char* filename, int lineno);
};

class DI_MemBlock SingleThreadAlloced {
public:
	void* operator	new(size_t n) { return Alloc(n); }
	void operator delete(void *p) { Free(p); }
	void* operator new(size_t n, void* placement) {return placement;}
	void operator delete(void *p, void* placement) {}
	unsigned long GetMyThreadId() const;
#ifdef _CRTDBG_MAP_ALLOC
	void* operator new(size_t n, const char* filename, int lineno) {return Alloc(n);}
	void operator delete(void* p, const char* filename, int lineno) {Free(p);}
	void* operator new(size_t n, int blocktype, const char* filename, int lineno) {return Alloc(n);}
	void operator delete(void *p, int blocktype, const char* filename, int lineno) { Free(p); }
#endif
private: // not implemented
	void* operator new[](size_t n, const char* filename, int lineno);
	void operator delete[](void* p, const char* filename, int lineno);
	void* operator new[](size_t n, int blocktype, const char* filename, int lineno);
	void operator delete[](void *p, int blocktype, const char* filename, int lineno);
	void* operator new[] (size_t n);
protected:
	void  operator delete[] (void* p);
private: // implementation
	static void* Alloc(size_t n);
	static void Free(void *p);
};

#if 0
/** A callback class for finer control over mutex locking.
*/
class mutex_callback: public Nbaseutil::mb_malloced {
public:
	/** Called if mutex is locked. Return timeout in units of 0.01 seconds, to wait for lock, or throw an exception.
	* This call is followed either by on_timeout() or got_mutex() call.
	* If 0 is returned, the timeout appears immediately.
	* @param default_timeout The default timeout to wait, in units of 0.01 seconds. This may depend on thread.
	* @param is_gui_thread Current thread is GUI thread, as registered by Mutex::SetGuiThreadID.
	*/
	virtual int on_locked(int default_timeout, unsigned int mutex_id, bool is_gui_thread, const char* my_filename, int my_lineno, unsigned long locking_thread_id, const char* locking_filename, int locking_lineno) {
		return default_timeout;
	}
	/** Called at timeout. Return additional timeout in milliseconds to wait for lock, or throw an exception
	*   If 0 is returned, the mutex throws an Exception.
	*/
	virtual int on_timeout(int default_timeout, unsigned int mutex_id, bool is_gui_thread, const char* my_filename, int my_lineno, unsigned long locking_thread_id, const char* locking_filename, int locking_lineno) {
		return 0;
	}
	/// Mutex acquired after waiting. This (and other methods) is not called if mutex was acquired on first try.
	virtual void got_mutex(unsigned int mutex_id, bool is_gui_thread, const char* my_filename, int my_lineno) {}
};

/** Set the new callback for mutex timeouts.
* Pointer to the previous callback is returned.
* Pass NULL to remove any callbacks.
*/

DI_MemBlock mutex_callback* set_mutex_callback(mutex_callback* new_callback);
#endif

} // namespace

#endif
#endif
