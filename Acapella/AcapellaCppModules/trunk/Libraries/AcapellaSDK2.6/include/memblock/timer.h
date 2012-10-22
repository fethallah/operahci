#ifndef x_PKI_CTG_ACAPELLA_MEMBLOCK_TIMER_H_INCLUDED
#define x_PKI_CTG_ACAPELLA_MEMBLOCK_TIMER_H_INCLUDED

namespace NIMacro {

	class TimerMgr;

	/**
	* A helper class for using automatic timer thread.
	* To add timer callback feature to your class, derive it from TimerCallback,
	* override the OnTimer() virtual member function and ensure that Start() and Stop()
	* are called at appropriate time moments.
	* 
	* Alternatively, one can use the AddTimerCallback template instead, 
	* which ensures that Start() is called after construction and
	* Stop() is called before destruction of the derived object.
	* When you use this approach, then wrap your class in AddTimerCallback template
	* and use the resulting typedef instead of your class in the rest of the code.
	*/
	class DI_MemBlock TimerCallback {
	public:
		/// The timer facility will call the callback roughly after each sleep seconds has been passed.
		TimerCallback(int sleep_seconds);
		/// The actual callback function, to be overridden in the derived class.
		virtual void OnTimer()=0;
		/// Start() must be called after construction or from the constuctor body of the most derived object. Start() is equivalent to Start(false).
		void Start();
		/**
		* Start() must be called after construction of most derived object.
		* @param trigger_first_call_immediately If true, the timer callback is scheduled to be called first time immediately. 
		*					Normally the first call happens after 'sleep_seconds'.
		*/
		void Start(bool trigger_first_call_immediately);
		/// Stop() must be called before destructing the most derived object, or from the destructor body of the most derived object.
		void Stop();
		virtual ~TimerCallback();
	private: // implementation
		friend class TimerMgr;
		void MakeCallback() throw();
	private:
		const int sleep_seconds_;
		boost_recursive_mutex mx_;
		bool active_;
	};

	/**
	* Adds a timer callback option to the client class.
	* Instead of class T, the client class has just to use AddTimerCallback<T>.
	* The class T must define a (non-virtual) function: void OnTimer();
	* The OnTimer() function may not remain waiting for other threads, and should not
	* carry out any time-consuming activitites.
	*/
	template<class T> 
	class AddTimerCallback: public T, public TimerCallback {
	public:
		/**
		* Construct with the desired sleep interval. The OnTimer() callback is called
		* from another thread periodically. The duration from the end of one call to 
		* start of the next call is at least the specified amount of seconds (wall clock),
		* but may be more, depending on system load and other registered timer callback
		* classes.
		*
		* This uses the default T ctor. If you need constructors with parameters, add 
		* analoguous constructors with more parameters here, they will be not instantiated
		* for classes where they are not used.
		*/
		AddTimerCallback(int sleep_seconds): T(), TimerCallback(sleep_seconds) {TimerCallback::Start();}

		AddTimerCallback(int sleep_seconds, bool trigger_first_call_immediately): T(), TimerCallback(sleep_seconds) {TimerCallback::Start(trigger_first_call_immediately);}

		/// The destructor stops the timer callback feature automatically.
		~AddTimerCallback() {TimerCallback::Stop();}

	private: // implementation
		/// Implementation, do not override. Instead provide T::OnTimer().
		virtual void OnTimer() {T::OnTimer();}
	};

} // namespace
#endif
