#ifndef x_ACAPELLA_MEMBLOCK_JOB_H_INCLUDED_
#define x_ACAPELLA_MEMBLOCK_JOB_H_INCLUDED_

#include "dataitem.h"

namespace NIMacro {

	class JobPoolImpl;
	typedef ThreadSharablePointer<JobPoolImpl> PJobPoolImpl;

	/**
	* A base class for parallel jobs. For running some kind of code in parallel
	* you need to derive a custom class from this base class. The objects of the 
	* derived class will be added to the JobPool instance.
	*
	* Most of the functions may be called only during job running, 
	* i.e. directly or indirectly from Run().
	*/
	class DI_MemBlock Job: public Nbaseutil::mb_malloced {
	public: // ctor/dtor

		/// Construct a new Job.
		Job(): jobindex_(-1), jobpoolimpl_(NULL), jobthread_id_(0) {}

		/// Dtor
		virtual ~Job() {}

	public: // virtual interface, defined/overriden in the derived class

		/// Ensure that any potentially mutable Acapella non-threadsafe objects accessible through this object are not visible to other threads.
		virtual void MakeCopy()=0;

		virtual void MakePhysicalCopy()=0;

		/** 
		Worker function. Override to do what is wanted. 
		Run() will be called in the worker thread. 
		Run() has to return the calculation results, or throw an exception to signal a failure.
		*/
		virtual DataItem Run()=0;

		/**
		* This is called after removing the job from job pool after completion or abortion, by default executes 'delete this;'. 
		* Override if the job is not created by the 'new' operator.
		* In this function the job is not in run mode any more, i.e. job pool is inaccessible.
		*/
		virtual void TerminateJob() {delete this;}

		/// Process a message sent by the master thread
		virtual DataItem ProcessMessage(int msg, DataItem& arg1) {return Undefined;}

		/**
		* If the derived Job object has contained MemBlock objects, 
		* then this function must be overridden to call 
		* SetSecondaryInt(MemBlock::key_thread_id, thread_id) 
		* for all contained MemBlock objects. This is used for 
		* consistency checks in Debug builds.
		*/
		virtual void AssignThreadId(unsigned long thread_id) {}

	protected: // Base class service functions for using in the derived class:

		/**
		* Checks if the job should be aborted, and throws an exception in that case. 
		* This may be called only during job run, and should be called periodically
		* after each few seconds from there.
		*/
		void CheckAbortion();

		/**
		* Notify the master thread (job pool) and continue immediately.
		* @param msg Message ID, arbitrary, should be documented for any given job pool.
		* @param arg1 Message data. The content is swapped away so the argument will be
		*				Undefined after returning from the function.
		*/
		void Notify(int msg, DataItem& arg1);

		/**
		* Post a message to the master thread (job pool) and wait for the answer.
		* @param msg Message ID, arbitrary, should be documented for any given job pool.
		* @param arg1 Message data. The content is swapped away so the argument will be
		*				Undefined after returning from the function.
		* @return Answer from the job pool. 'Undefined' is returned if the job pool did not
		*		handle this message (for example, if the job pool was deleted from the Acapella datablock).
		*/
		DataItem Consult(int msg, DataItem& arg1);

		/// Process the messages sent by the master thread, if any.
		void ProcessMessageQueue(int waiting_time=0);

		/**
		* Store a value by the job pool for using later in other Job-s of the same job pool. 
		* Different physical threads see different values.
		*/
		void SetThreadSpecificItem(const Nbaseutil::safestring& name, const DataItem& value);

		/**
		* Return a value stored by the job pool via SetThreadSpecificItem. 
		* Different physical threads see different values. 
		* If the name is not present, an Undefined value is returned.
		*/
		DataItem GetThreadSpecificItem(const Nbaseutil::safestring& name);

	public: // Getters

		/// Returns job index in pool, or -1.
		int GetJobIndex() const {return jobindex_;}

		/// Returns owning pool. Can be called only after the job has been added to a pool.
		JobPoolImpl& GetJobPoolImpl();

	private: // data modified by JobPoolImpl class:
		friend class JobPoolImpl;
		friend class JobPoolImpl_ST;
		friend class JobPoolImpl_MT;
		int jobindex_;
		PJobPoolImpl jobpoolimpl_;

	private: // data modified by WorkerThread class:
		friend class WorkerThread;
		unsigned long jobthread_id_;
	};

} // namespace
#endif
