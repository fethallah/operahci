#ifndef _IMACRO_NUMRANGE_H_INCLUDED_
#define _IMACRO_NUMRANGE_H_INCLUDED_

#include "vector.h"

namespace NIMacro {

	class DI_MemBlock NumRange;
	typedef DerivedPointer<NumRange> PNumRange;

	class DI_MemBlock NumRange: public Vector {
		typedef Vector super;
	public:
		static	PNumRange Create(int x, int y); 
		static	PNumRange Create(int x, int y, bool exclusive); 
		static PNumRange Create();
		virtual PMemBlock DoClone(copymode_t copy_mode) const;
		virtual SafeValue DoVerb(const char* pszCommand, int iArg, SafeValue vArg[]);
		virtual const char*	Class() const {return "NumRange";}	
		virtual bool Conforms(const char *szClassName) const;
		virtual SafeValue ResolveSubItem(const Nbaseutil::safestring& subitemname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed) const;	

		void SetExclusive(bool exclusive) {exclusive_=exclusive;} 
		bool IsExclusive() const {return exclusive_;}
	protected:
		virtual Nbaseutil::safestring DoGetDescription() const;

	protected:
		NumRange(int x, int y);
		NumRange(const NumRange& b);
	private:
		bool exclusive_;	// endpoint excluded from the range; ==false by default.
	};
} // namespace

#endif

