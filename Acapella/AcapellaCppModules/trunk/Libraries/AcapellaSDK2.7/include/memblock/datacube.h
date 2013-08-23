#ifndef _x_IMACRO_MEMBLOCK_DATACUBE_H_INCLUDED_
#define _x_IMACRO_MEMBLOCK_DATACUBE_H_INCLUDED_

#include "geomblock.h"

namespace NIMacro {

class DI_MemBlock DataCubeStub;
typedef DerivedPointer<DataCubeStub> PDataCubeStub;

class DI_MemBlock DataCube;
typedef DerivedPointer<DataCube> PDataCube;

/**
* The DataCube class is a densely packed array for representing 
* 3-dimensional objects.
* Memory layout is: sequence of images of width*height pixels. 
* There are "depth" number of images.
*/
class DI_MemBlock DataCube: public GeomBlock<3> {
	typedef GeomBlock<3> super;
public: // static interface

	/// Create a DataCube of specified dimensions, element type and raw factor.
	static PDataCube Create(int width, int height, int depth, Vector::DataType t, double rawfactor=1.0);

	/// Encapsulate an existing memory block into a DataCube of specified dimensions, element type and raw factor.
	static PDataCube Create(int width, int height, int depth, Vector::DataType t, double rawfactor, void* pData, MemChunk::ReleaseFunc1 fp);

	/// Create a DataCube sharing a TypedChunk, with specified dimensions and raw factor.
	static PDataCube Create(const TypedChunk& typedchunk, int width, int height, int depth, double rawfactor=1.0);

	/// Create an empty DataCube, used only for specific purposes.
	static PDataCube Create();

public:

	/// Return image layer width
	int Width() const {return GetDimSize(0);}
	
	/// Return image layer height
	int Height() const {return GetDimSize(1);}

	/// Return the number of image layers
	int Depth() const {return GetDimSize(2);}

	/// Return copy of a given image plane. Throws if i not in range 0 .. Depth()-1.
	PImage GetPlane(int i) const;

	/** 
	* Return non-owning reference to the part of the data cube corresponding
	* to the image plane i. Throws if i not in range 0 .. Depth()-1.
	* The returned smartpointer can only be dereferenced while datacube is alive and
	* not modified.
	*/
	const PImage GetTempPlane(int i) const;	

	/** 
	* Return non-owning reference to the part of the data cube corresponding
	* to the image plane i. Throws if i not in range 0 .. Depth()-1.
	* The returned smartpointer can only be dereferenced while datacube is alive and
	* not modified.
	*/
	PImage GetTempPlane(int i);	

	/**
	* Inserts a new image plane before plain i. 
	* Throws if i not in range 0 .. Depth().
	* Throws if Plane is incompatible (see ConformingPlane()).
	*/
	void InsertPlane(int i, PImage Plane);

	/**
	* Insert a new image plane in the end of the DataCube.
	* Throws if Plane is incompatible (see ConformingPlane()).
	*/
	void AddPlane(PImage Plane) {InsertPlane( Depth(), Plane);}

	/// Delete plane i. Throws if in not in range 0 .. Depth()-1.
	void DeletePlane(int i);

	/**
	* Replace the data of plane i in the DataCube.
	* Throws if Plane is incompatible (see ConformingPlane()).
	*/
	void ReplacePlane(int i, PImage Plane);

	/**
	* Composes a vector of length Depth() from points at (x,y) in all image planes.
	* Throws if point (x, y) out of range.
	*/
	PVector GetZVector(int x, int y) const;

	/**
	* Replace data in the "vertical" slice of the datacube at image layer points (x,y).
	* Throws if point (x, y) out of range.
	* Throws if v is incompatible (wrong length, non-convertible datatype or factor, not a vector).
	*/
	void ReplaceZVector(int x, int y, const PVector& v);

	/**
	* Create a DataCube mask in vector representation.
	* Includes all DataCube pixels on or above threshold.
	* For unknown or floating-point element types use the another overload instead.
	* @param threshold The raw threshold value (without factor).
	*/
	PIndexVector GetMaskVector(int threshold) const;	

	/**
	* Same as GetMaskVector(int) for integer datatypes, but works
	* correctly also in case of floating-point element types.
	*/
	PIndexVector GetMaskVector(double threshold) const;

public: // Overridden virtuals.

	virtual const char*		Class() const {return "datacube";}
	virtual PMemBlock		DoClone(copymode_t copy_mode) const;
	virtual bool			Conforms(const char *szClassName) const;

	/**
	* Supported verbs:
	* "create" - width, height, depth [,datatype]. Optional datatype is in string representation.
	*/
	virtual SafeValue DoVerb(const char* pszCommand, int iArg, SafeValue vArg[]);

	/**
	* Supported gettable items: "width", "height", "depth".
	*/
	virtual SafeValue ResolveSubItem(const Nbaseutil::safestring& pszSubItemName, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed) const;
	virtual const char*	ClassDescription() const;

protected: // implementation
	DataCube(const unsigned int* dimsize, size_t length, Vector::DataType t, double rawfactor);
	DataCube(const unsigned int* dimsize, size_t length, Vector::DataType t, double rawfactor, void* pData, MemChunk::ReleaseFunc1 fp);
	DataCube(const TypedChunk& typedchunk, const unsigned int* dimsize, double rawfactor);
	DataCube();	
	static void CheckElemTypeSupported(Vector::DataType t);	// check if element type t is supported; throw Exception if not.

protected: // overridden virtuals
	/**
	* Supported settable subitems:
	* number - replace image plane at (z).
	* number,number - replace Z-vector at (x, y).
	* number,number,number - replace pixel at (x,y,z).
	*/
	virtual bool DoSetSubItem(const Nbaseutil::safestring& subitemname, const SafeValue& item, const Nbaseutil::safestring& fullpathname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed);

	virtual Nbaseutil::safestring		DoGetDescription() const;

private:
	/**
	* Checks if the image plane is compatible with the DataCube, converts the data
	* if needed, returns the converted data.
	* Throws if Plane is not an image.
	* Throws if image dimensions do not match.
	* Throws if the image data is not convertible to the DataCube elemtype.
	* Throws if the image raw factor cannot be unified with the DataCube.
	*/
	const PImage ConformingPlane(const PImage Plane) const;	// check if the plane is suitable for holding in the data cube, and convert it into suitable format, or throw Exception.
};


} // namespace NIMacro


#endif
