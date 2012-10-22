#ifndef x_ACAPELLA_MEMBLOCK_JOBPOOL_H_INCLUDED_
#define x_ACAPELLA_MEMBLOCK_JOBPOOL_H_INCLUDED_

#include <vector>
#include "refcounted.h"

namespace NIMacro {

	class Job;

	class JobPoolImpl;
	typedef ThreadSharablePointer<JobPoolImpl> PJobPoolImpl;

	/**
	* A class encapsulating a pool of related jobs.
	* This class can be used as is, or it can be derived from
	* for implementing custom notification handlers.
	*
	* For every JobPool there is an owner thread. JobPool may be accessed
	* only by this thread. The access pattern goes like follows:
	* 1. The JobPool object is constructed.
	* 2. Jobs are added to it by AddJob() member function.
	* 3. Wait() is called for waiting for jobs completion, maybe repeatedly
	* 4. During Wait(), the HandleNotify() and HandleConsult() callback methods 
	*    may get called. These methods are called in the owner thread as well.
	* 5. Wait() may be terminated by timeout, by exception or normally.
	* 6. Results are gathered by GetResults().
	* 7. JobPool object is destroyed.
	*/
	class DI_MemBlock JobPool: public ThreadSharable {
	public: // static interface

		/// Return global info about the jobpool system.
		static void GetInfo(int& processor_count, int& current_thread_count, int& busy_thread_count, int& suggested_new_jobpool_threadcount);

		/**
		* Changes the global number of worker threads used by all JobPool objects. 
		* The change does not affect the type of already existing JobPool objects
		* (single-threaded or multi-threaded).
		*
		* @param desired_threadcount The desired number of global working threads (1 or more). The default value is given by the environment variable ACAPELLA_NUM_THREADS; if this does not exist, then the number of the processors in the system. 
		*/
		static void SetGlobalThreadCount(int desired_threadcount);

		/// Returns true if current thread is running as or on behalf for a JobPool worker thread.
		static bool IsCurrentThreadRunningAsWorker();

		/// Sets the flag returned by IsCurrentThreadRunningAsWorker() and returns the previous value.
		static bool SetCurrentThreadRunningAsWorker(bool as_worker);

		/**
		* A local BlockingOperation object has to be created in the scope where a thread can potentially 
		* run in a JobPool worker thread and can remain waiting for another JobPool worker thread.
		* If the current thread is not a JobPool worker thread, creating a BlockingOperation object has no effect.
		* Otherwise, it is used for thread scheduling in the JobPool mechanism, to avoid deadlocks.
		*/
		class DI_MemBlock BlockingOperation {
		public:
			BlockingOperation();
			~BlockingOperation();
		private:
			bool is_jobpool_worker_thread_;
		};

	public: // interface
		/**
		* Constructs a job pool.
		* @param allow_failing_jobs Allows some jobs to fail. The client should call the JobStatus() function to check for that. The job result is an error message.
		* @param headless Creates a job pool without the listening side. One may not call the Wait() function. The pool is destroyed when all added Job objects are released. The jobs are expected to communicate everything with other means than the pool messages.
		*/
		JobPool(bool allow_failing_jobs=false, bool headless=false);

		~JobPool();

		/// Allocates room for indicated number of jobs in the pool. This function is optional; it exists just for a slight performance improvement.
		void Reserve(int jobcount);

		/// Add a job into the pool. The ownership of the Job is moved to the pool. Returns job index.
		int AddNewJob(Job* job); // Note: "AddJob" would conflict with a windows macro.

		/// Return the number of jobs added to the job pool.
		int GetJobCount() const;

		enum jobstatus_t {
			js_not_existing,
			js_not_started,
			js_running,
			js_succeeded,
			js_failed,
		};

		/// Return current job status. Can be called only in job pool thread.
		jobstatus_t GetJobStatus(int jobindex);

		/// Wait until all jobs are finished; throws if Break() is called meanwhile.
		void Wait();

		/// Wait until all jobs are finished, or Break() is called, or timeout occurs. Returns number of unfinished jobs.
		int Wait(int timeout_in_s); 

		/**
		* Return pointer to job results array. Can be called only in job pool thread.
		* The array length and content may change in next calls to job pool in this thread.
		*/
		std::vector<DataItem>& GetResults();

		/// Break the currently executing Wait() ASAP, to be called from a notification handler.
		void Break(int reason);

		/// Abort the currently running Wait() ASAP. Call from another thread.
		void Abort(const Nbaseutil::safestring& abort_msg);

		enum msg_t {
			msg_job_started		= 100001,
			msg_job_succeeded	= 100002,
			msg_job_failed		= 100003,
		};

		/**
		* Post a notification to the job pool and continue immediately.
		* This function can be called from any thread.
		* The notifications are processed during Wait().
		* @param msg Message ID, arbitrary, should be documented for any given job pool.
		* @param arg1 Message data. The content is swapped away so the argument will be
		*				Undefined after returning from the function.
		*/
		void Notify(int msg, DataItem& arg1);

		/**
		* Post a message to the job pool and wait for the answer.
		* This function can be called from any thread.
		* The notifications are processed during Wait().
		* @param msg Message ID, arbitrary, should be documented for any given job pool.
		* @param arg1 Message data. The content is swapped away so the argument will be
		*				Undefined after returning from the function.
		* @return Answer from the job pool. 'Undefined' is returned if the job pool did not
		*		handle this message (for example, if the job pool was deleted from the Acapella datablock).
		*/
		DataItem Consult(int msg, DataItem& arg1);

		/**
		* Process a notify message from the job
		* @param job_index The index of the job.
		* @param msg Message ID. ID-s above 100000 are reserved, see msg_t enum for predefined constants.
		* @param arg1 Addittional message data, meaning depends on the message ID.
		*/
		virtual void HandleNotify(int job_index, int msg, DataItem& arg1) {}

		/// Process a consulting request from the job and return a result
		virtual DataItem HandleConsult(int job_index, int msg, DataItem& arg1) {return Undefined;}

	protected: // virtual overrides

		// Cannot clone, return iself. The impl_ data member itself is thread-safe to access.
		virtual PSharable DoClone() const {return this;} 


	public: // virtual interface
		/**
		* This function is called inside Wait() in order to check if it's OK to terminate the Wait().
		* The base class implementation will return true if the number of finished jobs is equal to the number of submitted jobs.
		*/
		virtual bool Finished();

	private:
		PJobPoolImpl impl_;
	};

	typedef ThreadSharablePointer<JobPool> PJobPool;

} //namespace
#endif
