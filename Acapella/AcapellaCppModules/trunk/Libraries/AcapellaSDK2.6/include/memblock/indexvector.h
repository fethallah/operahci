// A class which holds indexes or pointers into another MemBlock.
// The pointed/indexed MemBlock is Capture()-d during the lifetime of IndexVector object.

#ifndef _INDEXVECTOR_H_INCLUDED_
#define _INDEXVECTOR_H_INCLUDED_

#include <vector>
#include "vector.h"

namespace NIMacro {

class DI_MemBlock IndexVector;
typedef DerivedPointer<IndexVector> PIndexVector;


/// A class holding an array of indices into another (target) object. If the target is an Image, then the IndexVector represents a mask.
class DI_MemBlock IndexVector: public Vector {
	typedef Vector super;

public: // definitions

	/**
	* A struct describing target data (N-dimensional data array) properties for IndexVector and Point classes. 
	* For example, in case of stencils the TargetData structure describes the image type for what the stencil can be applied.
	* The TargetData structure stores the target dimensions and description. If description is not set explicitly, a 
	* standard description is composed by target dimensions.
	*/
	struct TargetData {
	public: // fields
		/// Target description
		Nbaseutil::safestring descr_;
		typedef std::vector<unsigned int, Nbaseutil::mb_allocator_typedef<unsigned int>::allocator> dimsize_t;
		/// Dimension sizes of the target. The target rank can be obtained as dimsize_.size().
		dimsize_t dimsize_;
	public: // interface
		
		/// Construct an empty targetdata object.
		TargetData() {}

		/**
		* Construct a TargetData object with automatically generated description. 
		* @param rank The number of dimensions of the indexvector target.
		* @param dimsize An array of rank length specifying the size of each dimension, starting with the most tightly packed dimension, e.g. image or datacube width.
		*/
		DI_MemBlock TargetData(int rank, const unsigned int* dimsize);

		/**
		* Construct a TargetData object with an explicitly specified description. 
		* @param descr An illustrative description of the target, e.g. "Image of 100x100 pixels".
		* @param rank The number of dimensions the target of the indexvector has.
		* @param dimsize An array of rank length specifying the size of each dimension, starting with the most tightly packed dimension, e.g. image or datacube width.
		*/
		TargetData(const Nbaseutil::safestring& descr, int rank, const unsigned int* dimsize)
			: descr_(descr)
			, dimsize_(rank) 
		{
			for (int k=0; k<rank; ++k) dimsize_[k] = dimsize[k];
		}

		/**
		* Construct a TargetData object by a representative target object (a Vector or a class derived from Vector). The rank and dimension sizes are set according to the argument object.
		* @param v An exemplary Vector, Image, Datacube or other Vector-derived class object.
		*/
		TargetData(const PVector v)
			: descr_(v->GetDescription())
			, dimsize_(v->GetRank())
		{
			for (unsigned int k=0; k<dimsize_.size(); ++k) dimsize_[k] = v->GetDimSize(k);
		}

		/**
		* A convenience ctor: construct by rank and dimension sizes as vararg arguments. Example: TargetData(2, width, height).
		* @param rank The number of dimensions. This must be followed by the same number of integer arguments specifying the dimension sizes, beginning from the most tightly packed (width).
		*/
		DI_MemBlock TargetData(int rank, ...);

	};
public: // static interface

	/// Create a dummy IndexVector object.
	static PIndexVector Create();

	/// Create an IndexVector object for indexing images. The index array is allocated and zero-filled.
	static PIndexVector Create(size_t length, unsigned int imagewidth, unsigned int imageheight);

	/// Create an IndexVector object for indexing objects similar to specified target. The index array is allocated and zero-filled.
	static PIndexVector Create(size_t length, PVector target=NULL);

	/// Create an IndexVector object for indexing objects similar to specified target. The index array is allocated and zero-filled.
	static PIndexVector Create(size_t length, const TargetData& target_data);

	/// Create an IndexVector object for indexing objects similar to specified target, from an existing index array.
	static PIndexVector Create(size_t length, unsigned int* p_data, int first_element_index, MemChunk::ReleaseFunc1 pFree, PVector target=NULL);

	/**
	* Create an IndexVector object for indexing images from an existing index array.
	* @param length Length of data (in elements)
	* @param p_data A malloc()-ed pointer pointing to a memory block of size (iLength+offset)*SizeOf(t) bytes.
	* @param first_element_index Specifies the index of first data element in pData. Preceding element(s) are not used. Normally this is 0.
	* @param pFree Pointer to the function for eventual releasing pData.
	* @param imagewidth The width of a target image.
	* @param imageheight The height of a target image.
	*/
	static PIndexVector Create(size_t length, unsigned int* p_data, int first_element_index, MemChunk::ReleaseFunc1 pFree, unsigned int imagewidth, unsigned int imageheight); 

	/**
	* Create an IndexVector object for indexing images by an existing raw index array.
	* @param length Length of data (in elements)
	* @param p_data A malloc()-ed pointer pointing to a memory block of size (iLength+offset)*SizeOf(t) bytes.
	* @param first_element_index Specifies the index of first data element in pData. Preceding element(s) are not used. Normally this is 0.
	* @param pFree Pointer to the function for eventual releasing pData.
	* @param target_data Specifies target image dimensions.
	*/
	static PIndexVector Create(size_t length, unsigned int* p_data, int first_element_index, MemChunk::ReleaseFunc1 pFree, const TargetData& target_data); 

	/// Create an IndexVector from an existing vector of indices.
	static PIndexVector Create(const PVector src, PVector target=NULL);

	/// Create an IndexVector from an existing vector of indices.
	static PIndexVector Create(const PVector src, unsigned int imagewidth, unsigned int imageheight);

	/// Create an IndexVector from an existing vector of indices.
	static PIndexVector Create(const PVector src, const TargetData& target_data);

public: // interface


	/// Set information about IndexVector target object. This is used mostly for checking and informational purposes.
	void SetTargetData(const Nbaseutil::safestring& descr, int rank, const unsigned int* dimsize);

	/// Set information about IndexVector target object. This is used mostly for checking and informational purposes.
	void SetTargetData(const TargetData& target_data) {
		SetTargetData(target_data.descr_, int(target_data.dimsize_.size()), &target_data.dimsize_[0]);
	}

	/// Fetches target information from a sample target object and calls SetTargetData() with that information.
	void SetTarget(PVector target) {
		SetTargetData(target->GetDescription(), target->GetRank(), target->GetDimSizes()->UnsignedIntPointer());
	}

	/// Return target data.
	TargetData GetTargetData() const {
		return TargetData(GetTargetDescr(), GetTargetRank(), GetTargetDimSizeArray());
	}

	/// Return target description.
	Nbaseutil::safestring GetTargetDescr() const {return target_descr_;}

	/// Return target array rank.
	int GetTargetRank() const {return targetrank_;}

	/** Return pointer to target dimension sizes array. The length of the array is reported by GetTargetRank(). 
	* Note that the returned pointer will point into the internal array which is destroyed together with the IntervalVector object.
	*/
	const unsigned int* GetTargetDimSizeArray() const {return targetdimsize_;}

	/// A specialized version of SetTargetData() for image targets.
	void SetImageDimensions(int w, int h);

	/// Return a vector containing x coordinates of points.
	PVector GetXVector() const; 
	
	/// Return a vector containing y coordinates of points.
	PVector GetYVector() const;


public: // virtual interface

	/**
	* Convert the object into the image representation. 
	* The base class method IndexVector::GetMaskImage() converts the indexvector object into a 0/1 8bpp image, if the indexvector target is an image.
	* The IntervalVector::GetMaskImage() override returns a 16bpp stencil in image representation.
	*/
	virtual PImage GetMaskImage() const; 

	/// Retrieve the dimensions of image in the end of Target chain. Calls throwhandler if the data structure is unsupported, the final target is not of rank 2 or the image dimensions cannot be retrieved by any other reason. See also GetTargetRank() and GetTargetDimSizeArray().
	virtual void GetImageDimensions(unsigned int& width, unsigned int& height, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed) const;

protected: // implementation

	IndexVector(size_t length);
	IndexVector(size_t length, unsigned int* p_data, MemChunk::ReleaseFunc1 pFree, size_t offset);
	IndexVector(const IndexVector& b); // clone constructor.
	IndexVector(const Vector& b);


public: // virtual overrides
	virtual Nbaseutil::safestring DoGetDescription() const;
	/**
	* Supports additional subitem names, mostly used by deserialization from HDF and other formats.
	*    - target_data: item must be a small numeric vector containing dimension sizes; current target data is replaced by the content of item.
	*/
	virtual bool DoSetSubItem(const Nbaseutil::safestring& subitemname, const SafeValue& item, const Nbaseutil::safestring& fullpathname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed);
	virtual const char*	Class() const {return "indexvector";}
	virtual PMemBlock DoClone(copymode_t copy_mode) const;
	virtual bool Conforms(const char* szClassName) const;
	virtual bool Consistent(Nbaseutil::safestring& msg, bool CheckContent=true) const;	
	virtual bool AddConvertArg(const Nbaseutil::safestring& option, const DataItem& value, ConvertOptions& opt, Nbaseutil::ThrowHandlerFunc throwhandler=LogWarnings()) const;
	virtual PMemBlock ConvertFrom(const PMemBlock source, const ConvertOptions& opt) const;
	virtual PMemBlock ConvertTo(const Nbaseutil::safestring& classname) const;
	virtual SafeValue ResolveSubItem(const Nbaseutil::safestring& subitemname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed) const;
	virtual const char* ClassDescription() const;
	virtual bool Entertain(MemBlockVisitor& visitor, const Nbaseutil::safestring& name, entertainmode_t mode=enter_deep);	
	virtual void IterateChildren(NIMacro::AcaVisitor& visitor, const NIMacro::TraverseNode& node);
	virtual void Append(PVector b);

protected:
	virtual ~IndexVector();
	void InitZero();
private: // data
	/// Target description
	Nbaseutil::safestring target_descr_;
	int targetrank_;
	unsigned int* targetdimsize_;
};

/**
* Converts  the Indexvector to shiftindex (indexvector with negative elements)
* @param poMatrix Indexvector with positive indexes, describes the shiftobject or shiftobjects on the virtual image.
* @param RealImageWidth Width of the real image to which the shiftobject or shiftobjects will be applied.
*/
DI_MemBlock PVector IndexVector2ShiftVector(PIndexVector poMatrix, int RealImageWidth);

} // namespace NIMacro

#endif
