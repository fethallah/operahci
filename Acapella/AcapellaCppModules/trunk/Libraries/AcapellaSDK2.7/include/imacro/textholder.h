#ifndef x_ACAPELLA_IMACRO_TEXTHOLDER_H_INGHUDED_
#define x_ACAPELLA_IMACRO_TEXTHOLDER_H_INGHUDED_

#ifdef ACAPELLA_WRAP_BOOST_MUTEX
#define ACAPELLA_INIT_MEMBER_LOCK(lockname, mutex) lockname(mutex, __FILE__, __LINE__)
#else
#define ACAPELLA_INIT_MEMBER_LOCK(lockname, mutex) lockname(mutex)
#endif

namespace NIMacro {
	class Package;
	using Nbaseutil::safestring;
	class ResolvePoint;
	typedef ThreadSharablePointer<ResolvePoint> PResolvePoint;

	class TextHolder;
	typedef ThreadSharablePointer<TextHolder> PTextHolder;
	class proc_entry;
	typedef ThreadSharablePointer<proc_entry> Pproc_entry;
	class Script;
	typedef ThreadSharablePointer<Script> PScript;
	struct ExecutionContext;
	class Module;

	struct LockedStringPtr {
		LockedStringPtr(Nbaseutil::boost_recursive_mutex& mx, const safestring& str)
			: mx_(mx)
			, ACAPELLA_INIT_MEMBER_LOCK(lock_, mx_)
			, str_(str) 
		{}
		LockedStringPtr(const LockedStringPtr& b)
			: mx_(b.mx_)
			, ACAPELLA_INIT_MEMBER_LOCK(lock_, mx_)
			, str_(b.str_) 
		{}
		const safestring* operator->() const {return &str_;}
		const safestring& operator*() const {return str_;}
	private:
		Nbaseutil::boost_recursive_mutex& mx_;
		Nbaseutil::boost_recursive_mutex::scoped_lock lock_;
		const safestring& str_;
	};

	/**
	* A struct for holding information about parsed procedures inside TextHolder text,
	* see TextHolder::ScanProcRecords().
	* The ProcRecord objects are created during TextHolder::SetText(). They are never 
	* deleted; if the the text is modified so that the procedure is deleted, the ProcRecord
	* 'deleted' flag is raised instead.
	*/
	struct ProcRecord {
	public: // interface

		/// A convenience ctor.
		ProcRecord(int startpos, unsigned int length, int startline, int changed_in_version, unsigned int initial_flags): startpos_(startpos), length_(length), deleted_(false), startline_(startline), changed_in_version_(changed_in_version), initial_flags_(initial_flags) {}
		
		/// A default ctor, needed for STL containers.
		ProcRecord(): startpos_(0), length_(0), deleted_(false), startline_(0), changed_in_version_(0), initial_flags_(0) {}
		
		/**
		* Checks if the ProcRecord is equal to b, assuming that b describes a procedure inside btext.
		* If the proc_entry member of this ProcRecord is not yet defined, the actual texts are not compared.
		* The comparison includes procedure definition startpos and length in any case.
		*/
		bool IsEqualTo(const ProcRecord& b, const safestring& btext) const;

		/// Marks the ProcRecord as deleted or not.
		void SetDeleted(bool deleted=true) {deleted_=deleted;}

		/// Checks is procedure has been deleted from the text.
		bool IsDeleted() const {return deleted_;}

		int GetStartPos() const {return startpos_;}

		int GetStartLine() const {return startline_;}

		int GetLength() const {return length_;}

		Pproc_entry GetProcEntry() const {return proc_entry_;}

		/**
		* Create the proc_entry_ field in ProcRecord and return it.
		* This function can only be called when the owning TextHolder is locked,
		* e.g. from an overridden ChangedProcCallback::Visit() callback function.
		* @param packagename The package where the procedure resides in.
		* @param th The TextHolder object containing this ProcRecord.
		*/
		Pproc_entry CreateProcEntry(const safestring& packagename, const TextHolder& th) const;

		unsigned int GetInitialFlags() const {return initial_flags_;}

	private: // data
		/// The procedure starting point (position of 'p' in 'proc' in bytes in the TextHolder text). If -1, the procedure has been deleted from the TextHolder text.
		int startpos_;
		/// The procedure length, starting from startpos_ and ending with terminating '}' (incl.)
		unsigned int length_;
		/// The procedure has been deleted. All other information is not valid any more.
		bool deleted_;
		/// The procedure starting line number (where the 'proc' keyword lies)
		int startline_;
		/// The TextHolder last version where the procedure interface or content were changed.
		int changed_in_version_;
		/// If/when the procedure body is parsed, then the proc_entry field will be filled in, otherwise this is NULL.
		mutable Pproc_entry proc_entry_;
		/// Initial flags like MOD_LOCAL
		unsigned int initial_flags_;
	};

	/// A virtual base class for using with TextHolder::ScanProcRecords().
	class ChangedProcCallback {
	public: 
		/**
		* Is called for every ProcRecord in TextHolder statisfying the version filter.
		* @param normname The procedure name in form "name/package".
		* @param procrecord Reference to the scanned ProcRecord.
		*/
		virtual bool Visit(const safestring& normname, const ProcRecord& procrecord)=0;
		virtual ~ChangedProcCallback() {}
	};

	/**
	* An abstract base class for holding any kind of Acapella script text pieces.
	* The following derived classes are used:
	*   - ScriptFile A text corresponding to a disk file or Opera database script.
	*   - proc_entry A procedure body
	*   - TempText A read-only temporary script.
	*
	* Each TextHolder object contains a pointer to a ResolvePoint. This is the 
	* resolve point to be used for name lookup when parsing the TextHolder text.
	* This may or may not coincide with the ResolvePoint where the procedures contained
	* in the TextHolder text are registered.
	*
	* All methods can be called from any thread, the TextHolder object uses internal locking.
	*
	* The text hold by a TextHolder object can change. In this case the version_ member gets
	* incremented. The primary user of this feature is the Script class, which needs to reparse
	* the script text when it has been changed.
	*/
	class DI_IMacro TextHolder: public ThreadSharable {
	public: // interface

		/**
		* Declares the TextHolder object readonly or not. 
		* Any attempts to modify the text in a readonly TextHolder object via the Replace()
		* member function will cause an exception, until the readonly flag is cleared.
		*/
		void SetReadOnly(bool readonly=true);

		/**
		* Initializes the text held by the TextHolder object. The procedures are extracted
		* from the text and stored in the procs_ member container, and the number of module
		* calls in the remaining text is counted, but the text is otherwise 
		* not parsed. The TextHolder version is incremented always. This member function
		* ignores the readonly flag setting.
		* 
		* @param text Acapella script text. The text should be in UTF-8 encoding. 
		*			If it appears not to be in UTF-8, it is assumed to be in ISO-8859-1 
		*			and is converted into UTF-8 accordingly. A log message on level 2 is generated in this case.
		*           The content will be swapped into the TextHolder object.
		*			The text can be encrypted, it will be decrypted automatically.
		* @param convert_exception_to_log If the text contains syntax errors detected 
		*             during procedure extraction, then the errors can be reported
		*             by exceptions or converted to log messages (level 2).
		*             In both cases the text is changed, but contained ProcRecords array remains empty.
		* @return True if there were no syntax errors while extracting procedures. If false
		*			  is returned, some parts of the text, like procedures, remained unparsed. 
		*/
		bool SetText(safestring& text, bool convert_exception_to_log=true);

		/// Changes the informal name returned by GetTextName().
		void SetTextName(const safestring& textname);

		/// Special flags to use with Replace().
		enum replaceflags {
			find_changed_content=0x7fffffff,
		};

		/**
		* Changes the held text content after initialization.
		* This function forwards to the virtual DoReplace() function.
		* Replaces a substring in the contained text with a new one. 
		* Procedures are extracted from the whole text again if there are any actual changes.
		* In this case the TextHolder version is also incremented.
		* This function causes an Exception if the object is readonly or encrypted.
		* @param pos The start position of replaced substring in the contained text. 
		*            Must point to an UTF-8 character boundary.
		* @param deletelen Length in bytes of the replaced substring. 
		*            Also pos+deletelen must point to an UTF-8 character boundary.
		*            If pos=0 and deletelen=find_changed_content arguments are passed,
		*            then the replacement text corresponds to the whole contained text.
		*            The function will find the smallest changed part in the text by 
		*            itself. This mode should be used when dealing with possibly large texts 
		*            and the caller does not have knowledge which exact place of the text
		*            was modified, in order to save memory in the UNDO buffer.
		* @param replacement The replacement text, corresponding to either to the
		*            replaced substring or to the full text if 'find_changed_content' mode is used.
		*			 This text cannot be encrypted.
		* @param notify_listeners This flag is forwarded to th DoReplace() virtual member function.
		*			It is meant for notifying the listeners registered by the derived object about the text change.
		*/
		bool Replace(size_t pos, size_t deletelen, const safestring& replacement, bool notify_listeners=true);

		/**
		* Sets the CVS/svn $Id: member string. This is also set automatically by SetText()/ReplaceText().
		*/
		void SetCvsId(const safestring& cvsid);

		/// Comparison operator. Returns true if the contained texts compare equal (case sensitive!).
		bool operator==(const TextHolder& b) const;

		/**
		* Used for accessing the contained text. If the text was encrypted, the encrypted text
		* is returned also from here.
		* The returned smartpointer holds the object locked for multithread-safety.
		* The returned smartpointer is logically of type 'safestring*'.
		* The returned smartpointer object should be destroyed shortly.
		*/
		LockedStringPtr GetText() const {return LockedStringPtr(mx_, text_);}

		/**
		* Another function to access contained text. Copies a substring out of the contained text.
		* If the specified substring lies fully or partially outside of the text, just the overlapping
		* part or an empty string is returned in the buffer.
		* @param pos The starting position in the contained text to copy. 
		*          If less than contained string length, then it must point to UTF-8 character boundary.
		* @param len The length of the copied substring in bytes. 
		*          If pos+len is less than contained string length, then pos+len must point to UTF-8 character boundary.
		* @param buffer The buffer to recieve the substring. Any previous content is overwritten.
		*/
		void GetSubstr(strlen_t pos, strlen_t len, safestring& buffer) const;

		/**
		* Return true if the zero-terminated \a text coincides with 
		* the held text starting part of the same length (case-sensitive!).
		* Can be used only if the held text is not encrypted.
		*/
		bool IsEqualToStartOf(const char* text) const;

		/**
		* Return the number of module calls found outside of procedures
		* in SetText()/ReplaceText(). This is used for initializing the size
		* of Script::calls_ array.
		*/
		int GetModCount() const {return modcount_;}

		/**
		* Return the version number of the content. This number is incremented
		* by each SetText(), as well as by each ReplaceText() which actually changes the content.
		*/
		int GetVersion() const {return version_;}

		/**
		* Sets the encryption flag.
		*/
		void SetEncrypted();

		/**
		* Returns true if SetEncrypted() has been called.
		*/
		bool IsEncrypted() const;

		/**
		* Returns true if readonly flag is set via SetReadOnly().
		*/
		bool IsReadOnly() const {return readonly_;}

		/** 
		* Return the informal text name, set by SetTextName();
		*/
		const safestring& GetTextName() const {return textname_;}

		/**
		* Return the $Id: field content, either extracted from the text or
		* set explicitly via SetCvsId(). Empty string is returned for scripts
		* not having $Id: field.
		*/
		const safestring& GetCvsId() const;

		/**
		* Scan extracted procedures and call visitor on each of them.
		* @param visitor The callback object.
		* @since_version Not used currently.
		*/
		void ScanProcRecords(ChangedProcCallback& visitor, int since_version=-1);

		/**
		* Return the resolvepoint used for looking up the module names in the script text.
		*/
		PResolvePoint GetResolvePoint() const {return resolvepoint_;}

		/**
		* Create new Module objects for each parsed procedure
		* and register them under the specified \a resolvepoint via ResolvePoint::AddModule_mt().
		* This is done also elsewhere, this method is called for TextHolder
		* objects whose text has been changed.
		*/
		void RegisterContainedProcedures(ResolvePoint& resolvepoint);

		/**
		* Find a parsed procedure with specified name from this TextHolder object.
		* @param normname Procedure name in normalized form NAME/PACKAGE
		* @return Pointer to the parsed procedure subscript if NULL in case of any error.
		*/
		PScript GetProcedure(const safestring& normname);

		/// For debug-build mutex lock consistency checks
		int mx_level() const;

		bool DeclareModule(Module& caller, const safestring& fullname, safestring& errmsg);

	public: // virtual interface

		/**
		* Return the line number in the file, where the encapsulated text starts (1-based).
		* The base class implementation returns 1.
		*/
		virtual int GetStartLine() const {return 1;}

		/**
		* Return the byte position in the file, where the encapsulated text starts (0-based).
		* The base class implementation returns 0.
		*/
		virtual int GetStartPos() const {return 0;}

		/**
		* Return the file name associated with the text, 
		* or an informal surrogate name if there is no real file.
		*/
		virtual const safestring& GetFileName() const=0;

		/**
		* If there is a cached parsed script present for this textholder,
		* return a pointer to it, otherwise NULL. The base class implementation
		* returns NULL.
		*/
		virtual PScript GetParsedScriptIfPresent() const {return NULL;}

		/**
		* Return the namespace name, if the encapsulated text logically
		* belongs to a namespace. The base class implementation returns an empty string.
		*/
		virtual safestring GetEnclosingPackageName() const {return "";}

		/**
		* If the encapsulated text is part of another TextHolder (like, a procedure),
		* return the version of the parent TextHolder, otherwise return the version of itself.
		* The base class implementations returns the version of itself (GetVersion()).
		*/
		virtual int GetFileVersion() const {return GetVersion();}

	protected: // virtual implementation functions

		/**
		* Called from Replace(). This should just replace the text as specified by the parameters,
		* and update the UNDO buffer if feasible. The \a deletelen parameter will not be \c find_changed_content.
		*/
		virtual bool DoReplace(size_t pos, size_t deletelen, const safestring& replacement, bool notify_listeners)=0;

	protected: // implementation
		TextHolder(const safestring& textname, safestring& text, PResolvePoint resolvepoint);
		TextHolder(const safestring& textname, PResolvePoint resolvepoint);
		void SwapTextHolder(TextHolder& b);

		virtual ~TextHolder();

		friend class Undoer;
		void replacetext(size_t pos, size_t deletelen, const safestring& replacement);

		bool CheckPossibleInputparamChange() {bool x = possible_inputparam_change_; possible_inputparam_change_=false; return x;}


	private: // friend interface
		friend class Script;

	private: // implementation
		bool FindChangedContent(const safestring& text, size_t& pos, size_t& deletelen, safestring& replacement) const;
		int ExtractProcedures();
		TextHolder(const TextHolder&);
		void operator=(const TextHolder&);

	private: // data
		mutable Nbaseutil::boost_recursive_mutex mx_;					// protects all data here
		safestring text_, textname_;
		safestring cvsid_;	// CVS identification string in the script file, or empty.
		int modcount_;

		// Current version of in-memory modifications.
		int version_;
		bool readonly_;
		bool possible_inputparam_change_; // last Replace() may have involved changes in input() module parameters.

		typedef std::map<safestring, ProcRecord> procs_t;
		procs_t procs_;

		PResolvePoint resolvepoint_;
	};

	class TempText;
	typedef ThreadSharablePointer<TempText> PTempText;

	/// A class for in-memory TextHolders (not associated with any file).
	class DI_IMacro TempText: public TextHolder {
		typedef TextHolder super;
	public:
		static PTempText Create(const safestring& textname, safestring& text, PResolvePoint resolvepoint, int startline);
	public: // virtual overrides

		virtual PSharable DoClone() const;

		virtual int GetStartLine() const {return startline_;}

		virtual int GetStartPos() const {return 0;}

		virtual const safestring& GetFileName() const;

		virtual bool DoReplace(size_t pos, size_t deletelen, const safestring& replacement, bool notify_listeners);

	protected:  // implementation
		TempText(const safestring& textname, safestring& text, int startline, PResolvePoint resolvepoint);

	private: // data
		int startline_;
	};

} // namespace

#endif
