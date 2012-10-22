#ifndef _IMACRO_MEMBLOCK_TRIPLE_H_INCLUDED_
#define _IMACRO_MEMBLOCK_TRIPLE_H_INCLUDED_

namespace NIMacro {

/**
* A 3-byte (24-bit) data type. This can be interpreted as:
*    - an unsigned integer in range 0..16777215
*    - three 8-bit unsigned integers for RGB color indices.
* The blue color has the lowest value in the large integer interpretation.
*/
struct triple {
public: // data
	/// The data buffer containing 3 unsigned integers. If interpreted as RGB, then blue color is in x[0].
	unsigned char x[3];
public: // interface
	/// Construct a zero value.
	triple() {x[0]=0; x[1]=0; x[2]=0; }
	
	/// Construct an RGB value from color indices in range 0..255 (overflow not checked).
	triple(int blue, int green, int red) {x[0]=(unsigned char) blue; x[1]=(unsigned char) green; x[2]=(unsigned char) red; }

	/// Assign the value from an integer in range 0..16777215 (overflow not checked). 8 least significant bits of z will be assigned to blue color, etc.
	void assign(unsigned int z) {
#ifdef _DEBUG
		if (z>>24!=0) {
			throw Nbaseutil::Exception(Nbaseutil::ERR_PROGRAM_ERROR, "Triple assignment overflow.");
		}
#endif
		x[0] = (unsigned char) (z & 0xFF); x[1] = (unsigned char) ((z >> 8) & 0xFF); x[2] = (unsigned char) ((z>>16) & 0xFF); 
	}
	/// Construct the value from an integer in range 0..16777215 (overflow not checked). 8 least significant bits of z will be assigned to blue color, etc.
	triple(unsigned int z) { assign(z); }

	/// Construct the value from a double in range 0..16777215 (range not checked). This ctor is mostly for silencing compiler warnings in templates.
	explicit triple(double z) { assign(Nbaseutil::debug_cast<unsigned int>(z+0.5));}
	explicit triple(float z) { assign(Nbaseutil::debug_cast<unsigned int>(z+0.5));}
	// Also explicit ctors for integers are needed then, to resolve ambiguites.
	explicit triple(int z) { assign(Nbaseutil::debug_cast<unsigned int>(z));}
	explicit triple(unsigned long z) { assign(Nbaseutil::debug_cast<unsigned int>(z));}
	explicit triple(long z) { assign(Nbaseutil::debug_cast<unsigned int>(z));}

#ifndef ACAPELLA_LONG_IS_INT64
	explicit triple(Nbaseutil::int64 z) { assign(Nbaseutil::debug_cast<unsigned int>(z));}
	explicit triple(Nbaseutil::uint64 z) { assign(Nbaseutil::debug_cast<unsigned int>(z));}
#endif

	/// Return the value in 3 lower bytes of an unsigned int, the lowest order byte is blue.
	operator unsigned int() const { return x[0] | (x[1]<<8) | (x[2] << 16); }

	/// Interprets the content as an int and adds the parameter to it.
	void operator+=(int k) { assign( unsigned(*this) + k); }

	/// Interprets the content as an int and subtracts the parameter from it.
	void operator-=(int k) { assign( unsigned(*this) - k); }

	/// Scales the color components by the value of k in range 0..255. Using k=255 would mean no change.
	triple weigh(unsigned char k) const {return triple( x[0]*k/255, x[1]*k/255, x[2]*k/255);}

	/// Compares the value with another triple object.
	bool operator==(const triple& b) const {return x[0]==b.x[0] && x[1]==b.x[1] && x[2]==b.x[2];}

	/// Return the red component of the triple value.
	unsigned char red() const {return x[2];}

	/// Return the green component of the triple value.
	unsigned char green() const {return x[1];}

	/// Return the blue component of the triple value.
	unsigned char blue() const {return x[0];}

	/// Return the value as an unsigned int, with blue as the lowest order byte. This is the same as operator unsigned int().
	unsigned int bgr() const {return static_cast<unsigned int>(*this);}

	/// Return the value as an unsigned int, with red as the lowest order byte. This is opposite to the internal byte order in the triple.
	unsigned int rgb() const {return x[2] | (x[1]<<8) | (x[0] << 16);}

};

}

namespace Nbaseutil {
	template<> inline int isinf<NIMacro::triple>(NIMacro::triple x) {return 0;}
	template<> inline int isnan<NIMacro::triple>(NIMacro::triple x) {return 0;}
	template<> inline int isfinite<NIMacro::triple>(NIMacro::triple x) {return 1;}
}


#endif
