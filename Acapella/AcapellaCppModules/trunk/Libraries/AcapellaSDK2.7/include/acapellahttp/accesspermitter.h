#ifndef x_PKI_CTG_ACAPELLA_ACAPELLAHTTP_ACCESSPERMITTER_H_5B8C6A9BFD0B354EB84E536C59992B89
#define x_PKI_CTG_ACAPELLA_ACAPELLAHTTP_ACCESSPERMITTER_H_5B8C6A9BFD0B354EB84E536C59992B89

namespace NAcapellaHttp {

	class AccessPermitter;
	typedef NIMacro::RefcountedPointer<AccessPermitter> PAccessPermitter;
	struct Request;
	class HtmlCmdHandler;

	class AccessPermitter: public NIMacro::ThreadSafeRefcountable {
	public: // virtual interface

		/**
		* Checks if the incoming request is allowed to proceed. If yes,
		* the it should return true. If not, then it should throw an
		* exception or set the output parameter \a location to a redirect URL
		* and return false.
		*/
		virtual bool CheckAccess(const Request& request, HtmlCmdHandler& handler, Nbaseutil::safestring& location)=0;
			
		/// Return a short description of the access permitter object. This is used in log messages.
		virtual Nbaseutil::safestring GetDescription()=0;
	};

} // namespace
#endif
