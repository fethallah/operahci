#ifndef x_ACAPELLA_IMACRO_EXPLICITARGUMENT_H_INCLEUDED_
#define x_ACAPELLA_IMACRO_EXPLICITARGUMENT_H_INCLEUDED_

#include "expressionparser.h"

namespace NIMacro {
	using Nbaseutil::safestring;
	class ModPar;
	class Module;
	struct ExecutionContext;
	class RunFrame;

	/**
	* This class is for internal Acapella core use.
	*
	* ExplicitArgument holds info about an explicit module parameter 
	*   occuring in the script text. Each ExplicitArgument is owned 
	*	by a certain ModuleCall. ExplicitArgument's are created during
	*   ModuleCall ctor, and are immutable after that. ExplicitArgument
	*   is destroyed together with the owning ModuleCall object.
	*   ExplicitArgument's are not reference-counted, so be careful when
	*	remembering pointers/references to them.
	*/
	class ExplicitArgument: public mb_malloced {
	public:
		/** Ctor
		* @param expr The explicit expression for the parameter as appearing in the script text.
		* @param par Reference to the corresponding ModPar module parameter object.
		*/
		ExplicitArgument(const safestring& expr, const ModPar& par)
			: par_(&par)
			, syntax_run_ok_(DetermineSyntaxOk())
			, implicit_context_(false)
		{
			Parse(expr);
		}

		ExplicitArgument(const safestring& expr)
			: par_(NULL)
			, syntax_run_ok_(false)
			, implicit_context_(true)
		{
			Parse(expr);
		}

		/// Default constructor to avoid MSVC STL warning C4510.
		ExplicitArgument()
			: par_(NULL)
			, syntax_run_ok_(false) 
			, implicit_context_(false)
		{}	

		// Implicitly generated copy ctor and assignment op are OK.

		/** Evaluate the explicit expression in the specified context and send to resulting
		*   value to the module implementation, by calling ModPar::Send().
		*   If the explicit expression equals "default", then ModPar::SendDefault()
		*	is called instead. In any case, the parameter is marked sent in ctx.sentflags.
		*
		*   This function is called during the
		*   input parameter sending process to the module. 
		*	Module::StartParamsSending() must be have called beforehand.
		*
		* @param ctx Current execution context. In particular, ctx.db and
		*			ctx.localdb are used for evaluating the expression.
		* @param runframe A valid reference to the current run frame, prepared by Module::StartParamsSending().
		* @param throwhandler Is called for errors.
		* @return False in case of an error, if the throwhandler did not want to throw.
		*/
		bool Send(ExecutionContext& ctx, RunFrame& runframe, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed) const;
		
		/// Return pointer to the corresponding ModPar object, or NULL for uninitialized object.
		const ModPar* GetModPar() const {return par_;}

		/** 
		* Return the initial expression string. Do not store the returned reference, it may be invalidated soon. 
		* For PAR_MULTI parameters, a comma-delimited list of expressions is returned.
		*/
		const safestring& GetExpr() const {return expr_.GetExpression();}

//		/** Append second and further arguments to a PAR_MULTI parameter. Alternatively,
//		* one could pass comma-delimited list of expressions into ExplicitArgument ctor.
//		*
//		* @param expr Explicit expression for an occurance of PAR_MULTI parameter as 
//		*			appearing in the script text.
//		*/
//		DI_IMacro void Append(const char* expr);

		/// Returns false if the expression probably cannot be evaluated in syntax run.
		bool SyntaxRunSuited() const {return syntax_run_ok_;}

		/// Swap() operation for more efficient copy
		void Swap(ExplicitArgument& b) { 
			expr_.swap(b.expr_); 
			const ModPar* par=par_; par_=b.par_; b.par_=par;
			bool syntax_run_ok=syntax_run_ok_; syntax_run_ok_=b.syntax_run_ok_; b.syntax_run_ok_=syntax_run_ok;
			bool implicit_context=implicit_context_; implicit_context_=b.implicit_context_; b.implicit_context_=implicit_context;
		}
	private: // implementation
		DI_IMacro bool DetermineSyntaxOk() const;
		void Parse(const safestring& expr);
	private:
		/// Expression in the parsed format. Also the original string expression is kept here inside.
		ParsedExpr expr_;

		/** For input parameters: pointer to the corresponding parameter, or NULL for 'uninitialized' object.
		* This is not reference because the ExplicitArgument objects are copied 
		* into the proper place and assignment operator is required for that.
		*/
		const ModPar* par_;
		bool syntax_run_ok_;
		bool implicit_context_;
	};

	class OutputArgument: public mb_malloced {
	public:
		/** Ctor
		* @param name New name to output from the module.
		* @param expr The explicit expression for the parameter as appearing in the script text.
		*/
		OutputArgument(const safestring& name, const safestring& expr): name_(name), expr_(expr) {}

		/// Default constructor to avoid MSVC STL warning C4510.
		OutputArgument() {}	

		// Implicitly generated copy ctor and assignment op are OK.

		/// Return new output name.
		const safestring& GetNewName() const {return name_;}

		/// Return explicit expression. Do not store the returned pointer, it may be invalidated soon. 
		const safestring& GetExpr() const {return expr_;}

		/// Swap() operation for more efficient copy
		void Swap(OutputArgument& b) { 
			name_.swap(b.name_); 
			expr_.swap(b.expr_); 
		}
	private:
		safestring name_, expr_;	
	};

} // namespace
#endif
