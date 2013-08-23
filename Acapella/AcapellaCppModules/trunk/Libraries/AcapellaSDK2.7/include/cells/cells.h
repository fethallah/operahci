#ifndef _IMACRO_CELLS_H_INCLUDED_
#define _IMACRO_CELLS_H_INCLUDED_

// A data structure for holding list of geometric objects along with their various attributes.

// Cells are referred to by indices, from 1 to n. There are no gaps. Index 0 is unused.

// Cells may hold an arbitrary number of attributes. Every attribute is presented in a separate MemBlock.
// The attributes may be calculated not for all cells.

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251 4511 4512 4514)
#endif

#ifndef DI_Cells
#ifdef _MSC_VER
#define DI_Cells __declspec(dllimport)
#else
#define DI_Cells
#endif
#endif

#include "bitvector.h"

#ifdef OBJECTS
#define ACAPELLA_TMP_SAVED_MACRO_OBJECTS OBJECTS
#undef OBJECTS
#endif
#ifdef OBJECTLIST
#define ACAPELLA_TMP_SAVED_MACRO_OBJECTLIST OBJECTLIST
#undef OBJECTLIST
#endif
#ifdef OBJECT_LIST
#define ACAPELLA_TMP_SAVED_MACRO_OBJECT_LIST OBJECT_LIST
#undef OBJECT_LIST
#endif

// Define transition macros for cell list objects.
// To use these macros in your own code, use
//  #define DEFINE_ACAPELLA_OBJECTLIST_MACROS
// before including <Cells/DI_Cells.h>

#undef ACAPELLA_USE_CELLS_NAMES
#define OBJECTS "objects"
#define OBJECTLIST "objectlist"
#define OBJECT_LIST "object list"
#define OBJECTINDEX "objectindex"
#ifndef ACAPELLA_USE_OBJECTS_NAMES
#	define ACAPELLA_USE_OBJECTS_NAMES
#endif

namespace NIMacro {

class DI_Cells Attribute;
typedef DerivedPointer<Attribute> PAttribute;

class DI_Cells Cells;
typedef DerivedPointer<Cells> PCells;

class ReCalc;
class RecalcMod;

/// An helper class for encapsulating data in the object lists. Only Attribute objects can be inserted directly in a Cells object.
/**
* An Attribute provides an abstraction of an object list attribute regardless of the actual attribute data type.
* It can encapsulate a Vector, IntervalVector or Image (of stencil subtype).
*
* It provides means for (partial) invalidation of attribute data for some or all objects.
* It also provides automatic recalculation support of invalidated data.
*/
class Attribute: public MemBlock {
	typedef MemBlock super;
	// A list of cell attributes.
public:
	/// An enum for listing the attributes with predefined recalculation method.
	enum AtrClass {
		atr_plain,
		atr_area,
		atr_border,
		atr_body,
		atr_high_low,
		atr_index,
		atr_zone,
		atr_outerzone,
		atr_zoneintensity,
		atr_outerzoneintensity,
		atr_center,
		atr_intensity,
		atr_roundness,
		atr_fixedmask,
		atr_growmask,
		atr_brightspots,
		atr_histo,

		atr_last_entry // must be last here
	};

private:
	/// Convert AtrClass enum to the attribute name, and vice versa.
	static void ConvAtrClass(AtrClass& cls, const char*& name);
public:
	// public conversion functions from class to name and vice versa:

	/// Convert attribute name to the corresponding AtrClass value.
	/** @return The corresponding AtrClass value, or atr_plain if the name is not known. */
	static AtrClass ToAtrClass(const char* AttrName) {
		if (!AttrName || !*AttrName) {
			return atr_plain;
		}
		AtrClass cls=atr_plain;
		ConvAtrClass(cls, AttrName);
		return cls;
	}

	/// Convert AtrClass value to the corresponding attribute name.
	/** @return The corresponding name (a static string), or an empty string if the parameter is invalid.*/
	static const char* ToAttrName(AtrClass cls) {
		const char* AttrName=NULL;
		ConvAtrClass(cls, AttrName);
		return AttrName;
	}
	/** @brief Create an attribute encapsulating an image. The image is supposed
	* to be a stencil in image representation.
	* @param n Number of objects.
	* @param Data The stencil image.
	* @param cls Attribute class. Currently only atr_index or atr_plain can be passed here.
	*/
	static PMemBlock CreateByImage( unsigned int n, PImage Data, AtrClass cls=atr_index);

	/** @brief Create an attribute encapsulating an image. The image is supposed
	* to be a stencil in image representation.
	* @param n Number of objects.
	* @param Data The stencil image.
	* @param AttrName Attribute class name. This is converted to attribute class by ToAtrClass() method.
	*/
	static PMemBlock CreateByImage( unsigned int n, PImage Data, const Nbaseutil::safestring& AttrName);	// create attribute for n cells, initialized by an Image

	/** @brief Create an attribute encapsulating a vector.
	*
	* @param n Number of objects.
	* @param Data The data vector. This can be either of Vector of n elements, or an IntervalVector of n intervals.
	* @param AttrName Attribute class name. This is converted to attribute class by ToAtrClass() method.
	*/
	static PMemBlock Create( unsigned int n, PVector Data, const Nbaseutil::safestring& AttrName);

	/** @brief Create an attribute encapsulating a vector.
	*
	* @param n Number of objects.
	* @param Data The data vector. This can be either of Vector of n elements, or an IntervalVector of n intervals.
	* @param cls Attribute class, or atr_plain for a non-recalcable attribute.
	*/
	static PMemBlock Create( unsigned int n, PVector Data, AtrClass cls);

	/** @brief Create an attribute encapsulating a vector with a custom recalcer.
	*
	* @param n Number of objects.
	* @param Data The data vector. This can be either of Vector of n elements, or an IntervalVector of n intervals.
	* @param cls Attribute class, usually atr_plain for a custom recalcable attribute.
	* @param recalcer The recalcer object. The Attribute class will take over managing the lifetime of the recalcer object.
	*  If you want to keep the recalcer object in shared use, call its ReCalc::Capture() and ReCalc::Release()
	*  functions appropriately.
	*/
	static PMemBlock Create( unsigned int n, PVector Data, AtrClass cls, ReCalc* recalcer);

	/// Create a recalcutable attribute by specifying a recalcable Mod object.
	/**
	* Typically 'this' is passed as the last argument.
	* The method calls Clone() and Cleanup() on the passed recalcer.
	* @param n Number of objects.
	* @param Data The data vector. This can be either of Vector of n elements, or an IntervalVector of n intervals.
	* @param recalcer The recalcer object, typically 'this'. A copy of it is made by the method, by calling RecalcMod::Clone() and RecalcMod::Cleanup().
	*/
	static PMemBlock Create( unsigned int n, PVector Data, RecalcMod* recalcer);

	/// A default creator, needed for MemBlockFactory.
	static PMemBlock Create() {return Create(0, Vector::Create(), atr_plain);}

	/// Invalidate object i.
	void Invalidate(unsigned int i);

	/// Invalidate info for objects i (incl.) to j (excl.)
	void Invalidate(unsigned int i, unsigned int j);

	/// Mark objects as valid, for objects i (incl.) to j (excl.)
	void Validate(unsigned int i, unsigned int j);

	/// Invalidate cells which are marked 0 in Mask. Other values in Mask[] are not processed.
	/**
	* @param Mask The mask array.
	* @param n Length of the mask array.
	*/
	void Invalidate(const unsigned int Mask[], unsigned int n);

	/// Set validness bits according to the code array.
	void SetValid(const int code[], unsigned int codelen);

	/// Assign a recalcer object to the attribute.
	/**
	* Recalcer object will be managed by the Attribute (by calling ReCalc::Capture(), ReCalc::Release() as appropriate).
	* @param r Pointer to a recalcer object, or NULL for removing any existing recalcer.
	*/
	void SetRecalcer(ReCalc* r);

	/// Return pointer to the recalcer object, or NULL if the Attribute has no recalcer currently.
	const ReCalc* GetRecalcer() const { return ReCalcer_;}

	/// Recalculate the attribute if needed.
	/**
	* If the attribute is currently fully valid, then no action is taken.
	* Otherwise, it is attempted to recalculate the attribute. For recalculation
	* the attribute either has to have a proper attribute class (not atr_plain)
	* or to have an explicitly set recalcer (see SetRecalcer()).
	* Usually for recalculating the attribute needs access to other atributes in the
	* object list. If the attribute has not yet included in the object list, then the
	* proper object list must be passed in as the pC parameter.
	*
	* @param ctx The execution context.
	* @param throwhandler A throwhandler object for handling errors.
	* @param AttrName Attribute name for error messages. If NULL is passed, the function tries to find out the real name automatically by the current attribute owner, if any.
	* @param pC Object list. The recalculation is done in the context of this object list. If NULL is passed, then the current attribute owner is used, if any.
	* @return True, if the attribute is now fully valid, and false if the recalculation failed by any reason, and a non-throwing throwhandler was used.
	*/
	bool Recalc(ExecutionContext& ctx, ThrowHandlerFunc throwhandler, const Nbaseutil::safestring& AttrName="", const Cells* pC=NULL); // recalculate invalid cells. return false or throw exception if this cannot be done.

	/// Return the number of objects ("attribute length").
	unsigned int CellCount() const {return CellCount_;}

	/// Announce a new number of objects.
	/** If the previous object count was smaller, then any newly appearing
	* objects are marked as invalid.
	*
	* This method does not resize the actual attribute data vector, this must be done
	* explicitly by the caller.
	*/
	void SetCellCount(int n);	// set cell count to n and possibly mark a part of values invalid.

	/// Rreports if all objects are valid.
	bool AllValid() const;

	/// Reports if all objects are invalid.
	bool AllInvalid() const;

	/// Returns the number of invalid objects.
	unsigned int InvalidCount() const;

	/// Return the actual attribute data, or NULL if the attribute data cannot be recalculated.
	/** If the attribute is not fully valid, recalculation is attempted. If this fails,
	* NULL is returned. Usually VectorData() or ImageData() functions are used instead of this function
	* for less type-checking.
	*/
	PMemBlock Data() const;

	/// Return data pointer, if the attribute has image data, otherwise NULL. The data values may be (partially) invalid, no recalculation is attempted.
	PImage  ImageDataRaw()	const {return ImageData_;}

	/// Return data pointer, if the attribute has vector data, otherwise NULL. The data values may be (partially) invalid, no recalculation is attempted.
	PVector VectorDataRaw() const {return VectorData_;}

	/// Return the actual attribute data, if the attribute has image data. NULL is returned if the attribute is invalid and cannot be recalculated.
	PImage  ImageData() const;

	/// 12.07.2010 (Acapella 2.4): Same as ImageData(), but can return a border-touching image-stencil.
	PImage ImageData(bool allow_bordertouch) const;

	/// Return the actual attribute data, if the attribute has vector data. NULL is returned if the attribute is invalid and cannot be recalculated.
	PVector VectorData() const;

	/// Get the detailed validness info of the attribute.
	/**
	* @param Base Index where the first object should lie in the returned vector. Must be nonnegative.
	*	Also the vector content is shifted by Base, if this is not zero.
	* @return A vector of type Vector::UnsignedInt and of length CellCount().
	*  First object (0) lies at the index Base. If object i is invalid, then
	* the array element is zero, otherwise it is i+Base. Note that passing Base=0
	* does not allow to distinguish if first object is valid or not.
	*/
	PVector GetCode(int Base=0) const;

	/// Assign vector data to the attribute. If the data length is less than CellCount(), then it is assumed that the missing objects are invalid.
	void SetVectorData(PVector v);

	/// Assign image data to the attribute. This should be a stencil in image representation, with Image::StencilCount() returning the same number as Attribute::CellCount().
	void SetImageData(PImage v);

	/// Return the attribute value for object k.
	/**
	* If object k is invalid and cannot be recalculated, an exception is thrown.
	* The returned item may be of following type:
	*  - for numeric attributes: a number
	*  - for string attributes: a string
	*  - for stencils in vector representation: a mask containing the pixels for object k.
	*  - for zone-like structures: an IntervalVector of zones for object k.
	*  - for zoneintensity-like attributes: a numeric vector.
	*  - for stencils in image representation: a small image containing the mask for object k.
	* @param k Object index. If out of range, an Exception is thrown.
	* @param hint For speeding up the Extract operations on the same Attribute object, the hint parameter is used.
	*             Initialize the hint variable to 0 before first call.
	*             By historic reasons, the hint is not placed in the Attribute class.
	*/
	SafeValue Extract(unsigned int k, unsigned int& hint) const; // find attribute value for cell k, pack it into a dataitem and return.

	/// Transfer autorecalc capabilities from b to this object.
	void TransferRecalcer(const PAttribute& b);

	/// Check if the attribute is valid for object Index.
	/**
	* @param Index Index of the object, must be in bounds.
	*/
	bool IsValid(unsigned int Index) const {return Validness_==0 ? false: (Validness_==1? true: Valid_[Index]!=0);}


	/// Shift the attribute data by k elements up or down, and adjust the cellcount and validity info appropriately.
	/** This is used in merging/appending. */
	void Shift(int k);

	/// Append data and validity info from b. Adjust cellcount accordingly.
	void Append(const Attribute& b);

	/** @brief Filter and/or shuffle the object according to the code array.
	* Deletes objects marked 0 in the code array.
	* Otherwise, moves object i to the position code[i], i=1..cellcount.
	* Apart from the first unused element, in the code array exactly newlen elements must be nonzero, different from each other and <= newlen.
	* i.e. code[] must define a compacting map for mapping interval [1..current_cellcount] to [1..newlen].
	* @param code 1-based array of new indices. It contains codelen+1 elements, first element [0] is unused and must be zero.
	*			The element values are also 1-based object indices.
	* @param codelen The length of the code array minus 1 (the unused element). Codelen should be equal to the attribute object count.
	* @param newlen The new objectcount of the attribute (number of nonzero entries in the code array).
	*/
	void Rearrange(const int code[], unsigned int codelen, unsigned int newlen);

	/// Create a new Attribute, copy over the attribute class and validity info, but not the pointed Vector or Image data.
	/** The data remains in the shared use with this Attribute. This method is used by Cells::CloneSurface().
	*/
	virtual PAttribute		CloneSurface() const;

	/// Announce the object list where the Attribute belongs. This is called from Cells::SetAttr().
	void	SetOwner(const Cells* owner) {Owner_ = owner;}

	/// Return the current owner object list, or NULL.
	const Cells* GetOwner() const {return Owner_;}

	/// Return true if the attribute has no recalculation support.
	bool IsStatic() const {return cls_==atr_plain && ReCalcer_==NULL;}

	/// Return true if attribute "base" may be used in/for recalculation of this attribute.
	bool CanBeRecalculatedBy(const safestring& base) const;

	/// Return true if any of attributes listed in bases may be used in/for recalculation of this attribute.
	bool CanBeRecalculatedBy(const safestringlist& bases) const;

	/// Change the attribute class. For this to have any effect one usually also has to call SetRecalcer(NULL).
	void SetAtrClass(AtrClass cls) {cls_ = cls;}

	/// Return the attribute class
	AtrClass GetAtrClass() const {return cls_;}

	/// Returns the current Acapella script execution context. Works only during a module call.
	/**
	* For this to work, the attibute must be included in an object list and the object list
	* must have been passed in as a module parameter into the module call.
	* @return Current execution context or ExecutionContext::Void() if the context cannot be determined.
	*/
	//ExecutionContext& GetExecutionContext() const;

public: // overridden virtuals
	virtual const char*		Class() const {return "attribute";}
	virtual PMemBlock		DoClone(copymode_t copy_mode) const;
	virtual bool			Conforms(const char *szClassName) const;
	virtual bool			Consistent(safestring& msg, bool CheckContent=true) const;
	virtual bool			AddConvertArg(const Nbaseutil::safestring& option, const DataItem& value, ConvertOptions& opt, Nbaseutil::ThrowHandlerFunc throwhandler=LogWarnings()) const;
	virtual PMemBlock		ConvertTo(const Nbaseutil::safestring& classname) const;
	virtual bool			Entertain(MemBlockVisitor& Visitor, const safestring& name, entertainmode_t mode=enter_deep);
	virtual void			IterateChildren(NIMacro::AcaVisitor& visitor, const NIMacro::TraverseNode& node);
	virtual SafeValue		ResolveSubItem(const safestring& pszSubItemName, ThrowHandlerFunc throwhandler) const;
	virtual const char*		ClassDescription() const;

protected: // overridden virtuals
	virtual bool			DoSetSubItem(const Nbaseutil::safestring& subitemname, const SafeValue& item, const Nbaseutil::safestring& fullpathname, ThrowHandlerFunc throwhandler=ThrowIndeed);
	virtual safestring		DoGetDescription() const;

private: // implementation
	/// Prepares changing of the validness info for some or all objects.
	/**
	*  Synchronizes Valid_ array and Validness_ indicator before changing some validness bits.
	* @param ToValid Pass true if you plan to mark some or all objects valid. Pass false if you plan to mark some or all objects invalid.
	* @param All Pass true if you plan to mark all objects at once invalid or valid. In this case the function always returns true.
	* @return True, if the validness info is already fully synchronized; caller may not set any Valid_ bits afterwards because the Valid_ array may not exist.
	*         False, if the caller must set the Valid_ array bits by hand.
	*/
	bool SyncValid(bool ToValid, bool All=false);

	virtual bool Equals(const MemBlock& b) const; // value compare to another item b, which must be of the same dynamic type. See operator==()
	virtual ~Attribute();
	virtual SafeValue ExtractFromVector(unsigned int k, unsigned int& hint) const;	// see Extract()
	virtual SafeValue ExtractFromImage(unsigned int k, unsigned int& hint) const;		// extract item k from image data

	Attribute(int n, PImage Data, AtrClass cls);	// create attribute for n cells, initialized by Data.
	Attribute(int n, PVector Data, AtrClass cls);	// create attribute for n cells, initialized by Data.

private: // data
	PImage ImageData_;		// Pointer to the actual attribute data.
	PVector VectorData_;
	mutable int Validness_;			// 0 = none valid, 1=all valid, 2 = validness given by Valid vector.
	unsigned int CellCount_;			// number of cells.
	const Cells* Owner_;	// pointer to the cell list containing this attribute, or NULL.
	AtrClass cls_;			// Static attributes have cls_==atr_plain && ReCalcer_==NULL.
	mutable ReCalc* ReCalcer_;		// Pointer to the recalculator. This may be NULL, in which case it is not yet installed in this object.
	BitVectorSmall Valid_;	// Bit 1 = attr valid, bit 0 = attr not valid.
public:
	bool DoNotDisturb_;		// See Entertain().
	mutable bool inclone_;			// a debug check flag.
};

/// A callback class to use with Cells::ReArrange().
class MemBlockAccepter: public Nbaseutil::mb_malloced {
public:
	/// Override and return true to accept the MemBlock for the action; return false to ignore the MemBlock.
	virtual bool Accept(const char* pszName, PMemBlock poItem) = 0;
	virtual ~MemBlockAccepter() {}
};


/// A class implementing an object list.
/**
* The object lists were initially called cell lists, hence the class name.
* A Cells object can contain any number of Attribute objects, stored by names.
* All Attribute objects must be of the same object count and equal to the Cells::CellCount().
*/
class Cells: public Jar {
	typedef Jar super;
private: // data
	unsigned int CellCount_;		// The number of objects.
	Cells() : CellCount_(0) {}
	virtual ~Cells();
public:

	/// Creates an empty object list - no attributes.
	static PCells Create();

	/// Return the current object count.
	unsigned int CellCount() const {return CellCount_;} // Get the number of cells.

	/// Announce the current object count. Should be called on an empty (cleared) object list.
	void SetCellCount(int n);

	/// Adds or replaces an attribute in the object list.
	/**
	* @param AttrName The name of the attribute.
	* @param atr Attribute itself. The object count of the attribute must match the current object list objectcount, unless the object list is empty.
	* @param autorecalc Default autorecalculation flag for the attribute. If the attribute already existed, this parameter
	*		is superseded by the autorecalc flag for existing attribute.
	*		If autorecalc==false, makes the attribute static. Otherwise, checks if attribute is dynamic. If it is not, tries to
	*		copy autorecalc info from existing attribute, if present. If this does not succedd, emits a warning.
	* @param force If true, then applies autorecalc parameter regardless of existence of the old attribute.
	*/
	void SetAttr(const Nbaseutil::safestring& AttrName, PAttribute atr, bool autorecalc, bool force=true);

	/// Find an attribute by name
	/**
	* @param AttrName Attribute name.
	* @param tryautorecalc If the attribute is invalidated, try to recalculate it.
	* @param throwhandler Throwhandler for processing errors by recalculations.
	* @return A valid attribute or NULL.
	*/
	PAttribute GetAttr(const Nbaseutil::safestring& AttrName, bool tryautorecalc=true, ThrowHandlerFunc throwhandler=DoNothing);

	/// Same as the other GetAttr().
	PAttribute GetAttr(const Nbaseutil::safestring& AttrName, bool tryautorecalc=true, ThrowHandlerFunc throwhandler=DoNothing) const {
		// return attribute
		return const_cast<Cells*>(this)->GetAttr(AttrName, tryautorecalc, throwhandler);
	}

	/// Return an attribute stencil.
	/**
	* @param AttrName Attribute name.
	* @param throwhandler Throwhandler for processing errors by recalculations.
	* @param tryautorecalc If the attribute is invalidated, try to recalculate it.
	* @return If the attribute is valid and is a stencil in vector representation, returns the stencil. Otherwise NULL is returned.
	*/
	PIntervalVector GetIntervalVector(const Nbaseutil::safestring& AttrName, ThrowHandlerFunc throwhandler=ThrowIndeed, bool tryautorecalc=true);

	/// Same as the other GetIntervalVector().
	const PIntervalVector GetIntervalVector(const Nbaseutil::safestring& AttrName, ThrowHandlerFunc throwhandler=ThrowIndeed, bool tryautorecalc=true) const {
		return const_cast<Cells*>(this)->GetIntervalVector(AttrName, throwhandler, tryautorecalc);
	}

	/// Delete attribute. If attribute not found, does nothing.
	void DelAttr(const Nbaseutil::safestring& AttrName);

//	/// Remove object i. This alters the indices of subsequent objects.
//	void Delete(int i);

//	/// Remove objects which are marked zero in the Code array.
//	void Delete(const char Code[], int CodeLen);

	/// Add objects from another object list into this list.
	/**
	* The objects in b are added in the end of this object list.
	* Only common attributes are kept.
	* @param b The other cell list.
	* @param deletegeometry Delete geometrical attributes (index, body, border, zone, outerzone).
	*         This helps to avoid the overlapping object list problem.
	* @param keepautorecalc Preserve autorecalc facilities for attributes which are non-static in both object lists. If false, all attributes are made static.
	*/
	void Append(const Cells& b, bool deletegeometry, bool keepautorecalc);

	/// Find out associated image dimensions for the cell list.
	/**
	* Finding out image dimensions is possible if the object list contains
	* an "index" attribute or any stencil attribute. Otherwise, (0,0) is returned.
	*/
	void GetImageDimensions(unsigned int& width, unsigned int& height) const;

	/// Invalidate all attributes. Static attributes will be deleted.
	void Invalidate();
	/// Invalidate all attributes for cell i. Static attributes will be deleted.
	void Invalidate(unsigned int i);
	/// Invalidate all attributes for cells i (incl.) to j (excl.). Static attributes will be deleted.
	void Invalidate(unsigned int i, unsigned int j);
	/// Invalidate all attributes for cells marked 0 in mask of n elements. Static attributes will be deleted.
	void Invalidate(unsigned int mask[], int n);
	/// Invalidate all dynamic attributes depending on any of attributes listed in bases.
	void Invalidate(const safestringlist& bases);
	/// Invalidate object i in all dynamic attributes depending on any of attributes listed in bases.
	void Invalidate(const safestringlist& bases, unsigned int i);
	/// Invalidate objects [i..j) in all dynamic attributes depending on any of attributes listed in bases.
	void Invalidate(const safestringlist& bases, unsigned int i, unsigned int j);
	/// Invalidate objects marked zero in mask in all dynamic attributes depending on any of attributes listed in bases.
	void Invalidate(const safestringlist& bases, unsigned int mask[], int n);

	/**
	* Reorder the object list.
	* See Attribute::Rearrange() for detailed description.
	* @param code 1-based array of new indices. It contains codelen+1 elements, first element [0] is unused and must be zero.
	*			The element values are also 1-based object indices.
	* @param codelen The length of the code array minus 1 (the unused element). Codelen should be equal to the attribute object count.
	* @param newlen The new objectcount of the attribute (number of nonzero entries in the code array).
	* @param Accepter If not NULL, then an additional filter for rearrangable attributes. If the accepter does not accept the attribute, it is not rearranged.
	*/
	void Rearrange(const int code[], unsigned int codelen, unsigned int newlen, MemBlockAccepter* Accepter=NULL);

	/// Extract all attributes of specified object and pack them into a container.
	/**
	* @param cellno Object index.
	* @param skip_unrecalcable_attrs Ignore invalid attributes which cannot be recalculated. Otherwise an exception will be thrown.
	*/
	PContainer Extract(unsigned int cellno, bool skip_unrecalcable_attrs=false) const;

	// MemBlock virtual methods:
	virtual const char*		Class() const {return OBJECTLIST;}
	virtual PMemBlock		DoClone(copymode_t copy_mode) const;
	virtual SafeValue		DoVerb(const char* pszCommand, int iArg, SafeValue vArg[]);
	virtual PJar			CloneSurface(bool CopyContent=true) const;
	virtual bool			Conforms(const char *szClassName) const;
	virtual const char*		ClassDescription() const;
	virtual bool			Consistent(safestring& msg, bool CheckContent=true) const;	// check the state of itself and base class parts and add error messages to msg.
	virtual void			DoSerialize(int ser_format, SerializeVisitor& visitor, const Nbaseutil::safestring& itemname, const Formatter& formatter, Nbaseutil::safestring& buffer) const;
	virtual bool			Entertain(MemBlockVisitor& Visitor, const safestring& name, entertainmode_t mode);
	virtual void			IterateChildren(AcaVisitor& visitor, const TraverseNode& node);
	virtual SafeValue		ResolveSubItem(const Nbaseutil::safestring& subitemname, ThrowHandlerFunc throwhandler=ThrowIndeed) const;

	// Overriden Container virtual methods.
	virtual void Put(const SafeValue& item, const safestring& label);

	/// This will transform Attribute into the contained Image/Vector.
	virtual SafeValue Get(const safestring& label) const;

protected:
	virtual bool			DoSetSubItem(const Nbaseutil::safestring& subitemname, const SafeValue& item, const Nbaseutil::safestring& fullpathname, ThrowHandlerFunc throwhandler=ThrowIndeed);
	virtual safestring		DoGetDescription() const;

public:
	// Keys for pimpl access
	enum {
		key_totalarea = 201,
		key_imagewidth = 202,
		key_imageheight = 203,
	};

};

} // namespace

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifndef DEFINE_ACAPELLA_OBJECTLIST_MACROS

#undef OBJECTS
#undef OBJECTLIST
#undef OBJECT_LIST

#ifdef ACAPELLA_TMP_SAVED_MACRO_OBJECTS
#define OBJECTS ACAPELLA_TMP_SAVED_MACRO_OBJECTS
#endif
#ifdef ACAPELLA_TMP_SAVED_MACRO_OBJECTLIST
#define OBJECTLIST ACAPELLA_TMP_SAVED_MACRO_OBJECTLIST
#endif
#ifdef ACAPELLA_TMP_SAVED_MACRO_OBJECT_LIST
#define OBJECT_LIST ACAPELLA_TMP_SAVED_MACRO_OBJECT_LIST
#endif

#endif
#endif

