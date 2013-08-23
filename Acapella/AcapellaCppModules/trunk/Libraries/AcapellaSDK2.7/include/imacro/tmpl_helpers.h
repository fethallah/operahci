#ifndef _IMACRO_TMPL_HELPERS_H_INCLUDED_
#define _IMACRO_TMPL_HELPERS_H_INCLUDED_

namespace NIMacro {
//@{
// A helper class for deleting objects in cooperation with ScopeGuard.
//
// Usage: 
// <pre>
//@@ A *tmp = new A();
//@@ ScopeGuard free_A = MakeGuard( Delete<A>(), tmp);
// </pre>
//@}
template <class T> 
struct Delete { 
	void operator()(T *t) const { delete t;} 
};

template <class T> 
struct DeleteAndNull { 
	void operator()(T*& t) const { delete t; t=NULL; } 
};

//@{
// A helper class for deleting arrays in cooperation with ScopeGuard.
//
// Usage: 
// <pre>
//@@ int *buff = new int[1024];
//@@ ScopeGuard free_buff = MakeGuard( DeleteArr<int>(), buff);
// </pre>
//@}
template <class T> 
struct DeleteArr { 
	void operator()(T *t) const { delete[] t;} 
};

//@{
// A helper class for setting variables in cooperation with ScopeGuard.
//
// Usage: 
// <pre>
//@@ extern int a_flag;
//@@ int previous_value=a_flag;
//@@ a_flag = 123456;
//@@ ON_BLOCK_EXIT( SetValue(a_flag), previous_value);
// </pre>
//@}

template <typename T>
struct SetValue {
	T& addr_;
	SetValue(T& addr): addr_(addr) {}
	void operator()(const T& value) const { addr_ = value;}
};


// maskindex, masklength- constant index array and length of the array, defines the mask 
// indexarray, indexlength- defines the shift object, the relative indexes and the length of the array
// correlation_image-output image, should be allocated outside
template<typename pixeltype>
void add_by_shiftobject(const int *maskindex, int masklength,
const int *indexarray, int indexlength,   pixeltype *correlation_image)
{
	
	pixeltype *correlation_image2;
	const int *ipoint1, *ipoint2, *array1, *array2;
	
	array2=indexarray+indexlength;			
	ipoint1=maskindex;
	ipoint2=maskindex+masklength;
	
	for(;ipoint1<ipoint2;ipoint1++)
	{	
		correlation_image2=correlation_image+*ipoint1;		
		array1=indexarray;
			
		while(array1<array2)
			(*(correlation_image2+*(array1++)))++;	
	}
}


template<typename pixeltype>
void append_image_return_to_original(int N, int per, pixeltype *fl_append, pixeltype *fl, int shift1, int shift2 )
{

	pixeltype  *apoint, *orpoint, *orpointN;
	int nper=shift1+shift1+per, startshift=shift2*nper+shift1;

	apoint=fl_append+startshift;
	orpoint=fl;
	orpointN=fl+N;
	int perstep=per*sizeof(pixeltype);

	for (;orpoint<orpointN;orpoint+=per, apoint+=nper)
		memcpy(orpoint, apoint, perstep);
		
}


inline void nodispatchfor(const char* fnname, Vector::DataType t1, Vector::DataType t2) {
	throw Exception(ERR_NOTIMPLEMENTED, Printf("Function %s not implemented for data types '%s' and '%s'.")(fnname)(Vector::TypeToString(t1))(Vector::TypeToString(t2)));
}

#ifdef DEFINE_ACAPELLA_DISPATCH_EXTENDED_MACROS


#define DISPATCH_UNSIGNED(ELEMTYPE,FUNCTION,ARGS) \
switch(ELEMTYPE) {\
case Vector::Byte: FUNCTION<unsigned char>ARGS; break;\
case Vector::UnsignedShort: FUNCTION<unsigned short>ARGS; break;\
case Vector::UnsignedInt: FUNCTION<unsigned int>ARGS; break;\
case Vector::UnsignedInt64: FUNCTION<Nbaseutil::uint64>ARGS; break;\
default: throw Exception(ERR_NOTIMPLEMENTED, Printf("Function " #FUNCTION " not implemented for data type '%s'")(Vector::TypeToString(ELEMTYPE)));\
};

#define DISPATCH_NUMERIC_NEXT(ELEMTYPE,FUNCTION,ARGS) \
switch(ELEMTYPE) {\
case Vector::Byte: FUNCTION,unsigned char ARGS; break;\
case Vector::UnsignedShort: FUNCTION,unsigned short ARGS; break;\
case Vector::UnsignedInt: FUNCTION, Nbaseutil::uint64 ARGS; break;\
case Vector::UnsignedInt64: FUNCTION, Nbaseutil::uint64 ARGS; break;\
case Vector::Char: FUNCTION,char ARGS; break;\
case Vector::Short: FUNCTION,short ARGS; break;\
case Vector::Int: FUNCTION,Nbaseutil::int64 ARGS; break;\
case Vector::Int64: FUNCTION, Nbaseutil::int64 ARGS; break;\
case Vector::Float: FUNCTION,float ARGS; break;\
case Vector::Double: FUNCTION,double ARGS; break;\
default: throw Exception(ERR_NOTIMPLEMENTED, Printf("Function " #FUNCTION " not implemented for data type '%s'")(Vector::TypeToString(ELEMTYPE)));\
};

#define DISPATCH_NUMERIC_TO_LARGER(ELEMTYPE1,ELEMTYPE2,FUNCTION,ARGS) \
switch(ELEMTYPE1) {\
case Vector::Byte: \
	switch(ELEMTYPE2) {\
	case Vector::Byte: FUNCTION<unsigned char,unsigned char>ARGS; break;\
	case Vector::UnsignedShort: FUNCTION<unsigned char,unsigned short>ARGS; break;\
	case Vector::UnsignedInt: FUNCTION<unsigned char,unsigned int>ARGS; break;\
	case Vector::Short: FUNCTION<unsigned char,short>ARGS; break;\
	case Vector::Int: FUNCTION<unsigned char,int>ARGS; break;\
	case Vector::Int64: FUNCTION<unsigned char,int64>ARGS; break;\
	case Vector::UnsignedInt64: FUNCTION<unsigned char,uint64>ARGS; break;\
	case Vector::Float: FUNCTION<unsigned char,float>ARGS; break;\
	case Vector::Double: FUNCTION<unsigned char,double>ARGS; break;\
	default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
	}\
	break;\
case Vector::Char: \
	switch(ELEMTYPE2) {\
	case Vector::Char: FUNCTION<char,char>ARGS; break;\
	case Vector::UnsignedShort: FUNCTION<char,unsigned short>ARGS; break;\
	case Vector::UnsignedInt: FUNCTION<char,unsigned int>ARGS; break;\
	case Vector::Short: FUNCTION<char,short>ARGS; break;\
	case Vector::Int: FUNCTION<char,int>ARGS; break;\
	case Vector::Int64: FUNCTION<char,int64>ARGS; break;\
	case Vector::UnsignedInt64: FUNCTION<char,uint64>ARGS; break;\
	case Vector::Float: FUNCTION<char,float>ARGS; break;\
	case Vector::Double: FUNCTION<char,double>ARGS; break;\
	default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
	}\
	break;\
case Vector::UnsignedShort: \
	switch(ELEMTYPE2) {\
	case Vector::UnsignedShort: FUNCTION<unsigned short,unsigned short>ARGS; break;\
	case Vector::UnsignedInt: FUNCTION<unsigned short,unsigned int>ARGS; break;\
	case Vector::Int: FUNCTION<unsigned short,int>ARGS; break;\
	case Vector::Int64: FUNCTION<unsigned short,int64>ARGS; break;\
	case Vector::UnsignedInt64: FUNCTION<unsigned short,uint64>ARGS; break;\
	case Vector::Float: FUNCTION<unsigned short,float>ARGS; break;\
	case Vector::Double: FUNCTION<unsigned short,double>ARGS; break;\
	default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
	}\
	break;\
case Vector::Short: \
	switch(ELEMTYPE2) {\
	case Vector::Short: FUNCTION<short,short>ARGS; break;\
	case Vector::UnsignedInt: FUNCTION<short,unsigned int>ARGS; break;\
	case Vector::Int: FUNCTION<short,int>ARGS; break;\
	case Vector::Int64: FUNCTION<short,int64>ARGS; break;\
	case Vector::UnsignedInt64: FUNCTION<short,uint64>ARGS; break;\
	case Vector::Float: FUNCTION<short,float>ARGS; break;\
	case Vector::Double: FUNCTION<short,double>ARGS; break;\
	default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
	}\
	break;\
case Vector::Triple: \
	switch(ELEMTYPE2) {\
	case Vector::UnsignedInt: FUNCTION<triple,unsigned int>ARGS; break;\
	case Vector::Int: FUNCTION<triple,int>ARGS; break;\
	case Vector::Int64: FUNCTION<triple,int64>ARGS; break;\
	case Vector::UnsignedInt64: FUNCTION<triple,uint64>ARGS; break;\
	case Vector::Float: FUNCTION<triple,float>ARGS; break;\
	case Vector::Double: FUNCTION<triple,double>ARGS; break;\
	default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
	}\
	break;\
case Vector::UnsignedInt: \
	switch(ELEMTYPE2) {\
	case Vector::UnsignedInt: FUNCTION<unsigned int,unsigned int>ARGS; break;\
	case Vector::Int64: FUNCTION<unsigned int,int64>ARGS; break;\
	case Vector::UnsignedInt64: FUNCTION<unsigned int,uint64>ARGS; break;\
	case Vector::Float: FUNCTION<unsigned int,float>ARGS; break;\
	case Vector::Double: FUNCTION<unsigned int,double>ARGS; break;\
	default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
	}\
	break;\
case Vector::Int: \
	switch(ELEMTYPE2) {\
	case Vector::Int: FUNCTION<int,int>ARGS; break;\
	case Vector::Int64: FUNCTION<int,int64>ARGS; break;\
	case Vector::UnsignedInt64: FUNCTION<int,uint64>ARGS; break;\
	case Vector::Float: FUNCTION<int,float>ARGS; break;\
	case Vector::Double: FUNCTION<int,double>ARGS; break;\
	default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
	}\
	break;\
case Vector::Int64: \
	switch(ELEMTYPE2) {\
	case Vector::Int64: FUNCTION<int64,int64>ARGS; break;\
	case Vector::Float: FUNCTION<int64,float>ARGS; break;\
	case Vector::Double: FUNCTION<int64,double>ARGS; break;\
	default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
	}\
	break;\
case Vector::UnsignedInt64: \
	switch(ELEMTYPE2) {\
	case Vector::UnsignedInt64: FUNCTION<uint64,uint64>ARGS; break;\
	case Vector::Float: FUNCTION<uint64,float>ARGS; break;\
	case Vector::Double: FUNCTION<uint64,double>ARGS; break;\
	default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
	}\
	break;\
case Vector::Float: \
	switch(ELEMTYPE2) {\
	case Vector::Float: FUNCTION<float,float>ARGS; break;\
	case Vector::Double: FUNCTION<float,double>ARGS; break;\
	default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
	}\
	break;\
case Vector::Double: \
	switch(ELEMTYPE2) {\
	case Vector::Double: FUNCTION<double,double>ARGS; break;\
	default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
	}\
	break;\
default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
}

#define DISPATCH_FLOAT_TO_COUNT(ELEMTYPE1,ELEMTYPE2,FUNCTION,ARGS) \
switch(ELEMTYPE1) {\
case Vector::Float: \
	switch(ELEMTYPE2) {\
	case Vector::Byte: FUNCTION<float,unsigned char>ARGS; break;\
	case Vector::UnsignedShort: FUNCTION<float,unsigned short>ARGS; break;\
	case Vector::UnsignedInt: FUNCTION<float,unsigned int>ARGS; break;\
	default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
	}\
	break;\
case Vector::Double: \
	switch(ELEMTYPE2) {\
	case Vector::Byte: FUNCTION<double,unsigned char>ARGS; break;\
	case Vector::UnsignedShort: FUNCTION<double,unsigned short>ARGS; break;\
	case Vector::UnsignedInt: FUNCTION<double,unsigned int>ARGS; break;\
	default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
	}\
	break;\
default: nodispatchfor(#FUNCTION, ELEMTYPE1, ELEMTYPE2);\
}

#define DISPATCH_COVER_STAGE1(ELEMTYPE,FUNCTION,ARGS) \
switch(ELEMTYPE) {\
case Vector::UnsignedInt: FUNCTION<unsigned int>ARGS; break;\
case Vector::Int: FUNCTION<int>ARGS; break;\
case Vector::UnsignedInt64: FUNCTION<uint64>ARGS; break;\
case Vector::Int64: FUNCTION<int64>ARGS; break;\
case Vector::Double: FUNCTION<double>ARGS; break;\
case Vector::String: FUNCTION<Nbaseutil::safestring>ARGS; break;\
case Vector::PolyType: FUNCTION<DataItem>ARGS; break;\
default: throw Nbaseutil::Exception(Nbaseutil::ERR_NOTIMPLEMENTED, Nbaseutil::Printf("Function " #FUNCTION " not implemented for data type '%s'")(Vector::TypeToString(ELEMTYPE)));\
};


#define DISPATCH_COVER_STAGE2(TYPENAME1,ELEMTYPE2,FUNCTION,ARGS) \
switch(ELEMTYPE2) {\
case Vector::UnsignedInt: FUNCTION<TYPENAME1, unsigned int>ARGS; break;\
case Vector::Int: FUNCTION<TYPENAME1, int>ARGS; break;\
case Vector::UnsignedInt64: FUNCTION<TYPENAME1, uint64>ARGS; break;\
case Vector::Int64: FUNCTION<TYPENAME1, int64>ARGS; break;\
case Vector::Double: FUNCTION<TYPENAME1, double>ARGS; break;\
case Vector::String: FUNCTION<TYPENAME1, Nbaseutil::safestring>ARGS; break;\
case Vector::PolyType: FUNCTION<TYPENAME1, DataItem>ARGS; break;\
default: throw Nbaseutil::Exception(Nbaseutil::ERR_NOTIMPLEMENTED, Nbaseutil::Printf("Function " #FUNCTION " not implemented for data type '%s'")(Vector::TypeToString(ELEMTYPE2)));\
};

#endif // #ifdef DEFINE_ACAPELLA_DISPATCH_EXTENDED_MACROS

} // namespace
#endif
