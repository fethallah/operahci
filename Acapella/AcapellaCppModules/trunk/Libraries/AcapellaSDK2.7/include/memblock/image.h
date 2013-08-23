// The class for holding images.

#ifndef _IMAGE_H_INCLUDED_
#define _IMAGE_H_INCLUDED_

#include <math.h>

#include "geomblock.h"
//#include "error.h"
//#include "intervalvector.h"
// #include "fraction.h"

namespace NIMacro {

class DI_MemBlock IndexVector;
typedef DerivedPointer<IndexVector> PIndexVector;
class DI_MemBlock IntervalVector;
typedef DerivedPointer<IntervalVector> PIntervalVector;

/// A class representing a 2D image.
class DI_MemBlock Image : public GeomBlock<2> {
	typedef GeomBlock<2> super;

public: // enums and constants

	/// Supported bit-per-pixel depths. These are equivalent to the unsigned int data types. Use ElemType() instead for more flexibility.
	enum bpp_t {
		bpp8 = 8,
		bpp16 = 16,
		bpp24 = 24,
		bpp32 = 32,
		bpp64 = 64,
	};

	/// Enums for using with LgType facility.
	enum logic_class_t {
		/// image pixels represent some data, e.g. fluorescence intensity. This is the default.
		lg_data=0,	
		/// Image pixels represent colors or greyscale tones - visual image.
		lg_visual=11,
		/// Image represents simple 0/1 mask. The element type is Byte (8bpp), raw factor is always 1. Any non-zero pixel value indicates the mask area.
		lg_mask=12,	
		/// Image represents a stencil, pixel value is object number+1 or is zero (no object). Bit depth is either 16 or 32 bpp.
		lg_stencil=13,	
	};

	/// Enums for using with SecondaryItem facility.
	enum key_constants {
		/// Number of objects for lg_stencil type images. This may be more than image.max if last objects are of zero size.
		key_stencilcount = 10301,
		/// Kinda obsolete. Records logical image edges outside of real image, see Enlarge() module.
		key_edgewidth = 10302,
	};
public: // static interface
	/// Create an image from an existing pixel array. Obsolete interface function.
	static PImage Create(void *p, MemChunk::ReleaseFunc1 pFree, unsigned int width=640, unsigned int height=512, bpp_t bpp=Image::bpp8, double factor=1.0, int layers=1, Vector::DataType t=Vector::Void) {
		return Create(p, pFree, width, height, t==Vector::Void? BppToDataType(bpp): t, factor);
	}

	/// Create an image and allocate memory for the pixel array. Obsolete interface function.
	static PImage Create(unsigned int width, unsigned int height, bpp_t bpp=Image::bpp8, double factor=1.0, int layers=1, Vector::DataType t=Vector::Void) {
		return Create(width, height, t==Vector::Void? BppToDataType(bpp): t, factor);
	}

	/// Create a default "factory" image.
	static PImage Create() {return Create(1,1);}
	
	/// Create an image and allocate memory for the pixel array.
	static PImage Create(unsigned int width, unsigned int height, Vector::DataType t, double factor=1.0);

	/// Create an image from an existing pixel array.
	static PImage Create(void *p, MemChunk::ReleaseFunc1 pFree, unsigned int width, unsigned int height, Vector::DataType t, double factor=1.0);

	/// Create an image from the TypedChunk object. Memory will be in shared use initially.
	static PImage Create(const TypedChunk& chunk, unsigned int width, unsigned int height, double factor);

	/// Check if the string is a name of Image logical type, return the type value or throw.
	static logic_class_t String2Type(const Nbaseutil::safestring& s);

	/// Return the default datatype corresponding to the specified bit depth.
	static Vector::DataType BppToDataType(bpp_t bpp) {
		switch(bpp) {
		case bpp8: return Vector::Byte;
		case bpp16: return Vector::UnsignedShort;
		case bpp24: return Vector::Triple;
		case bpp32: return Vector::UnsignedInt;
		case bpp64: return Vector::Double;
		default: throw "bpp not supported.";
		}
	}

public: // interface

	/// Return bytes-per-pixel value. Deprecated, use ElemType() in new code instead.
	int Bpp() const {
		return SizeOf(ElemType());
	}

	/// Return bits-per-pixel value. Deprecated, use ElemType() in new code instead.
	bpp_t bpp() const {
		return bpp_t(Bpp()*8);
	}

	/// Change bit depth value. Deprecated, use SetElemType() instead.
	void Setbpp(bpp_t Newbpp) {
		SetElemType(BppToDataType(Newbpp));
	}

	/// Return 3 for RGB images, 1 otherwise.
	int GetLayers() const {return ElemType()==Triple? 3: 1;}

	/// Obsolete, does nothing.
	void SetLayers(int layers) {}

	/// Same as GetLgType(), with result converted to Image::logic_class_t.
	logic_class_t GetType() const {
		return logic_class_t(GetLgType());
	}

	/// Same as SetLgType(). Set the image logic type. Throws if the image representation does not match the type.
	void SetType(logic_class_t typ);

	/// Obsolete, do not use in new code. See Enlarge() module.
	int GetEdgeWidth() const;

	/// Obsolete, do not use in new code. See Enlarge() module.
	void SetEdgeWidth(int k);

	/// Convert image in a 8 bpp image. For RGB images an average of colors is taken. Throws if the content is not convertable into 8bpp representation.
	void Depth8();

	/**
	* Find, cut and return a rectangle containing all pixels with value k. This is useful for finding objects from lg_stencil type Images.
	* @param k Value of pixels to look for.
	* @param x0 Output parameter. If not NULL, the x index of upper-left corner of the rectangle is stored in *x0.
	* @param y0 Output parameter. If not NULL, the y index of upper-left corner of the rectangle is stored in *y0.
	* @return A rectangle cut out from the original image.
	*/
	PImage ExtractCoded(unsigned int k, int* x0=NULL, int* y0=NULL) const; 


	/// Can be called only for lg_stencil type Images. Returns the number of stencils assigned to the image by previous SetStencilCount(). If this has been not set, then returns image.max value.
	int GetStencilCount() const;

	/**
	* Can be called for non-negative values only for lg_stencil type Images. 
	* Announces the number of stencils in a lg_stencil type image. 
	* This must be equal or greater than image.max. 
	* One can call it with -1 for any logical type to announce that the stencilcount is unknown.
	*/
	void SetStencilCount(int k);

	/**
	* This is a faster version of SetLgType(lg_stencil) + SetStencilCount(k).
	* The image must be in correct type and edges cleared.
	*/
	void SetTypeToStencil(int stencilcount);

	/// Returns the mask consisting of non-zero pixels in the image, in vector representation.
	PIndexVector GetMaskVector() const;

	/**
	* Interprets the image as a stencil in image representation, returns the corresponding stencil in vector representation. 
	* If the image data type is not UnsignedShort, attempts to convert it beforehand.
	* The raw factor is ignored by this function.
	*/
	PIntervalVector GetStencilVector() const; // convert stencil into vector representation.

	/**
	* Crop a rectangle from an image and return as a new image.
	* NULL is returned in case of vanishing crop area.
	* 
	* The rectangle may lay partially outside of the image. The outside pixels will 
	* be initialized with zero bit pattern. 
	*
	* The rectangle will be normalized before use, i.e. one can pass x2,y2
	* less than x1,y1 (with the exception of value -1). 
	* Thus this function never flips the image.
	*
	* @param crop_x1,crop_y1 The left-top corner of crop area, incl.
	* @param crop_x2,crop_y2 The bottom-right corner of crop area, excl. If any of 
	*		those parameters is -1, it will be replaced by image width or height, 
	*		resulting in no crop in the corresponding edge.
	*/
	PImage Crop(int crop_x1, int crop_y1, int crop_x2, int crop_y2) const;

protected: // implementation

		Image(const unsigned int* dimsize, size_t length, DataType elem_type, double factor):
			GeomBlock<2>(dimsize, length, elem_type, factor),
			width(dimsize_[0]),
			height(dimsize_[1])
		{}

		Image(const unsigned int* dimsize, size_t length, DataType elem_type, double factor, void *p, MemChunk::ReleaseFunc1 p_free, size_t offset=0):
		   GeomBlock<2>(dimsize, length, elem_type, factor, p, p_free, offset),
		   width(dimsize_[0]),
		   height(dimsize_[1])
		{}

		Image(const unsigned int* dimsize, size_t length, DataType elem_type, double factor, void* p, MemChunk::ReleaseFunc2 p_free, void* callback_data, size_t offset=0):
			GeomBlock<2>(dimsize, length, elem_type, factor, p, p_free, callback_data, offset),
			width(dimsize_[0]),
			height(dimsize_[1])
		{}
		Image(const unsigned int* dimsize, const TypedChunk& chunk, double factor):
			GeomBlock<2>(dimsize, chunk, factor),
			width(dimsize_[0]),
			height(dimsize_[1])
		{}

		Image(const Image& b);

	/// Perform the crop. No error checks are done, the arguments must be valid.
	PImage DoCrop(int crop_x1, int crop_y1, int crop_x2, int crop_y2) const;


public:  // virtual overrides
	bool					AddConvertArg(const Nbaseutil::safestring& s, const DataItem& value, ConvertOptions& opt, Nbaseutil::ThrowHandlerFunc throwhandler) const;
	virtual Nbaseutil::safestring LgType2String(int lg_type) const;
	virtual int				String2LgType(const Nbaseutil::safestring& lg_type);
	virtual const char*		Class() const {return "image";}
	virtual PMemBlock		DoClone(copymode_t copy_mode) const;
	virtual bool			Conforms(const char *szClassName) const;
	virtual bool			Consistent(Nbaseutil::safestring& msg, bool CheckContent=true) const;	
	virtual PMemBlock		ConvertFrom(const PMemBlock source, const ConvertOptions& opt) const;
	virtual SafeValue		DoVerb(const char* pszCommand, int iArg, SafeValue vArg[]);
	virtual SafeValue		ResolveSubItem(const Nbaseutil::safestring& subitemname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed) const;
	virtual const char*		ClassDescription() const;
	virtual void			SetElemType(DataType t, bool ResizeBuffer=false, bool yCopyContent=false); 

protected: // virtual overrides
	virtual bool	Equals(const MemBlock& b) const;
	virtual void	DoSetLgType(int lg_type);
	virtual bool	DoSetSubItem(const Nbaseutil::safestring& subitemname, const SafeValue& item, const Nbaseutil::safestring& fullpathname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed);
	virtual Nbaseutil::safestring DoGetDescription() const;

public: // back-compatibility fields
	const unsigned int& width;
	const unsigned int& height;
};

template< typename PixelType >
void DivideTo8bpp( const PixelType *const pbSrc, const int iSrcLength, unsigned char *pbDst, double* pdDivisor=NULL);

} // namespace NIMacro
#endif
