

#ifndef _IMACRO_DATABLOCK_H_INCLUDED_
#define _IMACRO_DATABLOCK_H_INCLUDED_

#ifdef _MSC_VER
#pragma warning(disable: 4275)
#endif
#include <vector>

namespace NIMacro {

// Forward declarations
class DataItem;
class DI_IMacro DataBlockCallback;
typedef RefcountedPointer<DataBlockCallback> PDataBlockCallback;
class ItemVisitor;
class Instance;
typedef ThreadSharablePointer<Instance> PInstance;
class DataBlock;
typedef ThreadSharablePointer<DataBlock> PDataBlock;
struct ExecutionContext;

/// The character appearing in the beginning of debug info item names in the datablock.
const char debug_name_char='>';

/**
* DataBlock is essentially a couple of Containers plus specific access methods.
* An important difference is in that Container is considered immutable inside modules and every
* piece of code should use Container::Clone() or Container::CloneToBranch() before making changes. DataBlock instead is usually
* continuously changing during script run.
*
* Another difference is in the transaction support. Items put into the DataBlock are
* by default not visible before Commit() is called.
*
* There is no multithreading support in the DataBlock except of PDataBlock smartpointer,
* DataBlock methods can be called in one thread at a time only. For passing over to another thread
* one has to make a copy with the Clone() method.
*/
class DI_IMacro DataBlock: public ThreadSharable {
	typedef ThreadSharable super;
	class CallBacks;

public: // typedefs
	typedef Container map_t;
	typedef Container::const_iterator const_iterator;
	typedef Container::iterator iterator;

public: // interface
	// CONSTRUCTION-DESTRUCTION

	// Change 27.04.2005: make writethrough true by default, should be helpful for RMCA people.
	/** The only method to create a new DataBlock.
	* @param writethrough Suppress the transaction support - see Commit().
	*/
	static PDataBlock Create(bool writethrough=true);

	/// Returns a common empty DataBlock. It is not allowed to change it.
	static DataBlock& Void();

	/// Dtor
	~DataBlock();

	/// Return the number of committed items.
	int size() const {return data->Length();}

	/// Return the number of committed plus number of uncommited items.
	int Length() {return data->Length() + transaction->Length();}

	/// Find out if the datablock is in the writethrough mode.
	bool IsWriteThrough() const {return writethrough_;}

	// Append the names of the committed data items to a. Return the number of appended items.
	int List(safestringlist& a);

	/// Return true if both datablocks contain the same elements and all these compare equal.
	bool operator==(const DataBlock& b) const;

	/* Make a copy of DataBlock. All contained DataItems are duplicated. Pointed MemBlocks are not duplicated.
	* @param creator If different from -1, flush all creator module numbers to that value. Value -2 indicates that
	*		data item is created before entering the (sub)script.
	*/
	PDataBlock Clone(int creator=-1) const;

	/// Make and return a surface copy of the DataBlock committed data container. All contained MemBlock/Sharable items will remain in shared use.
	PContainer ToContainer() const;

	// scanning by iterators

	/// Return iterator to the committed data container begin
	const_iterator begin() const {return data->begin();}

	/// Return iterator to the committed data container end
	const_iterator end() const {return data->end();}

	/// Return iterator to the committed data container begin
	iterator begin() {return data->begin();}

	/// Return iterator to the committed data container end
	iterator end() {return data->end();}

	// Scanning by visitors

	/// Scan all items and apply visitor to each of them.
	void ScanItems( ItemVisitor& Visitor);

	/// Scan all MemBlock data items (conforming to pszClassName, if given), and call Visitor.Visit() for all of them.
	bool ScanItems( MemBlockVisitor& Visitor, const char* pszClassName=NULL);

	/** Represent the data block as a SET() module call. See also Data2Script module.
	* @param iMode	iMode==0: include only items fully representable in the script.
					iMode==1: include MemBlocks in star-notation and current memory pointers.
	*/
	safestring ToString(int iMode=0) const;

	/**
	* Finds bracketed pieces in the expression, evaluates these in the context
	* of this datablock and replaces bracketed expressions with the evaluation results.
	* For example, "vector[i]" might be changed to "vector.42".
	* Any strings not valid as subkeys will be converted to @-notation.
	* @param ctx Parent execution context for evaluating expressions containing function-style module calls.
	* @param expression The studied and modified expression.
	* @param throwhandler The throw handler is consulted in case of evaluation errors.
	*/
	void EvaluateBrackets(ExecutionContext& ctx, safestring& expression, ThrowHandlerFunc throwhandler=ThrowIndeed) const;

	/**
	* Verify the internal consistency of the datablock and all contained objects.
	* @param errmsg Any error messages are added to this parameter.
	* @return 0 in case of success, error code in case of any inconsistencies found.
	*/
	unsigned int Verify(safestring& errmsg);

	/**
	* Search for a MemBlock hierarchy object in this datablock and in deep recursion of other contained MemBlock items.
	* Appends the name(s) (relative to this datablock) of found locations to the names parameter.
	* @param addr The address of the MemBlock or a Vector-derived object data buffer (in the latter case the found name will end with ".@buffer").
	* @param names Found location names are added to this parameter, separated by commas.
	* @param pref Each found name is prefixed by pref in the output.
	*/
	void GetItemName(const MemBlock* addr, safestring& names, const safestring& pref) const;

	/**
	* Fetch item from the "system" container of this DataBlock. Return true if found.
	* The item has to be have stored earlier by SetSysItem().
	* Normally only the system container of the DataBlock associated with the the top-level execution context is used.
	* @param toplevelname Item top-level name (container key) in the system container. Any dots in the name are not interpreted.
	* @param item If found, the item is copied to here.
	* @return True, if the item was found.
	*/
	bool GetSysItem(const safestring& toplevelname, DataItem& item) const;

	/**
	* Fetch item from the "system" container of this DataBlock.
	* The item has to be have stored earlier by SetSysItem().
	* Normally only the system container of the DataBlock associated with the the top-level execution context is used.
	* @param toplevelname Item top-level name (container key) in the system container. Any dots in the name are not interpreted.
	* @return The found item value or Undefined.
	*/
	DataItem GetSysItem(const safestring& toplevelname) const {return sys_? sys_->Get(toplevelname): DataItem();}

	/**
	* Sets an item in the "system" container of this DataBlock.
	* Normally only the system container of the DataBlock associated with the the top-level execution context is used.
	* @param toplevelname Item top-level name (container key) in the system container. Any dots in the name are not interpreted.
	* @param item The item value.
	*/
	void SetSysItem(const safestring& toplevelname, const DataItem& item);

	// ITEM LEVEL DATA QUERIES

	/// Test existence of the top-level item in the datablock. Item name cannot contain subkeys.
	bool ExistsTopLevel(const safestring& toplevelname) const;

	/** Resolve a item name or expression and return the item value.
	* This function is of limited functionality, do not use it if the expression structure is unknown - use the GetItem() version with an ExecutionContext argument.
	*
	* @param itempath The name of the item. This can contain subkeys and bracketed expressions.
	*			Expressions are evaluated in the context of this DataBlock.
	*			In case of errors an exception is thrown.
	*			Some predefined constant names like "pi", "oo" can appear here as well.
	*			The name can begin with the token "_", this is identified with this datablock itself.
	*           Function-style local module calls and __line__ et. al. keywords are not supported.
	* @param item The found or calculated data item is stored here, if found.
	* @return True, if item found.
	*/
	bool GetItem(const safestring& itempath, DataItem& item) const;

	/** Same as GetItem(const safestring&, DataItem&), but has an additonal throw handler argument.
	* This function is of limited functionality, do not use it if the expression structure is unknown - use the GetItem() version with an ExecutionContext argument.
	* @param itempath Item path name.
	* @param item The result value, if any.
	* @param throwhandler In case of errors in subkeys or bracketed expressions the throwhandler is called. If the item is just not existing, the throwhandler is not called.
	* @return True, if item found.
	*/
	bool GetItem(const safestring& itempath, DataItem& item, ThrowHandlerFunc throwhandler) const;

	/**
	* The most general expression evaluation and data retrieval function from the DataBlock.
	* @param ctx Current execution context. This is used for module name lookup when evaluating function-style module calls, as well as for resolving __file__, __line__, __script__ and __text__ keywords.
	* @param itempath The data item name, or an expression to evaluate in the context of this datablock. The expression can contain subkeys and bracketed expressions, which are first evaluated also in the context of this datablock.
	* @param item The result value, if any.
	* @param throwhandler In case of errors in subkeys or bracketed expressions the throwhandler is called. If the item is just not existing, the throwhandler is not called.
	* @return True in case of success. False, if there was no such subitem, or if throwhandler was called, but did not want to throw an exception.
	*/
	bool GetItem(ExecutionContext& ctx, const safestring& itempath, DataItem& item, ThrowHandlerFunc throwhandler=ThrowIndeed) const;

	/// The same as GetItem(const safestring&, DataItem&), but returns directly the found value or Undefined.
	DataItem GetItem(const safestring& itempath) const;

	/** Find a top-level item. Returns iterator to that item if found, otherwise this->end().
	* @param toplevelname The name of the top-level item to find. No subkeys or predefined keywords ("_", "pi", etc.) are supported.
	*/
	const_iterator FindTopLevelItem(const safestring& toplevelname) const;

	/// Essentially same as GetItem(const safestring&), retained for backward compatibility.
	SafeValue GetSafeValue(const safestring& itempath) const {
		DataItem it; GetItem(itempath, it); return it;
	}

	/// Return subitem representation or 0, if the item does not exist. May throw exceptions if itempath contains invalid bracketed expressions, is invalid itself or the found item is not convertible to an int.
	int GetInt(const safestring& itempath) const {
		DataItem it; GetItem(itempath, it); return it? it.GetInt(): 0;
	}

	/// Return subitem in double representation or 0.0. May throw exceptions if itempath contains invalid bracketed expressions, is invalid itself or the found item is not convertible to a double.
	double GetDouble(const safestring& itempath) const {
		DataItem it; GetItem(itempath, it); return it? it.GetDouble(): 0.0;
	}

	/// Return subitem in MemBlock representation or NULL. May throw exceptions if itempath contains invalid bracketed expressions, is invalid itself or the found item is not convertible to a MemBlock.
	PMemBlock GetMemory(const safestring& itempath) const {
		DataItem it; GetItem(itempath, it); return it? it.GetMemory(): NULL;
	}

	/// Return subitem in specified MemBlock class representation or NULL. May throw exceptions if itempath contains invalid bracketed expressions, is invalid itself or the found item is not convertible to a MemBlock.
	PMemBlock GetMemory(const safestring& itempath, const char* classname) const;

	/// Return subitem in string representation or an empty string. May throw exceptions if itempath contains invalid bracketed expressions, is invalid itself or the found item is not convertible to a string.
	safestring GetString(const safestring& itempath) const {
		DataItem it; GetItem(itempath, it); return it? it.GetString(): safestring();
	}

	/// Return subitem converted to a void pointer or NULL. May throw exceptions if itempath contains invalid bracketed expressions, is invalid itself or the found item is not convertible to a pointer.
	const void* GetPointer(const safestring& itempath) const {
		DataItem it; GetItem(itempath, it); return it? it.GetPointer(): NULL;
	}

	/// Return Sharable smartpointer or NULL. May throw exceptions if itempath contains invalid bracketed expressions, is invalid itself or the found item is not a Sharable.
	PSharable GetSharable(const safestring& itempath) const {
		DataItem it; GetItem(itempath, it); return it? it.GetSharable(): NULL;
	}

	// CONTENT CHANGING

	// DATABLOCK LEVEL MANIPULATIONS

	/** For non-writethrough DataBlocks makes the changes visible. For writethrough DataBlocks does nothing.
	*
	* @param ctx The ctx.inst settings specify if the DataBlock consistency check has to be performed. If yes, then ctx also provides the context for reporting warnings (errors are reported by exceptions).
	* @param call_index The sequence number of the module call in the current Script, which is responsible for any changes in the DataBlock.
	*					If UpdateCreators==false, this is used only for data items having iCreator==-1 (update always).
	* @param update_creators Store the ModuleNo parameter in all changed/added items except those having iCreator==-2 (update never).
	*/
	void Commit(ExecutionContext& ctx, int call_index, bool update_creators=true);

	/// Deprecated, use another overload of Commit() in new code.
	void Commit(int ModuleNo, bool UpdateCreators=true);

	/// Forget any not-yet committed changes (clears the transaction container).
	void Rollback();

	/// Copy ctor, copies (clones) the data, transaction and system containers, plus any other attributes, except callbacks. Containers are only surface cloned. For deep copy use the Clone() method.
	DataBlock(const DataBlock& b);

	/// Assignment op, copies (clones) the data, transaction and system containers, other attributes (writethrough, callbacks, parent instance etc) remain unchanged. Containers are only surface cloned. For deep copy use the Clone() method.
	DataBlock& operator=(const DataBlock& b);

	/**
	* Copy the contents of this datablock into another datablock. Optionally replace the creator module numbers of committed data.
	* @param other Another dtablock to copy the contents into.
	* @param creator Replace item creator module numbers of commited data with this creator. Note that creator numbers are script-specific.
	*				 If you merge data to a datablock in another instance or for using with another script, then you should pass a suitable creator module number.
	*				 If you pass -1 (the default), the item creators remain unchanged.
	* @param IncludeTransaction Copy also transaction area, in addition to commited items. Note that uncommited items remain uncommited.
	* @param WarnOverwrites Report warnings about overwritten items.
	* @param resolveattrs Convert Attribute type items to contained Vector-s or Image-s before passing to other. This is relevant only for cell lists.
	*/
	void Merge(DataBlock& other, int creator=-1, bool IncludeTransaction=true, bool WarnOverwrites=false, bool resolveattrs=false) const;

	/** Copy the committed content of this datablock into a Container.
	* @param merge_to The container to copy the content into.
	* @param resolveattrs Convert Attribute type items to contained Vector-s or Image-s before passing to other. This is relevant only for cell lists.
	*/
	void Merge(PContainer merge_to, bool resolveattrs=false) const;

	/// Append data from a container into the DataBlock. Appended data is not Committed.
	void Append(const PContainer append_from, bool warn_overwrites=false);

	/// Makes a full physical copy of contained MemBlock type items.
	void MakeCopy();

	/// Delete both the commited and not yet commited data items.
	void Clear();

	/** Wrap a container into the DataBlock, no copying is made.
	* Note that Container may be modified by the DataBlock afterwards.
	* Previous DataBlock content is returned.
	* Items not currently committed are discarded.
	* No Commit() is required afterwards.
	*/
	PContainer Swap(PContainer b);

	/**
	* Change the content of two datablocks (committed and non-committed data areas)
	* Other attributes remain the same.
	*/
	void Swap(PDataBlock b);

	/**
	* Changes the writethrough property of the DataBlock - see the constructor description.
	* @param writethrough New writethrough mode setting.
	* @return Previous writethrough mode setting.
	*/
	bool SetWriteThrough(bool writethrough=true) {
		if (!writethrough_ && writethrough) {
			Commit(-1);
		}
		bool prev = writethrough_;
		writethrough_ = writethrough;
		return prev;
	}

	// ITEM MANIPULATION
	/**
	* Add, replace or delete a top-level item in the datablock. If the datablock is not writethrough,
	* the item is not visible before Commit().
	*
	* @param ItemName The name of the item. This must be single alphanumeric word, no subkeys allowed.
	*		If the name is "_", then the value must be a container, and the whole data container is replaced
	*		and immediately committed. The transaction area must be empty for that operation.
	* @param ItemValue The new value for the item. If Undefined, then the item is deleted.
	* @param WarnOverwrites If true and an item with this name already exists, a warning is issued
	*		to the parent Instance of this DataBlock, if any.
	*/
	void SetTopLevelItem(const safestring& ItemName, const DataItem& ItemValue, bool WarnOverwrites=false);

	// ITEM MANIPULATION
	/**
	* Add, replace or delete a top-level item or a subitem in the datablock. If the datablock is not writethrough,
	* the changes are not visible before Commit().
	*
	* @param itempath The name of the item. This may contain subkeys. Any expressions are evaluated in the context
	*		of this datablock. See another overload for finer control. In case of errors an Exception is thrown.
	* @param ItemValue The new value for the item. If Undefined, then the item is deleted.
	* @param WarnOverwrites If true and an item with this name already exists, a warning is issued
	*		to the parent Instance of this DataBlock, if any.
	*/
	void SetItem(const safestring& itempath, const DataItem& ItemValue, bool WarnOverwrites=false);


	/* Delete a top-level item from the datablock.
	* If the datablock is not writethrough, the changes are not visible before Commit().
	* For deleting subitems, see SetItem().
	* @param name Name of the top-level item, no subkeys allowed.
	*/
	void DelItem(const safestring& name) {
		// Allow for names starting by debug_name_char, for debugger support.
		DEBUG_ASSERT(is_alphanumeric(name) || (!name.empty() && name[0]==debug_name_char));
		SetTopLevelItem(name, Undefined);
	}

	/// Delete any existing items matching the name, containing the wildcard chars * and ?.
	void DelWildCard(const safestring& name);

	/// Delete item immediately, bypassing transaction area. The transaction area must be empty. Return true if datablock callbacks were notified.
	bool DelItemNow(const safestring& item);

	/**
	* Specific replacement function used by set/copy/keep/delete modules.
	* Adds, replaces or deletes data item in a datablock.
	* If the datablock is not writethrough, the item is not visible before Commit().
	*
	* @param ctx Current execution context
	* @param aState Datablock for evaluating any expressions and names.
	* @param Name The top-level data item name in bSate to be modified. This must be a regular alphanumeric name.
	*	Name "_" is considered to be a special case and is identified with the datablock itself.
	*	Name may be modified in this function; do not use it afterwards.
	* @param sSub The subkey for Name which is to be modified. If empty, the toplevel Name item
	*	will be replaced or deleted. The subkey may contain brackets and unevaluated expressions therein.
	*	These will be evaluated in the aState context. sSub may be modified in this function; do not use it afterwards.
	*	In case of errors an exception is thrown, if ctx.exec.CallSyntaxRun() returns false; otherwise errors are silently ignored.
	* @param x The value to be inserted or replaced. If this is Undefined, then the item or subitem will be deleted instead, if present.
	* @param warn_overwrites If true and a top-level item will be replaced, a warning is issued
	*		to the parent Instance of this DataBlock, if any.
	*/
	void SetItem(ExecutionContext& ctx, const DataBlock& aState, safestring& Name, safestring& sSub, const DataItem& x, bool warn_overwrites=false);

	/// Back-compatibility fn; use SetItem() instead.
	void SetSafeValue(const safestring& ItemName, const SafeValue& ItemValue, bool WarnOverwrites=false) {
		SetItem(ItemName, ItemValue, WarnOverwrites);
	}

	/**
	* Rename a top-level item of DataBlock. If the datablock is not writethrough,
	* the changes are not visible before Commit().
	* Returns true if item was found, false if item was not found.
	*
	* @param oldname The old name of the item. This must be single alphanumeric word, no subkeys allowed.
	* @param newname The new name of the item. This must be single alphanumeric word, no subkeys allowed.
	* @param WarnOverwrites If true and an item with this name already exists, a warning is issued
	*		to the parent Instance of this DataBlock, if any.
	*/
	bool RenameItem(const safestring& oldname, const safestring& newname, bool WarnOverwrites=false);

	// CALLBACK SUPPORT

	/** Register a callback. Callback methods will be called whenever
	* data block state is changed or data items are queried for.
	* Datablock takes over the deletion of the callback -
	* callback will be deleted together with the DataBlock (unless it overrides DataBlockDeleted() virtual method).
	* Multiple callbacks may be registered at the same time for a DataBlock.
	* A single callback may be registered only by one DataBlock.
	*/
	void AddCallback(PDataBlockCallback callback);

	/// Unregister the callback from DataBlock (and also avoid it's automatic deletion).
	void RemoveCallback(PDataBlockCallback callback);

	/// Return a vector containing pointers to the currently registered callbacks.
	void ListCallbacks(std::vector<PDataBlockCallback>& buffer);

	/// A helper class for disabling callbacks temporarily, create a local scoped instance of this class for doing that.
	class DisableCallbacks {
	public:
		DisableCallbacks(DataBlock& db): db_(db), cb_(db.callbacks_) {db_.callbacks_=NULL;}
		~DisableCallbacks() {db_.callbacks_=cb_;}
	private:
		DataBlock& db_;
		DataBlock::CallBacks* cb_;
	};

public:
	/// Return internal pointer to the commited data container. Note that this Container may be later changed by the DataBlock.
	PContainer GetContainer() const {return data;}

	/// Return internal pointer to the not-yet-commited data container. Note that this Container may be later changed by the DataBlock.
	PContainer GetTransaction() const {return transaction;}

	/// Return internal pointer to the system data container. Note that this Container may be later changed by the DataBlock.
	PContainer GetSys() const {return sys_;}

	/// While delay_commit flag is raised, Commit() calls are ignored.
	void SetDelayCommit(bool delay_commit) {delay_commit_ = delay_commit;}

	/// Report if delay_commit flag is raised.
	bool GetDelayCommit() const {return delay_commit_;}

	/// Force datablock consistency check after the next module run, if consistencychecks are on. By default the check is run only if the datablock content changes.
	void SetVerifyNeeded() {verify_needed_ = true;}

	/// Copy all callback pointers from the parent datablock, if any.
	void CopyCallBacksFrom(const DataBlock& parent);

protected: // virtual overrides
	virtual PSharable DoClone() const;
	virtual SafeValue DoVerb(const char* verb, const SafeValue& arg1, const SafeValue& arg2);
private:
	// Implementation
	DataBlock(bool writethrough);

	/// Convert c into DataBlock. All items are copied.
	DataBlock(const Container& c);

	bool GetItem_internal(const safestring& toplevel_name, const safestring& subkey, DataItem& item, ThrowHandlerFunc throwhandler=ThrowIndeed) const;

	const DataItem& FindItemInTransaction(const safestring& ItemName) const;

private: // data

	PContainer data;			// The map of commited name-value pairs
	PContainer transaction;		// The map of not yet committed name-value pairs.
	PContainer sys_;			// System area

	// Callback support
	class CallBacks;
	CallBacks* callbacks_;
	friend class DisableCallbacks;

	bool writethrough_, verify_needed_, delay_commit_;
};

/**
* A base class for DataBlock callback classes.
* One can add callbacks to the datablocks.
* The datablock will call callback virtual member functions when specific events occur.
* The callback will override one or more functions to implement specific behaviour.
*
* For adding a callback to the 'current' DataBlock during the script run, override the
* Executive::GetDataBlockCallback() virtual member function and run the script under the
* control of overridden Executive.
*
* The content changing callbacks (ItemCreated(), ItemChanged(), ItemDeleted()) will be called
* at the time when the changes of the DataBlock become visible. In case of non-writethrough
* datablocks this happens at the time of DataBlock::Commit(), usually executed after the module run.
* In case of writethrough datablocks the callbacks are called immediately.
*
* By default, the callbacks are expected to be allocated dynamically (with the 'new' operator)
* and owned by the DataBlock. When the DataBlock dies, it will delete all attached callbacks.
* (But see the DataBlockDeleted() member function below how to override this).
* This also means that one callback can be generally attached to a single datablock at a time.
*
* Note that using callbacks will affect the performance of the DataBlock.
*/
class DI_IMacro DataBlockCallback: public Refcountable {
public:
	DataBlockCallback() {} // : db_(NULL) {}
	virtual ~DataBlockCallback() {}

	/**
	* Called when a new item is inserted in the DataBlock.
	* The default implementation calls ItemChanged, passing Undefined as the old value.
	* The name is always a top-level data item name.
	* If the DataBlock is not write-through, this callback is called only during DataBlock::Commit().
	*/
	virtual void ItemCreated(const char* name, const DataItem& value) {ItemChanged(name, value, Undefined);}

	/**
	* Called when an item is deleted from the DataBlock.
	* This callback is called only if the specified item exists in the datablock.
	* The name is always a top-level data item name.
	* If the DataBlock is not write-through, this callback is called only during DataBlock::Commit().
	*/
	virtual void ItemDeleted(const char* name, const DataItem& value) {}

	/*
	* Called when the value of the item is changed.
	* This is called also for newly created items, unless ItemCreated() is overridden.
	* The name is always a top-level data item name.
	* If the DataBlock is not write-through, this callback is called only during DataBlock::Commit().
	*
	* If the name is "_" and DataBlock is not write-through, then the ItemChanged() callback is called once for "_",
	* after that the "_" container contents are placed in the transaction area and committed again,
	* thus calling ItemCreated() or ItemChanged() callbacks again for each value.
	*/
	virtual void ItemChanged(const char* name, const DataItem& newvalue, const DataItem& oldvalue) {}

	/**
	* Called when a data item or subitem value is copied out of the DataBlock.
	* This callback may be preceded by a ItemQueried() callback.
	* The name may designate a subitem, i.e. contain subkeys and indices.
	* In that case the passed value is also that of subitem.
	*/
	virtual void ItemFetched(const char* name, const DataItem& value) {}

	/**
	* Called when a data item is searched for, or it's existence checked.
	* The name is always a top-level item name.
	* This call may be followed by an ItemFetched() call, which may designate a subitem of this top-level item.
	*/
	virtual void ItemQueried(const char* name, bool exists) {}

	/**
	* Will be called when a datablock is deleted.
	*/
	virtual void DataBlockDeleted() {}

	/// Finds first callback in the callback chain which can be cast to type T. Returns NULL if none found.
	template<class T> T* FindCallback();

};

/**
* Special exception for processing "default" keyoword:
* see DataBlock::GetItem() and ExplicitArgument::Send().
*/
class DefaultParameterValue: public Exception {
public:
	DefaultParameterValue(): Exception(ERR_MACROSYNTAX, "Illegal use of 'default' keyword in the script.") {}
};

/** InputStreamEx is an enhanced version of InputStream.
* It can be used only during module run.
* It provides a transparent support of Executive-overriden file redirection
* and for current in-memory data item redirection.
* See "Acapella Data Exchange.doc"  for more details
*
* The InputStreamEx class objects cannot be allocated dynamically by the new operator.
*/
class DI_IMacro InputStreamEx: public InputStream {
public:
	/**
	* @param filename Allowed filenames are the same as for InputStream, plus "<itemname" and "<<itemname" data item references.
	*                 In this case the corresponding data item is read from ctx.db and it's content is used.
	*				  All names, except data item references and memory region, are
	*				  checked by the executive for possible redirections.
	* @param ctx Execution context providing support for vector and string redirection, and for Executive overrides.
	* @param autounzip Automatically unzip gzip-compressed files.
	*/
	InputStreamEx(const safestring& filename, ExecutionContext& ctx, bool autounzip=true);
	~InputStreamEx();
private: // implementation
	InputStream::InitData HandleRedirection(const safestring& filename, ExecutionContext& ctx);
	void* operator new(size_t sz); // unimplemented - use only automatic objects of InputStreamEx!
	void* operator new[](size_t sz); // unimplemented - use only automatic objects of InputStreamEx!
private: // data
	bool redirected_;
	ExecutionContext& ctx_;
};

/**
* OutputStreamEx is an enhanced version of OutputStream,
* which automatically saves string and vector output items in the datablock.
* Also special name "console" is handled and the output is sent to the console
* during dtor call.
*
* Note: ctx and db parameters must be valid during the lifetime of OutputStreamEx.
* I.e. one should only create local OutputStreamEx objects.
*/
class DI_IMacro OutputStreamEx: public OutputStream {
	typedef OutputStream super;
public:
	/**
	* Constructor.
	* @param filename Allowed filenames are the same as for OutputStream, plus "<itemname" and "<<itemname" data item references.
	*                 In the latter cases the file content is stored in the datablock as a string or a vector, correspondingly.
	*				  All names, except data item references, are
	*				  checked by the executive for possible redirections.
	* @param db The datablock where the resulting string or vector data items will be stored by the OutputStreamEx destructor.
	* @param ctx Execution context providing support for Executive redirections and console output.
	* @param append Append content to the existing file. If the file does not exist, it is created.
	*/
	OutputStreamEx(const safestring& filename, DataBlock& db, ExecutionContext& ctx, bool append=false);
	~OutputStreamEx();
	void Close();
private: // implementation
	void* operator new(size_t sz); // unimplemented - use only automatic objects of OutputStreamEx!
	void* operator new[](size_t sz); // unimplemented - use only automatic objects of OutputStreamEx!
#if defined(_MSC_VER) && defined(_DEBUG)
	void* operator new(size_t nSize, int blocktype, const char* lpszFileName, int nLine); // unimplemented - use only automatic objects of OutputStreamEx!
	void* operator new[](size_t nSize, int blocktype, const char* lpszFileName, int nLine); // unimplemented - use only automatic objects of OutputStreamEx!
#endif
	void Cleanup();
private: // data
	DataBlock& db_;
	ExecutionContext& ctx_;
	bool append_;
};

} //namespace NIMacro
#endif
