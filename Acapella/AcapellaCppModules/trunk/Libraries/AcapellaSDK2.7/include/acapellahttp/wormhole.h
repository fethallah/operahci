#ifndef x_ACAPELLA_ACAPELLAHTTP_WORMHOLE_H_INCLUDED
#define x_ACAPELLA_ACAPELLAHTTP_WORMHOLE_H_INCLUDED

#include <boost/thread/condition.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
#include "acapellahttp.h"

namespace NAcapellaHttp {

	class WormHoleImpl;
	typedef NIMacro::RefcountedPointer<WormHoleImpl> PWormHoleImpl;
	class WormHoleWaiter;
	class WormHole;
	typedef NIMacro::DerivedPointer<WormHole> PWormHole;
	class RemoteWormHoleImpl;
	class WormHoleRSVP;
	using Nbaseutil::boost_mutex;
	using Nbaseutil::boost_recursive_mutex;
	using Nbaseutil::boost_condition;
	using NIMacro::PContainer;

	/// A constant for indicating infinite wait (this is std::numeric_limits<int>::max(), which is not used directly because of the conflict with Windows max macro).
	static const int c_wait_forever = 2147483647;

#ifdef ACAPELLA_PTR64
	// an unsigned int type which is updated atomically in MT environment on the platforms we are supporting
	typedef Nbaseutil::uint64 atomic_uint_t;
#else
	typedef unsigned int atomic_uint_t;
#endif

	/// A type for ordering wormhole events
	typedef atomic_uint_t wormhole_event_seq_t;

	/**
	* An opaque class for exchanging data between different Acapella scripts.
	* This is actually only an interface class, the real wormhole is implemented in the WormHoleImpl class.
	* For each wormhole there is exactly one WormHoleImpl object. This can be referred to by any
	* number of WormHole objects. If the WormHole object lives in the same process, it refers to the WormHoleImpl
	* object by an embedded smartpointer. If it lives in another process or on another machine, it refers to the WormHoleImpl
	* by an URL. The WormHole class abstracts the actual source of data from the script.
	*/
	class DI_AcapellaHttp WormHole: public NIMacro::MemBlock {
		typedef NIMacro::MemBlock super;
	public: // static interface

		/// Create a new wormhole living in the same process.
		static PWormHole Create();

		/// Create a wormhole interface object for accessing the wormhole residing at the specified URL.
		static PWormHole Create(const Nbaseutil::safestring& wormholeurl);

		/**
		* Create a wormhole on the remote server and return an interface object for accessing it.
		* Throws an exception if the wormhole cannot be created.
		* @param server_url_prefix Server location, in format "http://caneland:8282".
		* @return Wormhole interface object. 
		*/
		static PWormHole CreateOnServer(const Nbaseutil::safestring& server_url_prefix);

	public: // interface

		/// Bitflags for composing the sendoptions parameter for various Send* member functions.
		enum SendOptions_t {
			/// Default value (no flags raised).
			send_default = 0,
			/// Do not throw exceptions inside the function (sender aborted, wormhole closed, etc), just ignore the error conditions and send the event in any case.
			send_nothrow = 1<<0,
			/// Delete any existing no-wait events with the same name from the queue.
			send_discard_old = 1<<1,
			/// Do not attempt to make a deep copy of the passed container. This should be used only if the caller knows that there are no external references to the container innards in the calling context.
			send_skip_deep_copy = 1<<2,
			/// When making a deep copy of the passed container, log the container subnodes actually copied. The log can be accessed later by the GetAndClearLog() member function.
			send_log_deep_copy = 1<<3,
		};

		/**
		* Sends data into the wormhole. The data is passed in the form of distinct events.
		* @param eventname Name of the event. This may be an arbitrary string except "default".
		* @param eventdata The data container. The parameter will be NULL after the call. A deep copy of the container is done for parts referenced outside of the container, unless send_skip_deep_copy option is passed.
		* @param send_options A flagmask of options controlling the send process, composed of SendOptions_t bitflags.
		* @return True if the event was put in the queue, false if the wormhole has been closed.
		*/
		bool Send(const Nbaseutil::safestring& eventname, NIMacro::PContainer& eventdata, SendOptions_t send_options = send_default);

		/**
		* Sends data into the wormhole and waits for the answer. The data is passed in the form of distinct events.
		* @param eventname Name of the event. This may be an arbitrary string except "default".
		* @param eventdata The data container. The parameter will be NULL after the call. A deep copy of the container is done for parts having refcounts>1 (unless send_skip_deep_copy option is specified).
		* @param timeout_in_s The wait timeout, in seconds. If timeout occurs, NULL pointer is returned. The default is to wait indefinitely.
		* @param send_options A flagmask of options controlling the send process, composed of SendOptions_t bitflags.
		* @return The answer as prepared by the event reader.
		*/
		NIMacro::PContainer SendAndWait(const Nbaseutil::safestring& eventname, NIMacro::PContainer& eventdata, int timeout_in_s=c_wait_forever, SendOptions_t send_options = send_default);

		/**
		* Sends a response to an event requiring a response (SendAndWait() has been called in another thread). If the wormhole is not waiting for a response, an exception occurs.
		* @param eventdata The data container. Passing NULL means response failure. The parameter will be NULL after the call. A deep copy of the container is done for parts having refcounts>1 (unless send_skip_deep_copy option is specified).
		* @param send_options A flagmask of options controlling the send process, composed of SendOptions_t bitflags.
		*/
		void SendRSVP(NIMacro::PContainer& eventdata, SendOptions_t send_options = send_default);

		/**
		* This schedules an event to be posted in the queue after 
		* another event with the specified name has arrived. 
		* If the currently processed event requires a response, it is put on hold.
		* The response will not be returned, and the calling thread will be waiting.
		* If the postponed event will be processed finally later, the response will
		* be returned to the waiting thread. This can easily cause deadlocks, use with 
		* care!
		* @param eventname Name of the event to postpone. This may be an arbitrary string except "default".
		* @param eventdata Data for the postponed event. The parameter will be NULL after the call. A deep copy of the container is done for parts having refcounts>1 (unless send_skip_deep_copy option is specified).
		* @param until_event Name of another event to wait for. If empty, any incoming event qualifies.
		*/
		void PostPone(const Nbaseutil::safestring& eventname, NIMacro::PContainer& eventdata, const Nbaseutil::safestring& until_event="");

		/**
		* Request data from the wormhole. This function is primarily called from the WormHoleWaiter class.
		* If there is no data in the wormhole, the function returns false immediately.
		* @param eventname Variable for receiving the event name.
		* @param eventdata Variable for receiving the event data.
		* @param rsvp_required The event sender is waiting for a response. The caller should call SendRSVP() before calling Receive() next time. If it fails to do so, a wait failure appears in the sender.
		* @return True if an event was read from the wormhole. False if there is no events in the queue. In the latter case response_required parameter will be false if wormhole has been closed, and true otherwise.
		*/
		bool Receive(Nbaseutil::safestring& eventname, NIMacro::PContainer& eventdata, bool& rsvp_required);

		/// Return an URL which can be passed to the Create(safestring) function in another process or machine for contacting this wormhole.
		Nbaseutil::safestring GetUrl() const;

		/**
		* Schedules an exception to be thrown when any thread calls Send() next time.
		* This function returns immediately. An "__aborted" event will be posted
		* in the wormhole each time when an exception is thrown.
		* @param errorcode Exception error code.
		* @param message Exception message.
		* @param once_only If true, the exception is cleared after the first exception is thrown. If false, it will rethrown each time when Send() is called.
		*/
		void AbortSender(unsigned int errorcode, const Nbaseutil::safestring& message, bool once_only=false);


		/**
		* Check if somebody has called AbortSender(). If yes, an exception is thrown inside this function.
		* If the "sender" thread does not need to call Send() for prolonged periods,
		* it should call CheckAbortion() periodically instead.
		* Avoid calling CheckAbortion() from a closed loop with no external function calls,
		* the needed checking might be optimized out by the compiler.
		*/
		void CheckAbortion();

		/**
		* Close the wormhole. A closed wormhole does not accept events any more.
		* The wormhole readers can check for the closedness flag after Receive() and abandon the read.
		* Instead of calling Close() one can send an event "__closed" to the wormhole.
		*/
		void Close() {
			NIMacro::PContainer dummy;
			Send("__closed", dummy, send_nothrow);
		}

		struct status_t {
			int eventcount_;
			bool haserror_, isclosed_;
			unsigned int errorcode_;
			Nbaseutil::safestring errormessage_;
			status_t(): eventcount_(0), haserror_(false), isclosed_(false), errorcode_(0) {}
		};

		status_t GetStatus() const;

		void GetAndClearLog(Nbaseutil::safestring& buffer);

	public: // virtual overrides
		virtual const char* Class() const {return "wormhole";}
		virtual bool Conforms(const char *szClassName) const;
		virtual bool Entertain(NIMacro::MemBlockVisitor& visitor, const Nbaseutil::safestring& name, entertainmode_t mode=enter_deep);
		virtual void IterateChildren(NIMacro::AcaVisitor& visitor, const NIMacro::TraverseNode& node);
		virtual bool AddConvertArg(const Nbaseutil::safestring& option, const NIMacro::DataItem& value, NIMacro::ConvertOptions& opt, Nbaseutil::ThrowHandlerFunc throwhandler=NIMacro::LogWarnings()) const;
		virtual bool Consistent(Nbaseutil::safestring& msg, bool check_content=true) const;
		virtual const char* ClassDescription() const;
		virtual NIMacro::PMemBlock DoClone(copymode_t copy_mode) const;
		virtual bool DoSetSubItem(const Nbaseutil::safestring& subitemname, const NIMacro::SafeValue& item, const Nbaseutil::safestring& fullpathname, Nbaseutil::ThrowHandlerFunc throwhandler=Nbaseutil::ThrowIndeed);
		virtual Nbaseutil::safestring DoGetDescription() const;
		virtual NIMacro::SafeValue ResolveSubItem(const Nbaseutil::safestring& subitemname, Nbaseutil::ThrowHandlerFunc throwhandler=Nbaseutil::ThrowIndeed) const;
		virtual bool Equals(const MemBlock& b) const; 

	private: // friend interface
		friend class WormHoleWaiter;
		wormhole_event_seq_t Register(WormHoleWaiter& waiter, int slot_index);
		void Unregister();

	private: // implementation
		void Init(const Nbaseutil::safestring& wormholeurl);
		WormHole();
		WormHole(const WormHole& b);
		WormHole(const Nbaseutil::safestring& url);
		~WormHole();
		void operator=(const WormHole& b); // unimplemented

	private: // data
		/// If the wormhole implementation lives in the same process, then this is a smartpointer to it.
		PWormHoleImpl wormholeimpl_;

		/// If the wormhole implementation lives in another process, this is the implementation object communicating with it.
		RemoteWormHoleImpl* remoteimpl_;

		/// If a packet is read through Receive which requires response, this flag will be set until a response is returned.
		bool waiting_for_rsvp_;

		/// If the wormhole implementation lives in a different process, then this is the URL for contacting it.
		mutable Nbaseutil::safestring wormholeurl_;
	};

	DI_AcapellaHttp WormHole::SendOptions_t operator|(WormHole::SendOptions_t a, WormHole::SendOptions_t b);

	/**
	* An helper class for waiting data from one or more wormholes. This is typically a short-lived class.
	* Objects of this class cannot be passed to other threads. While the WormHoleWaiter object is alive,
	* it owns the corresponding wormholes - the wormholes cannot be passed to another WormHoleWaiter object at
	* the same time.
	*/
	class DI_AcapellaHttp WormHoleWaiter {
	public:
		/**
		* Construct the WormHoleWaiter object for waiting data from the specified wormholes.
		* If any wormhole is already owned by another WormHoleWaiter object, an ERR_IO Exception is thrown.
		* @param wormholes A vector of wormholes. The parameter may be not NULL. The Vector may contain only wormhole objects.
		*/
		WormHoleWaiter(NIMacro::PVector wormholes);

		/// Destructor
		~WormHoleWaiter();

		/**
		* Wait for data from wormholes. If Abort_mt() is called while waiting, Wait() will be exited by an Exception.
		* @param eventname Variable for receiving the event name.
		* @param eventdata Variable for receiving the event data.
		* @param wormhole_index Variable for receiving the index of the wormhole from where the event arrived.
		* @param timeout_in_s Timeout in seconds. Default is to wait indefinetly.
		* @param rsvp_required The event sender is waiting for a response. The caller should call SendRSVP() before calling Wait() next time. If it fails to do so, a wait failure appears in the sender.
		* @return If timeout occurs, false is returned, otherwise true.
		*/
		bool Wait(Nbaseutil::safestring& eventname, NIMacro::PContainer& eventdata, int& wormhole_index, bool& rsvp_required, int timeout_in_s=c_wait_forever);


		/// Aborts a pausing Wait() call. To be called from other threads then Wait().
		void Abort_mt(const Nbaseutil::safestring& message = "Script aborted", unsigned int errorcode = Nbaseutil::ERR_USER_INTERVENTION);

	private: // implementation
		void Register();
		void Unregister(int n);
		void CheckIfAborted_lk();
	private: // friend interface
		friend class WormHoleImpl;
		/// Called from WormHoleImpl when new data arrives into wormhole. Can be called from any thread.
		void Notify_mt(int slotindex);
	private: // data
		NIMacro::PVector wormholes_;

		// waiter_mx_ protects signalled_slot_ and is used together with waiter_cnd_
		boost_mutex waiter_mx_;
		boost_condition waiter_cnd_;
		/**
		* If at the time of waiting a new event arrives and the waiter gets notified, 
		* this contains the index of the corresponding wormhole so that the waiter does not have
		* to cycle through the wormholes to find the right one. This is raised only if all wormholes are empty.
		*/ 
		int signalled_slot_;
		/// This is used for determining if it is ok to raise the signalled_slot_ flag
		bool all_wormholes_empty_;

		Nbaseutil::safestring abortmessage_;
		unsigned int abort_errorcode_;
	};

	const int c_infinite_inline_threshold = 2147483647;

	DI_AcapellaHttp	void SetInlineThreshold(int inline_threshold);

	/**
	* Serialize a container in special Acapella event XML format.
	* @param c The container to serialize. This will be set to NULL by the function. For improving performance, do not keep around other smartpointers to the container or its internals, if possible.
	* @param inline_threshold Binary vectors above the threshold will be registered by the in-process HTTP server 
	*					and their URL-s encoded in the XML. The client is expected to download the URL-s when deserializing the XML.
	*					Naturally, this is an error-prone approach. Pass c_infinite_inline_threshold for avoiding this feature.
	* @param transfer_encoding Binary vectors below the threshold are serialized inside the XML, according to the transfer_encoding schema.
	*				Possible values: "base64", "zlib base64", "binary", "zlib". The first two methods place the base64-encoded content 
	*					as a text inside the corresponding XML node. The last two nodes place the binary data in the end of the XML file,
	*					after a terminating zero byte marking the end of the proper XML part. In all cases the binary data
	*					is encoded in little-endian format compatible with Intel architectures.
	* @param xml The output parameter. The serialized content will be appended to this string.
	*					If transfer_encoding is "zlib" or "binary", then the output contains a binary part after XML,
	*					separated by a single zero byte. This format is called XML+binary.
	* @param hostname The hostname of this computer as visible to the client deserializing the XML. This is important only 
	*					if binary vectors are not inlined and will be fetched later by URL-s, see the inline_threshold parameter. 
	*					Pass an empty string to use the default hostname.
	*/
	DI_AcapellaHttp	void SerializeEventXml(NMemBlock::PContainer& c, int inline_threshold, const Nbaseutil::safestring& transfer_encoding, Nbaseutil::safestring& xml, const Nbaseutil::safestring& hostname);

	/**
	* Unserialize an XML file created by SerializeEventXml().
	* @param xml The XML or XML+binary file.
	* @return The unserialized container.
	*/
	DI_AcapellaHttp	NMemBlock::PContainer UnserializeEventXml(const Nbaseutil::safestring& xml);

	/**
	* Unserialize an XML file created by SerializeEventXml().
	* @param xml The XML or XML+binary file.
	* @return The unserialized container.
	*/
	DI_AcapellaHttp	NMemBlock::PContainer UnserializeEventXml(const Nbaseutil::Carrier& xml);

	/**
	* Unserialize an XML file created by SerializeEventXml(). This function does not work for XML+binary format files (will throw an exception).
	* @param eventxml_node The root node (EventXml).
	* @return The unserialized container.
	*/
	DI_AcapellaHttp	NMemBlock::PContainer UnserializeEventXml(const TiXmlElement* eventxml_node);

	/// Converts an XML subtree in Acapella event xml format to a dataitem. This function does not work for XML+binary format files (will throw an exception).
	DI_AcapellaHttp	void Xml2DataItem(const TiXmlElement* node, const Nbaseutil::safestring& vdataurl, NIMacro::DataItem& item);

	/// A class for transporting external referencing errors out of this library. See Wormhole::SendEvent() module.
	class DI_AcapellaHttp ExcExternalRef: public std::exception {
		typedef std::exception super;
	public:
		ExcExternalRef(const Nbaseutil::safestring& name, const NMemBlock::MemBlock* ptr, const NMemBlock::MemBlock* root);
		virtual ~ExcExternalRef() throw() {}
		virtual const char* what() const throw();
	public:
		const Nbaseutil::safestring name_;
		const NMemBlock::MemBlock* const ptr_;
		const NMemBlock::MemBlock* const root_;
	};

} // namespace

#endif
