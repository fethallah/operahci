#ifndef x_PKI_CTG_ACAPELLA_MEMBLOCK_THREADMGR_H_INCLUDED
#define x_PKI_CTG_ACAPELLA_MEMBLOCK_THREADMGR_H_INCLUDED

namespace NIMacro {

	class ThreadMgrImpl;
	class ShutdownHelper;

	class ThreadNotifCallback {
	public:
		enum threadnotif_event_t {
			thread_start,
			thread_exit,
		};
		virtual ~ThreadNotifCallback() {}
		virtual void Notify(threadnotif_event_t evt, int thread_no)=0;
	};

	/**
	* A singleton class for keeping account on running Acapella threads (threads running Acapella scripts).
	*/
	class DI_MemBlock AcaThreadMgr {
	public: // static interface

		/**
		* To be called in the beginning of an Acapella thread. This function may not be called again in the 
		* same physical OS thread, without calling NotifyThreadExit() first.
		* @param threadname Informal short name of the thread, for including in messages.
		* @return The thread number. These are assigned sequentally, starting from 0. Each new call to NotifyThreadStart() will return a new number.
		*/
		static unsigned int NotifyThreadStart(const Nbaseutil::safestring& threadname);

		/// To be called in the end of an Acapella thread.
		static void NotifyThreadExit();

		/// Get the thread number of the current thread.
		static unsigned int GetThreadNo();

		/// Get the informal name of the current thread.
		static Nbaseutil::safestring GetThreadName();

		/// Get both the thread numebr and thread name of the current thread.
		static void GetThreadInfo(unsigned int& thread_no, Nbaseutil::safestring& threadname, int& acc_session_id);

		/// Set the informal name for the current thread; returns the previous name.
		static Nbaseutil::safestring SetThreadName(const Nbaseutil::safestring& threadname);

		/// Announce that the current thread is running a task associated with the specified Acc session. Pass 0 to mark the current thread as not related to any Acc session.
		static void SetAccSessionId(int session_id);

		/// Return Acc session id associated with the current thread, or 0.
		static int GetAccSessionId();

		/// Return the number of currently executing Acapella threads.
		static int GetThreadCount(bool exclude_this_thread);

		/// Return the thread numbers and names of all currently executing Acapella threads.
		static void ListThreads(std::map<unsigned int, Nbaseutil::safestring>& list, bool exclude_this_thread);

		/// Register a callback object
		static void AddCallback(ThreadNotifCallback* callback);

		/// Unregister a callback object
		static void RemoveCallback(ThreadNotifCallback* callback);

		/// Notify the system that we are going to shut down the program. This currently concerns ShutdownHelper objects. Returns true if all threads managed by ShutdownHelper objects have exited during the timeout.
		static bool Shutdown(int timeout_s);

	private:
		friend class Logger;
		friend class ShutdownHelper;
		static ThreadMgrImpl* impl_;
	};

	/**
	* A helper class for terminating threads not running Acapella scripts.
	* Create a local ShutdownHelper object in the thread function. After waiting 
	* on the condition call its CheckShutdown() member function. This will return true
	* after AcaThreadMgr::Shutdown() has been called, in this case the thread should be
	* ended gracefully with no further actions.
	*/
	class DI_MemBlock ShutdownHelper {
	public:
		ShutdownHelper(boost_condition& cnd);
		~ShutdownHelper();
		bool CheckShutdown();
	private:
		boost_condition& cnd_;
	};

} // namespace
#endif
