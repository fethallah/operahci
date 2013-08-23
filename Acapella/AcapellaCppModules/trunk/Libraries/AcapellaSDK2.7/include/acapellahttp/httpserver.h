#ifndef xACAPELLA_HTTP_H__3999F923_903C_11D7_B8C2_006097965996__INCLUDED_
#define xACAPELLA_HTTP_H__3999F923_903C_11D7_B8C2_006097965996__INCLUDED_

#include <time.h>
#include <map>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "acapellahttp.h"

namespace NIMacro {
	class DataItem;
}

class TiXmlElement;
class TiXmlDocument;

namespace NAcapellaHttp {

class HttpServer;
struct Request;
class HtmlCmdHandlerImpl;
class RequestQueue;
using Nbaseutil::boost_mutex;
using Nbaseutil::boost_recursive_mutex;
using Nbaseutil::boost_condition;
class AccessPermitter;
typedef NIMacro::RefcountedPointer<AccessPermitter> PAccessPermitter;
class HtmlCmdHandler;
typedef NIMacro::RefcountedPointer<HtmlCmdHandler> PHtmlCmdHandler;

/**
* An abstract base class for implementig handlers for serving requests arriving to HttpServer object.
* An handler object is registered by the server via the
* HttpServer::RegisterHandler() function. The server will dispatch
* the incoming requests to handlers.
*
* The derived HtmlCmdHandler class must override the GetContent()
* member function in order to supply the proper content. For more
* advanced use the class may override also other virtual functions.
*
* The handlers are working in single-threaded mode. The server takes care of
* launching needed number of working threads and syncronizing them.
* The handler is called in some
* worker thread, but always not from more than one thread at a time, except of
* the CanServe() and Abort() member functions, which may be called any time from any thread.
*
* The server-handler interaction sequence by servicing the request looks like follows.
* On almost each step the handler may redirect the request to somewhere else or to the previous page,
* in this case some steps are skipped as described below.
*	-# Server calls HtmlCmdHandler::CanServe(). This function is not called on redirect handler.
*	-# Server calls HtmlCmdHandler::BeginTransaction().
*   -# If the handler returns a redirect handler pointer, then server composes the packet with the redirect handler and continues with EndTransaction() on this handler.
*	-# If there were any POST parameters in the request, server calls HtmlCmdHandler::Post(). This is not called for GET parameters.
*	-# Server calls HtmlCmdHandler::GetContent().
*	-# If the response does begin with "HTTP/1.1", then the server assumes this is the complete packet, processing finished.
*	-# Otherwise, if the response does not begin with "HTTP/1.1", then:
*		-# Server calls HtmlCmdHandler::ComposeHttpHeader() and uses the response for return packet headers.
*		-# If the response from GetContent() contains HTML tag, then server assumes this is the whole HTML page, processing finished.
*		-# Otherwise, if the response from GetContent() does not contain HTML tag, then:
*			-# Server calls HtmlCmdHandler::ComposeHtmlPage() and uses the result for HTML page part of the return packet.
*			-# If the response from ComposeHtmlPage() contains a <tt>&amp;%b;</tt> placeholder, then server calls HtmlCmdHandler::ComposeHtmlBody() and replaces the placeholder with the response.
*			-# If the resulting text contains a <tt>&amp;%c;</tt> placeholder, then server replaces the placeholder with the response from the earlier GetContent() call.
*	-# Server calls HtmlCmdHandler::EndTransaction().
*	-# Server calls HtmlCmdHandler::NotifyPacketComplete(). This function is not called on redirect handler.
*	-# Server sends the response packet back to the client.
*	-# Server calls HtmlCmdHandler::EndServe(). This function is not called on redirect handler.
*
* Thus, the handler has several means for providing its own content. The main function to override is GetContent(), other function overrides are needed in advanced scenarious.
*/
class DI_AcapellaHttp HtmlCmdHandler: public NIMacro::ThreadSafeRefcountable {
public:
	HtmlCmdHandler();
	virtual ~HtmlCmdHandler();

	/**
	* Return the reference to the HttpServer object this handler is attached to.
	* It is not allowed to call this member function before attaching the handler to the server by HttpServer::RegisterHandler().
	*/
	HttpServer& Server() const;

	/**
	* Return the URL prefix part of the server as seen by the client, e.g. "http://altair.perkinelmer.net:8283".
	* This function can be used only during servicing a request.
	*/
	const Nbaseutil::safestring& GetUrlPrefix() const {return url_prefix_;}

	/**
	* This function will be called by the server as the first step in servicing the request.
	* The request servicing is later terminated by an EndTransaction() call.
	* The base class implementation does nothing. Override to prepare the handler for servicing the request, if needed.
	*
	* If the handler wants to redirect the task to another handler, then it can return a pointer
	* to another handler. The server composes the response packet immediately by the help of redirect handler.
	* The redirect handler must not be a regular HtmlCmdHandler registered by the server
	* as this could break the thread scheduling mechanism. Instead, this handler must create and maintain the redirect
	* handler by its own.
	* The NotifyPacketComplete() function is called for this handler and not for the 'other' handler.
	*
	* @param arg The URL tail part after the handler name, without starting slash (any GET-style parameters are stripped off).
	* @param request A reference to the original Request structure.
	* @return NULL or redirect handler pointer. The redirect handler must be valid/alive at least until EndTransaction() is called on this object.
	*/
	virtual PHtmlCmdHandler BeginTransaction(const Nbaseutil::safestring& arg, const Request& request) {return NULL;}

	/**
	* This is called by the server exactly once per each BeginTransaction() call, also in case of failures.
	* The base class implementation does nothing.
	* Override to perform any cleanup of handler's data after servicing a request.
	* The EndTransaction() service may not leak exceptions to the caller because it may be called during stack unwinding caused by another exception.
	*
	* @param arg The URL tail part after the handler name, without starting slash (any GET-style parameters are stripped off).
	*/
	virtual void EndTransaction(const Nbaseutil::safestring& arg) {}

	/// A typedef for the container holding the POST and GET parameters sent in with the request.
	typedef std::map<Nbaseutil::safestring,Nbaseutil::safestring> postdata_t;

	/**
	* Called by the server before for POST actions; method may return an URL for redirecting the call, or "<back>" to display the previous page.
	* If an empty string is returned, the server continues with normal fashion by calling GetContent() etc.
	* The base class implementation does nothing and returns an empty string.
	*
	* This function is not called for GET requests. In order to access parameters contained in the GET request the handler
	* must call GetPostData() function later from GetContent() et. al. when composing the packet.
	*
	* @param arg The URL tail part after the handler name, without starting slash (any GET-style parameters are stripped off).
	* @param postdata The container holding POST and GET style parameters. The postdata argument can be cleared by the handler during Post() (typically the content is swapped over to a private location).
	*/
	virtual Nbaseutil::safestring Post(const Nbaseutil::safestring& arg, postdata_t& postdata) {return "";}

	/**
	* This is the main function to override. This can produce either:
	*    - a piece of HTML which should go inside &lt;BODY&gt; tags;
	*    - the whole body enclosed in &lt;BODY&gt; tags;
	*    - the whole HTML page enclosed in the &lt;HTML&gt; tags;
	*    - the whole server packet together with the HTTP headers.
	* If some parts are missing, other virtual functions will be called
	* to fill in the missing parts, see the HtmlCmdHandler class description for details.
	* This function is pure virtual, no base class implementation existing.
	*
	* @param arg The URL tail part after the handler name, without starting slash (any GET-style parameters are stripped off).
	* @param text The text buffer to fill in. It is empty in the beginning of the call.
	*/
	virtual void GetContent(const Nbaseutil::safestring& arg, Nbaseutil::safestring& text)=0;

	/**
	* Override to provide custom HTTP header lines.
	* Base class implementation creates headers for UTF-8 text/html output (response from DefaultHttpHeaderTextHtml() function).
	* This function is called only if the response from GetContent() does not begin with "HTTP/1.1".
	*
	* @param arg The URL tail part after the handler name, without starting slash (any GET-style parameters are stripped off).
	* @param text The text buffer to fill in. It will be empty in the beginning of the call.
	*/
	virtual void ComposeHttpHeader(const Nbaseutil::safestring& arg, Nbaseutil::safestring& text);

	/**
	* Override to supply a different HTML page structure, if needed.
	* This function is only called if the response from GetContent() does not begin with "HTTP/1.1" and does not contain &lt;HTML&gt; tags.
	* The returned text should contain &%b; placeholder to
	* include the body part of HTML together with the &lt;BODY&gt; tags.
	* The returned text can include &amp;%l; placeholder. The server will fill it in with the text returned by the HtmlCmdHandler::GetTitle() call.
	* The base class implementation will return a &lt;HTML&gt; page template having the following features:
	*   - /file/OnlineHelp.css included as the stylesheet.
	*   - Additional style for tables added to prevent wrapping of negative numbers between the sign and the number.
	*   - The &lt;TITLE&gt; tags containing the &amp;%l; placeholder.
	*   - The &amp;%b; placeholder in place of the HTML body part.
	* @param arg The URL tail part after the handler name, without starting slash (any GET-style parameters are stripped off).
	* @param text The text buffer to fill in. It will be empty in the beginning of the call.
	*/
	virtual void ComposeHtmlPage(const Nbaseutil::safestring& arg, Nbaseutil::safestring& text);

	/**
	* This function is only called by the server for filling in the &amp;%b; placeholder in the ComposeHtmlPage() response.
	* Override to supply the &lt;BODY&gt;..&lt;/BODY&gt; part of HTML page.
	* The returned text should containt &%c; placeholder to include the
	* GetContent() output.
	* The base class implementation returns <tt>"<body>&%c;</body>"</tt>.
	* @param arg The URL tail part after the handler name, without starting slash (any GET-style parameters are stripped off).
	* @param text The text buffer to fill in. It will be empty in the beginning of the call.
	*/
	virtual void ComposeHtmlBody(const Nbaseutil::safestring& arg, Nbaseutil::safestring& text);

	/**
	* Override to provide the proper title for HTML page. Return title string in UTF-8, without HTML quoting.
	* This function is only called by the server for filling in the &amp;%l; placeholder in the ComposeHtmlPage() response.
	* @param arg The URL tail part after the handler name, without starting slash (any GET-style parameters are stripped off).
	*/
	virtual Nbaseutil::safestring GetTitle(const Nbaseutil::safestring& arg) {return arg;}

	/**
	* Server will call this after forming complete response packet
	* This will be called only if there were no errors in preparing the packet.
	* If the handler has redirected the request by BeginTransaction() function, then NotifyPacketComplete() is called only
	* for the original handler, after the redirected-to handler has prepared the packet.
	*
	* The response packet will be the concatenation of headers and msg arguments. They are separately visible here only for some performance considerations.
	*
	* @param arg The URL tail part after the handler name, without starting slash (any GET-style parameters are stripped off).
	* @param headers First part of the HTTP response packet, either empty or containing HTTP packet headers, starting with "HTTP/1.1".
	* @param msg Second part of the HTTP response packet, either the whole packet or HTTP packet body part only.
	*/
	virtual void NotifyPacketComplete(const Nbaseutil::safestring& arg, const Nbaseutil::safestring& headers, const Nbaseutil::safestring& msg) {}

	/**
	* This function can be called from any thread. It is called by the server in order to abort any current request service ASAP.
	* If you override this function, be sure to call the base class version as well.
	* After calling this function the Aborted() function will return true.
	*/
	virtual void Abort();

	/**
	* Can be checked inside of handler to find out if the execution should be aborted immediately. Will return true if either HtmlCmdHandler::Abort()
	* has been called for this object, or HttpServer::Abort() has been called for the owner server.
	* If the service request may take long time, then the handler should periodically check this function and throw an exception to abort the execution.
	*/
	bool Aborted() const;

	/**
	* Before passing over the request to the handler, the server calls this function to check if the handler is ready to accept it at this moment.
	* The call is made from one of the server worker threads.
	* This may be another thread than the one the handler is currently servicing a request, if any,
	* The default implementation returns NULL if it is busy (currently handling another request),
	* otherwise it marks itself as busy and returns pointer to itself.
	* Advanced handlers can create temporary handler objects for serving the request in parallel.
	* If the handler returns non-NULL, then the server is quaranteed to call at least EndServe() so the handler can mark itself idle again.
	* @param request Reference to the incoming request structure.
	* @return Either NULL for indicating busy state, or a pointer to a handler that can serve the request (usually itself).
	*/
	virtual PHtmlCmdHandler CanServe(const Request& request);

	/// Is called eventually for each handler object returned from CanServe(). The base class implemenation marks itself non-busy.
	virtual void EndServe();

	/// Bitflags for encoding various properties the handler might have or support (in general or for the current transaction).
	enum handler_prop_t {
		/// Default property value (zero: no property flags raised).
		prop_default = 0,
		/// The handler works in HTTP Keep-Alive mode.
		prop_is_keepalive = 1<<0,
		/// The handler uses HTTP-quoted slashes in the URL tail and wants to unquote them by itself.
		prop_wants_urltail_quoted = 1<<1,
		/// The returned content might be PHP and should be run through the PHP if it contains "<?php" marker.
		prop_auto_php = 1<<2,
		/// The handler may block for long time when servicing the request
		prop_is_blocking = 1<<3,
	};

	/// Return the properties the handler supports for the current transaction.
	virtual handler_prop_t GetHandlerProp() {return prop_default;}

	/**
	* General backdoor for arbitrary extensions. The following verbs are defined:
	* @param verb The action or query name.
	* @param arg1 First argument, meaning depends on verb.
	* @param arg2 Second argument, meaning depends on verb.
	* @return If the handler implements the verb, it returns a suitable value. If it does not implement it, then the Undefined value is returned.
	*/
	virtual NIMacro::DataItem DoVerb(const Nbaseutil::safestring& verb, const NIMacro::DataItem& arg1=NIMacro::Undefined, const NIMacro::DataItem& arg2=NIMacro::Undefined) {return NIMacro::Undefined;}


	/// Returns true for keep-alive handlers (those derived from a KeepAlive* handler class, or declaring prop_is_keepalive property by itself).
	bool IsKeepAlive() {
		return (GetHandlerProp() & prop_is_keepalive) != 0;
	}

	/// Returns true for handlers which may block for long time when servicing the transaction (e.g. /accpoll).
	bool IsBlocking() {
		return (GetHandlerProp() & prop_is_blocking) != 0;
	}


	/// Return true if the http server should filter the returned package automatically through PHP if it contains "<?php" character sequence. The base class implementation returns false.
	bool AutoPhpEnabled() {
		return (GetHandlerProp() & prop_auto_php) != 0;
	}

	/// Return true if the URL tail should be kept HTTP-quoted by the server component.
	bool WantsUrlTailQuoted() {
		return (GetHandlerProp() & prop_wants_urltail_quoted) != 0;
	}

	/// If the resource is login-protected, returns a pointer to the access checker object, otherwise NULL.
	virtual PAccessPermitter IsAccessProtected();

protected:
	/** A convenience function for derived classes; compose default HTTP headers for a text/html response.
	* @param text The buffer to fill in.
	* @param status HTTP status code to fill in the headers, e.g. "302 See Other".
	*/
	static void DefaultHttpHeaderTextHtml(Nbaseutil::safestring& text, const Nbaseutil::safestring& status="200 OK", bool keep_alive=false);

	/** A convenience function for derived classes; compose default HTTP headers for a binary response.
	* @param ContentType HTTP content type to fill in the headers, e.g. "image/jpg".
	* @param ContentLength Length of the binary HTTP body part, in bytes.
	* @param text Buffer for the output headers.
	* @param status The HTTP status code to inject in the generated header.
	*/
	static void DefaultHttpHeaderBinary(const Nbaseutil::safestring& ContentType, int ContentLength, Nbaseutil::safestring& text, const Nbaseutil::safestring& status="200 OK", bool keep_alive=false);

protected: // service functions

	/**
	* The handler can use this function in order to access the HTTP parameters for POST or GET method, during the
	* GetContent() et al methods. If called not in suitable time, NULL is returned.
	* If the handler has swapped away the post data during the Post() call, then
	* a pointer to an empty container is returned by this function.
	*/
	postdata_t* GetPostData();

private: // implementation

	// called by the server
	void SetPostData(postdata_t* postdata);

	void operator=(const HtmlCmdHandler&); // not implemented
	HtmlCmdHandler(const HtmlCmdHandler&); // not implemented

protected: // data
	/// If true, then Aborted() function has been called after start of the request.
	bool aborted_;

	/// Protects busy_
	boost_mutex busy_mx_;
	/// Shows that a request handling is on its way. Raised in CanServe(), cleared in EndServe().
	bool busy_;

private:	// data
	friend class HttpServer;
	HttpServer* server_;
	Nbaseutil::safestring url_prefix_;
	HtmlCmdHandlerImpl& impl_;
	void* reserved_;
};



inline HtmlCmdHandler::handler_prop_t operator|(HtmlCmdHandler::handler_prop_t a, HtmlCmdHandler::handler_prop_t b) {
	return HtmlCmdHandler::handler_prop_t(static_cast<unsigned int>(a)|static_cast<unsigned int>(b));
}

/**
* Base class for Html handlers for binary data passing
* In derived class, override BeginTransaction() and initialize ContentType_ and ContentLength_ properly.
* Also, override GetContent() and pass binary raw data only, of length ContentLength_.
*/
class DI_AcapellaHttp HtmlCmdHandlerBinary: public HtmlCmdHandler {
protected:
	/// Should be set to the proper type, e.g. "image/png" in overridden BeginTransaction().
	Nbaseutil::safestring ContentType_;
	/// Should be set to the proper value (binary file length in bytes), in overridden BeginTransaction().
	int ContentLength_;
public:
	HtmlCmdHandlerBinary(): ContentLength_(-1) {}
	/// Overridden virtual function, prepares headers for a binary HTTP packet.
	virtual void ComposeHttpHeader(const Nbaseutil::safestring& arg, Nbaseutil::safestring& text);

	/// Overridden virtual function, prepares HTTP packet body part consisting only of single binary file (<tt>"&amp;%c;"</tt>)
	virtual void ComposeHtmlPage(const Nbaseutil::safestring& arg, Nbaseutil::safestring& text) {
		text += "&%c;";
	}
	/// Overridden virtual function, fails with an exception (this should be never called).
	void ComposeHtmlBody(const Nbaseutil::safestring& arg, Nbaseutil::safestring& text) {
		throw NIMacro::Exception(NIMacro::ERR_PROGRAM_ERROR, "HtmlCmdHandlerBinary::ComposeHtmlBody() should not be called.");
	}
};

/**
* An HTTP server handler for serving files from server's disk.
* This can be used as a final class.
* No HtmlCmdHandlerFile is created by default. The client code can create multiple HtmlCmdHandlerFile objects
* for serving different disk directories.
* See also Acapella FileServer() module.
*
* This handler supports a level of redirection. If the request URL contains an optional GET-style parameter
* called act, then its value is interpreted as a name of another handler. If the handler is registered,
* the request is redirected to that handler, passing the actual disk filename as the argument. This affects
* also the online_help handler as it is derived from this handler. Thus, if one uses an URL like
* "%ATTACH_URL%/abc.script?act=open" in Wiki extended online help documentation,
* then Acapella will redirect the request to the "open" handler, which just
* opens the script file in EvoShell. If not running in EvoShell, the "open" handler is not registered
* and the file content is just returned to the browser, it is now up to the browser what it makes with it.
*/
class DI_AcapellaHttp HtmlCmdHandlerFile: public HtmlCmdHandlerBinary {
public: // interface
	/**
	* Construct an HtmlCmdHandlerFile object for serving files
	* in the directory specified by 'serverroot' parameter.
	* In this directory there should be a file 'mime.types' which maps
	* file extensions to file types. If it is missing, then the mime.types file
	* used by the "file" command handler (/file/mime.types) is used, if that handler is currently running,
	* otherwise an exception occurs.
	* Format of the mime.types file is.
	*
	* <pre>
	* # comment
	* application/excel              xls
	* image/jpeg                     jpeg jpg jpe
	* </pre>
	* @param serverroot Directory name to serve files from. This can also be a list of directory names, separated by semicolons (on Windows) or colons (on Linux).
	* @param cachable Specifies if the served files will be marked cachable (expire time 24 hours), or non-cachable.
	* @param dirlistings Specifies that the server is allowed to return directory listing for directory names.
	* @param autophp Specifies that any files containing character sequence "<?php" to be automatically filtered through PHP.
	*/
	HtmlCmdHandlerFile(const Nbaseutil::safestring& serverroot, bool cachable=true, bool dirlistings=false, bool autophp=false);

	/**
	* Searches for the file in the serverroot directory or directories.
	* @return The full pathname when found, otherwise an empty string.
	* @param filename Searched filename, relative to the serverroot. May not start with '/' or '\\'.
	*/
	Nbaseutil::safestring FindFile(const Nbaseutil::safestring& filename) const;

	/// Return the list of maintained directories.
	Nbaseutil::safestringlist GetRootDirs() const {return root_;}

protected: // overridden virtuals
	/// Overridden virtual method, composes binary headers with cache control settings.
	virtual void ComposeHttpHeader(const Nbaseutil::safestring& arg, Nbaseutil::safestring& text);
	/// Overridden virtual method, loads the file and determines content-type and content-length parameters.
	virtual PHtmlCmdHandler BeginTransaction(const Nbaseutil::safestring& arg, const Request& request);
	/// Overridden virtual method, releases the redirect handler.
	virtual void EndTransaction(const Nbaseutil::safestring& arg);
	/// Overridden virtual method, returns the file content.
	virtual void GetContent(const Nbaseutil::safestring& arg, Nbaseutil::safestring& text);

	/// Takes care to return the autophp flag if passed to the constructor as well as for all .php files.
	virtual handler_prop_t GetHandlerProp() {
		return (autophp_ || content_type_=="application/x-httpd-php")? prop_auto_php: prop_default;
	}

private: // implementation
	bool IsRedirected(const Nbaseutil::safestring& filename, const Request& request);

private: // data
	Nbaseutil::safestring content_type_;
	Nbaseutil::safestringlist root_;
	bool cachable_, dirlistings_, autophp_;
	Nbaseutil::Carrier carrier_;
	std::map<Nbaseutil::safestring, Nbaseutil::safestring> mimetypes_;
	PHtmlCmdHandler redirect_;
};


/**
* A base class template for HTML content handlers maintaining keep-alive connections.
* For each client connection a new handler object is created by the Clone() method.
* This remains alive until the connection is closed.
* Among other things, this means that different requests to the same handler can be serviced in parallel.
* This is the same as KeepAliveHandler, but derived from HtmlCmdHandler instead.
*/
class DI_AcapellaHttp KeepAliveHtmlHandler: public HtmlCmdHandler {
	typedef HtmlCmdHandler super;

protected: // virtual interface

	/**
	* In overridden class, return a new exemplar of the overridden class for serving one keep-alive connection.
	* The code typically reads just: return new CLASSNAME();
	*/
	virtual PHtmlCmdHandler Clone(const Request& request)=0;

protected: // overridden virtuals

	/// Calls the Clone() virtual function; do not override.
	virtual PHtmlCmdHandler CanServe(const Request& request);

	/// Takes care about declaring this handler as Keep-Alive.
	virtual handler_prop_t GetHandlerProp();

};

/**
* A base class template for binary content handlers maintaining keep-alive connections.
* For each client connection a new handler object is created by the Clone() method.
* This remains alive until the connection is closed.
* Among other things, this means that different requests to the same handler can be serviced in parallel.
* This is the same as KeepAliveHtmlHandler, but derived from HtmlCmdHandlerBinary instead.
*/
class DI_AcapellaHttp KeepAliveHandler: public HtmlCmdHandlerBinary {
	typedef HtmlCmdHandlerBinary super;

protected: // virtual interface

	/**
	* In overridden class, return a new exemplar of the overridden class for serving one keep-alive connection.
	* The code typically reads just: return new CLASSNAME();
	*/
	virtual PHtmlCmdHandler Clone(const Request& request)=0;

protected: // overridden virtuals

	/// Calls the Clone() virtual function; do not override.
	virtual PHtmlCmdHandler CanServe(const Request& request);

	/// Takes care about declaring this handler as Keep-Alive.
	virtual handler_prop_t GetHandlerProp();

};

class DI_AcapellaHttp XmlHandler: public KeepAliveHandler {
	typedef KeepAliveHandler super;

public: // static interface
	/// Default translation of Acapella error codes to HTTP status codes.
	static Nbaseutil::safestring AcapellaErrCode2HttpStatusCode(unsigned int acapella_errorcode_mask);

public: // interface
	XmlHandler(): cachable_(false) {}

protected: // virtual interface

	/**
	* Handle an incoming request. In case of errors this function should throw an Exception, this is formatted as
	* an XML response packet and sent back with an HTTP 4xx series error code.
	*
	* @param arg Extra HTTP packet request line, following the handler name.
	* @param request The HTTP request packet, for any case.
	* @param rootnode The incoming XML structure; NULL if not present.
	* @param xml_response The response XML structure. This is not an TixXmlElement because it is often easier to construct the XML directly in a string buffer.
	*/
	virtual void Transaction(const Nbaseutil::safestring& arg, const Request& request, const TiXmlElement* rootnode, Nbaseutil::safestring& xml_response)=0;

	/**
	* This virtual function can be overridden optionally when needed.
	* This is called from inside the catch(...) handler. This function should rethrow the exception, catch it and initialize
	* all output parameters according to the exception. The XmlHandler::TranslateException() base class version implements the default functionaolity.
	* @param reporter The schema name to look up the id value.
	* @param id The error ID in the schema corresponding to the 'reporter' parameter. 0 if the error ID is unknown or not established.
	* @param acapella_errorcode_mask A bitmask corresponding to Acapella error and warning code bits (see baseutil/imacroexception.h).
	* @param http_status_code The status code for response HTTP packet, e.g "400 Bad Request".
	* @param message_plain Textual message in plain format, UTF-8 encoding. One of message_plain or message_html should be non-empty.
	* @param message_html Textual message in HTML format, UTF-8 encoding. One of message_plain or message_html should be non-empty.
	*/
	virtual void TranslateException(
		Nbaseutil::safestring& reporter,
		int& id,
		unsigned int& acapella_errorcode_mask,
		Nbaseutil::safestring& http_status_code,
		Nbaseutil::safestring& message_plain,
		Nbaseutil::safestring& message_html
	) throw();

	/**
	* Normally XmlHandler supports POST requests with text/xml content. However, sometimes it might be useful
	* to support GET requests as well. In this case the derived class can override the TranslateGET method
	* and create the needed XML structure under doc. The defaul implementation in XmlHandler base class does nothing.
	*/
	virtual void TranslateGET(const Request& request, TiXmlDocument& doc) {}

	/**
	* Normally XmlHandler supports POST requests with only text/xml or application/www-url-encoded content.
	* However, sometimes it might be useful to support other POST types as well. If a packet with another content
	* type comes in, this method is called instead of Transaction(). If it returns false, an error is reported to
	* the client. If it returns true, it has also to form the output response in the xml_response parameter;
	* the Transaction() method is not called in this case.
	* The XmlHandler base class implementation just returns false from this method.
	*/
	virtual bool AcceptNonXmlPost(const Nbaseutil::safestring& contenttype, const Request& request, Nbaseutil::safestring& xml_response) {return false;}


	/// Composes a non-cachable HTTP header, according to currently stored ContentType_ and ContentLength_ member values.
	virtual void ComposeHttpHeader(const Nbaseutil::safestring& arg, Nbaseutil::safestring& text);

protected: // subclass interface
	void SetCachable(bool cachable) {cachable_ = cachable;}

private: // overridden "final" virtuals
	virtual PHtmlCmdHandler BeginTransaction(const Nbaseutil::safestring& arg, const Request& request);
	virtual void GetContent(const Nbaseutil::safestring& arg, Nbaseutil::safestring& text);

private: // data
	Nbaseutil::safestring xml_buffer_;
	bool cachable_;
};


/// Typedef for using with HttpServer::SetLogHandler() and HttpServer::GetLogHandler().
typedef void (*LogHandlerFunc)(unsigned int errcode, const Nbaseutil::safestring& message, void* data);

/// An abstract base class for classes to be used in HttpServer::SubmitAndWait(). See HttpServer::SubmitAndWait() for details.
class HttpSubmitter {
public:
	virtual bool Submit(const Nbaseutil::safestring& submitbutton="")=0;
	virtual ~HttpSubmitter(){}
};

/**
* An abstract base class for counselors to be used with HttpServer::AddCounselor() and HttpServer::Consult().
* The counselor concept makes it possible to perform interactions between different system parts (DLL-s) which are otherwise independent.
*/
class HttpServerCounselor: public Nbaseutil::mb_malloced {
public:
	HttpServerCounselor(): server_(NULL) {}

	/**
	* Called by HttpServer::Consult(). Override to handle certain verbs. Should return true and assign the result if the verb is handled.
	* Currently used verbs include:
	*   * "notify_server_started" - broadcast by the server start to all counselors.
	*   * "get_acapella_environment" - obtain the main application/environment name, e.g. "EvoShell". This will be available in PHP as $__acapella_environment.
	*   *
	*/
	virtual bool Consult(const Nbaseutil::safestring& verb, const NIMacro::DataItem& param, NIMacro::DataItem& result)=0;

	/// Dtor removes this object from the HttpServer counsellors' list.
	DI_AcapellaHttp virtual ~HttpServerCounselor();
private:
	friend class HttpServer;
	HttpServer* server_;
};


/**
* An abstract base class for HTML-based ImageView (and other?) users. These users can register
* themselves by the view managers in order to get notifications about view content or setting changes.
*/
class ViewCallback {
public: // enums
	enum event_t {
		view_changed, // view content and settings changed
		view_data_only_changed, // view content (image data) only changed
		view_xml_only_changed, // only visual settings changed
		view_deleted, // view has been deleted
	};
public: // virtual interface
	/// This will be called by view changes, in an arbitrary thread.
	virtual void NotifyViewChanged_mt(const Nbaseutil::safestring& label, event_t event, const Nbaseutil::safestring& itempath)=0;
	virtual ~ViewCallback() {}
};

/** The class implementing an HTTP server.
* Usually only one server object is available, accessed by static HttpServer::Instance() member function.
*
* The HTTP server works in multithreaded mode. All member functions can be called from any thread unless marked otherwise.
*/
class DI_AcapellaHttp HttpServer {
public:
	/** Access the main instance of the server.
	* This will start the server if not yet started (see InstanceNoStart() for avoiding that).
	* @param deleteinstance If true, this will stop the server and delete the istance. Used only in Windows Debug builds to avoid false positives from memory leak detectors.
	*/
	static HttpServer& Instance(bool deleteinstance=false);

	/**
	* Access the main instance of the server.
	* Do not start the server if not yet started.
	* Use this function when registering the handlers.
	*/
	static HttpServer& InstanceNoStart();

	/// Checks whether the main instance is existing.
	static bool IsInstanceAvail();

	/// Checks if a server is running on specified port in the current process; if not, tries to start it; if the port is taken by another process, an exception is thrown.
	static HttpServer* StartServerOnPort(int port);

	/**
	* To be called before unloading the client DLL.
	* The acapellahttp library will get rid of any potential pointers into the client library memory,
	* thus ensuring that it is safe to unload the client DLL from the process memory space.
	* @param reserved Reserved for future use, do not use.
	*/
	void PrepareShutdown(void* reserved=NULL);

	/**
	* Ctor, creates a new custom server object on first available port in the range [firstport, firstport+1000].
	* For the main server instance use HttpServer::Instance() instead.
	* @param start Call Start() at once. By passing false one can defer the server start to until later (this is used in InstanceNoStart()).
	*/
	HttpServer(bool start=true);

	/**
	* Ctor, creates a new custom server object on the specified port, or throw if it is taken.
	* For the main server instance use HttpServer::Instance() instead.
	* The server is started immediately.
	* @param port The port the server should listen on. On Linux, only root may use privileged ports. If the port is not free, and exception is thrown.
	* @param start_servicing When starting up the server, immediately start to accept HTTP requests. If this is false, one must call StartServicing() later.
	*/
	HttpServer(unsigned short port, bool start_servicing=true);

	/**
	* If not yet started, create the server thread and start to serving the incoming requests.
	* @param start_servicing When starting up the server, immediately start to accept HTTP requests. If this is false, one must call StartServicing() later.
	*/
	void Start(bool start_servicing=true);

	/// If HttpServer(port,false) or Start(false) has been called, call this one to start accepting HTTP requests.
	void StartServicing();

	// If server has been started and not yet stopped, stop the server and terminate the server threads.
	void Stop();

	/// Virtual, to get dynamic_cast working.
	virtual ~HttpServer();

	/// Return the default host name this server can be contacted by. 
	Nbaseutil::safestring GetHostName() const;

	/**
	* Return the URL prefix part for accessing this server object, e.g. "http://altair.perkinelmer.net:8283".
	* @param host The server name as suitable for the client. If empty, the name "localhost" will be used. If "?", the default server name (as returned by this->GetHostName()) is used.
	*/
	Nbaseutil::safestring GetUrlPrefix(const Nbaseutil::safestring& host) const;

	/// Return the TCP port number where the server is listening on.
	int GetPort() const {return socket_.GetPort();}

	/// Return true if the server thread is running and no abortion signal has arrived.
	bool Working() {return thread_!=NULL && !terminating_;}

	/** Search for a handler by handler registered name.
	* @param  cmd Handler name as specified in the RegisterHandler() call.
	* @return Pointer to the handler, or NULL. Use with care because if the handler is carrying out a service request it is not allowed to call its functions from another thread.
	*/
	PHtmlCmdHandler FindHandler(const Nbaseutil::safestring& cmd) const;

	/// Return the name under which the handler is registered by the HTTP server. If not found, an empty string is returned.
	Nbaseutil::safestring GetHandlerName(PHtmlCmdHandler handler) const;

	/**
	* Register a request handler. This must be allocated by new, and will be deleted by HttpServer,
	* if the autodelete_ protected member flag is raised (the default).
    * Be sure to call PrepareShutdown() on the HttpServer object before the client library has been unloaded from the process memory space.
	*/
	void RegisterHandler(const Nbaseutil::safestring& cmd, PHtmlCmdHandler handler);

	/**
	* If there exists a registered handler for the specified verb, then it
	* is removed from the registry. If the handler is currently busy with a request,
	* then server gives it 5 seconds to finish the request.
	* Finally, if handler's autodelete_ field is true, the handler is deleted.
	*/
	void UnRegisterHandler(const Nbaseutil::safestring& cmd);

	static void RunPhpOn(Nbaseutil::safestring& html);

	/// Thread function accepting the requests, do not call from outside.
	void operator()();

	void ListVerbs(Nbaseutil::safestring& text);

	/**
	* Call submitter.Submit() and wait until the specified handler has processed
	* a Post notification().  If Submit() returns false or timeout occurs, returns false.
	* It is up to the caller to ensure that calling submitter.Submit() will post data to the specified handler.
	*/
	bool SubmitAndWait(HtmlCmdHandler& handler, HttpSubmitter& submitter, int timeout_secs);

	/// Abort any running requests and do not accept any new requests. Return true if successful. The parameter is currently not used.
	bool Abort(int reserved=0);

	/// Check if abortion is in effect.
	bool Aborted() const;

	/// Obsolete, use LOGGER() macro or Logger::Log() instead. This function just redirects the message to Logger::Log(), on level2_error, level3_warning or level8_verbose, depending on the ERR_ERROR and ERR_WARNING bits in the \a errcode.
	void Log(unsigned int errcode, const Nbaseutil::safestring& msg);

	/// If you plan to exit the process via _exit(), you can call this function to close the server socket.
	void CleanupBeforeForceAbort();

	/**
	* Add a counselor for servicing Consult() requests.
	* This mechanism is used for communication between binarily independent DLL-s.
	* The counselor ownership is NOT passed over to HttpServer in order to avoid DLL download order issues.
	* The same counselor may be added only once.
	*/
	void AddCounselor(HttpServerCounselor& counselor);

	/// Remove a previously added counselor. This function is called automatically from a councelor's destructor. The caller will be responsible for destruction of the counsellor afterwards.
	void RemoveCounselor(HttpServerCounselor& counselor);

	/**
	* Ask help from all requested counselors.
	* @return Returns true, if any of the councelors handled the verb.
	* @param verb Command verb. Currently defined command verbs are:
	*    - "__get_private_scriptfiles__": Returns a vector of PResolvePoint smartpointers to private module resolve points for the script currently active in an editor.
	*    - "notify_server_started": This message is broadcasted to all registered counsellors from HttpServer::Start().
	* @param param Additional parameter, meaning depends on the verb.
	* @param result If any of counsellors handled the verb, then the result is returned here. The result from the most recently registered counsellor handling this verb is returned.
	* @param broadcast If false, then the counsellors are consulted in the reverse order of registration, until a counselor is found who handles the verb. For broadcast messages all counsellors are called.
	*/
	bool Consult(const Nbaseutil::safestring& verb, const NIMacro::DataItem& param, NIMacro::DataItem& result, bool broadcast=false);

	/**
	* Register the view callback for getting notifications about view changes.
	* The caller is obliged to call RemoveViewCallback() before the callback object is destroyed.
	* @param callback Reference to a callback object, derived from ViewCallback.
	* @param label The view name the callback is interested in. Empty string means all views.
	*/
	void RegisterViewCallback(ViewCallback& callback, const Nbaseutil::safestring& label);

	/// Unregister a view callback.
	void RemoveViewCallback(ViewCallback& callback);

	void NotifyViewCallbacks(const Nbaseutil::safestring& label, ViewCallback::event_t event, const Nbaseutil::safestring& itempath="");

	/// Calculate the approximate memory usage of the server component and the registered handlers.
	void CalcMemUsage(std::vector<std::pair<Nbaseutil::safestring, Nbaseutil::uint64> >& buffer);

	/// Restrict the range of allowed ports to try when starting up the server without explicit port.
	void SetPortRange(const Nbaseutil::PortRange& range, int first_port_to_try, bool loopback_only);

	/// Sets preferred TCP port which should be used by spawned slave processes, if possible.
	void SetPreferredPort(int port);

	/// Apply performance tuning options and such
	void SetOption(const Nbaseutil::safestring& name, const NIMacro::DataItem& value);

	/**
	* Can be called from inside an HTTP service thread. Notifies the server that thread might be blocked for indefinite time 
	* and the server should launch new threads meanwhile for servicing the requests if needed. Make sure to call NotifyThreadUnblocked()
	* afterwards.
	*/
	void NotifyThreadBlocked();

	/**
	* Can be called from inside an HTTP service thread if it called NotifyThreadBlocked() before. Notifies the server 
	* that the service thread is now running normally again.
	*/
	void NotifyThreadUnblocked();

private:
	HttpServer(const HttpServer&); // no copies please!
	void operator=(const HttpServer&); // no copies please!

public: // implementation
	void HandleRequest(HtmlCmdHandler& handler, Request& request, Nbaseutil::safestring& headers, Nbaseutil::safestring& msg);

private: // implementation
	friend struct Request;
	friend class RequestQueue;

	void RegisterStdHandlers();
	//static void FindPostData(const Nbaseutil::safestring& request, HtmlCmdHandler::postdata_t& postdata);

	/// Check if access from client, using httpmethod( "GET" or "POST") is ok to proceed. If not, throw.
	void CheckAccessGranted(const Nbaseutil::safestring& client, const Nbaseutil::safestring& httpmethod) const;

	void ComposePacket(const Request& request, HtmlCmdHandler& handler, Nbaseutil::safestring& http_headers, Nbaseutil::safestring& http_body);

	void HandleException(Request& request, Nbaseutil::safestring& headers, Nbaseutil::safestring& msg);

	void QueueRequest(Request* request, AcceptedSocket as, Nbaseutil::safestring& keepalive_buffer);

	bool IsClientDistanceKnown() const;

	void WaitWhileServicingStarted();

public: // constants
	static const unsigned int max_allowed_request_size_ = 10*1000*1000;	// do not allow client requests over 10MB.

private: // data

	bool loopback_only_; // must be before socket_
	ServerSocket socket_;

	boost::thread* thread_;
	volatile bool terminating_;
	bool aborted_;

	boost_mutex started_mx_;
	boost_condition started_cnd_;
	bool started_;
	bool servicing_;
	bool use_environment_ACC_PORT_;
	bool accessedViaProxy_;
	Nbaseutil::PortRange range_;
	int first_port_to_try_, preferred_port_;

	typedef std::map<Nbaseutil::safestring, PHtmlCmdHandler> cmd_handlers_t;
	cmd_handlers_t cmd_handlers_;
	mutable boost_mutex cmd_handlers_mutex_;

	typedef std::vector<HttpServerCounselor*> counselors_t;
	counselors_t counselors_;
	boost_mutex counselors_mx_; // protects counselors_;

	RequestQueue* requestqueue_;

	typedef std::vector<std::pair<ViewCallback*, Nbaseutil::safestring> > viewcallbacks_t;
	viewcallbacks_t viewcallbacks_;
	boost_mutex viewcallbacks_mx_; // protects viewcallbacks_


};

/**
* The parsed request structure passed to HtmlCmdHandler::BeginTransaction() method.
* This method may store a pointer to the Request object. The object is alive and immutable
* until HtmlCmdHandler::EndTransaction() is called.
*
* This structure is immutable after initial composition, except postdata_, which might be swapped out from here in the POST method.
*/
struct Request {
	/**
	* The Request ctor reads in one HTTP packet from the socket.
	* @param socket The socket to be read.
	* @param lookahead_buffer If the incoming TCP stream contains more data than a single HTTP packet,
	*				then this is put into the lookahead_buffer. For reading keep-alive HTTP connections
	*				pass an empty buffer first time, and pass back the returned buffer to next Request()
	*				ctors in the same keep-alive HTTP connection.
	* @param max_allowed_request_size Maximum size of HTTP packet headers to be read in.
	*				If this limit is reached when reading HTTP packet headers, an exception is raised.
	*				This parameter does not affect reading packet body (with known content-length).
	*/
	Request(AcceptedSocket& socket, Nbaseutil::safestring& lookahead_buffer, unsigned int max_allowed_request_size);
public: // data
	/// "POST" or "GET"
	Nbaseutil::safestring method_;
	/// First slash-separated token in the URL path after server-port, does not include any slashes itself. Empty if accessing the root URL.
	Nbaseutil::safestring cmd_;
	/// Rest of the URL path after cmd_, without any starting slash, no URL-unquoting applied.
	Nbaseutil::safestring tail_;
	/// HTTP Host specification, in "server:port" format.
	Nbaseutil::safestring host_;
	/// Not in use, kept for binary compatibility
	Nbaseutil::safestring reserved1_;
	/// The whole incoming HTTP packet, unprocessed.
	Nbaseutil::safestring packet_;
	/// The client IP as available by the HTTP socket.
	Nbaseutil::safestring client_ip_;
	/// Typedef for cookies_.
	typedef std::map<Nbaseutil::safestring, Nbaseutil::safestring> cookies_t;
	/// HTTP cookies which were extracted from the request.
	cookies_t cookies_;
	/// A sequential request number
	const int id_;
	/// Used for assigning next id_ numbers.
	static int next_id_;
	/// Shows that the previous request for the same handler was not fulfilled.
	bool prev_request_abandoned_;
	/// The client has specified "Connection: close".
	bool connection_close_;
	/// The client specified Accept-Encoding: gzip with non-zero q value.
	bool accept_gzip_;
	/// The clock() value at the time of receiving the request.
	clock_t clock_;
public: // service functions
	DI_AcapellaHttp static void FindPostData(const Nbaseutil::safestring& request, HtmlCmdHandler::postdata_t& postdata);
	DI_AcapellaHttp static void FindCookies(const Nbaseutil::safestring& request, cookies_t& cookies);
	DI_AcapellaHttp Nbaseutil::safestring GetPostItem(const Nbaseutil::safestring& name) const;
	DI_AcapellaHttp Nbaseutil::uint64 CalcMemUsage() const;
	bool IsKeepAlive() const {return !connection_close_;}
private:
	friend void HttpServer::ComposePacket(const Request& request, HtmlCmdHandler& handler, Nbaseutil::safestring& http_headers, Nbaseutil::safestring& http_body);
	friend void HttpServer::HandleRequest(HtmlCmdHandler& handler, Request& request, Nbaseutil::safestring& headers, Nbaseutil::safestring& msg);
	mutable HtmlCmdHandler::postdata_t postdata_; // mutable, as it will be swapped out to the request handler
};

/// The same as QuoteHttp(), using functional-style interface.
DI_AcapellaHttp Nbaseutil::safestring Ascii2HTTP(const Nbaseutil::safestring& s);

/// The same as UnquoteHttp(), using functional-style interface.
DI_AcapellaHttp Nbaseutil::safestring HTTP2Ascii(const Nbaseutil::safestring& s);

/**
* Return the IP or name of this machine (without port), which would be visible to the other computer
* identified by the \a remote_host parameter. Current implementation is not sophisticated, it only distinguishes
* localhost addresses. This should be enough for supporting disabling network interfaces during the run of Acapella.
* @param remote_host The name or IP (dotted-decimal IPv4 or IPv6 hex address without brackets) of the other machine. May not include the port number.
*/
DI_AcapellaHttp Nbaseutil::safestring GetMyHostNameFor(const Nbaseutil::safestring& remote_host);

/// Extract the host name from the string in the HTTP Host field format (name, IPv4 dotted-decimal, IPv6 hex in brackets, optionally followed with colon and port).
DI_AcapellaHttp Nbaseutil::safestring ExtractHost(const Nbaseutil::safestring& host_port);

/// Extract the port from the string in the HTTP Host field format (name, IPv4 dotted-decimal, IPv6 hex in brackets, optionally followed with colon and port). If the port field is non-numeric, an exception will be thrown.
DI_AcapellaHttp int ExtractPort(const Nbaseutil::safestring& host_port);

/// Combines ExtractHost() and ExtractPort().
DI_AcapellaHttp void SplitHostSpec(const Nbaseutil::safestring& hostspec, Nbaseutil::safestring& host, int& port);

/// Compose HTTP Host field from host and port. This involves putting an IPv6 hex address into brackets.
DI_AcapellaHttp Nbaseutil::safestring ComposeHostSpec(const Nbaseutil::safestring& host, int port);

/// Format a HTTP 303 See Other response in the buffer.
DI_AcapellaHttp void FormatLocation(const Nbaseutil::safestring& location, const Nbaseutil::safestring& hostspec, Nbaseutil::safestring& buffer);

/**
* Replace the global access permitter which will used by default for all handlers which have
* not overriden HtmlCmdHandler::IsAccessProtected().
* @param ap Smartpointer to the access permitter object. This can be NULL (protection switched off).
*/
DI_AcapellaHttp void SetDefaultAccessPermitter(PAccessPermitter ap);

/// Announce product name and version to this library. This info is set as $HTTP_POST_VARS['product_name'] and $HTTP_POST_VARS['product_ver'] PHP variables for PHP-processed pages.
DI_AcapellaHttp void SetProductInfo(const Nbaseutil::safestring& name, const Nbaseutil::safestring& version);

/// Special exception for reporting out-of-sync POST operations inside the handler; the server will resort to GET operation and display a warning in the top of page.
class CookieMismatchException: public Nbaseutil::Exception {
public:
	CookieMismatchException(unsigned int reason, const Nbaseutil::safestring& msg): Nbaseutil::Exception(reason, msg) {}
};

} // namespace


#endif
