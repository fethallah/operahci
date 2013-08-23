#ifndef x_ACAPELLA_HTTP_ASOCKET_H_ICLUDED_
#define x_ACAPELLA_HTTP_ASOCKET_H_ICLUDED_

// Avoid Boost preprocessor errors when including <acapella/DI_acapellaR.h>
#ifndef _DEBUG
#undef __MSVC_RUNTIME_CHECKS
#endif
#include <boost/version.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>

#include <map>
#include <time.h>
#include "safestring.h"
#include "printfer.h"
#include "imacroexception.h"

namespace Nbaseutil {

#if defined(_DEBUG) // && !defined(__INTEL_COMPILER)
#define ACAPELLA_WRAP_BOOST_MUTEX
#endif

	/**
	* Constants for lock levels to be used in ACAPELLA_MUTEX_INIT and ACAPELLA_MUTEX_DEFINE.
	* 
	* For avoiding deadlocks caused by inconsistent locking order all Acapella mutexes
	* are assigned a global order. Only a mutex with a higher lock level than the current last
	* locked mutex (in the same thread) can be locked at any time moment. The lock levels are 
	* compile-time for most C++ classes, only the ResolvePoint and Package mutexes are assigned run-time
	* values as these are locked in hierarchical order in regard of each other.
	*/
	enum locklevel_t {
		/// For initializing static data in subsystems; no other lock can be acquired beforehand.
		locklevel_subsystem_init=400,
		/// For initializing static data in subsystem component
		locklevel_subsystem_init2=420,
		/// A level 1 subsystem which is locked while performing operations inside the subsystem, including script parsing
		locklevel_subsystem1=510,
		/// A level 2 subsystem which is locked while performing operations inside the subsystem, including script parsing
		locklevel_subsystem2=520,
		/// A level 3 subsystem which is locked while performing operations inside the subsystem, including script parsing
		locklevel_subsystem3=530,
		/// A level 4 subsystem which is locked while performing operations inside the subsystem, including script parsing
		locklevel_subsystem4=540,
		/// A level 5 subsystem which is locked while performing operations inside the subsystem, including script parsing
		locklevel_subsystem5=550,
		/// For locking the Script object while parsing and reparsing the script
		locklevel_script=700,
		/// For locking the root ResolvePoint object when looking up modules during script parse; other ResolvePoint and Package lock levels are calculated from this value. 
		locklevel_resolvepoint_core=800,
		/// Marks the end of the range reserved for ResolvePoint and Package locks
		locklevel_resolvepoint_range_end=970,
		/// For locking non-threadsafe third-party libraries for serializing the access
		locklevel_library=980,
		/// A level 1 service lock for a temporary lock of the shared resource used by the service, no script parsing
		locklevel_service1=1010,
		/// A level 2 service lock for a temporary lock of the shared resource used by the service, no script parsing
		locklevel_service2=1020,
		/// A level 3 service lock for a temporary lock of the shared resource used by the service, no script parsing
		locklevel_service3=1030,
		/// A level 4 service lock for a temporary lock of the shared resource used by the service, no script parsing
		locklevel_service4=1040,
		/// A level 5 service lock for a temporary lock of the shared resource used by the service, no script parsing
		locklevel_service5=1050,
		/// Special locklevel for WeakThreadSharablePointerBase::mx_, do not use for anything else
		locklevel_weakpointer=1060,
		/// Locking the threadsafe smarpointers' refcount field
		locklevel_refcount=1200,
		/// For locking the logger facility
		locklevel_service_log=1300,
		/// For locking the Debug build memory allocation subsystem
		locklevel_service_mb_alloc=1400,
		/// For locking trivial mutexes where no other functions are called during the lock
		locklevel_trivial = 1500,
	};

#ifdef ACAPELLA_WRAP_BOOST_MUTEX

	DI_baseutil unsigned long MyGetCurrentThreadId();
	
	/// A wrapper around boost::mutex for Debug checks
	class boost_mutex: private boost::noncopyable {
	public:
		class scoped_lock: private boost::noncopyable  {
		public:
			scoped_lock(boost_mutex& mx, const char* filename, int lineno)
				: lock_start_(clock())
				, mx_(EnterLock(mx, filename, lineno))
				, m_mutex(mx.mx_)
				, lk_(m_mutex) 
			{
				++mx_.lock_count_;
				if (!mx_.recursive_ && mx_.lock_count_>1) {
					--mx_.lock_count_;
					ThrowAssertException(filename, lineno, Printf("boost_mutex already locked by %s(%d)\n")(mx_.filename_)(mx_.lineno_).c_str());
				}
				if (lock_start_+CLOCKS_PER_SEC<clock() && mx_.level()<locklevel_service_log) {
					NotifyLongMutexWait(filename, lineno);
				}
				prev_filename_ = mx_.filename_;
				prev_lineno_ = mx.lineno_;
				prev_threadid_ = mx.threadid_;
				mx_.filename_ = filename;
				mx_.lineno_ = lineno;
				mx_.threadid_ = MyGetCurrentThreadId();
			}
			~scoped_lock() {
				LeaveLock(mx_);
				if (--mx_.lock_count_<0) {
					mx_.lock_count_ = 0;
					ThrowAssertException(mx_.filename_, mx_.lineno_, "Boost_mutex not locked!");
				}
				mx_.filename_ = prev_filename_;
				mx_.lineno_ = prev_lineno_;
				mx_.threadid_ = prev_threadid_;
			}
			bool operator!() const {return !lk_;}
		private:
			// implementation
			DI_baseutil boost_mutex& EnterLock(boost_mutex& mx, const char* filename, int lineno);
			DI_baseutil void LeaveLock(boost_mutex& mx);
			DI_baseutil void NotifyLongMutexWait(const char* filename, int lineno);
		private:
			clock_t lock_start_;
			boost_mutex& mx_;
			boost::recursive_mutex& m_mutex;
			boost::recursive_mutex::scoped_lock lk_;
			const char* prev_filename_;
			int prev_lineno_;
			unsigned long prev_threadid_;
			friend class boost_condition;
		};
		boost_mutex(int level)
			: lock_count_(0)
			, filename_("")
			, lineno_(0)
			, recursive_(false)
			, level_(level)
		{}
		~boost_mutex() {
			if (lock_count_>0) {
				ThrowAssertException(filename_, lineno_, "boost_mutex locked at the time of destruction!");
			}
		}
		/**
		* In debug build only, to use in DEBUG_ASSERT().
		*/
		bool IsLockedByThisThread() {
			boost::recursive_mutex::scoped_lock tmplock(mx_);
			if (lock_count_==0) {
				return false;
			} else if (lock_count_==1) {
				return true;
			} else if (lock_count_>1 && recursive_) {
				return true;
			} else {
				ThrowAssertException(filename_, lineno_, "boost_mutex: invalid lock_count_");
				return false; // to silence compiler warnings
			}
		}
		int level() const {return level_;}
		DI_baseutil static int GetCurrentLockLevel();
	protected:
		boost_mutex(bool recursive, int level)
			: lock_count_(0)
			, filename_("")
			, lineno_(0)
			, recursive_(recursive)
			, level_(level)
		{}
	private:
		friend class scoped_lock;
		friend class boost_condition;
		friend class LocksDebugger;
		boost::recursive_mutex mx_;
		int lock_count_;
		const char* filename_;
		int lineno_;
		unsigned long threadid_;
		bool recursive_;
		int level_;
	};

	class boost_recursive_mutex: public boost_mutex {
	public:
		boost_recursive_mutex(int level): boost_mutex(true, level) {}
	};

	class boost_condition {
	public:
		void wait(boost_mutex::scoped_lock& lock) {
			if (!lock) {
				throw boost::lock_error();
			}
			// What about recursive mutexes?
			if (lock.mx_.lock_count_!=1) {
				ThrowAssertException(lock.mx_.filename_, lock.mx_.lineno_, "boost_mutex not locked or recursively locked before wait!");
			}
			lock.mx_.lock_count_ = 0;
			cnd_.wait(lock.lk_);
			if (lock.mx_.lock_count_!=0) {
				ThrowAssertException(lock.mx_.filename_, lock.mx_.lineno_, "boost_mutex not locked after wait!");
			}
			lock.mx_.lock_count_ = 1;
	    }
		bool timed_wait(boost_mutex::scoped_lock& lock, const boost::xtime& xt)
		{
			if (!lock) {
				throw boost::lock_error();
			}
			if (lock.mx_.lock_count_!=1) {
				ThrowAssertException(lock.mx_.filename_, lock.mx_.lineno_, "boost_mutex not locked or recursively locked before wait!");
			}
			lock.mx_.lock_count_ = 0;
			bool res = cnd_.timed_wait(lock.lk_, xt);
			if (lock.mx_.lock_count_!=0) {
				ThrowAssertException(lock.mx_.filename_, lock.mx_.lineno_, "boost_mutex not locked after wait!");
			}
			lock.mx_.lock_count_ = 1;
			return res;
		}
		void notify_one() {
			cnd_.notify_one();
		}
		void notify_all() {
			cnd_.notify_all();
		}
	private:
		boost::condition cnd_;
	};

#	define BOOST_MUTEX_SCOPED_LOCK(mx) Nbaseutil::boost_mutex::scoped_lock boost_mutex_lock(mx, __FILE__, __LINE__)
#	define BOOST_RECURSIVE_MUTEX_SCOPED_LOCK(mx) Nbaseutil::boost_recursive_mutex::scoped_lock boost_mutex_lock(mx, __FILE__, __LINE__)
#	define BOOST_MUTEX_NAMED_SCOPED_LOCK(name, mx) Nbaseutil::boost_mutex::scoped_lock name(mx, __FILE__, __LINE__)
#	define BOOST_RECURSIVE_MUTEX_NAMED_SCOPED_LOCK(name, mx) Nbaseutil::boost_recursive_mutex::scoped_lock name(mx, __FILE__, __LINE__)

#	define ACAPELLA_MUTEX_INIT(name,level) name(level)
#	define ACAPELLA_MUTEX_DEFINE(name,level) name(level)

#else

	// In Release build, just use the original Boost classes.

	typedef boost::mutex boost_mutex;
	typedef boost::recursive_mutex boost_recursive_mutex;
	typedef boost::condition boost_condition;

#	define BOOST_MUTEX_SCOPED_LOCK(mx) boost::mutex::scoped_lock boost_mutex_lock(mx)
#	define BOOST_RECURSIVE_MUTEX_SCOPED_LOCK(mx) boost::recursive_mutex::scoped_lock boost_mutex_lock(mx)
#	define BOOST_MUTEX_NAMED_SCOPED_LOCK(name, mx)  boost::mutex::scoped_lock name(mx)
#	define BOOST_RECURSIVE_MUTEX_NAMED_SCOPED_LOCK(name, mx) boost::recursive_mutex::scoped_lock name(mx)

#	define ACAPELLA_MUTEX_INIT(name,level) name()
#	define ACAPELLA_MUTEX_DEFINE(name,level) name

#endif


#define ACAPELLA_STATIC_INIT(expr) \
	static bool inited=false;\
	if (!inited) {\
		static boost_mutex ACAPELLA_MUTEX_DEFINE(g_mx, locklevel_subsystem_init2);\
		BOOST_MUTEX_SCOPED_LOCK(g_mx);\
		if (!inited) {\
			expr;\
			inited = true;\
		}\
	}

#define ACAPELLA_STATIC_INIT2(level,expr) \
	static bool inited=false;\
	if (!inited) {\
		static boost_mutex ACAPELLA_MUTEX_DEFINE(g_mx, level);\
		BOOST_MUTEX_SCOPED_LOCK(g_mx);\
		if (!inited) {\
			expr;\
			inited = true;\
		}\
	}

#define ACAPELLA_STATIC_INIT_SERVICE(expr) \
	static bool inited=false;\
	if (!inited) {\
		static boost_mutex ACAPELLA_MUTEX_DEFINE(g_mx, locklevel_service2);\
		BOOST_MUTEX_SCOPED_LOCK(g_mx);\
		if (!inited) {\
			expr;\
			inited = true;\
		}\
	}


	/**
	* A class template for simple class locking.
	* This wraps an existing class and locks separately all method calls 
	* by providing a proxy. Example usage:
	*	Lockable<std::map<int,int> > x;
	*	x.Locked()->push_back(123);
	*/
	template<typename T>
		class Lockable {
		public:
		/// A proxy object, holding the mutex locked while alive.
		class Lck {
		public:
			Lck(Lockable<T>& t): t_(t), lk_(new boost::mutex::scoped_lock(t.mx_)) {}
			Lck(const Lck& b): t_(b.t_), lk_(b.lk_) {b.lk_=NULL;}
			~Lck() {delete lk_;}
			T* operator->() {return &(t_.t_);}
			T& operator*() {return t_.t_;}
		private:
			void operator=(const Lck&); // not implemented
			Lockable<T>& t_;
			mutable boost::mutex::scoped_lock* lk_;
		};
		/// Returns a proxy object which keeps the mutex locked while alive.
		typename Lockable<T>::Lck Locked() {
			return typename Lockable<T>::Lck(*this);
		}
		private: // data
		friend class Lck;
		T t_;
		boost::mutex mx_;
	};

#ifdef _MSC_VER
	typedef iptr_t socket_handle_t;
#	pragma warning(push)
#	pragma warning(disable:4251)
#else
	typedef int socket_handle_t;
#endif

	/**
	* A socket base class for abstracting a TCP socket (IPv4 or IPv6).
	* This class implements some common behaviour for derived classes (ClientSocket, ServerSocket and AcceptedSocket).
	* A SocketBase object wraps a OS level socket. The OS socket ownership is passed over to another
	* object in case of copying and assignments, in std::auto_ptr fashion. Thus the SocketBase and derived 
	* classes are not strictly copy-constructible and are not suitable for holding in standard STL containers.
	*
	* The SocketBase class is not usable by itself; only derived classes should be instantiated.
	*/
	class DI_baseutil SocketBase {
	public:
		/// Default constructor, constcuts an unusable (INVALID_SOCKET) socket. On Windows, WSAStartup() for WSA version 2.0 is also called for the first constructor call.
		SocketBase();

	public:

		/// The destructor, closes the socket unless the socket ownership has passed over to another object.
		~SocketBase();

		/// Closes the socket prematurely.
		void Close();

		/**
		* Append incoming data from the socket to the buffer.
		* In case of any network errors a Nbaseutil::Exception will be thrown.
		* The function will return normally in case of following situations:
		*     * the peer has gracefully closed the connection;
		*     * if expected_buffer_length>0: the buffer size reaches expected_buffer_length;
		*     * if expected_buffer_length<=0: the buffer size reaches or exceeds maxlen parameter value, or there is currently no more data available.
		*
		* @param buffer The read data is appended to the buffer string. The previous content, if any, is not deleted.
		* @param maxlen Maximum allowed length of the buffer parameter. This parameter is ignored if 
		*			expected_buffer_length>0. The Read() function will return at the first opportunity when 
		*			the buffer grows equal or larger to the maxlen value. The buffer is guaranteed to not grow significantly 
		*			larger than the maxlen value. In case of graceful connection close the buffer may remain shorter.
		* @param expected_buffer_length If this is greater than zero, the function attempts to read in the
		*			specified amount of data so that the total buffer length would equal expected_buffer_length.
		*			The maxlen parameter is ignored in this regime.
		*			In case of an error an exception is thrown. 
		*			The expected_buffer_length parameter should be used when reading the HTTP packet body with known Content-Length.
		*/
		void Read(Nbaseutil::safestring& buffer, int maxlen, int expected_buffer_length=0);

		/// A structure for holding information about HTTP headers
		struct HttpPacketInfo {
			/// The first line of the HTTP packet
			safestring startline_;
			/// Content type, e.g. "text/xml", without additional parameters.
			safestring content_type_;
			/// Typedef for the header fields container.
			typedef std::map<safestring,safestring> headers_t;
			/// Container of all header fields.
			headers_t headers_;

		public: // interface
			/// Parse the HTTP status code out of the first line of HTTP response.
			DI_baseutil int GetHttpStatusCode();
		};

		typedef std::map<safestring,safestring> postdata_t;

		/** 
		* Sends an HTTP request to the socket. This does only write to the socket, nothing is read.
		* @param url URL to query. This must start with "http://". The Host field will be extracted from here. 
		* @param method HTTP query method ("GET" or "POST").
		* @param keep_alive Insert either "Connection: Keep-Alive" or "Connection: close" header in the HTTP request.
		* @param content_type HTTP content type (empty if no content).
		* @param content HTTP request body (empty if no content).
		* @param additional_headers Additional HTTP headers to include in the packet. If present, it must end with "\r\n".
		*/
		void SendHttpRequest(
			const safestring& url, 
			const safestring& method="GET", 
			bool keep_alive=true, 
			const safestring& content_type="", 
			const safestring& content="",
			const safestring& additional_headers=""
		);

		/**
		* Reads a full HTTP packet from the socket. 
		* If in Content-Type field there is a charset notification present, 
		* the packet body is converted to UTF-8 from the indicated charset.
		* This function only reads this socket. For HTTP redirection support use the standalone Nbaseutil::ReadHttpPacket() function.
		* @param packetinfo The structure is filled up with the information from HTTP packet startline and headers.
		* @param buffer The packet body (not headers) is appended to the buffer string. The previous content, if any, is not deleted.
		* @param maxlen Maximum allowed length of the total packet. If there is more data available, a Nbaseutil::Exception will be thrown.
		*/
		void ReadHttpPacket(HttpPacketInfo& packetinfo, safestring& buffer, int maxlen);

		/**
		* Reads HTTP packet headers from the socket. This function should be used for reading the response to a HEAD request.
		* This function only reads this socket.
		* @param packetinfo The structure is filled up with the information from HTTP packet startline and headers.
		*/
		void ReadHttpHeaders(HttpPacketInfo& packetinfo);

		/// Send the buffer content to the socket, starting from position pos. Throws ConnectionClosedException in case of errors.
		void Write(const Nbaseutil::safestring& buffer, int pos=0);

		/// Send the buffer content to the socket. Throws ConnectionClosedException in case of errors.
		void Write(const void* buffer, size_t bufferlength);

		/// Implements std::auto_ptr-like ownership-moving copy. The b object is handled as closed after that point and is not usable after that point.
		SocketBase(const SocketBase& b);

		/**
		* Implements std::auto_ptr-like ownership-moving assignment. 
		* This object's OS socket will be closed and the ownership of b's OS socket transferred over to this object.
		* Afterwards, the b object is handled as closed and will be unusable for socket operations.
		*/
		void operator=(const SocketBase& b);

		/// Returns true if the constructed socket is valid (not INVALID_SOCKET). 
		bool IsValid() const;

		/// Returns true if the constructed socket is an IPv6 socket. If the socket is not valid, the return value is meaningless.
		bool IsIPv6() const {return ipv6_;}

	protected: // implementation
		/// Init() must be called by the derived class in order to create a valid socket.
		void Init(bool ipv6);

	private: // implementation
		void ReadHttpConnection(HttpPacketInfo& packetinfo, safestring& buffer, int maxlen, bool headers_only);

	protected: // data
		socket_handle_t socket_;
		bool ipv6_;
	protected: // implementation
		/// Construct the object by the OS socket handle. The object assumes the ownership of the socket.
		SocketBase(socket_handle_t socket, bool ipv6);
	};

	class ServerSocket;

	/// A class for sockets returned from ServerSocket::Accept() function. This socket is used for all communication with the client.
	class DI_baseutil AcceptedSocket: public SocketBase {
		typedef SocketBase super;
	public:
		/// Return client machine IP address in the usual dot notation. This info comes from the accept() system call.
		safestring GetClientIP() const {return client_ip_;}
		/// Create a closed socket. This is only for the case when later a proper accepted socket is assigned to the object.
		AcceptedSocket();
	private:
		AcceptedSocket(socket_handle_t socket, const safestring& client_ip, bool ipv6);
		friend class ServerSocket;
		safestring client_ip_;
	};

	/// A possible port range class for using with the corresponding ServerSocket::Init() overload ctor.
	class DI_baseutil PortRange {
	public:
		/// Init with a spec like "8000-8010,8025,8027". Overlapping and empty ranges are normalized/ignored automatically. If the spec is not parsable, an exception is thrown and the port range remains empty.
		PortRange(const safestring& range_spec);
		/// Init with a spec like "8000-8010,8025,8027". Overlapping and empty ranges are normalized/ignored automatically. If the spec is not parsable, an exception is thrown and the current port range remains unchanged.
		void Init(const safestring& range_spec);
		/// Return normalized range specification.
		safestring GetRangeSpec() const;
		/// Return next valid port in range after prev_port. If prev_port too large, wraps over to the range start. If range is empty, an exception is thrown.
		int GetNextPortAfter(int prev_port) const;
		/// Check if port belongs to the range
		bool Contains(int port) const;
	private:
		typedef std::map<int, int> range_t;
		static void AddInterval(int a, int b, range_t& r);
		static void Normalize(range_t& r);
	private:
		range_t range_;
	};


	/// A portable network socket class for implementing server-side sockets.
	class DI_baseutil ServerSocket: public SocketBase {
		typedef SocketBase super;
	public: // interface

		/**
		* Construct a TCP stream socket and bind to the specified port for listening for incoming connections.
		* @param port The TCP port number. If this is 0, a dummy invalid socket is created and the rest of parameters are ignored.
		* @param ipv6 Create a dual-stack socket for accepting both IPv4 and IPv6 incoming connections. This parameter has no effect if the computer does not support IPv6 protocol.
		* @param loopback_only Listen only to the loopback interface. Depending on the created socket type, this is either "127.0.0.1" or "::1". 
		*						It is not possible to create a socket listening to both and only IPv4 and IPv6 loopback interfaces at the same time.
		*						If this parameter is false, the socket listens to all interfaces.
		*/
		ServerSocket(unsigned short port, bool ipv6, bool loopback_only);

		/// Construct a TCP stream socket and bind to the first free port in range [firstport, lastport] for listening for incoming connections. See the other constructor for other parameter descriptions.
		ServerSocket(unsigned short firstport, unsigned short lastport, bool ipv6, bool loopback_only);

		/**
		* Wait for incoming connection and return a socket for serving the connection.
		* If terminating_flag becomes true during the wait, throws AcceptHasBeenTerminatedException exception at first chance.
		* The program execution is paused while in Accept(), so make sure to not call this function in a GUI update thread.
		*/
		AcceptedSocket Accept(); 

		/**
		* This function can be called from any thread.
		* Sends an abort signal to a possible ongoing Accept() call in another thread. 
		* The Accept() will throw AcceptHasBeenTerminatedException exception in another thread ASAP.
		* The socket will be closed and unusable after that call.
		*/
		void Abort();

		/// A special exception class; will be thrown from the Accept() call if Abort() has been called meanwhile from another thread.
		class AcceptHasBeenTerminatedException: public std::exception {};

		/// Return the port number the ServerSocket is listening on.
		unsigned short GetPort() const {return port_;}

		/// After construction by ServerSocket(0, 0) Init() should be called for opening the socket.
		void Init(unsigned short firstport, unsigned short lastport, bool ipv6, bool loopback_only);

		/// Init the socket and bind/listen to the first free port in the passed range, starting from first_port (this may be out of the specified range, e.g. 0).
		void Init(const PortRange& range, unsigned short first_port, bool ipv6, bool loopback_only);

	private: // implementation
		bool Init(unsigned short port, bool ipv6, bool loopback_only);
		unsigned short port_;

		boost_mutex aborted_mx_;
		unsigned long aborted_; 
		int abort_pipe_[2];
	};

	/// A client socket
	class DI_baseutil ClientSocket: public SocketBase {
		typedef SocketBase super;
	public: // interface
		ClientSocket() {}
		ClientSocket(const Nbaseutil::safestring& server, int port);
		ClientSocket(const Nbaseutil::safestring& url);
	private: // implementation
		void Init(const Nbaseutil::safestring& server, int port);
		bool Init(const Nbaseutil::safestring& server, int port, Nbaseutil::safestring& error_msg);
		bool InitLocalHost(int port);
	};

	/// This will be thrown if socket has been closed by the client before serving the request.
	class DI_baseutil ConnectionClosedException: public Nbaseutil::Exception {
	public:
		ConnectionClosedException();
	};

	/**
	* Read an HTTP packet from the specified URL. This function supports HTTP redirection.
	* If server returns an HTTP error, an exception is thrown.
	* @param url The URL to read.
	* @param packetinfo The structure is filled up with the information from HTTP packet startline and headers.
	* @param buffer The packet body (not headers) is appended to the buffer string. The previous content, if any, is not deleted.
	* @param maxlen Maximum allowed length of the total packet. If there is more data available, a Nbaseutil::Exception will be thrown.
	*/
	DI_baseutil void ReadHttpPacket(
		const safestring& url, 
		SocketBase::HttpPacketInfo& packetinfo, 
		safestring& buffer, 
		int maxlen);

	/**
	* Return the IP address of the first network interface of the current machine, in the dotted decimal or IPv6-colon notation. 
	* If there is an environment variable ACAPELLA_HOSTNAME present, its value is returned instead. 
	* In case of errors or if there are no network interfaces, "localhost" is returned.
	* See also ExecutionContext::GetHost() in the IMacro project.
	* Note that this name may not be suitable for contacting the Acapella server, use HttpServer::GetUrlPrefix("?") instead.
	*/
	DI_baseutil safestring GetDefaultHostName();

	/**
	* It seems that popen() calls are not really thread-safe and can lock up with themselves and with the malloc subsystem,
	* at least in glibc 2.11.1 (see fogbugz #36603). Use a global mutex around popen() calls for any case (don't know if this actually helps or is needed).
	*/
	DI_baseutil boost_mutex& GlobalPopenMutex();

} // namespace

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
