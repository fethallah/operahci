#ifndef _PVALUE_H_INCLUDED_
#define _PVALUE_H_INCLUDED_

#include "dataitem.h"

namespace NIMacro {


/** A class for encapsulating value of any type, derived from DataItem.
*  It differs from DataItem in that it is much more picky and throws on each error suspicion.
*/
class DI_MemBlock SafeValue: public DataItem {
public:
	SafeValue() {}
	SafeValue(ItemType t): DataItem(t) {}
	SafeValue(int k): DataItem(k) {}
	SafeValue(unsigned int k): DataItem(k) {}
	SafeValue(Nbaseutil::int64 k): DataItem(k) {}
	SafeValue(Nbaseutil::uint64 k): DataItem(k) {}
	SafeValue(double f): DataItem(f) {}
	SafeValue(const char* s): DataItem(s) {}
	SafeValue(const std::string& s): DataItem(s.c_str()) {}
	SafeValue(const Nbaseutil::safestring& s): DataItem(s) {}
	SafeValue(const PMemBlock& m): DataItem(m) {}
	SafeValue(MemBlock& m): DataItem(m) {}
	SafeValue(const MemBlock& m): DataItem(const_cast<MemBlock&>(m)) {}
	SafeValue(ThreadSharable& sh): DataItem(sh) {}
	//template <class T> SafeValue(ThreadSharablePointer<T> p): DataItem(p) {}
	explicit SafeValue(void* p): DataItem(p) {}
	SafeValue(const SafeValue& b): DataItem(b) {}
	SafeValue(const DataItem& b): DataItem(b) {}

//	SafeValue& operator=(const SafeValue& b);
//	bool operator==(const SafeValue& b) const;

	int GetInt() const;
	Nbaseutil::int64 GetInt64() const;
	double GetFloating() const;
	double GetDouble() const {return GetFloating();}
	PMemBlock GetMemory() const;
	PSharable GetSharable() const;
	Nbaseutil::safestring GetString() const;	// Return item value, if this is any string. Otherwise throw.
	const void* GetPointer() const;
	bool GetBool() const;

	int GetInt(Nbaseutil::ThrowHandlerFunc throwhandler) const;
	double GetFloating(Nbaseutil::ThrowHandlerFunc throwhandler) const;
	double GetDouble(Nbaseutil::ThrowHandlerFunc throwhandler) const {return GetFloating(throwhandler);}
	PMemBlock GetMemory(Nbaseutil::ThrowHandlerFunc throwhandler) const;
	PSharable GetSharable(Nbaseutil::ThrowHandlerFunc throwhandler) const;
	Nbaseutil::safestring GetString(Nbaseutil::ThrowHandlerFunc throwhandler) const;	// Return item value, if this is any string. Otherwise throw.
	const void* GetPointer(Nbaseutil::ThrowHandlerFunc throwhandler) const;
	bool GetBool(Nbaseutil::ThrowHandlerFunc throwhandler) const;
};








} //namespace

#endif
