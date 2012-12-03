#ifndef _IMACRO_JAR_H_INCLUDED_
#define _IMACRO_JAR_H_INCLUDED_

#include "executive.h"

namespace NIMacro {

class DI_IMacro Jar;
typedef DerivedPointer<Jar> PJar;

class DataBlock;
typedef ThreadSharablePointer<DataBlock> PDataBlock;

class Instance;
class JarIteratorImpl;

/** @brief Jar is essentially a Container with a couple of features added. These
* are not useful by itself, but are just introduced here as they will become
* important in the Cells class, derived from Jar. 
*
* The features are defined
* here in order to have better access to them at the compile time (Cells 
* class is defined in an external module library and IMacro.dll does not 
* know about it at compile-time.
*/
class DI_IMacro Jar: public Container {
	typedef Container super;
public:
	/// Create an empty Jar
//	static PJar Create();

	/// Forwards to Container::Put(), present for historic reasons.
//	void SetItem(const char* ItemName, const NIMacro::SafeValue& ItemValue) {Put(ItemValue, ItemName);}

	/// Forwards to Container::Delete(), present for historic reasons.
//	void DelItem(const char* ItemName) {Delete(ItemName);}

	/// Forwards to Container::clear(), present for historic reasons.
//	void DelAllItems() {clear();}
	
	/// Obsolete, use GetItemName instead.
//	const char* MemBlockName(const MemBlock* p) const;	

//	OVERRIDDEN MEMBLOCK_VIRTUAL_METHODS
	virtual const char*		Class() const {return "jar";}
	virtual PMemBlock		DoClone(copymode_t copy_mode) const;
	virtual bool			Conforms(const char *szClassName) const;
	virtual bool			Consistent(safestring& msg, bool CheckContent=true) const;
//	virtual PMemBlock		ConvertFrom(const PMemBlock source, const ConvertOptions& opt) const;
//	virtual bool			Entertain(MemBlockVisitor& Visitor, const safestring& name, entertainmode_t mode=enter_deep);
//	virtual SafeValue		ResolveSubItem(const Nbaseutil::safestring& pszSubItemName, ThrowHandlerFunc throwhandler=ThrowIndeed) const;
	virtual const char*		ClassDescription() const;

protected:
	virtual safestring	DoGetDescription() const;

public:
	/// Return a new Jar containing the same physical MemBlocks. Before modification each contained top-level item must be cloned separately.
	virtual PJar CloneSurface(bool CopyContent=true) const;	

	/// Find out the name(s) of MemBlock, if it is accessible via this Jar. See also MemBlockName().
	Nbaseutil::safestring GetItemName(const MemBlock* addr) const;

	/// Set the feature support flags according to inst.
	void CopyFeatureSupportFrom(const Instance& inst);

//	/// Obsolete, do not use. Kept for binary backward compatibility.
//	void SetExecutionContext(ExecutionContext* p_ctx);

//	/// Return the current execution context. Outside of module calls ExecutionContext::Void() is returned.
//	ExecutionContext& GetExecutionContext() const;

protected:
//	virtual bool Equals(const MemBlock& b) const; // value compare to another item b, which must be of the same dynamic type. See operator==()
	Jar();	// Creates empty container
	virtual ~Jar();

	enum support_t {
		support_silent=0,
		support_warn=1,
		support_off=2,
	};
	support_t feature_attributecreation, feature_attributerecalculation;
};

}
#endif
