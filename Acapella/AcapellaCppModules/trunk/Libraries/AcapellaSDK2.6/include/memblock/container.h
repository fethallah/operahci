// Acapella Container class
#ifndef _CONTAINER_H_INCLUDED_
#define _CONTAINER_H_INCLUDED_

#include "memblock.h"

namespace NIMacro {

class DI_MemBlock Container;
typedef DerivedPointer<Container> PContainer;

typedef std::map<Nbaseutil::safestring, SafeValue> container_map_t;

/// An iterator class for iterating over container contents
class DI_MemBlock ContainerIterator: public Nbaseutil::mb_malloced {
public:

	/// Default ctor
	ContainerIterator() {}

	/// Ctor
	ContainerIterator(container_map_t::iterator it): it_(it) {}

	/// Return the item name
	const Nbaseutil::safestring& Label() {return it_->first;}

	/// Return the data item
	SafeValue* operator->() {return &it_->second;}

	/// Return the data item
	const SafeValue* operator->() const {return &it_->second;}

	/// Return the data item
	SafeValue& operator*() {return it_->second;}

	/// Return the data item
	const SafeValue& operator*() const {return it_->second;}

	const ContainerIterator& operator++() const {
		++it_;
		return *this;
	}
	void operator++(int dummy) const {++(*this);}

	bool operator!=(const ContainerIterator& b) const {return it_!=b.it_;}
	bool operator==(const ContainerIterator& b) const { return it_==b.it_;}

	container_map_t::iterator Iterator() const {return it_;}

private: // data
	mutable container_map_t::iterator it_;
};

// The protected implementation classes.
class ContainerImpl;
class OrderedImpl;

/// A MemBlock hierachy class for holding a collection of named data items. 
/** Container is implemented by std::map internally, 
* hence it has the same complexity guarantees.
* The held data type is SafeValue, not DataItem, by historical reason.
* However, as DataItem-s and SafeValue-s are seamlessly converted into each other
* the client code may work with DataItem's instead.
*
* A container can be in two modes: unordered and ordered. Actually "unordered" means
* a "natural" alphabetic order of keys, imposed by the underlying std::map representation.
* An ordered container maintains an additional extra ordering of the keys. This extra ordering
* takes a bit more processing, but allows for O(1) access by index, among other things.
*
* Container supports special key syntax for inserting keys in a specific position. The syntax reads e.g.:
*   - set(x.e:@first = 6)
*   - set(x.b:@last = 5)
*   - set(x.f:@before:e = 8)
*   - set(x.b:@after:a = 5)
* The container methods which support this syntax are SetSubItem() (supports also dotted paths)
* and Insert() (supports only single names and position qualifiers).
*/
class DI_MemBlock Container: public MemBlock {
	typedef MemBlock super;

public: // static interface

	/// Creates an empty container. See also Cnt() functions.
	static PContainer Create();

	/// Creates an empty ordered container.
	static PContainer CreateOrdered();

public:
	/// Provide iterator typedef
	typedef ContainerIterator iterator;
	/// Provide iterator typedef
	typedef ContainerIterator const_iterator;

// Container methods:
	/**
	* Replaces or creates a top-level item in the container.
	* @param name Top-level item name. If the container will be visible at the script level, then the name
	*	must be a valid Acapella data item name (is_alphanumeric() is true). 
	*   If the container is for internal use, then any string can be used as a name.
	*   See Nbaseutil::QuoteName() about how to create valid Acapella names.
	* @param value Item value. If it is Undefined, then this call is equivalent to Delete(name). 
	*        If the value is a complicated data structure, then it may not
	*		 contain pointers to this container (no cyclic data structures allowed!).      
	*/
	void SetTopLevelItem(const Nbaseutil::safestring& name, const DataItem& value) {
		Put(value, name);
	}

	/**
	* @brief NB! Usage of this function is not encouraged because of the illogical order of parameters due to historic reasons, use SetTopLevelItem() or SetSubItem() instead.
	* @param item Item value. The value should not be Undefined.
	* @param label Top-level item name. If the container will be visible at the script level, then the name
	*			must be a valid Acapella data item name (see is_alphanumeric()). If the container is for internal use, then any string can be used as a name.
	*/
	virtual void Put(const SafeValue& item, const Nbaseutil::safestring& label);
	
	/// Moves item into the container under specified label. The item object will be Undefined afterwards.
	void MoveInto(const Nbaseutil::safestring& label, SafeValue& item);

	/**
	* @brief Fetch an item from the Container by name.
	* @param label Top-level item name.
	* @return Item value, or Undefined if the item does not exist.
	*/
	virtual SafeValue Get(const Nbaseutil::safestring& label) const;

	/**
	* Return PMemBlock pointer if specified subitem exists and is of MemBlock type. 
	* Otherwise return NULL.
	* @param ItemName Top-level item name.
	* @param classname If present, then the function returns non-NULL only in case the object
	*        conforms to the specified class.
	*/
	PMemBlock GetMemory(const Nbaseutil::safestring& ItemName, const char* classname=NULL) const; // CCPTR_OK

	/// Return the number of top-level items in the container.
	int	Count() const;

	/// Delete a top-level item by name. If there is no such item, does nothing. See also: erase().
	virtual void Delete(const Nbaseutil::safestring& label);

	/// Delete \a count items, starting at position \a index. This is O(count) for ordered containers, O(N) otherwise.
	void DeleteAt(unsigned int index, unsigned int count);

	/// Same as Count().
	int			Length() const;

	/// Return STL-style iterator to the beginning of the container. The iteration is done in alphabetic order of item names.
	iterator begin() const;

	/// Return STL-style iterator to one-past-end of the container.
	iterator end() const;

	/**
	* @brief Searches for a top-level item in the container.
	* @return Iterator to the found item, or end() if not found.
	*/
	iterator find(const Nbaseutil::safestring& label) const;

	/// Searched for a top-level item in the container, returns true if found.
	bool exists(const Nbaseutil::safestring& label) const;

	/// Deletes a single top-level item from the container. See also Delete().
	virtual void erase(iterator it);

	/// Deletes all items from the container.
	virtual void clear();

	/**
	* Scan all top-level items and apply visitor to each of them.
	* @param visitor A visitor object derived from ItemVisitor class.
	*/
	void ScanItems(ItemVisitor& visitor) const;

	/**
	* @brief Scans all top-level memblock type items in the Container.
	* 
	* The Visitor's Visit() method is called for each item.
	* @param Visitor A visitor object derived from the NIMacro::MemBlockVisitor class.
	* @param pszClassName If specified, then only memblocks conforming to the 
	*			specified class are processed.
	*/
	void ScanItems(MemBlockVisitor& Visitor, const char* pszClassName=NULL); // CCPTR_OK
	
	/**
	* Scan all top-level memblock type items and call Visitor for
	* each of them. 
	* @param Visitor A visitor object derived from the MemBlockVisitor class.
	* @param pszClassName If given, then only memblocks conforming to the 
	*			specified class are considered.
	*/
	void ScanItems(MemBlockVisitor& Visitor, const char* pszClassName=NULL) const; // CCPTR_OK

	/// Fast swap of the content with another Container.
	virtual void Swap(Container& b);

	/// Add or replace top-level items from another container. See also: Merge().
	void Append(const PContainer from);

	/// Merge another container. Matching subcontainers are merged recursively. See also: Append().
	void Merge(const PContainer from);

	/// Compose a list of top-level item names.
	int	List(Nbaseutil::safestringlist& a) const;

	/// Change the container type to "ordered" container or not.
	void SetOrdered(bool ordered);

	/// Report if the container is ordered or not.
	bool IsOrdered() const;

	/**
	* Return element by the index from an ordered or unordered container. If the container is not ordered,
	* the method is slower (O(N)) and uses alphabetical order of the item names.
	* If index is out of bounds, an exception is thrown.
	*/
	SafeValue GetAt(unsigned int index) const;

	/**
	* Return iterator to an element by the index from an ordered or unordered container. If the container is not ordered,
	* the method is slower (O(N)) and uses alphabetical order of the item names.
	* If index is out of bounds, \c end() is returned.
	*/
	iterator GetIterAt(unsigned int index) const;

	/**
	* Return the order position of the pointed element, or a number equal to Length(), if iter==end().
	* This operation is O(N) for all containers.
	* @param iter A valid iterator to an element in the container, or a value equal to the end().
	*/ 
	unsigned int GetPos(iterator iter) const;

	/**
	* Return the order position of the element by the label, or a number equal to Length(), if not found.
	* This operation is O(N) for all containers.
	*/
	unsigned int GetPos(const Nbaseutil::safestring& label) const {
		return GetPos(find(label));
	}

	/// Return item name by position. This is O(1) for ordered containers and O(N) for unordered containers.
	Nbaseutil::safestring GetNameByPos(int pos) const;

	/**
	* Replace the element at the index by a new one. 
	* If index is out of range or the name collides with another name in the container,
	* an exception is thrown.  If the container is not ordered, it is converted to an ordered container.
	* @param index The position in the container to replace. This must be less than or equal to the current container size. 
	*			In the latter case a new element is added.
	* @param new_name New element name. This can coincide with the previous name at the same position, but not with any other names - 
	*			in the latter case an exception is thrown.
	* @param value The new value.
	*/
	void ReplaceAt(unsigned int index, const Nbaseutil::safestring& new_name, const SafeValue& value);

	/**
	* Replace the element value at the index by a new one. 
	* If index is out of range, an exception is thrown.
	* If the container is not ordered, it is converted to an ordered container.
	*/
	iterator ReplaceValueAt(unsigned int index, const SafeValue& value);

	/**
	* Insert a new element in the container. If the container is not ordered, it is converted to an ordered container.
	* @param index The new element is inserted before the specified index. If index>=Count(), the element is added to the end of the ordered sequence.
	* @param name Name of the inserted element. If the name collides with an existing name in the container, an exception is thrown.
	* @param value The element value.
	* @return An iterator to the newly inserted item.
	*/
	iterator InsertAt(unsigned index, const Nbaseutil::safestring& name, const SafeValue& value);

	/// Insert or replace an item. The location is a simple name or in Acapella special syntax, e.g. "b:@before:a".
	iterator Insert(const Nbaseutil::safestring& location, const SafeValue& value);

	///@cond implementation_details
	/// Return reference to the item name by position. This is O(1) for ordered containers and O(N) for unordered containers.
	const Nbaseutil::safestring& GetNameByPosRef(int pos) const;
	///@endcond

	/**
	* Swaps the positions of elements in an ordered container. Changes the container to an ordered container, if it was not.
	* The parameters must be in range [0..Length()-1], otherwise an exception is thrown.
	*/
	void SwapElements(unsigned int k1, unsigned int k2);

public: // Overridden virtuals
	virtual const char*		Class() const {return "container";} // CCPTR_OK
	virtual PMemBlock		DoClone(copymode_t copy_mode) const;
	virtual bool			Conforms(const char *szClassName) const;
	virtual bool			Consistent(Nbaseutil::safestring& msg, bool CheckContent=true) const;	
	virtual bool			AddConvertArg(const Nbaseutil::safestring& option, const DataItem& value, ConvertOptions& opt, Nbaseutil::ThrowHandlerFunc throwhandler=LogWarnings()) const;
	virtual PMemBlock		ConvertFrom(const PMemBlock source, const ConvertOptions& opt) const;
	virtual PMemBlock		ConvertTo(const Nbaseutil::safestring& classname) const;
	virtual SafeValue		DoVerb(const char* pszCommand, int iArg, SafeValue vArg[]); // CCPTR_OK
	virtual bool			Entertain(MemBlockVisitor& visitor, const Nbaseutil::safestring& name, entertainmode_t mode=enter_deep);
	virtual void			IterateChildren(NIMacro::AcaVisitor& visitor, const NIMacro::TraverseNode& node);
	virtual SafeValue		ResolveSubItem(const Nbaseutil::safestring& subitemname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed) const;
	virtual const char*		ClassDescription() const; // CCPTR_OK

protected: // Overridden virtuals
	virtual bool DoSetSubItem(const Nbaseutil::safestring& subitemname, const SafeValue& item, const Nbaseutil::safestring& fullpathname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed);
	virtual Nbaseutil::safestring DoGetDescription() const;
	virtual void DoSerialize(int ser_format, SerializeVisitor& visitor, const Nbaseutil::safestring& itemname, const Formatter& formatter, Nbaseutil::safestring& buffer) const;
	virtual bool Equals(const MemBlock& b) const; // value compare to another item b, which must be of the same dynamic type. See operator==()

protected: // implementation
	/// Creates an empty container
	Container(bool is_ordered);
	Container(ContainerImpl* impl);
	virtual ~Container();

private: // implementation
	int Size() const; // override MemBlock::Size(), this is of no use here and is not implemented, call Count() instead.
	void SerializeAcapella(SerializeVisitor& visitor, const Nbaseutil::safestring& itemname, const Formatter& formatter, Nbaseutil::safestring& buffer) const;
	OrderedImpl& GetOrderedImpl();
	OrderedImpl& GetOrderedImplIfOrdered() const;
	void ListItems(Nbaseutil::safestring& buffer) const;
	void ListItems(Nbaseutil::safestring* namearray) const;
	void ListValues(Nbaseutil::safestring* valarray) const;
private: // data
	ContainerImpl* impl_;	// the implementation
private: // unimplemented functions
	void Get(int) const; // not implemented, it is here to avoid conversion of NULL pointer to a safestring.
};

/// Convenience function for creating an empty container.
inline PContainer Cnt() {return Container::Create();}

/// Convenience function for packing a small number of items into a container
DI_MemBlock PContainer Cnt(const Nbaseutil::safestring& name1, const SafeValue& val1);

/// Convenience function for packing a small number of items into a container
DI_MemBlock PContainer Cnt(const Nbaseutil::safestring& name1, const SafeValue& val1,
			   const Nbaseutil::safestring& name2, const SafeValue& val2);

/// Convenience function for packing a small number of items into a container
DI_MemBlock PContainer Cnt(const Nbaseutil::safestring& name1, const SafeValue& val1,
			   const Nbaseutil::safestring& name2, const SafeValue& val2,
			   const Nbaseutil::safestring& name3, const SafeValue& val3);

/// Convenience function for packing a small number of items into a container
DI_MemBlock PContainer Cnt(const Nbaseutil::safestring& name1, const SafeValue& val1,
			   const Nbaseutil::safestring& name2, const SafeValue& val2,
			   const Nbaseutil::safestring& name3, const SafeValue& val3,
			   const Nbaseutil::safestring& name4, const SafeValue& val4);

DI_MemBlock PContainer Cnt(const Nbaseutil::safestring& name1, const SafeValue& val1,
			   const Nbaseutil::safestring& name2, const SafeValue& val2,
			   const Nbaseutil::safestring& name3, const SafeValue& val3,
			   const Nbaseutil::safestring& name4, const SafeValue& val4,
			   const Nbaseutil::safestring& name5, const SafeValue& val5);

DI_MemBlock PContainer Cnt(const Nbaseutil::safestring& name1, const SafeValue& val1,
			   const Nbaseutil::safestring& name2, const SafeValue& val2,
			   const Nbaseutil::safestring& name3, const SafeValue& val3,
			   const Nbaseutil::safestring& name4, const SafeValue& val4,
			   const Nbaseutil::safestring& name5, const SafeValue& val5,
			   const Nbaseutil::safestring& name6, const SafeValue& val6);

DI_MemBlock PContainer Cnt(const Nbaseutil::safestring& name1, const SafeValue& val1,
			   const Nbaseutil::safestring& name2, const SafeValue& val2,
			   const Nbaseutil::safestring& name3, const SafeValue& val3,
			   const Nbaseutil::safestring& name4, const SafeValue& val4,
			   const Nbaseutil::safestring& name5, const SafeValue& val5,
			   const Nbaseutil::safestring& name6, const SafeValue& val6,
			   const Nbaseutil::safestring& name7, const SafeValue& val7);

DI_MemBlock PContainer Cnt(const Nbaseutil::safestring& name1, const SafeValue& val1,
			   const Nbaseutil::safestring& name2, const SafeValue& val2,
			   const Nbaseutil::safestring& name3, const SafeValue& val3,
			   const Nbaseutil::safestring& name4, const SafeValue& val4,
			   const Nbaseutil::safestring& name5, const SafeValue& val5,
			   const Nbaseutil::safestring& name6, const SafeValue& val6,
			   const Nbaseutil::safestring& name7, const SafeValue& val7,
			   const Nbaseutil::safestring& name8, const SafeValue& val8);

DI_MemBlock PContainer Cnt(const Nbaseutil::safestring& name1, const SafeValue& val1,
			   const Nbaseutil::safestring& name2, const SafeValue& val2,
			   const Nbaseutil::safestring& name3, const SafeValue& val3,
			   const Nbaseutil::safestring& name4, const SafeValue& val4,
			   const Nbaseutil::safestring& name5, const SafeValue& val5,
			   const Nbaseutil::safestring& name6, const SafeValue& val6,
			   const Nbaseutil::safestring& name7, const SafeValue& val7,
			   const Nbaseutil::safestring& name8, const SafeValue& val8,
			   const Nbaseutil::safestring& name9, const SafeValue& val9);

DI_MemBlock PContainer Cnt(const Nbaseutil::safestring& name1, const SafeValue& val1,
			   const Nbaseutil::safestring& name2, const SafeValue& val2,
			   const Nbaseutil::safestring& name3, const SafeValue& val3,
			   const Nbaseutil::safestring& name4, const SafeValue& val4,
			   const Nbaseutil::safestring& name5, const SafeValue& val5,
			   const Nbaseutil::safestring& name6, const SafeValue& val6,
			   const Nbaseutil::safestring& name7, const SafeValue& val7,
			   const Nbaseutil::safestring& name8, const SafeValue& val8,
			   const Nbaseutil::safestring& name9, const SafeValue& val9,
			   const Nbaseutil::safestring& name10, const SafeValue& val10);

} // namespace NIMacro

#endif
