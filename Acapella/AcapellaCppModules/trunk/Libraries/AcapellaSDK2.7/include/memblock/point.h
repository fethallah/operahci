#ifndef x_IMACRO_MEMBLOCK_POINT_H_INCLUDED_
#define x_IMACRO_MEMBLOCK_POINT_H_INCLUDED_
#include "vector.h"
#include "indexvector.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

namespace NIMacro {
	
	class DI_MemBlock Point;
	typedef DerivedPointer<Point> PPoint;

	/// A simple class encapsulating a point in N-dimensional space. The point coordinates are stored as vector elements: x, y, etc.
	class DI_MemBlock Point: public Vector {
		typedef Vector super;
	public: // static interface

		// Change 29.04.2004: use signed ints to allow points outside of image borders
		/// Create a 2D point (a point in image). The coordinates may be negative to indicate points outside of image borders.
		static	PPoint Create(int x, int y); 
		/// Create a N-dimensional point, N is specified by rank and coordinates by the coord array having rank elements.
		static	PPoint Create(int coord[], int rank); 

		/// Create a 2D point and attach target image description.
		static	PPoint Create(int x, int y, const IndexVector::TargetData& target_data); 

		/// Create a point by a single index and attach the target object description. The index is the physical index into the N-dimensional data.
		static	PPoint Create(unsigned int index, const IndexVector::TargetData& target_data); 

		/// Create a dummy point.
		static	PPoint Create() {return Create(0,0);}

	public: // interface
		/// Return target description.
		const IndexVector::TargetData& TargetData() const {return target_data_;}			// Return the reference to the target data

	public: // virtual overrides
		virtual PMemBlock DoClone(copymode_t copy_mode) const;
		virtual const char*	Class() const {return "Point";}	
		virtual bool Conforms(const char *szClassName) const;
		virtual bool DoSetSubItem(const Nbaseutil::safestring& subitemname, const SafeValue& item, const Nbaseutil::safestring& fullpathname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed);
		virtual bool Entertain(MemBlockVisitor& visitor, const Nbaseutil::safestring& name, entertainmode_t mode=enter_deep);	
		virtual void IterateChildren(NIMacro::AcaVisitor& visitor, const NIMacro::TraverseNode& node);
		virtual SafeValue ResolveSubItem(const Nbaseutil::safestring& subitemname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed) const;	
		virtual void ResizeVector(size_t n, bool yCopyContent=true, bool yInitToZero=true);
//		virtual int GetRank() const {return Length();}

	protected: // virtual overrides
		virtual Nbaseutil::safestring DoGetDescription() const;

	protected: // implementation
		Point(int rank);
		Point(const Point& b);

	private: // data
		IndexVector::TargetData target_data_;
	};

}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
