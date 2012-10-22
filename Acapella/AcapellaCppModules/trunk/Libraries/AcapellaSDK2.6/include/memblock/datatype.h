// DATA TYPES
#ifndef _DATATYPE_H_INCLUDED_
#define _DATATYPE_H_INCLUDED_


namespace NIMacro {

/// Types for DataItem and SafeValue multitype values
enum ItemType {
	/// An int.
	Integer		=1,
	/// A double.
	Floating	=2,
	/// A naked data pointer (void*) 
	Pointer		=4,
	/// Pointer to a MemBlock hierarchy object.
	Memory		=5,
	/// A zero-terminated string 
	Asciiz		=6,	
	/// Synonym for Asciiz
	Stringy		=Asciiz, 
	/// Pointer to an object derived from ThreadSharable virtual base class.
	Sharable	=7,
	/// A DataItem or SafeValue 
	PolyType	=8,
//	/// A softlink to another data item (not yet implemented).
//	SoftLink	=9,
	/// A value of type Nbaseutil::int64
	Integer64	=10,
	/// Empty (no value). 
	Undefined	=11,
};

/// A wrapper class for convenient definition of vector element types. The actual classes derive from this class to inherit the DataType enum definition.
class DataTypeBase {
public:
/**
* Possible element types for vector data. Most of these map to basic C types on the given architecture. Note that 'long' is not used as its size 
* depends too much on the platform. The numeric values are the same as for NIMacro::TypedChunk::DataType and NIMacro::Vector::DataType enums.
*/
enum DataType {
	/// 32-bit signed integer
	Int,
	/// 32-bit unsigned integer
	UnsignedInt, 
	/// 16-bit signed integer
	Short,
	/// 16-bit unsigned integer
	UnsignedShort,
	/// 8-bit signed integer
	Char,
	/// 8-bit unsigned integer
	Byte,
	/// Synonym for Byte
	UnsignedChar = Byte,
	/// 32-bit IEEE float
	Float,
	/// 64-bit IEEE double
	Double,
	/// Raw void* pointer, size 32 or 64 bits depending on compiler and hardware, usage discouraged.
	VoidStar,
	/// PMemBlock hierarchy smartpointers.
	PMemory,
	/// Objects of triple type (interpreted as either 24-bit unsigned integer or 3 8-bit unsigned integers (RGB code, blue channel at the lowest value)).
	Triple,
	/// Placeholder for indicating no type, missing type or unknown type.
	Void,
	/// Obsolete, do not use.
	SubVector,
	/// Objects of Nbaseutil::safestring type.
	String,
	/// Objects of DataItem type.
	PolyType,
	/// 64-bit signed integer
	Int64,
	/// 64-bit unsigned integer
	UnsignedInt64,
	/// marks last item. must be last in this list.
	DataType_end,
};
};

//@{ Conversion from C type to NIMacro::DataType //@}
inline DataTypeBase::DataType TypeOf(const unsigned char dummy) {return DataTypeBase::Byte;}
inline DataTypeBase::DataType TypeOf(const char dummy) {return DataTypeBase::Char;}
inline DataTypeBase::DataType TypeOf(const unsigned short dummy) {return DataTypeBase::UnsignedShort;}
inline DataTypeBase::DataType TypeOf(const short dummy) {return DataTypeBase::Short;}
inline DataTypeBase::DataType TypeOf(const unsigned int dummy) {return DataTypeBase::UnsignedInt;}
inline DataTypeBase::DataType TypeOf(const int dummy) {return DataTypeBase::Int;}
inline DataTypeBase::DataType TypeOf(const float dummy) {return DataTypeBase::Float;}
inline DataTypeBase::DataType TypeOf(const double dummy) {return DataTypeBase::Double;}
inline DataTypeBase::DataType TypeOf(const void* dummy) {return DataTypeBase::VoidStar;}
inline DataTypeBase::DataType TypeOf(const Nbaseutil::int64 dummy) {return DataTypeBase::Int64;}
inline DataTypeBase::DataType TypeOf(const Nbaseutil::uint64 dummy) {return DataTypeBase::UnsignedInt64;}
inline DataTypeBase::DataType TypeOf(const Nbaseutil::safestring& dummy) {return DataTypeBase::String;}

// special void class for templates.
class VoidType {};
inline DataTypeBase::DataType TypeOf(VoidType dummy) {return DataTypeBase::Void;}

#ifndef NULL
#define NULL 0
#endif

} // namespace

#endif
