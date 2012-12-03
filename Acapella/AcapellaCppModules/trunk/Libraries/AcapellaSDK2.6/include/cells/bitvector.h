#ifndef _IMACRO_BITVECTOR_H_INCLUDED_
#define _IMACRO_BITVECTOR_H_INCLUDED_

// A semi-automatically resizable bit vector.
#include <stdlib.h>
#include <memory.h>
#include <string.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4511 4512 4514)
#endif

namespace NIMacro {

/// @cond Implementation_details

// There are two types of bitvectors defined, with the same interface.
// BitVectorSmall holds packed bits and is 8 times smaller than BitVectorFast.
// BitVectorFast holds one bit per byte and is typically 10% faster than BitVectorSmall.
// Of course, in case of huge arrays which do not fit well in physical memory, BitVectorFast may become a lot slower.

// There is no cpp file, all functions are defined in the class body.
// The two classes are not derived from a virtual interface base class because virtual functions could not be inlined.
class DI_Cells BitVectorSmall {
	unsigned int n_;	// length of vector in bits
	unsigned int size_;	// length of allocated memory buffer, in bytes.
	unsigned char* p_;	// pointer to bit array.
public:
	BitVectorSmall(int n): n_(n), size_((n-1)/8+1) {p_ = (unsigned char*) mb_calloc(size_, 1); }	// create with number of bits. Bits are inited to zero.
	BitVectorSmall() : n_(0), size_(4) { p_ = (unsigned char*) mb_calloc(size_,1); }					// create an empty vector.
	~BitVectorSmall() { if (p_) mb_free(p_); p_ = NULL;}

	BitVectorSmall& operator=(const BitVectorSmall& b) {
		if (p_) mb_free(p_);
		n_ = b.n_;
		size_ = b.size_;
		p_ = (unsigned char*) mb_malloc(size_);
		memcpy(p_, b.p_, size_);
		return *this;
	}

	unsigned int Length() const {return n_;}	// return the number of bits

	unsigned char operator[] (unsigned int i) const {			// return the bit value at index i. No bounds-checking.
		return (unsigned char) ((p_[i>>3] & (1<<(i&7)))? 1:0);
	}
	unsigned char GetSafe(unsigned int i) { if (i<n_) return (*this)[i]; else return 0;}	// return the bit value, with bounds-checking.

	void Set(unsigned int i, unsigned char x=1) {			// set the bit value x at index i. No bounds-checking.
		register unsigned char* p = p_ + (i>>3);
		if (x) {
			*p |= (1<<(i&7));
		} else {
			*p &= ~(1<<(i&7));
		}
	}
	void Clear(unsigned int i) {Set(i,0);}					// clear the bit value at index i. No bounds-checking.

	unsigned int SetLength(unsigned int n) {							// Resize the array to hold n bits. Init new bits to zero. Return the old length.
		unsigned int size = (n-1)/8+1;
		if (n==0) size=0;
		if (size>size_ || (size < size_>>1 && size_ > 4) ) {
			if (size>size_ && size < 2*size_) size = 2*size_;	// exponential growth when adding bits one-by-one.
			p_ = (unsigned char*) mb_realloc(p_, size);
			if (size>=size_) {
				memset(p_ + size_, 0, size-size_);
			}
			size_ = size;
		}
//		if (n>n_) {
//			int i=n_;
//			do { Clear(i++);} while (i&7);
//		}
		unsigned int n_old = n_;
		n_ = n;
		return n_old;
	}
	void SetSafe( unsigned int i, unsigned char x=1) { if (i>=n_) {SetLength(i+1);} Set(i,x); } // Set the bit value, resizing the array if needed.
	void ClearSafe( unsigned int i) { if (i>=n_) {SetLength(i+1);} Set(i,0); } // Clear the bit value, resizing the array if needed.
	void SetAll(unsigned char x=1) {		// set all bits to x.
		memset(p_, x?255:0, size_);
	}
	void Shift(int k) {
		// Shift the array by k bits.
		if (k>0) {
			int n=SetLength(Length()+k);
			if (k%8==0) {
				memmove(p_+k/8 , p_, (n-1)/8+1);
				memset(p_, 0, k/8);
			} else {
				for (int i=n-1; i>=0; i--) Set(i+k, (*this)[i]);
				for (int j=0; j<k; j++) Clear(j);
			}
		} else {
			int n=Length();
			if (k%8==0) {
				memmove(p_, p_ + (-k)/8, (n+k-1)/8+1);
				SetLength(n+k);
			} else {
				for (int i=0; i<n; i++) Set(i, (*this)[i-k]);
				SetLength(n+k);
			}
		}
	}
private:
	BitVectorSmall(const BitVectorSmall& b);	// unimplemented; use operator=.

};

/// An internal class for holding validity data in Attribute class.
class BitVectorFast {
	unsigned int n_;		// length of vector in bits
	unsigned int size_;	// length of allocated memory buffer, in bytes.
	unsigned char* p_;	// pointer to bit array.
public:
	BitVectorFast(unsigned int n): n_(n), size_(n) { p_ = (unsigned char*) mb_calloc(size_, 1); }	// create with number of bits. Bits are inited to zero.
	BitVectorFast() : n_(0), size_(1) { p_ = (unsigned char*) mb_calloc(size_,1); }					// create an empty vector.
	~BitVectorFast() { if (p_) mb_free(p_);}

	BitVectorFast& operator=(const BitVectorFast& b) {
		if (p_) mb_free(p_);
		n_ = b.n_;
		size_ = b.size_;
		p_ = (unsigned char*) mb_malloc(size_);
		memcpy(p_, b.p_, size_);
		return *this;
	}

	unsigned int Length() const {return n_;}	// return the number of bits

	unsigned char operator[] (unsigned int i) {			// return the bit value at index i. No bounds-checking.
		return p_[i];
	}
	unsigned char GetSafe(unsigned int i) { if (i<n_) return (*this)[i]; else return 0;}	// return the bit value, with bounds-checking.

	void Set(unsigned int i, unsigned char x=1) {			// set the bit value x at index i. No bounds-checking.
		p_[i] = x;
	}
	void Clear(unsigned int i) {Set(i,0);}					// clear the bit value at index i. No bounds-checking.

	void SetLength(unsigned int n) {							// Resize the array to hold n bits. Init new bits to zero.
		unsigned int size = n;
		if (size>size_ || (size < size_>>1 && size_ > 4) ) {
			if (size>size_ && size < 2*size_) size = 2*size_;	// exponential growth when adding bits one-by-one.
			p_ = (unsigned char*) realloc(p_, size);
			if (size>=size_) {
				memset(p_ + size_, 0, size-size_);
			}
			size_ = size;
		}
		n_ = n;
	}
	void SetSafe( unsigned int i, unsigned char x=1) { if (i>=n_) {SetLength(i+1);} Set(i,x); } // Set the bit value, resizing the array if needed.
	void ClearSafe( unsigned int i) { if (i>=n_) {SetLength(i+1);} Set(i,0); } // Clear the bit value, resizing the array if needed.
	void SetAll(unsigned char x=1) {		// set all bits to x.
		memset(p_, x, size_);
	}
	void Shift(int k) {
		// Shift the array by k bits.
		if (k>0) {
			SetLength( Length()+k);
			memmove(p_+k, p_, n_);
			memset(p_,0,k);
		} else {
			memmove(p_, p_-k, n_);
			SetLength( Length()+k);
		}
	}
private:
	BitVectorFast(const BitVectorFast& b);	// unimplemented; use operator=.

};
/// @endcond

} // namespace

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
