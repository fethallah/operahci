#ifndef x_ACAPELLA_MEMBLOCK_STENCIL_H_INCLUDED
#define x_ACAPELLA_MEMBLOCK_STENCIL_H_INCLUDED

#include "memblock.h"
#include "indexvector.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251) //  needs to have dll-interface to be used, bla-bla-bla
#endif

namespace NIMacro {

class DI_MemBlock Stencil;
typedef DerivedPointer<Stencil> PStencil;

class DI_MemBlock Image;
typedef DerivedPointer<Image> PImage;

class DI_MemBlock IntervalVector;
typedef DerivedPointer<IntervalVector> PIntervalVector;
//template struct DI_MemBlock DerivedPointer<IntervalVector>;		// This trick is needed for avoiding MSVC++ warning C4251.

/** A MemBlock hierachy class for abstracting a 2D stencil object.
*/
class DI_MemBlock Stencil: public MemBlock {
	typedef MemBlock super;

public: // enums and typedefs
	enum repr_t {
		repr_image,
		repr_vector,
	};
public: // static interface

	/// Creates a dummy mask object.
	static PStencil Create();

	/// Creates a mask from a representation as an intervalvector or as an image.
	static PStencil Create(PVector intervalvector_or_image);

public: // interface

	repr_t GetRepresentation() const {return vector_repr_? repr_vector: repr_image;}

	void SetContent(PVector intervalvector_or_image);

	PIntervalVector GetVectorRepr() const;

	PImage GetImageRepr() const;

	IndexVector::TargetData GetTargetData() const;

public: // Overridden virtuals
	virtual const char*		Class() const {return "stencil";} // CCPTR_OK
	virtual PMemBlock		DoClone(copymode_t copy_mode) const;
	virtual bool			Conforms(const char *szClassName) const;
	virtual bool			Consistent(Nbaseutil::safestring& msg, bool CheckContent=true) const;	
	virtual bool			AddConvertArg(const Nbaseutil::safestring& option, const DataItem& value, ConvertOptions& opt, Nbaseutil::ThrowHandlerFunc throwhandler=LogWarnings()) const;
	virtual PMemBlock		ConvertFrom(const PMemBlock source, const ConvertOptions& opt) const;
	virtual PMemBlock		ConvertTo(const Nbaseutil::safestring& classname) const;
	virtual bool			Entertain(MemBlockVisitor& visitor, const Nbaseutil::safestring& name, entertainmode_t mode=enter_deep);
	virtual void			IterateChildren(NIMacro::AcaVisitor& visitor, const NIMacro::TraverseNode& node);
	virtual SafeValue		ResolveSubItem(const Nbaseutil::safestring& subitemname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed) const;
	virtual const char*		ClassDescription() const; // CCPTR_OK

protected: // Overridden virtuals
	virtual Nbaseutil::safestring DoGetDescription() const;
	virtual void DoSerialize(int ser_format, SerializeVisitor& visitor, const Nbaseutil::safestring& itemname, const Formatter& formatter, Nbaseutil::safestring& buffer) const;
	virtual bool DoSetSubItem(const Nbaseutil::safestring& subitemname, const SafeValue& item, const Nbaseutil::safestring& fullpathname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed);
	virtual bool Equals(const MemBlock& b) const; // value compare to another item b, which must be of the same dynamic type. See operator==()

protected: // implementation
	Stencil();

private: // data
	PImage image_repr_;
	PIntervalVector vector_repr_;
};

} // namespace NIMacro

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif