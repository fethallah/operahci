#ifndef _INTERVALVECTOR_H_INCLUDED_
#define _INTERVALVECTOR_H_INCLUDED_

#include "indexvector.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251) //  needs to have dll-interface to be used, bla-bla-bla
#endif

namespace NIMacro {
class DI_MemBlock IntervalVector;
typedef DerivedPointer<IntervalVector> PIntervalVector;


/**
* Unpacks zones-like 3-layer intervalvector object X and initializes output pointers properly. 
* StartAndEnd() MUST be called beforehand because it may move/reformat the data arrays.
* The output pointers will point into the inners of X and will be valid only while X object is alive.
* The memory is managed by the X object, the caller may not deallocate the arrays.
* @param X The 3-layer intervalvector to unpack. If some other data structure is passed in, the behaviour is undefined.
* @param inner Output parameter. A pointer to the IndexVector data array inside the X object. 
*			The elements are indices into the final target image, e.g. zone points. Array starts from index 0.
* @param medium Output parameter. A pointer to the intermediate data array inside the X object. 
*			The elements are indices into the inner array (e.g. to the beginning of a single zone data in the inner array).
*			The zone continues until the start of next zone. There is an extra element in the end medium array pointing
*			after the end of the last zone. The medium array starts from index 0.
* @param outer Output parameter. A pointer to the main data array inside the X object.
*			The elements are indices into the medium array (e.g. to the first zones of objects). The object data in the 
*			medium array continues until the start of the next object. 
*			There is an extra element in the end of outer array pointing
*			after the end of the last object. The outer array starts from index 1, element 0 is unused and may not be dereferencable.
*/
DI_MemBlock void unpack_three_layer_const(const PIntervalVector X, const unsigned int*& inner, const unsigned int*& medium, const unsigned int*& outer);

/// Deprecated, use type-correct 'unsigned int' overload instead.
DI_MemBlock void unpack_three_layer_const(const PIntervalVector X, const int*& inner, const int*& medium, const int*& outer);


/**
* This class is capable to define intervals (element ranges) in a target vector. 
* The target vector is often an IndexVector containing indices into an Image object. 
* In this case the IntervalVector represents arbitrary collection of objects in the Image object (a stencil).
*
* However, the target chain may be longer. In general, there may be arbitray 
* number of IntervalVectors chained after each other. The chain will be terminated by a Vector object which is not
* an IntervalVector. If the final Vector is an IndexVector, then it will be used with some target by itself, but this target is not
* physically a part of the chain (IndexVector can be applied to different targets).
* 
* Each different structure is assigned a different logic_class_t type. One can extend 
* the nomenclature by defining a new logic_class_t constant and enhancing the DetectType() member function to recognize it.
*
* Because of historic reasons the design and interface of this class are quite cumbersome. Use with care!
*/
class DI_MemBlock IntervalVector: public IndexVector {
	typedef IndexVector super;
public: // enums and constants

	/**
	* Represents logical class of the data structure accessible through this IntervalVector object.
	* The logic class is set automatically by construction of the object (see DetectType()).
	*/
	enum logic_class_t {

		/** A stencil of 2D objects. Defines geometrical location of each object. The target is an IndexVector into an Image. Example object list attribute: body.
		*/
		lg_stencil=23,

		/**
		* A subobject stencil. For each object there is an ordered array of subobjects, 
		* the data structure defines an attribute for each subobject. 
		* The target is an IntervalVector of any type.
		* Example object list attributes: zone, zone.x.
		*/
		lg_subobj=24,

		/**
		* Subobject scalar attribute. Defines a single number for each subobject, 
		* i.e. for each object there is an ordered array of numbers. 
		* The target is a numeric Vector. Example object list attribute: zoneintensity, body.x.
		*/
		lg_subnum=25,

//		/// Not in use, reserved for future implementation.
//		lg_stencil_ordered=26,	// stencil containing ordered border-like objects.

		/** A stencil of 3D objects. Defines geometrical location of each object. The target is an IndexVector into a DataCube.
		*/
		lg_datacubestencil=28,	// represents mask with object separation in a 3D datacube.

		/**
		* A stencil of 1D objects. Defines a set of elements for each object in the final target 1D vector.
		* The target is an IndexVector into a Vector. Example: nanmap and infmap output parameters from the ConvElems() module.
		*/
		lg_vectorstencil=29,

		/// Terminating element of IntervalVector::logic_class_t enum.
		lg_end
	};

	/**
	* Type of functions which can be passed to the SetIndexConverter() member function. 
	* The converter function is supposed to map logical zone indices to the physical pixel indices.
	* This facility is used for converting ZoneMask() module input parameters.
	*/
	typedef int (*indexconverter_t)(double x, int roundingmode);

public: // static interface
	/**
	* Create an IntervalVector (vector of intervals in another target vector) by interval start and end index arrays (non-compact representation). 
	* See also pack_two_layer().
	* @param Count Number of intervals.
	* @param FirstIndex - the index of first meaningful element in Start[] and End[]. 
	*		This is typically either 0 or 1.
	*		If FirstIndex is 1, then Start[0] and End[0] are unused and shoud contain zero.
	* @param Start Array of interval start indices in the target vector. 
	*				Start array has to contain at least Count+FirstIndex elements.
	* @param End[] Array of indices one past interval ends in the target vector. 
	*		End array has to contain at least Count+FirstIndex elements.
	* @param pFree Address of the deallocation function for eventual releasing the Start and End arrays. 
	*		Pass NULL if the arrays need not to be released by the IntervalVector.
	* @param poTarget The target vector containing the interval data. 
	*		Each interval starts at index Start[i] in the target vector and ends just before index End[i]. 
	*		I.e., for every i in (FirstIndex .. FirstIndex + Count -1) the following elements must exist in target(s):
	*		poTarget[ Start[i] ] .. poTarget[ End[i] - 1]
	*/
	static PIntervalVector Create(unsigned int Count, unsigned int FirstIndex, unsigned int Start[], unsigned int End[], MemChunk::ReleaseFunc1 pFree, PVector poTarget); 

	
	/**
	* Create a zero-initialized IntervalVector in the compact representation. All intervals will be [0,0) initially.
	* @param Count Number of intervals.
	* @param poTarget The target vector.
	*/
	static PIntervalVector Create(unsigned int Count, PVector poTarget);

	/**
	* Creates the IntervalVector object in a compact representation. See also pack_two_layer().
	* This method is basically the same as another Create() function overload except that here is no End array parameter. 
	* The interval starting at Start[i] is assumed to end just before Start[i+1].
	* There must be an extra element in the end of Start array pointing after the end of the last interval in the target. The extra element is not 
	* taken into account in the Count parameter.
	*/
	static PIntervalVector Create(unsigned int Count, unsigned int FirstIndex, unsigned int Start[], MemChunk::ReleaseFunc1 pFree, PVector poTarget); 


	/**
	* Creates a lg_subnum type IntervalVector from an array of numeric data vectors. 
	* For lg_subnum type IntervalVector there must appear an Vector target object.
	* This is also created by this function and the data is copied over there.
	* @tparam T Template parameter, specifies the type of subvectors. 
	*			This must be a type supported by Vector class (there is an overload of Vector::CType2ElemType() for that type).
	* @param count The number of subvectors int the vecarr array.
	* @param vecarr Pointer to the array of subvectors.
	* 
	*/
	template<typename T>
	static PIntervalVector CreateFromStdVectors(unsigned int count, const std::vector<T>* vecarr); 

	/**
	* Creates a lg_stencil type IntervalVector from an array of vectors of indices.
	* For lg_stencil type IntervalVector there must appear an IndexVector target object.
	* This is also created by this function and the indices are copied over there.
	* The target properties for the IndexVector are specified by the targetdata parameter.
	* @param count The number of subvectors int the vecarr array.
	* @param vecarr Pointer to the array of subvectors.
	* @param targetdata The final target data specification to be assigned to the created IndexVector target.
	* 
	*/
	static PIntervalVector CreateFromStdVectors(unsigned int count, const std::vector<unsigned int>* vecarr, const TargetData& targetdata); 

	/**
	* Creates an lg_subnum type IntervalVector from a vector of vectors.
	* For lg_subnum type IntervalVector there must appear an Vector target object.
	* This is also created by this function and the data is copied over there.
	* @param vector_of_vectors Vector of type PMemory, containing subvector pointers.
	* @param t The type of the created Vector target. All subvector data is converted into that type.
	*/
	static PIntervalVector CreateFromVectorOfVectors(const PVector vector_of_vectors, DataType t=UnsignedInt); 

	/**
	* Creates a lg_stencil type IntervalVector from a vector of vectors. 
	* For lg_stencil type IntervalVector there must appear an IndexVector target object.
	* This is also created by this function and the indices are copied over there.
	* The target properties for the IndexVector are specified by the targetdata parameter.
	* @param vector_of_vectors Vector of vectors. The subvectors must be convertible to UnsignedInt type.
	* @param targetdata The final target data specification to be assigned to the created IndexVector target.
	*/
	static PIntervalVector CreateFromVectorOfVectors(const PVector vector_of_vectors, const TargetData& targetdata); 

	/* 
	* Create a new IntervalVector by cloning the top-level array (interval start and end indices) from the sample IntervalVector
	* and attaching the specified target to it. 
	* and sufficiently large so that intervals would not point outside of it.
	* @param sample The initial IntervalVector providing the interval start and end indices in the target.
	* @param target The target vector to attach to the created IntervalVector. The target must be of one of supported types (see logic_class_t description).
	*/
	static PIntervalVector CreateBySample(const PIntervalVector sample, PVector target);

	/// Create an empty dummy IntervalVector.
	static PIntervalVector Create();

	/// Convert lg_* type constant of this class into the string representation.
	static Nbaseutil::safestring logic2string(logic_class_t t);

	/// Reverse to logic2string().
	static logic_class_t string2logic(const Nbaseutil::safestring& s);

public: // interface

	/// Return logical class of this object. See the logic_class_t enum description.
	logic_class_t GetType() const {return logic_class_t(GetLgType());}

	/**
	* Return a pointer to the internal array of interval start indices. If the IntervalVector object is in the compact mode,
	* then the same array defines also the interval end indices. In this case there is an extra element int the end 
	* of the array pointing after the last interval in the target. Call EnsureStartAndEnd() beforehand in order to ensure
	* that the object is in the compact representation.
	* @param FirstIndex The index of first meaningful element when the array is accessed through the returned pointer. 
	*		Earlier elements may not be dereferencable.
	*/
	const unsigned int* Start(int FirstIndex) const; 

	/**
	* Return a pointer to the internal array of interval end indices. 
	* @param FirstIndex The index of first meaningful element when the array is accessed through the returned pointer. 
	*		Earlier elements may not be dereferencable.
	*/
	const unsigned int* End(int FirstIndex) const; 

	/// Non-const overload of the other overload of Start().
	unsigned int* Start(int FirstIndex); 

	/// Non-const overload of the other overload of End().
	unsigned int* End(int FirstIndex);

	/** Ensure that the object is in a compact representation. 
	* In this case there is no separate End array and all data is accessible through the Start() method.
	* If the compactification is not possible, an exception will be thrown.
	*/
	void EnsureStartAndEnd() const;

	/// Return the End array or NULL, if the object is in compact representation.
	const PVector EndArray() const {return EndArray_;} 
	
	/// Return the End array or NULL, if the object is in compact representation.
	PVector EndArray() {return EndArray_;} 

//	PVector EndPointer() {return EndArray_;} // Return the address of malloc()-ed End[] array, or NULL, if there is no End[] array.

	/// Return the number of intervals held by this object.
	unsigned int IntervalCount() const { return IntervalCount_;}

	/**
	* Reformat the internal data to the compact representation (i.e. interval start indices are used also as preceding interval end indices).
	* @return Return value is an error code:
	*			- 0: the object already was in compact representation or was succesfully converted into the compact representation.
	*			- 1: error - intervals are overlapping.
	*			- 2: target type not yet supported, or cannot be reformatted (e.g. IntervalVector points directly into an image, specifying horizontal pixel row slices).
	*			- 3: rearranging of intervals is needed, but AllowRearrange=false was specified.
	* @param AllowRearrange Allow changing the order of intervals if needed for compactification.
	* @param ppiCode  If not NULL and rearranging was performed, then a pointer to the rearrangement order array is stored here. 
	*				If a non-NULL *ppiCode is returned, the caller has to release it with mb_free() when done.
	*				The ppiCode array will contain the initial indices for objects, in the new order.
	*/
	int Reformat(bool AllowRearrange, unsigned short** ppiCode=NULL); 

	/**
	* A conversion operator. If the target of this object is also an IntervalVector object, then merges these two layers into one intervalvector.
	* The resulting object will contain basically the same intervals as the current target, but only those which are accessible through this object.
	* If the target of this object is not an IntervalVector, then the method does nothing and returns a clone of itself.
	*/
	PIntervalVector MergedTarget() const; 

	/// Return the part of final IndexVector target accessible through this IntervalVector. If there is no IndexVector in the end of target chain, an exception is thrown.
	PIndexVector MergedIndexVector() const; 

	/**
	* Return the part of final Vector target accessible through this IntervalVector. 
	* If there is no Vector-conforming class in the end of target chain, an exception is thrown.
	* If the final chain element is an IndexVector, then this method is basically the same to the MergedIndexVector() except that the result is a simple numeric vector.
	*/
	PVector MergedVector() const;  

	/**
	* Returns a lg_subnum intervalvector pointing to x or y coordinates of points instead of IndexVector.
	* @param xy Either 'x' or 'y'.
	*/
	PIntervalVector GetXYVector(char xy) const; 

	/// Resizes the object to contain n intervals. 
	void SetIntervalCount(unsigned int n);

	/// Mode constant for calling the zone index converter function.
	enum rangespec_t {
		/// A single zone index is to be converted
		single_point=0,
		/// The sart index of a zone range.
		range_start=1,
		/// The end index of a zone range.
		range_end=2,
	};

	/** Assign custom zone index converter. This affects the ConvertIndex() member function.
	*	This function should be called only for IntervalVectors representing zone structures.
	*	In this case the index converter should map the logical zone indices to physical pixel indices in such a way
	*	that the ZoneMask() module result would look approximately the same as in case of the standard zone structure.
	*	@param x A pointer to the index conversion function. Pass NULL to revert to the default converter.
	*/
	void SetIndexConverter(indexconverter_t x) {indexconverter_=x;}

	/**
	* Convert logical zone index to physical. The default converter returns the z argument rounded. 
	* If a custom index converter has been installed by SetIndexConverter(), then it will be called by the same arguments.
	* This function is called for converting ZoneMask() module input parameters, for example.
	*/
	int ConvertIndex(double z, rangespec_t mode) const; 

	/**
	* Analyze the dynamic composition of this data structure and return the relevant logic class this object conforms to.
	* Throws in case of unsupported structures. This is called from constructors.
	*/
	logic_class_t DetectType() const;

	/// Return the target object, no checks or conversions done.
	PVector Target() {return target_;}

	/// Return the target object, no checks or conversions done.
	const PVector Target() const {return target_;}

	void SetTarget(PVector target);

	/// If the target is an IndexVector or IntervalVector, return it, otherwise throw.
	PIndexVector IndexVectorTarget();

	/// If the target is an IndexVector or IntervalVector, return it, otherwise throw.
	const PIndexVector IndexVectorTarget() const;

	/// If the target chain ends with an object of IndexVector class, return it, otherwise return NULL.
	PIndexVector GetFinalIndexVectorTarget();

public: // virtual overrides
	virtual PMemBlock DoClone(copymode_t copy_mode) const;
	virtual const char*	Class() const {return "intervalvector";}	
	virtual bool Conforms(const char *szClassName) const;
	virtual bool Consistent(Nbaseutil::safestring& msg, bool CheckContent) const;
	virtual PMemBlock ConvertTo(const Nbaseutil::safestring& classname) const;
	virtual SafeValue DoVerb(const char* pszCommand, int iArg, SafeValue vArg[]);
	virtual SafeValue ResolveSubItem(const Nbaseutil::safestring& subitemname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed) const;
	virtual void Rearrange(const int code[], unsigned int codelen, unsigned int newlen, Nbaseutil::ThrowHandlerFunc th=ThrowIndeed);
	virtual PImage GetMaskImage() const;
	virtual void GetImageDimensions(unsigned int& width, unsigned int& height, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed) const;
	virtual PMemBlock op(char opcode, const PMemBlock y0, const op_options& options) const;
	virtual void Append(PVector b);
	virtual bool Entertain(MemBlockVisitor& visitor, const Nbaseutil::safestring& name, entertainmode_t mode=enter_deep);	
	virtual void IterateChildren(NIMacro::AcaVisitor& visitor, const NIMacro::TraverseNode& node);

protected: // virtual overrides
	/**
	* Supports additional subitem names, mostly used by deserialization from HDF and other formats.
	*    - inner_index_array: item must be an IndexVector; the final target in the IntervalVector chain is replaced by item.
	*    - inner_vector: item must be a Vector; the final target in the IntervalVector chain is replaced by item.
	*    - interval_count: item must be an integer number; the intervalcount is changed and the IntervalVector array resized. Any added intervals are initialized to zero size.
	*    - medium_index_array: item must be an IntervalVector; the current target is replaced by it; if the current target is an IntervalVector as well, its target will be set as the target of item.
	*    - outer_index_array: item must be an IntervalVector; the current interval data will be replaced by the data from item. The current target is preserved.
	*    - target: item must conform to Vector; if there is an IndexVector in the end of the target chain, its target data is set to match item.
	*    - target_data: item must be a small numeric vector containing dimension sizes; if there is an IndexVector in the end of the target chain, its target data is replaced by the content of item.
	*    - type: item must be a string, specifying a valid logical class for IntervalVector, see LgType2String(); the logical type of the object is changed to the specified type (see DoSetLgType()).
	*/
	virtual bool DoSetSubItem(const Nbaseutil::safestring& subitemname, const SafeValue& item, const Nbaseutil::safestring& fullpathname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed);
	virtual Nbaseutil::safestring DoGetDescription() const;

	/*
	* Returns the object logical type, one of: "stencil", "subobj", "subnum", "datacubestencil", "vectorstencil".
	*/
	virtual Nbaseutil::safestring LgType2String(int lg_type) const;
	virtual int	String2LgType(const Nbaseutil::safestring& lg_type);
	virtual void DoSetLgType(int lg_type);
	virtual bool Equals(const MemBlock& b) const; 
	virtual PVector DoCloneSlice(unsigned int k, unsigned int l) const; // return a slice of itself corresponding to elements k to l (excl.) 

private: // implementation
	IntervalVector(unsigned int Count, unsigned int FirstIndex, unsigned int Start[], unsigned int End[], MemChunk::ReleaseFunc1 pFree, PMemBlock poTarget); 
	IntervalVector(const IntervalVector& b);

	// to avoid misuse of StartAndEnd - it may move data underneath of us.
	friend void unpack_two_layer_const(const PIntervalVector X, const int*& inner, const int*& outer);
	friend DI_MemBlock void unpack_three_layer_const(const PIntervalVector X, const int*& inner, const int*& medium, const int*& outer);
	friend void unpack_two_layer(PIntervalVector X, int*& inner, int*& outer);
	friend void unpack_two_layer_const(const PIntervalVector X, const unsigned int*& inner, const unsigned int*& outer);
	friend DI_MemBlock void unpack_three_layer_const(const PIntervalVector X, const unsigned int*& inner, const unsigned int*& medium, const unsigned int*& outer);
	friend void unpack_two_layer(PIntervalVector X, unsigned int*& inner, unsigned int*& outer);
	unsigned int* StartAndEnd(int FirstIndex, bool AllowRearrange=false, bool yReformat=true, unsigned short** ppiCode=NULL) ;	
	const unsigned int* StartAndEnd(int FirstIndex, bool AllowRearrange=false, bool yReformat=true, unsigned short** ppiCode=NULL) const;	
//	void DropOrderedBorders();	// announce that the object is going to be modified or was modified and the contained borders may not be ordered any more.
	void AddEndArray();	// Creates the EndArray_, if this is not present, and inits it properly.
	void RebuildAs(logic_class_t lg_type);
	void UpdateType(); // set the logic type match the actual structure.

private: // data
	friend class Reformat_tmpl_helper;
	unsigned int IntervalCount_;	
	PVector EndArray_;
	indexconverter_t indexconverter_;
	PVector target_;
};



/**
* Unpacks body-like 2-layer intervalvector object X and initializes output pointers properly. 
* StartAndEnd() MUST be called beforehand because it may move/reformat the data arrays.
* The output pointers will point into the inners of X and will be valid only while X object is alive.
* The memory is managed by the X object, the caller may not deallocate the arrays.
* @param X The 2-layer intervalvector to unpack. If some other data structure is passed in, the behaviour is undefined.
* @param inner Output parameter. A pointer to the IndexVector data array inside the X object. 
*			The elements are indices into the final target image, e.g. zone points. Array starts from index 0.
* @param outer Output parameter. A pointer to the main data array inside the X object.
*			The elements are indices into the inner array (to the start of object data in the inner array).
*			The object data in the 
*			inner array continues until the start of the next object. 
*			There is an extra element in the end of outer array pointing
*			after the end of the last object. The outer array starts from index 1, element 0 is unused and may not be dereferencable.
*/
inline void unpack_two_layer_const(const PIntervalVector X, const unsigned int*& inner, const unsigned int*& outer) {
	outer = X->StartAndEnd(1);	// cell counting starts from 1
	inner = X->IndexVectorTarget()->UnsignedIntPointer();
}

/// Deprecated, use type-correct 'unsigned int' overload instead.
inline void unpack_two_layer_const(const PIntervalVector X, const int*& inner, const int*& outer) {
	outer = reinterpret_cast<const int*>(X->StartAndEnd(1));	// cell counting starts from 1
	inner = reinterpret_cast<const int*>(X->IndexVectorTarget()->IntPointer());
}


/// A non-const version of unpack_two_layer_const(). See unpack_two_layer_const() for the parameter descriptions.
inline void unpack_two_layer(PIntervalVector X, unsigned int*& inner, unsigned int*& outer) {
	outer = (X->StartAndEnd(1));	// cell counting starts from 1
	inner = (X->IndexVectorTarget()->UnsignedIntPointer());
}

/// Deprecated, use type-correct 'unsigned int' overload instead.
inline void unpack_two_layer(PIntervalVector X, int*& inner, int*& outer) {
	outer = reinterpret_cast<int*>(X->StartAndEnd(1));	// cell counting starts from 1
	inner = reinterpret_cast<int*>(X->IndexVectorTarget()->IntPointer());
}

/**
* Reverse of Unpack_two_layer_const().
* @param cellcount The number of objects. The outer array must contain at least cellcount+2 elements.
* @param inner The array of indices in the target image. The array must be allocated by mb_malloc() and contain at least outer[cellcount+1] elements. 
*				The data in the inner array starts from index 0. 
* @param outer The array of indices into the inner array, indicating the start of object data in the inner array. 
*		The array must be allocated by mb_malloc() and contain at least cellcount+2 elements.
*		Element 0 is unused and must be set to 0. Element outer[cellcount+1] must point after the end of the last object in the inner array.
* @param target A sample target image of correct width and height, for initializing the target data in the encapsulated IndexVector.
* @return A body-like 2-level IntervalVector (type lg_stencil). The ownership of inner and outer arrays is taken over by the IntervalVector object.
*/
inline PIntervalVector pack_two_layer(int cellcount, unsigned int* inner, unsigned int* outer, PImage target) {
	return IntervalVector::Create( cellcount, 1, (outer), &Nbaseutil::mb_free, 
		IndexVector::Create( outer[cellcount+1], (inner), 0, &Nbaseutil::mb_free, target));
}

/// Deprecated, use type-correct 'unsigned int' overload instead.
inline PIntervalVector pack_two_layer(int cellcount, int* inner, int* outer, PImage target) {
	return IntervalVector::Create( cellcount, 1, reinterpret_cast<unsigned int*>(outer), &Nbaseutil::mb_free, 
		IndexVector::Create( outer[cellcount+1], reinterpret_cast<unsigned int*>(inner), 0, &Nbaseutil::mb_free, target));
}

/// The same as the other overload of pack_two_layer() except the target data is specified by width and height only.
inline PIntervalVector pack_two_layer(int cellcount, unsigned int* inner, unsigned int* outer, int width, int height) {
	// reverse of Unpack_two_layer_const
	// the arrays must be initialized properly, i.e. outer[cellcount+1] must contain the length of inner array.
	return IntervalVector::Create( cellcount, 1, (outer), &Nbaseutil::mb_free, IndexVector::Create( outer[cellcount+1], (inner), 0, &Nbaseutil::mb_free, width, height));
}

/// Deprecated, use type-correct 'unsigned int' overload instead.
inline PIntervalVector pack_two_layer(int cellcount, int* inner, int* outer, int width, int height) {
	// reverse of Unpack_two_layer_const
	// the arrays must be initialized properly, i.e. outer[cellcount+1] must contain the length of inner array.
	return IntervalVector::Create( cellcount, 1, reinterpret_cast<unsigned int*>(outer), &Nbaseutil::mb_free, IndexVector::Create( outer[cellcount+1], reinterpret_cast<unsigned int*>(inner), 0, &Nbaseutil::mb_free, width, height));
}

/// The same as the other overload of pack_two_layer() except the target data is specified by an IndexVector::TargetData object.
inline PIntervalVector pack_two_layer(int cellcount, unsigned int* inner, unsigned int* outer, const IndexVector::TargetData& target_data) {
	// reverse of Unpack_two_layer_const
	// the arrays must be initialized properly, i.e. outer[cellcount+1] must contain the length of inner array.
	return IntervalVector::Create( cellcount, 1, (outer), &Nbaseutil::mb_free, 
		IndexVector::Create( outer[cellcount+1], (inner), 0, &Nbaseutil::mb_free, target_data));
}

/// Deprecated, use type-correct 'unsigned int' overload instead.
inline PIntervalVector pack_two_layer(int cellcount, int* inner, int* outer, const IndexVector::TargetData& target_data) {
	// reverse of Unpack_two_layer_const
	// the arrays must be initialized properly, i.e. outer[cellcount+1] must contain the length of inner array.
	return IntervalVector::Create( cellcount, 1, reinterpret_cast<unsigned int*>(outer), &Nbaseutil::mb_free, 
		IndexVector::Create( outer[cellcount+1], reinterpret_cast<unsigned int*>(inner), 0, &Nbaseutil::mb_free, target_data));
}

/** Convert a two-layer intervalvector into a simple vector of type PMemory
* which contains pointers to separated short IndexVectors.
*/
DI_MemBlock PVector intervalvector_to_pmemory_vector(const PIntervalVector IV);

/// Reverse to intervalvector_to_pmemory_vector().
DI_MemBlock PIntervalVector pmemory_vector_to_intervalvector(const PVector V);

/** Transforms shift indexes calculated for image pomatrix to 
* shift indexes for image poImage
* Invariant in this transformation are x and y coordinates relative to the center of the image,
* i.e. x-x0 and y-y0 are constants in the transformation, where x0 and y0 are coordinates of the image center
*/
DI_MemBlock const PVector PrepareMatrixes_f(const PImage poMatrix, const PImage poImage);

} // namespace NIMacro

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
