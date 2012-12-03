#ifndef x_PKI_CTG_ACAPELLA_IMACRO_CLOSURE_H_INCLUDED
#define x_PKI_CTG_ACAPELLA_IMACRO_CLOSURE_H_INCLUDED

#include "executive.h"

namespace NIMacro {

	class Closure;
	typedef DerivedPointer<Closure> PClosure;
	class Module;
	typedef ThreadSharablePointer<Module> PModule;
    class ResolvePoint;
    typedef ThreadSharablePointer<ResolvePoint> PResolvePoint;

	class DI_IMacro Closure: public MemBlock {
		typedef NIMacro::MemBlock super;
	public: // static interface
		typedef PClosure type;

		/// Create a new closure object.
		static PClosure Create(ExecutionContext& ctx, PModule enclosed_module, PContainer bound_args, bool suppresswarnings, const PResolvePoint& rp);

		/// Create a dummy factory object
		static PClosure Create();

		static const char* const sc_name_;
		static const char* const sc_descr_;

	public: // interface
		void Run(ExecutionContext& ctx, const Container& explicit_args);
		
		void SetModule(PModule m) {enclosed_module_ = m;}
		PModule GetModule() const {return enclosed_module_;}
		bool SuppressWarnings() const {return suppresswarnings_;}

	public: // virtual overrides
		virtual const char* Class() const {return "closure";}
		virtual bool Conforms(const char *szClassName) const;
		virtual bool Entertain(NIMacro::MemBlockVisitor& visitor, const Nbaseutil::safestring& name, entertainmode_t mode);
		virtual void IterateChildren(NIMacro::AcaVisitor& visitor, const NIMacro::TraverseNode& node);
		virtual bool AddConvertArg(const Nbaseutil::safestring& option, const NIMacro::DataItem& value, NIMacro::ConvertOptions& opt, Nbaseutil::ThrowHandlerFunc throwhandler=NIMacro::LogWarnings()) const;
		virtual bool Consistent(Nbaseutil::safestring& msg, bool check_content=true) const;
		virtual const char* ClassDescription() const;
		virtual NIMacro::PMemBlock DoClone(copymode_t copy_mode) const;
		virtual bool DoSetSubItem(const Nbaseutil::safestring& subitemname, const NIMacro::SafeValue& item, const Nbaseutil::safestring& fullpathname, Nbaseutil::ThrowHandlerFunc throwhandler=Nbaseutil::ThrowIndeed);
		virtual Nbaseutil::safestring DoGetDescription() const;
		virtual NIMacro::SafeValue ResolveSubItem(const Nbaseutil::safestring& subitemname, Nbaseutil::ThrowHandlerFunc throwhandler=Nbaseutil::ThrowIndeed) const;
		virtual bool Equals(const MemBlock& b) const;
		virtual SafeValue DoVerb(const char* pszCommand, int iArg, SafeValue vArg[]);

	protected: // implementation
		Closure(PModule enclosed_module, PContainer bound_args, bool suppresswarnings, const PResolvePoint& rp);
		void PassExplicitParams(ExecutionContext& ctx, const Container& args, RunFrame& runframe);
	protected: // data
		PModule enclosed_module_;
		PContainer bound_args_;
		PResolvePoint rp_; // for looking up the enclosed module again.
		const bool suppresswarnings_;

	};


} // namespace
#endif

