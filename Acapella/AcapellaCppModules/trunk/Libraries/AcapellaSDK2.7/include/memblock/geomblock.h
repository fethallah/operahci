#ifndef x_ACAPELLA_MEMBLOCK_GEOMBLOCK_H_INCLUDED
#define x_ACAPELLA_MEMBLOCK_GEOMBLOCK_H_INCLUDED

#include "vector.h"

namespace NIMacro {

	/**
	* A class template capable of defining a N-cube type structures (N=2,3,...) on top of Vector class.
	* The Image and DataCube classes are derived from specializations of this template.
	* @param rank The template argument specifying the number of dimensions for the object.
	*/
	template<int rank>
	class GeomBlock: public Vector {
	public: // static interface
		/**
		* Create a new object by encapsulating a TypedChunk data array.
		* @param dimsize The array specifying the dimension sizes. The array size must be equal to the object rank.
		* @param chunk The data array to encapsulate. It will remain physically in shared use at first.
		* @param factor Raw factor for the data.
		*/
		static PVector Create(const unsigned int* dimsize, const TypedChunk& chunk, double factor) {
			return new GeomBlock<rank>(dimsize, chunk, factor);
		}
	protected: // interface for derived classes.
		/// Ctor for using by derived classes.
		GeomBlock(const unsigned int* dimsize, size_t length, DataType elem_type, double factor):
			Vector(length, elem_type, factor)
		{
			ChangeDimSizes(dimsize, rank);
		}

		/// Ctor for using by derived classes.
		GeomBlock(const unsigned int* dimsize, size_t length, DataType elem_type, double factor, void *p, MemChunk::ReleaseFunc1 p_free, size_t offset=0):
			Vector(length, elem_type, factor, p, p_free, offset)
		{
			ChangeDimSizes(dimsize, rank);
		}

		/// Ctor for using by derived classes.
		GeomBlock(const unsigned int* dimsize, size_t length, DataType elem_type, double factor, void *p, MemChunk::ReleaseFunc2 p_free, void* callback_data, size_t offset=0):
			Vector(length, elem_type, factor, p, p_free, callback_data, offset)
		{
			ChangeDimSizes(dimsize, rank);
		}

		/// Ctor for using by derived classes.
		GeomBlock(const unsigned int* dimsize, const TypedChunk& chunk, double factor):
			Vector(chunk, factor)
		{
			ChangeDimSizes(dimsize, rank);
		}

		/// Ctor for using by derived classes.
		GeomBlock(const GeomBlock<rank>& b):
			Vector(b)
		{
			ChangeDimSizes(b.dimsize_, rank);
		}
	public: // virtual overrides
		virtual int GetRank() const {return rank;}
		virtual unsigned int GetDimSize(int dimension) const {return (dimension>=0 && dimension<rank)? dimsize_[dimension]: 0;}
		virtual const unsigned int* GetDimSizeArray() const {return dimsize_;}
		virtual void ChangeDimSizes(const unsigned int* dimsize, int ndimsizes) {
			if (rank!=ndimsizes) {
				throw Nbaseutil::Exception(Nbaseutil::ERR_PROGRAM_ERROR, Nbaseutil::Printf("GeomBlock::ChangeDimSizes() is called with an invalid number of dimensions %d for: %s")(ndimsizes)(GetDescription()));
			}
			size_t length=1;
			for (int i=0; i<rank; ++i) {
				dimsize_[i] = dimsize[i];
				length*=dimsize[i];
			}
			if (Length()!=length) {
				ResizeVector(length);
			}
		}
		virtual PVector GetDimSizes() const {
			PVector v = Vector::Create(rank, Vector::UnsignedInt);
			unsigned int* dst = v->UnsignedIntPointer();
			for (int i=0; i<rank; ++i) {
				dst[i] = dimsize_[i];
			}
			return v;
		}
	private: // data
		friend class Image; // by historic reasons Image class has exposed public fields width and height.
		unsigned int dimsize_[rank];
	};

} // namespace

#endif
