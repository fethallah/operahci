#ifndef x_IMACRO_MEMBLOCK_memblockimplbase_h_INCLUDED_
#define x_IMACRO_MEMBLOCK_memblockimplbase_h_INCLUDED_

#include <map>
#include "memblock.h"

namespace NIMacro {

/**
* A base class for implementing cached secondary properties of objects, like image.mean etc.
* An object of this class is created in the MemBlock::Pimpl() call as needed.
*/
class MemBlockImpl: public Nbaseutil::mb_malloced {
public:
	// construct with backreference to the host object.
	MemBlockImpl(const MemBlock* Owner): Owner_(Owner) {}
	virtual ~MemBlockImpl() {}

	/// Delete secondary data, which can be recalculated automatically. Override if derived class holds some extra cached data.
	virtual void FlushSecondary(); 

	/// Copy non-secondary data from other object.
	virtual int CloneFrom(const MemBlockImpl* Src);	

	/// Copy all attributes from \a src.
	void CloneFull(const MemBlockImpl& src);

	/// Cache a secondary data item. Key must have a value appearing in MemBlock::MapKey, Vector::MapKey, etc.
	void SetSecondaryItem(int Key, const DataItem& Value);

	/// Retrieve or calculate a secondary data item.
	int SecondaryInt(int Key, Nbaseutil::ThrowHandlerFunc throwhandler) const;

	/// Check if the data item has been calculated, and retrieve it if it is.
	bool ExistsSecondaryInt(int Key, int& Value) const;

	double SecondaryDouble(int Key, Nbaseutil::ThrowHandlerFunc throwhandler) const;
	bool ExistsSecondaryDouble(int Key, double& Value) const;

	PMemBlock SecondaryMemBlock(int Key, Nbaseutil::ThrowHandlerFunc throwhandler) const;

	DataItem SecondaryItem(int Key, Nbaseutil::ThrowHandlerFunc throwhandler) const;

	bool ExistsSecondaryItem(int Key, DataItem& Value) const;


	/**
	* Execute a command on the object. Return Undefined if command is not supported. Throw if command failed.
	* @param pszCommand Verb ot execute; supported verbs depend on the actual MemBlockImpl-derived object class. MemBlockImpl::DoVerb() supports no verb.
	* @param iArg Size of the vArg array.
	* @param vArg Array of parameters.
	*/
	virtual SafeValue DoVerb(const char* pszCommand, int iArg, SafeValue vArg[]);

	/// Backreference to the MemBlock object which owns me.
	const MemBlock* Owner_;	

	/// Traverse the pimpl and call visitor for sticky attributes.
	void Traverse(TraverseVisitor& visitor, const TraverseNode& node);

	/// Iterate the pimpl object and call visitor for each attribute conforming to the mode.
	void IterateChildren(AcaVisitor& visitor, const TraverseNode& node);

	/// Support MemBlock::Entertain for cached attributes.
	bool Entertain(MemBlockVisitor& Visitor, const Nbaseutil::safestring& name, MemBlock::entertainmode_t mode);

protected:
	/// Override in derived class to calculate the value
	virtual DataItem CalculateSecondaryItem(int Key, Nbaseutil::ThrowHandlerFunc throwhandler) const;

private:
	/// Cached values.
	/// NB! Cannot easily use hash map here, as the MD5 checksum depends on the order of sticky attributes.
	typedef std::map<int,DataItem> cache_t;
	cache_t cache_;
};

} // namespace
#endif
