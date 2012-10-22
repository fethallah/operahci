#ifndef x_IMACRO_DBCONN_H_INCLUDED_
#define x_IMACRO_DBCONN_H_INCLUDED_

namespace NIMacro {

	class DbConn;
	typedef ThreadSharablePointer<DbConn> PDbConn;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4275)
#endif
	class Mod;

	/**
	* Base class for database drivers. Each object corresponds to a connection to a database.
	* The actual database driver classes must be registered via RegisterDbDriver() function, for example during ExportedClasses() call in the module library.
	*/
	class DI_IMacro DbConn: public ThreadSharable {
	public:
		/// An example creator function. Each derived class should provide it's own relevant static creator function and register it's address via RegisterDbDriver() call.
		static PDbConn Create() {throw "Never called";}

		/// Derived class should call DisConnect in its destructor!!! Cannot call it here because here the dynamic object type has been lost already.
		virtual ~DbConn()=0;

		/// Return the driver type as passed to RegisterDbDriver().
		const safestring& GetDbType() const;

		/** Return the percent of readyness to provide this connection (0 (no way) - 100 (I am the best driver to perform this connection)).
		* This forwards to DoCanConnect() function.
		* The default is to return 50%. Parameters are the same as for Connect().
		*/
		int CanConnect(const safestring& username, const safestring& service, const safestring& extradata="");

		/* Connect to the remote database, or throw.
		* Parameters correspond to the input parameters of dbopen() module.
		* @param service Datasource name, Oracle service name, etc., as suited for the driver. 
		* @param extradata Any additional data needed for creating the connection. The shared connections are differentiated by username and service only.
		*/
		void Connect(const safestring& username, const safestring& password, const safestring& service, const safestring& extradata="");

		/// Return true if the connected_ flag is up and DoVerifyConnection() returns true. 
		bool IsConnected() const;

		/// Disconnect and try to reconnect, reusing last username/password etc. This calls DoDisConnect() and DoConnect().
		void ReConnect();

		/// Disconnect from the database. This forwards to DoDisConnect() if necessary.
		void DisConnect();

		/** Run a SQL query and return the results in an IMacro table.
		* @param sql SQL query or command.
		* @param fetchcount Number of records in the temporary buffer.
		* @param toponly Fetch only one temporary buffer and return, cancelling the operation.
		* @param rowcount Output parameter - number of records processed. 
		*				Needed in the case when the SQL command is not a query and there is thus no output table.
		* @param caller Calling module or NULL. Can be used for warning reporting.
		*/
		PTable Execute(const char* sql, int fetchcount, bool toponly, int& rowcount, Mod* caller);

		/// Return a description of the connection. Forwards do DoGetDescription().
		safestring GetDescription() const;

		/// Fallback for arbitrary extensions. Meaning of parameters depend on verb. If the driver doesn't support the passed verb, the call should be forwarded to the base class.
		virtual SafeValue Verb(const char* verb, const SafeValue& arg1=SafeValue(), const SafeValue& arg2=SafeValue());
	protected:
		DbConn();
		/// Virtual functions to be overriden in the actual driver class. See the corresponding nonvirtual method descriptions without Do... prefix.
		virtual int DoCanConnect(const safestring& username, const safestring& service, const safestring& extradata) {return 50;}
		virtual void DoConnect(const safestring& username, const safestring& password, const safestring& service, const safestring& extradata)=0;
		virtual void DoDisConnect()=0;
		virtual bool DoVerifyConnection() const {return true;}
		virtual safestring DoGetDescription() const;
		virtual PTable DoExecute(const char* sql, int fetchcount, bool toponly, int& rowcount, Mod* caller)=0;
		virtual SafeValue DoVerb(const char* verb, const SafeValue& arg1, const SafeValue& arg2) {return SafeValue();}
	private:
		mutable Mutex mx_; // protects the following data 
		bool connected_;
		safestring username_, password_, service_, extra_data_, db_type_;
		friend class DbConnManager;
	};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

	typedef PDbConn (*ConnCreatorFunc)(void* callback_data);

	/** The actual database driver classes must be registered via RegisterDbDriver() function, for example during ExportedClasses() call in the module library.
	* @param creator A function pointer to the static Create() function in the derived class, which creates a new object on the heap.
	* @param db_type Database driver type, like "ODBC" or "Oracle".
	* @param callback_data Is passed back to the creator function.
	*/
	DI_IMacro void RegisterDbDriver(ConnCreatorFunc creator, const safestring& db_type, void* callback_data);

}

#endif


