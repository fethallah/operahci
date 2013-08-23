// Module registration structures.
// This header file should be included in every module implementation DLL.
// See the documentation file "modular.pdf" for more information.

#ifndef MODREG_INCLUDED
#define MODREG_INCLUDED

namespace NIMacro {

	/// The bit flags for the module, to be used e.g. in the Mod::module() flags argument. 
	enum ModuleFlags {

		/**
		* Internal technical flag, do not use.
		*/
		MOD_CREATOR2=(1<<0),

		/// A local procedure, visible only in the same file where it is defined.
		MOD_LOCAL=(1<<1),

		/// A module has side effects; used for generating warnings in strict mode.
		MOD_SIDEEFFECTS=(1<<2),

		/// An internal module for use by PerkinElmer developers only, not showing up in documentation unless explicitly requested.
		MOD_INTERNAL=(1<<3),

		/// The Input() module and like. Used for internal optimizations.
		MOD_INPUT_MODULE=(1<<4),

		/// The ProvideInput() module and like. Used for internal optimizations.
		MOD_PROVIDEINPUT_MODULE=(1<<5),

		/// The SingleWell() module and like. Used for internal optimizations.
		MOD_SINGLEWELL=(1<<6),

		/// Procedure definition placeholder. Used for internal optimizations.
		MOD_PROCDEF=(1<<7),

		/// Experimental module (just to have something for playing around with feedback of experts - maybe not included in the installation routine).
		MOD_EXPERIMENTAL=(1<<10),

		/// Under Construction (a kind of gamma-code not approved jet for being in a release)
		MOD_UNDER_CONSTRUCTION=(1<<11), 

		/// Beta quality code (a kind of beta-code but feature complete and approved to be in the next release).
		MOD_BETA=(1<<12),

		// A free bit
		// MOD_???=(1<<13),

		/// External (code that will be developed by a customer)
		MOD_EXTERNAL=(1<<14),
	
		/// Reserved for the future implementation of unit tests: module does not support automatic unit tests.
		MOD_NO_UNIT_TEST=(1<<15),

		/// Module running has no effect and can be skipped as an optimization.
		MOD_COMMENT=1<<16,

		/// Module is doing some data exchange in addition to current datablock, and needs special synchronization in multithreaded regime.
		MOD_IO=(1<<17),			

		/**
		* In syntax check stage the module implementation itself needs to be run. 
		* This is appropriate if the module sets new items in the data environment 
		* which are not mentioned in registered parameters list, and later modules
		* have no default values for these parameters.
		*/
		MOD_SYNTAX_RUN=(1<<18),	

		/**
		* To be used with MOD_SUBSCRIPT and MOD_NEW_MODULES. Announces that the subscript text is supposed
		* to be considered as injected inline in the calling environment. Thus the local procedure definitions 
		* will be visible to the calling environment, as well as Using() directives, for example. 
		* The MOD_INJECTED flag is present currently for Include() and Eval() modules.
		*/
		MOD_INJECTED=(1<<19),

		/**
		* The explicit arguments appearing in the module call are not analyzed nor passed to the module.
		* The module has to declare a PAR_SYSTEM parameter named "&args" in order to receive the explicit argument string.
		*/
		MOD_NOPARSEDARGS=(1<<20),

		// Added 06.12.2006 (Mantis #3523)
		/**
		* The module is using "cell" names, these should not be translated to "objects".
		*/
		MOD_OBJECT_AWARE=(1<<21),

		/**
		* The module is actually dynamically constructed from a procedure text. This flag is set automatically by
		* parsing procedures. This flag is incomatible with CLASS_MODULE.
		*/
		MOD_PROCEDURE=(1<<22),

		/**
		* Module is implemented as a derived class of Mod virtual base class. See mod.h.
		* This flag is automatically set by RegisterClassModule() function or when the ExportedClasses() function 
		* returns MOD_CLASS_LIBRARY constant.
		*/
		CLASS_MODULE =(1<<23),	

		/// Module involves executing a subscript. Usually also MOD_NEW_MODULES flag is appropriate. See also MOD_INJECTED flag.
		MOD_SUBSCRIPT=(1<<24),	

		/// Syntax run of the module may register new IMacro modules. Syntax run is performed in the parsing stage.
		MOD_NEW_MODULES=(1<<25), 

		/// Implies PAR_NODIMENSIONCHECK for all parameters.
		MOD_NODIMENSIONCHECK=(1<<26), 

		// A hidden module is not shown in the module tree of user interface.
		MOD_HIDDEN= (1<<27),	

		/// A deprecated module is not shown in the module tree of user interface. Code might be removed in the future.
		MOD_DEPRECATED=((1<<28) | MOD_HIDDEN),

		/**
		* Do not delete output data items from the datablock before calling the module.
		* This is needed if the module searches for it's input data by nonstandard methods.
		*/
		MOD_PRESERVE_INPUT=(1<<29),

		/**
		* The module needs extra attention at parsing time. This flag has to be 
		* specified by RegisterModule() already. By parse time the module is
		* registered if needed, and Mod::DoVerb("parse") is called.
		*/
		MOD_EXTRA_PARSING=(1<<30),

	};


	/// Module parameter flags to be used in Mod::input() and Mod::output() function calls.
	enum ParameterFlags {
		// Not needed in class-type modules.
		PAR_INTEGER=0,		
		// Not needed in class-type modules.
		PAR_DOUBLE=1,		
		// Not needed in class-type modules.
		PAR_STRING=2,		
		// Not needed in class-type modules.
		PAR_POINTER=4,		
		// Not needed in class-type modules.
		PAR_MEMBLOCK=8,		
		// Parameter name encodes a unit - 05.02.2004
		PAR_UNIT=16,		
		// Not needed in class-type modules.
		PAR_INPUT=32,		
		// The parameter can appear as a positional parameter in the module call.
		PAR_POSITIONAL=64,	
		// Not needed in class-type modules.
		PAR_OUTPUT=128,		
		// The parameter can appear multiple times on the command line. This also requires PAR_DUMMY
		PAR_MULTI=256,
		// Is used together with PAR_STRING flag. Specifies that the value specified in the command-line should be regarded as a string constant even if it is not surrounded by quotes.
		PAR_NOQUOTE=512,	
		// The parameter cannot be fetched from the current pipeline state if it is not present on the command line.
		PAR_NOFETCH=1024,	
		// A synonum for PAR_NOFETCH
		PAR_EXPLICIT=1024,	
		// ThreadSharablePointer<T> parameter. Not needed in class-type modules.
		PAR_SHARABLE=2048,	
		// Not needed in class-type modules.
		PAR_ARG_BASE=4096,	
		// Not needed in class-type modules.
		PAR_BOOLEAN=8192,	
		// A commonly used parameter (the default)
		PAR_COMMON=		0,			
		// Parameter is presumably used quite rarely
		PAR_RARE=		(1<<14),	
		// Parameter is useful only for module developers
		PAR_DEBUG=		(1<<15),		
		// A system parameter, for managing information from IMacro system data block. Such a parameter does not show up in module descriptions.
		PAR_SYSTEM=		(1<<16),	
		// Shows that the default value is an ASCIIZ string, specifying a data item name in current pipeline state, which should serve as the default value. 
		PAR_DYNADEFAULT=(1<<17),	
		// Is used along with PAR_DYNADEFAULT flag to indicate that if the PAR_DYNADEFAULT default value is not found in the pipeline state, null default value is accepted.
		PAR_NULLDEFAULT=(1<<18),	
		// This is a dummy parameter, for descriptional purposes only. Real parameter transfer proceeds by other mechanisms.
		PAR_DUMMY=		(1<<19),	
		// Not needed in class-type modules.
		PAR_SAFESTRING= (1<<20),	
		// This parameter value is needed in syntax check run.
		PAR_SYNTAX=		(1<<21),	
		// Not needed in class-type modules.
		PAR_BOOLEAN_CPLUSPLUS=(PAR_BOOLEAN | 1<<22),		
		// This is a polytype SafeValue parameter
		PAR_SAFEVALUE=  (1<<23),	
		// Use together with PAR_NOQUOTE to receive the input parameter together with quotes.
		PAR_KEEPQUOTES= (1<<24),	
		// The parameter is a file name, therefore literal backslashes are not escape characters.
		PAR_FILENAME=(1<<25),		
		// Script will specify parameter names, as in case of Set() and CalcAttr() modules. This param should also have PAR_DUMMY flag.
		PAR_USERNAMED=(1<<26),		
		// The parameter is Jar-derived and serves as a local context for resolving other parameters.
		PAR_LOCALCONTEXT=(1<<27),	
		// The image dimensions of the parameter may differ from image dimensions of other input parameters. Currently image dimensions are checked for classes: Image, IndexVector, IntervalVector, Cells.
		PAR_NODIMENSIONCHECK=(1<<28),	
		// The parameter is a directory name.
		PAR_DIRNAME= (PAR_FILENAME | (1<<29)),	
		// The parameter is a name of another IMacro module or procedure
		PAR_MODULENAME=(1<<30),		
		// Used together with PAR_USERNAMED flag to indicate that the module call syntax should just list the user-defined names, like Delete() module, no value allowed. Used in NESPMacroEditor::ParamView::AddParam().
		PAR_NOVALUE=(1<<31),		
		// Used together with PAR_FILENAME flag, specifies that filename is used for inputting data. This affects the file selection dialogs, for example.
		PAR_FILENAME_INPUT=(1<<31), 
								
	};

} // namespace
#endif

