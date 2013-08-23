#ifndef x_IMACRO_LICFIELD_H_INCLUED
#define x_IMACRO_LICFIELD_H_INCLUED

//#include <algorithm>
//#include <vector>

namespace NIMacro {

using Nbaseutil::uint64;

const int c_lic_maxcount = sizeof(uint64)*8;

/** 
* A bitfield class used for license control mechanisms.
*/
class licfield: public mb_malloced {
	uint64 lic_;	// The bitfield
public:
	licfield()
		: lic_(0)
	{}
	licfield(int bit)
		: lic_(0) 
	{
		SetBit(bit);
	}
	licfield(uint64 init64)
		: lic_(init64) 
	{}

	// Default copy ctor and assignment op are ok.

	/** Raise the bit in the licfield.
	*/
	void SetBit(int bit) {
		lic_ |= ((uint64)1) << bit;
	}
	/// Add all bits in mask to the licfield
	void AddBits(const licfield& mask) {
		lic_ |= mask.lic_;}

	/** Remove bit from the field, if set. 
	*/
	void DropBit(int bit) {
		uint64 x = ((uint64)1) << bit;
		lic_ &= ~x;
	}

	/// Return true if any bits are raised.
	bool HasAnyBit() const {return lic_!=0;} 

	/// Return true if bit is set.
	bool HasBit(int bit) const {
		return (lic_ & (((uint64)1) <<bit)) !=0;
	}

	/// Return true if this licfield contains any raised bit of b.
	bool ContainsAnyBitOf(const licfield& b) const {
		return (lic_ & b.lic_)!=0;
	}

	/// Remove all bits from the licfield.
	void Clear() {
		lic_=0;
	}

	/// Raise all bits.
	void SetAllBits() {
		lic_ = static_cast<uint64>(-1);
	}

	operator uint64() const {return lic_;}
};

} // namespace
#endif
