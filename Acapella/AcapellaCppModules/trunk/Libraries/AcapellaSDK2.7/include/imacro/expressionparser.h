#ifndef _EXPRESSIONPARSER_H_INCLUDED_
#define _EXPRESSIONPARSER_H_INCLUDED_

#include <locale.h>
#include <stack>

#include "executive.h"

namespace NIMacro {
	class DataBlock;
	struct ExecutionContext;

	/// Possible types of explicit module parameter value expressions.
	enum StringValType {
		/// A numeric const, e.g: -123.456
		VAL_NUMCONST=0,
		/// A boolean constant
		VAL_BOOLCONST=1,
		/// A string const, e.g: "Siemens Incorporated"
		VAL_STRCONST=2,
		/// A reference to a value in the current data block, e.g: edge
		VAL_CURDB=3,			
		/// A syntactically invalid value.
		VAL_INCORRECT=5,
		/// An arithmetic or string expression, e.g:  12+ln(3.4) 
		VAL_EXPRESSION=6,
		/// A memory address of a MemBlock class hierarchy object in the current process memory space, e.g. *12003467
		VAL_MEMBLOCKADDR=7        
	};

	/// Determines the type of the string as a module parameter explicit value. Assumes that the string is trimmed.
	DI_IMacro StringValType ValType(const safestring& s);

	DI_IMacro bool allowed_in_dos_file_names(unsigned char c);

	/** Extract path from filename.
	* Returned path is terminated by slash or backslash.
	*/
	DI_IMacro safestring FilePath(const safestring& filename);

	/// Add base before relativepath, if necessary, and return absolute path.
	DI_IMacro safestring AbsolutePath(const safestring& base, const safestring& relativepath);

	/// Deprecated overload, use other InitByExpression() overloads.
	DI_IMacro bool InitByExpression(DataItem& item, const safestring& expression, const DataBlock* context, ThrowHandlerFunc throwhandler=ThrowIndeed, const PContainer local_context=NULL, bool is_filename=false, ExecutionContext* ctx=NULL);


	/// Deprecated overload, use InitByConstantExpression() instead.
	DI_IMacro bool InitByExpression(DataItem& item, const safestring& expression, ThrowHandlerFunc throwhandler=ThrowIndeed, bool is_filename=false);

	/**
	* Evaluate an expression in a context.
	* @param item The evaluation result is stored here in case of success.
	* @param expression An Acapella expression to evaluate.
	* @param ctx The execution context for looking up the variable names and providing support for parallel() and function-like module call mechanism.
	*			One can pass here Mod::GetCurrentExecutionContext() inside module functions.
	* @param throwhandler Is consulted in case of problems.
	* @param is_filename Specifies whether the backslashes are to be interpreted as path separators.
	* @return True, if the expression was evaluated successfully.
	*/
	DI_IMacro bool InitByExpression(DataItem& item, const safestring& expression, ExecutionContext& ctx, ThrowHandlerFunc throwhandler=ThrowIndeed, bool is_filename=false);

	/**
	* Evaluate an expression in a context and a local context.
	* @param item The evaluation result is stored here in case of success.
	* @param expression An Acapella expression to evaluate.
	* @param ctx The execution context for looking up the variable names and providing support for parallel() and function-like module call mechanism.
	*			One can pass here Mod::GetCurrentExecutionContext() inside module functions.
	* @param local_context Local context, the data items are looked up first in local_context, then in ctx.db.
	* @param throwhandler Is consulted in case of problems.
	* @param is_filename Specifies whether the backslashes are to be interpreted as path separators.
	* @return True, if the expression was evaluated successfully.
	*/
	DI_IMacro bool InitByExpression(DataItem& item, const safestring& expression, ExecutionContext& ctx, Container& local_context, ThrowHandlerFunc throwhandler=ThrowIndeed, bool is_filename=false);


	/**
	* Evaluate a constant expression (not depending on datablock items). 
	* If the expression contains references to current datablock items, the evaluation fails.
	* @param item The evaluation result is stored here in case of success.
	* @param expression An Acapella constant expression to evaluate.
	* @param ctx The execution context for providing support for parallel() and function-like module call mechanism.
	*			One can pass here Mod::GetCurrentExecutionContext() inside module functions.
	* @param throwhandler Is consulted in case of problems.
	* @param is_filename Specifies whether the backslashes are to be interpreted as path separators.
	* @return True, if the expression was evaluated successfully.
	*/
	DI_IMacro bool InitByConstantExpression(DataItem& item, const safestring& expression, ExecutionContext& ctx, ThrowHandlerFunc throwhandler=ThrowIndeed, bool is_filename=false);


/// Possible operators in Acapella expressions. Operators are listed in precedence order, except parens, which are handled differently anyway.
enum operators { 
	leftparen, rightparen, 	leftbracket, rightbracket,
	missingparameters,
	commaOp,	// for separating function call arguments, e.g. in substr("alpha",2,2)
	subkeyconcatOp,
	assignOp,
	orOp, andOp, xorOp, 
	eqOp, str_eqOp, neqOp, str_neqOp, gtOp, str_gtOp, ltOp, str_ltOp, geOp, str_geOp, leOp, str_leOp, 
	sequenceOp, concatOp, 
	plusOp, minusOp, 
	multiplyOp, divideOp, modulusOp,
	unaryPlus, unaryMinus, 
	notOp,
	powerOp,
	evalOp,
	subkeyOp,
	subkeyconcatOpLiteral,
	/// fnCall must be the last element in this enum and serves as a starting index for built-in functions.
	fnCall,		
};

inline bool IsUnaryOp(operators op) {
	return op==unaryPlus || op==unaryMinus || op==notOp || op==evalOp;
}


/// A class for encapsulating different types of tokens in Acapella expressions.
class ExprToken {
public: // enums 
	/// Possible token types.
	enum TokenType { intconstant, doubleconstant, stringconstant, oper, varname, memblock, intconstant64};

public: // interface

	ExprToken(int k): Type_(intconstant), k_(k) {}
	ExprToken(int64 k): Type_(intconstant64), k64_(k) {}
	ExprToken(double d): Type_(doubleconstant), d_(d) {}
	ExprToken(const char* s, int len): Type_(stringconstant) {name_ = safestring_new(s, len);}
	ExprToken(const char* s): Type_(stringconstant) {name_ = safestring_new(s);}
	ExprToken(const Nbaseutil::safestring& s): Type_(stringconstant) {name_ = safestring_new(s);}
	ExprToken(const std::string& s): Type_(stringconstant) {name_ = safestring_new(s.c_str());}
	ExprToken(operators op): Type_(oper), op_(op) {}
	ExprToken(const char*s, int len, bool Variable): Type_(varname) {
		// trim:
		while( *s && len>0 && (*s==' ' || *s=='\t')) { ++s; --len;}
		while(len>0 && (s[len-1]==' ' || s[len-1]=='\t')) {--len;}
		name_ = safestring_new(s, len);
	}
	ExprToken(const NIMacro::MemBlock* m): Type_(memblock), m_(m) { m->Capture(); }
	~ExprToken() {
		if (Type_==stringconstant || Type_==varname) {safestring_delete(name_);}
		if (Type_==memblock) m_->Release();
	}
	ExprToken(const ExprToken& b) {
		Type_ = b.Type_;
		switch(Type_) {
		case intconstant: k_ = b.k_; break;
		case intconstant64: k64_ = b.k64_; break;
		case doubleconstant: d_ = b.d_; break;
		case stringconstant: case varname: name_ = safestring_new(b.name_->c_str(), b.name_->length()); break;
		case oper: op_ = b.op_; break;
		case memblock: m_ = b.m_; m_->Capture(); break;
		}
	}
	ExprToken& operator=(const ExprToken& b) {
		if (Type_==stringconstant || Type_==varname) {safestring_delete(name_);}
		const NIMacro::MemBlock* m=NULL;
		if (Type_==memblock) m=m_;
		Type_ = b.Type_;
		switch(Type_) {
		case intconstant: k_ = b.k_; break;
		case intconstant64: k64_ = b.k64_; break;
		case doubleconstant: d_ = b.d_; break;
		case stringconstant: case varname: name_ = safestring_new(b.name_->data(), b.name_->length()); break;
		case oper: op_ = b.op_; break;
		case memblock: m_ = b.m_; m_->Capture(); break;
		}
		if (m) m->Release();
		return *this;
	}
	bool operator ==(const ExprToken& b) {
		if (Type_ != b.Type_) return false;
		switch(Type_) {
		case intconstant: return k_==b.k_;
		case intconstant64: return k64_ == b.k64_; 
		case doubleconstant: return d_ == b.d_;
		case stringconstant: case varname: return *name_ == *(b.name_);
		case oper: return op_ == b.op_; 
		case memblock: return m_ == b.m_;
		default: throw NIMacro::Exception(ERR_PROGRAM_ERROR, "X12754");
		}
	}
	ExprToken& operator=(const DataItem& x) {
		if (Type_==stringconstant || Type_==varname) {safestring_delete(name_);}
		const NIMacro::MemBlock* m=NULL;
		if (Type_==memblock) m=m_;
		switch(x.GetType()) {
		case Integer: k_ = x.GetInt(); Type_ = intconstant; break;
		case Integer64: k64_ = x.GetInt64(); Type_ = intconstant64; break;
		case Floating: d_ = x.GetFloating();  Type_ = doubleconstant; break;
		case Asciiz: name_ = safestring_new(x.GetStringPointer(), x.GetStringLength()); Type_ = stringconstant; break;
		case Memory: m_ = x.GetMemory().Pointer(); m_->Capture(); Type_ = memblock; break;
		default:
			throw NIMacro::Exception(ERR_PROGRAM_ERROR, "Error: EXH159");
		}
		if (m) m->Release();
		return *this;
	}
private: // implementation
	/// Ensure proper DLL border passing capabilities.
	safestring* safestring_new(const char* s, size_t n) {
		void* buff = mb_malloc(sizeof(safestring));
		return new (buff) safestring(s, n);
	}
	safestring* safestring_new(const char* s) {
		void* buff = mb_malloc(sizeof(safestring));
		return new (buff) safestring(s);
	}
	safestring* safestring_new(const safestring& s) {
		void* buff = mb_malloc(sizeof(safestring));
		return new (buff) safestring(s);
	}
	void safestring_delete(const safestring* s) {
		s->~safestring();
		mb_free(const_cast<safestring*>(s));
	}
public: // data
	/// Token type
	TokenType Type_;
	/// Union of possible values. Only the union element corresponding to the Type_ field can be accessed.
	union {
		const NIMacro::MemBlock* m_;
		int k_;
		int64 k64_;
		double d_;
		operators op_;
		Nbaseutil::safestring* name_; // cannot put non-trivial type like safestring directly into a union.
	};
};

	/// A class for representing a parsed expression. See InitByExpression() for simpler way of evaluating expressions.
	class DI_IMacro ParsedExpr {
	public: // typedefs
		typedef Nbaseutil::mb_allocator_typedef<ExprToken>::allocator ExprTokenAllocator;
		typedef ::std::vector<ExprToken, ExprTokenAllocator> tokenarray_t;

		typedef Nbaseutil::mb_allocator_typedef<ExprToken*>::allocator ExprTokenPtrAllocator;
		typedef ::std::vector<ExprToken*, ExprTokenPtrAllocator> token_ptr_array_t;
	
	public: // enums

		/// Possible options (bit flags) for using in Parse().
		enum expr_options_t {
			/// Default mode, this coinceds with the first overload of ExprPolish().
			expr_default=0,
			/// Do not delay evaluating the iif() function second and third argument until the first argument has been evaluated. This mode is used in CalcAttr(), for example.
			expr_no_lazy_iif=1<<0,
		};

	public: // interface

		/// Ctor, creates an empty object, use Parse() to initialize it.
		ParsedExpr(): options_(expr_default) {}

		/// Ctor, creates a parsed expression corresponding to (a part of) another ParsedExpr.
		ParsedExpr(tokenarray_t::const_iterator begin, tokenarray_t::const_iterator end): tokenarray_(begin, end), options_(expr_default) {} 

		/**
		* Parse the expression to initialize this object.
		* @param options Possible extra options.
		* @param expression The Acapella expression.
		* @param is_filename The expression is assumed to yield a filename. This affects the interpretation of backslashes. 
		* @param is_par_noquote If the expression starts with a lone ampersand, it will be ignored in the parsing. 
		* @param throwhandler A handler for handling any errors. 
		* @return True if parsing succeeded, false if parsing failed and throwhandler did not want to throw. In any case, the string
		*         expression is stored in the object and can be accessed via the GetExpression() member function.
		*/
		bool Parse(expr_options_t options, const safestring& expression, bool is_filename, bool is_par_noquote, ThrowHandlerFunc throwhandler);

		/**
		* Substitute variable references in the token stack with their values in specified context.
		* Do not substitute tokens whose addresses are mentioned in NULL-terminated exclude array.
		* @param ctx The execution context for looking up variables and resolving expressions.
		* @param exclude A vector of pointers to ExprToken objects in the token stack which may not be substituted. The pointers can be obtained by a TokenExists() call.
		* @param throwhandler A handler for handling any errors. 
		*/
		void Substitute(ExecutionContext& ctx, token_ptr_array_t& exclude, ThrowHandlerFunc throwhandler=ThrowIndeed);

		/// Not yet implemented
		void Optimize();

		/**
		* Prepares the expression to be used as a calcattr() or tablefilter() formula in vector format.
		* @param columns A vector of pointers to the tokens corresponding to the identified table columns or object list attributes.
		* @return True, if any changes were made in the parsed formula. In this case the formula can be only evaluated in vector style, by substituting full columns or attributes inside the parsed expression.
		*/
		bool PrepareVectorFormula(token_ptr_array_t& columns);

		/// Reverts any changes made by PrepareVectorFormula().
		void UndoPrepareVectorFormula();

		/** Checks if parsed token stack contains a variable references of token.
		* Comparison is case-insesensitive.
		* @return True if any references were found. 
		* @param tokenname Name of the token to search.
		* @param vals If true is returned, then the list of relevant ExprToken addresses is stored in vals parameter. 
		*/
		bool TokenExists(const safestring& tokenname, token_ptr_array_t& vals);

		/**
		* Evaluate the token stack.
		* @return True in case of success.
		* @param result The result value is stored here. This will be Undefined if there were errors, but throwhandler did not want to throw.
		* @param ctx Current execution context for data lookup and evaluation support.
		* @param poLocalContext An auxiliary datablock for searching for variables. If not NULL, poLocalContext is studied first.
		* @param throwhandler Handles errors during the operation.
		*/
		bool EvalPolish(DataItem& result, ExecutionContext& ctx, const PContainer poLocalContext=NULL, ThrowHandlerFunc throwhandler=ThrowIndeed) const;

		/// Enum values which can be returned from Vectorization() (Acapella 2.4).
		enum ov_type {
			/// The expression is not vectorizable; one has to evaluate it row-wise for object lists and tables.
			ov_not_vectorizable = 0,
			/// The expression is trivially vectorizable and can be evaluated "as is" by passing the table or object list as the local context.
			ov_vectorizable = 1,
			/// The expression is vectorizable, but the caller has to first call PrepareVectorFormula() on it.
			ov_vectorizable_with_prepare = 2,
		};

		/// Returns the expression type regarding of vectorization in tables and object lists. See enum ov_type for details.
		ov_type Vectorization() const;

		/// Returns true if the expression is trivially vectorizable. This is a back-compatibility function, use Vectorization() in new code instead.
		bool OnlyVectorOps() const;

		/// Returns true if the expression contains calls to the parallel() function.
		bool HasParallel() const;

		/// Find out if the result of the expression is a boolean type.
		bool IsBooleanPolish() const;

		/// Return an iterator to the begin of internal ExprToken array.
		tokenarray_t::const_iterator begin() const {return tokenarray_.begin();}

		/// Return an iterator to the end of internal ExprToken array.
		tokenarray_t::const_iterator end() const {return tokenarray_.end();}

		/// Return true if there are no tokens in the parsed expression.
		bool empty() const {return tokenarray_.empty();}
		
		/// Return reference to the last token in the parsed expression. Can be called only if empty() returns false.
		const ExprToken& back() const {return tokenarray_.back();}

		/// Remove the last token from the parsed expression. Can be called only if empty() returns false.
		void pop_back() {tokenarray_.pop_back();}

		/// Return the initial expression.
		const safestring& GetExpression() const {return expr_;}

		/// Standard swap operation.
		void swap(ParsedExpr& b) {
			tokenarray_.swap(b.tokenarray_);
			std::swap(options_, b.options_);
			expr_.swap(b.expr_);
		}
	private: // implementation
		tokenarray_t tokenarray_;
		expr_options_t options_;
		safestring expr_; // initial expression
	};

	class DI_IMacro TokenStack;
	typedef RefcountedPointer<TokenStack> PTokenStack;

	/// A dynamically allocated and refcounted variant of ParsedExpr.
	class DI_IMacro TokenStack: public Refcountable, public ParsedExpr {
	public: // static interface
		/** Parse an Acapella expression and create a token stack in reverse polish notation.
		* @return A smartpointer to the token stack, or NULL if there were errors and throwhandler did not want to throw.
		* @param Expression The Acapella expression.
		* @param IsFileName The expression is assumed to yield a filename. This affects the interpretation of backslashes. 
		* @param throwhandler A handler for handling any errors. 
		*/
		static PTokenStack Expr2Polish(const safestring& Expression, bool IsFileName=false, ThrowHandlerFunc throwhandler=ThrowIndeed);

		/** Parse an Acapella expression and create a token stack in reverse polish notation.
		* @return A smartpointer to the token stack, or NULL if there were errors and throwhandler did not want to throw.
		* @param options Extra options affecting expression parsing.
		* @param Expression The Acapella expression.
		* @param IsFileName The expression is assumed to yield a filename. This affects the interpretation of backslashes. 
		* @param throwhandler A handler for handling any errors. 
		*/
		static PTokenStack Expr2Polish(expr_options_t options, const safestring& Expression, bool IsFileName=false, ThrowHandlerFunc throwhandler=ThrowIndeed);

	public: // interface
		/// Make copy of the token stack.
		PTokenStack Clone() const;
	};

	/**
	* Replace IMacro variables and variable expressions in the string with corresponding numeric values.
	* This is primarily meant for preparing formulas for the libformulC library. In particular, INF and NAN
	* values are replaced by "inf()" and "nan()" pseudofunctions, which are recognized by our libformulC.
	* 
	* Examples of recognized IMacro variable names:
	*	a	image.length	image[ round(sin(0.5)*23), x]
	* Example: substring k is not replaced (is considered a function name).
	*	a - k (b + c)
	* Example of usage: safestring s = FillNumericExpression("a*x+b", GetCurrentDataBlock(), false, false, MakeList("x")("y"));
	*
	* @param expr The initial expression
	* @param ctx Current execution context for variable and module name lookup. In an IMacro module one can use the GetExecutionContext() method for passing this argument.
	* @param throw_if_not_found Throw an Exception if named was not found in datablock (and not included in 'unreplaced'). If false, name is not replaced.
	* @param throw_if_not_numeric Throw an Exception if the parameter was found in the datablock, but was not convertible into a number. If false, name is not replaced.
	* @param minlen Minimum length of the name, so that it will be searched from datablock. Shorter names are just appended to 'unreplaced' output array.
	* @param unreplaced In input: the names which should be not replaced; in output: array of names which were not replaced (either too short, not found or not numeric).
	* @param vecargs If present, occurences of IMacro MemBlock-type variables is not considered as an error. 
	*		Instead the corresponding PMemBlocks are assigned a letter a-z and stored in the vecargs and vecletters params.
	* @param vecletters The output string for storing assigned memblock letters. Must be present and empty if vecargs is not NULL.
	* @param local_context If specified, then the unknown names are first looked up in this context and only after that in db.
	*		If local_context is a Cells cell list object, then the name "cellindex" in the formula will be mapped to a vector of cell indices 0..cellcount-1.
	*		If local_context and vecargs are both present, then vectors are allowed to appear only in local_context. If a vector is found in db instead, an Exception is thrown.
	*/
	DI_IMacro safestring FillNumericExpression(const safestring& expr, ExecutionContext& ctx, bool throw_if_not_found, bool throw_if_not_numeric, int minlen, safestringlist& unreplaced, PVector vecargs=NULL, safestring* vecletters=NULL, PContainer local_context=NULL);

	/// Deprecated, use another overload.
	DI_IMacro safestring FillNumericExpression(const safestring& expr, const DataBlock& db, bool throw_if_not_found, bool throw_if_not_numeric, int minlen, safestringlist& unreplaced, PVector vecargs=NULL, safestring* vecletters=NULL, PContainer local_context=NULL);

	/** A class for forcing C locale decimal point until the end of current scope.
	*  Usage: 
	* @code
	*		{
	*			ForceCNumericLocale my_force_numeric_dot;
	*			...
	*		}
	* @endcode
	*/
	class ForceCNumericLocale {
		char prev_locale[256];
		void* operator new(size_t sz);	// unimplemented
	public:
		ForceCNumericLocale() {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
			strncpy(prev_locale, setlocale(LC_NUMERIC, NULL), sizeof(prev_locale));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
			prev_locale[sizeof(prev_locale)-1]=0;
			setlocale(LC_NUMERIC, "C");
		}
		~ForceCNumericLocale() {
			setlocale(LC_NUMERIC, prev_locale);
		}
	};

	// Replace Attribute with contained Vector or Image. Other memblock types are unchanged.
	// If validate==false, can return objects, whose content is not in sync with the rest of cell list.
	// Throws an exception, if the Attribute did not contain the data or this could not be recalculated (if validate==true).
	DI_IMacro PMemBlock ResolveAttribute(PMemBlock m, bool validate /*=true*/, ThrowHandlerFunc throwhandler);

	DI_IMacro PMemBlock ResolveAttribute(PMemBlock m);	// for binary back-compatibility

	/// A helper class for parsing assignment-like expressions appearing in Acapella functions like cnt() and tbl().
	/**
	* This class essentially just holds a name-value pair.
	* It is used mostly internally in the Acapella expression parser. It is exported
	* in order to support xml*() functions in imacro_xml library. It is derived from ThreadSharable
	* so it would be possible to encapsulate it in a DataItem in the parsed expression stack.
	*/
	class Assignment: public ThreadSharable {
	public:
		/// Construct an assignment object of name and value.
		Assignment(const safestring& name, const DataItem& value)
			: name_(name)
			, value_(value) 
		{
			if (!is_alphanumeric(name)) {
				throw Exception(ERR_BADDATA, Printf("Assigned name is not alphanumeric: %s")(name));
			}
		}
		const safestring& GetName() const {return name_;}
		const DataItem& GetValue() const {return value_;}
		virtual Nbaseutil::safestring GetDescription() const {
			// use value_.GetDescription(), GetExpression() seems to be too lengthy in case of images, for example.
			return Printf("Assignment object: %s={%s}")(name_)(value_.GetDescription()).str();
		}
		virtual PSharable DoClone() const {
			Assignment* b = new Assignment(name_, value_);
			b->value_.MakeCopy();
			return b;
		}
	private:
		safestring name_;
		DataItem value_;
	};
	typedef ThreadSharablePointer<Assignment> PAssignment;

	/// A typedef for parsed Acapella expressions. Exported, because the calcattr() module in Cells library needs to manipulate the stack.
	typedef std::stack<DataItem> DataItemStack;

	/// A typedef for the function implementing the xml*() Acapella functions. Exported for the imacro_xml library.
	typedef PMemBlock (*xml_func_t)(int type, int narg, DataItemStack& vals);

	/// A structure for holding the description of Acapella built-in functions. These are pointers to static strings.
	struct fndescr {
		const char *name, *returntype, *signature, *descr, *example, *category;
	};

	/// Return pointer to the first entry in the Acapella built-in functions' descriptions.
	DI_IMacro const fndescr* FunctionDescriptionsBegin();

	/// Return pointer past the last entry in the Acapella built-in functions' descriptions.
	DI_IMacro const fndescr* FunctionDescriptionsEnd();

	/// Find a description of an Acapella built-in function, or return NULL.
	DI_IMacro const fndescr* FindFunctionDescription(const Nbaseutil::safestring& funcname);

	/// Return the number of UTF8 characters in the string.
	DI_IMacro size_t get_string_length_in_characters(const char* utf8_string);

	/// Return the number of printable character columns for the string in UTF-8 encoding.
	DI_IMacro int get_string_length_in_columns(const safestring& utf8_string);

	/// Return true if the name looks like something which can be used at the left-hand side in an assignment in the Acapella Set() module call.
	DI_IMacro bool IsAssignableDataitemName(const safestring& name);

} // namespace
#endif
