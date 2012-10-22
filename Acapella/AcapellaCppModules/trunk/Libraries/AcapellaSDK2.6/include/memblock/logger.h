#ifndef x_PKI_CTG_ACAPELLA_MEMBLOCK_LOGGER_H_INCLUDED
#define x_PKI_CTG_ACAPELLA_MEMBLOCK_LOGGER_H_INCLUDED

#include <deque>
#include "refcounted.h"

namespace NIMacro {

	using Nbaseutil::safestring;
	class LoggerImpl;

	/// A callback virtual base class for registering extra logger destinations, see Logger::AddLogSink().
	class LogSink {
	public:
		/// This will be called for each log message which is currently being logged (has passed the verbosity level and filters).
		virtual void Log(const safestring& topic, const safestring& subtopic, int level, const safestring& message, const safestring& threadname)=0;
		virtual ~LogSink() {}
	};

	/// A callback virtual base class for registering extra logger destinations, see Logger::AddLogSink(). This class gets a raw log line as it would appear in the main log file.
	class RawLogSink {
	public:
		/// This will be called for each log message which is currently being logged (has passed the verbosity level and filters).
		virtual void Log(const safestring& log_line)=0;
		virtual ~RawLogSink() {}
	};


	/**
	* A singleton class for performing logging for different subsystems.
	* This class provides functions for two different usage:
	*    - functions for the Acapella and other code for logging messages;
	*    - functions for the client code to control the logging amount and destinations.
	*/
	class DI_MemBlock Logger {
	public:
		/// Identifies subsystem to be logged. These enum constants are bit flags.
		enum topic_t {
			/// Special flag, contains zero bitmask indicating that no topics are selected.
			topic_none=0,
			/// Special flag containing all bits. Allows for apllying a single operation to all topics.
			topic_all=0xffffffff,
			/// Wormhole subsystem.
			topic_wormhole=1<<0,
			/// Messages related to multithreading setup and communication.
			topic_thread=1<<2,
			/// Messages related to IO from/to disk files
			topic_file=1<<3,
			/// Messages related to loading module libraries, module registration and lookup.
			topic_module=1<<4,
			/// Messages related to the execution of actual Acapella scripts.
			topic_run=1<<5,
			/// Messages related to the HTTP server subsystem and for service handlers not having their own topic.
			topic_http=1<<6,
			/// Messages related to the Acapella Connector subsystem.
			topic_acc=1<<7,
			/// Messages related to the HtmlView subsystem.
			topic_htmlview=1<<8,
			/// Messages related to data conversions.
			topic_conv=1<<9,
			/// Messages related to features used for back-compatibility with previous software versions.
			topic_backcompatibility=1<<10,
			/// Messages related to the DataBundle subsystem.
			topic_databundle=1<<11,
			/// Licensing subsystem
			topic_license=1<<12,
			/// Debugger subsystem
			topic_debugger=1<<13,
			/// Parsing of Acapella scripts and expressions
			topic_parsing=1<<14,
			/// Cache manager
			topic_cache=1<<15,
			/// General Acapella library
			topic_acapella=1<<16,
			/// Online help facility
			topic_online_help=1<<17,
			/// Graphical User Interface
			topic_gui=1<<18,
			/// Third-party libraries
			topic_extern=1<<19,
			/// Input, ProvideInput and Output mechanism
			topic_input=1<<20,
			/// Module call tracing
			topic_call=1<<21,
			/// Performance optimization (both memory and speed)
			topic_performance=1<<22,
			/// Exceptions and error propagation
			topic_exception=1<<23,
			/// Columbus database
			topic_columbus=1<<24,
			/// UI package
			topic_ui=1<<25,
			/// AAL package
			topic_aal=1<<26,

			/// Placeholder for additional custom main topics, accessed by their name only.
			topic_custom=1<<31,
		};

		enum level_t {
			/// Used for switching logging off in the SetLogging() call.
			level0_no_logging=0,
			/// A fatal/critical error, further functioning of the subsystem is impossible or questionable.
			level1_critical=1,
			/// An error has occured in the functioning of the subsystem (topic) itself.
			level2_error=2,
			/// A warning related to the logged topic.
			level3_warning=3,
			/// An abnormal sitation, handled automatically, like an error from another subsystem passed through the logged subsystem.
			level4_abnormal=4,
			/// A main event concerning the given topic.
			level5_event_major=5,
			/// A secondary event concerning the given topic.
			level6_event_minor=6,
			/// More detailed event descriptions.
			level7_event_details=7,
			/// More lengthy descriptions of the subsystem state.
			level8_verbose=8,
			/// Maximum verbosity descriptions of the subsystem state.
			level9_maxverbose=9,
		};

		/**
		* Check if this a topic is being logged, and if the specified level is being logged at all. 
		* This is a fast operation. Note that a positive result does not mean that this particular
		* topic is logged at the specified level, use GetLogging() for that if needed.
		* 
		* @param topic Main logging topic, any topic_t enum value except topic_none or topic_all.
		* @param level The log message level, this is actually an integer value representing the logging verbosity.
		*/
		static bool IsLogging(topic_t topic, level_t level) { return level<=all_log_level_ || ((topic & logmask_)!=0 && level<=max_logged_level_);}

		/**
		* Log a message through the logging facility.
		* If the topic is not being logged, this function does nothing. In order to avoid
		* needless construction of the message string, it is suggested to call
		* Logger::IsLogging(topic, level) beforehand to check if calling Logger::Log() is needed at all.
		* @param topic Main logging topic, any topic_t enum value except topic_none or topic_all.
		* @param subtopic Informal subtopic name. This is used for switching off and on logging in a more fine-grained fashion.
		* @param level The log message level, this is essentially an integer value in range 1..9 representing the logging verbosity. Pass a level_t enum constant except level0_no_logging.
		* @param message The log message, in plaintext, no formatting currently supported.
		*/
		static void Log(topic_t topic, const safestring& subtopic, level_t level, const safestring& message);

		/**
		* Log a topic message through the logging facility, by identifying the main topic by name.
		* This function is intended primarily for logging custom topics. 
		* If the name matches one of system topics, the function is identical to the overload using topic_t enum.
		* If the topic is not logged, this function does nothing. In order to avoid
		* needless construction of the message string, it is suggested to call
		* Logger::IsLogging(Logger::String2Topic(topicname), level) beforehand to check if logging is switched on at all.
		* @param topic Main logging topic.
		* @param subtopic Informal subtopic name. This is used for switching off and on logging in a more fine-grained fashion.
		* @param level The log message level, this is essentially an integer value in range 1..9 representing the logging verbosity. Pass a level_t enum constant except level0_no_logging.
		* @param message The log message, in plaintext, no formatting currently supported.
		*/
		static void Log(const safestring& topicname, const safestring& subtopic, level_t level, const safestring& message);

		/**
		* Unconditionally log a line to the log file. The log sinks registered with AddLogSink() are not notified.
		* However, the log sinks registered with AddRawLogSink() are notified.
		* @param log_line Log line. This should match the current log line format. It should not contain line breaks.
		*/
		static void Log(const safestring& log_line);

		/**
		* Adjust logging level of a single topic or all topics ("all"). By default all logging is off (level0_no_logging).
		* @param topicname The main topic name. For system topics this coincides with the topic_t enum constant name sufix after "topic_", any other strings are considered custom topics. Pass "all" to set logging level for all topics at once.
		* @param level The maximum logged level for the specified topic. All messages with levels less or equal to the specified level will be logged. Pass level0_no_logging for turning the logging off.
		*/
		static void SetLogging(const safestring& topicname, level_t level);

		/// Return the logging level for the specified topic.
		static level_t GetLogging(topic_t topic);

		/// Return the logging level for the specified topic. For system topic names this is the same as another overload of GetLogging().
		static level_t GetLogging(const safestring& topicname);

		/// Return the current maximum log level for any topic. This is a fast operation.
		static level_t GetMaxLoggedLevel() {return max_logged_level_;}

		/**
		* Add a rule to enable logging of certain topics or subtopics. 
		* The filters are applied to the log messages which have passed the level threshold for the given topic.
		* If there is a matching enabling rule for the log message, or if there is no disabling rule for the topic, it will be passed to logger output.
		* @param topicname_pattern A pattern matching the topic name. Wildcards '*' and '?' can be used here.
		* @param subtopic_pattern A pattern matching the subtopic name. Wildcards '*' and '?' can be used here.
		* @param message_pattern A pattern matching the log message. Wildcards '*' and '?' can be used here.
		*/
		static void AddEnablingRule(const safestring& topicname_pattern, const safestring& subtopic_pattern, const safestring& message_pattern="*");

		/**
		* Add a rule to suppress logging of certain topics or subtopics. 
		* The filters are applied to the log messages which have passed the level threshold for the given topic.
		* If there is a matching enabling rule for the log message, or if there is no disabling rule for the topic, it will be passed to logger output.
		* @param topicname_pattern A pattern matching the topic name. Wildcards '*' and '?' can be used here.
		* @param subtopic_pattern A pattern matching the subtopic name. Wildcards '*' and '?' can be used here.
		* @param message_pattern A pattern matching the log message. Wildcards '*' and '?' can be used here.
		*/
		static void AddDisablingRule(const safestring& topicname_pattern, const safestring& subtopic_pattern, const safestring& message_pattern="*");


		/// Remove all rules added by Add*Rule().
		static void ClearRules();

		/**
		* Change the log destination file. 
		* @param filename Log destination filename. Default is "stderr". Pass an empty string to turn file logging off. Pass "stderr" or "stdout" to direct log messages to STDERR or STDOUT respectively. 
		* @param logformat Log message format, containing placeholders for filling in by the logger:
		*           - %d Date in format YYYY-MM-DD
		*			- %t Local time in format HH:MM:SS
		*			- %l The verbosity level the log message belongs to (1-9)
		*			- %i Thread number (a small integer)
		*			- %a Acc session ID
		*			- %p Process ID
		*			- %r Thread name or ID (see SetThreadName()).
		*			- %h Topic name
		*			- %s Subtopic name
		*			- %m Log message
		*			- %f Script location
		*			- %% A verbatim percent sign.
		*/
		static void SetLogFile(const safestring& filename, const safestring& logformat="%d %t %l {%i/%a} [%h/%s] %m |PID=%p|TID=%r|SESS=%a| %f");

		/**
		* Change the log destination file, plus specify a size limit for the log file. See the other overload for common parameter 
		* descriptions.
		* @param sizelimit_in_MB Size limit for the log file, in megabytes.
		*/
		static void SetLogFile(const safestring& filename, int sizelimit_in_MB, const safestring& logformat="%d %t %l {%i/%a} [%h/%s] %m |PID=%p|TID=%r|SESS=%a| %f");

		/// Add an extra logging destination. The passed object might be alive until the program termination or until calling RemoveLogSink() on it.
		static void AddLogSink(LogSink& logsink);			

		/// Remove an extra logging destination.
		static void RemoveLogSink(LogSink& logsink);			

		/// Add an extra logging destination. The passed object has to be alive until the program termination.
		static void AddRawLogSink(RawLogSink& rawlogsink);			

		/// Return current logger settings in XML.
		static safestring GetLoggerStatus();

		/// Obsolete, use AcaThreadMgr::SetThreadName() instead.
		static void SetThreadName(thread_id_t tid, const safestring& threadname);

		/// Obsolete, use AcaThreadMgr::GetThreadName() instead.
		static safestring GetCurrentThreadName();

		/// Obsolete, use AcaThreadMgr::GetThreadNo() instead.
		static int GetCurrentThreadNo();

		/// Convert topic enum to the corresponding topic name
		static safestring Topic2String(topic_t topic);

		/// Convert topic name to the corresponding topic enum. If name is not known, topic_custom is returned.
		static topic_t String2Topic(const safestring& topicname);

		/// Obsolete, use AcaThreadMgr::SetAccSessionId() instead.
		static void SetAccSessionId(int session_id);

		/// Obsolete, use AcaThreadMgr::GetAccSessionId() instead.
		static int GetAccSessionId();

		/// Obsolete, use AcaThreadMgr::NotifyThreadExit() instead.
		static void NotifyThreadExit();


		/// A helper structure for filtering the log lines with ExtractLogLines().
		struct LogFilter {
			/// Acc session ID to retrieve. 0=messages from outside of sessions, -1=all messages.
			int session_id;
			/// Last log level to retrieve.
			int level;
			/// First second (incl) to include in the extracted lines.
			time_t start;
			/// Last second (excl) to include in the extracted lines. 0=all lines present in the log file.
			time_t finish;
			/// Thread number as seen in the log file. -1=all threads.
			int threadno;
			/// Topic name. Empty=all topics.
			safestring topic;
			/// Subtopic name. Emprt=all subtopics.
			safestring subtopic;
			/// Additional filter - contains words to include or exclude (minus-prepended).
			safestring filter;
			/// Default ctor, extracts all lines from the log file.
			LogFilter(): session_id(-1), level(9), start(0), finish(0), threadno(-1) {}
		};

		/** 
		* Retrieve the log lines from the current log file, matching the specified criteria.
		* If there is no log file or the log is redirected to stderr or stdout, then no lines are retrieved.
		* @param filter The structure for filtering the log lines.
		* @param sink The output parameter. Extracted log lines are appended here.
		* @return The number of lines added to the \a sink.
		*/
		static int ExtractLogLines(const LogFilter& filter, std::deque<safestring>& sink);

		/// Return the maximum Acc session ID encountered in the current log file.
		static unsigned int GetMaxSessionId();

	private:
		static unsigned int logmask_;
		static level_t all_log_level_;
		static level_t max_logged_level_;
		static LoggerImpl* logger_;
	};

} // namespace
#endif
