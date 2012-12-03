#ifndef x_IMACRO_UIMACRO_HTML_OUTPUT_DEFINED_
#define x_IMACRO_UIMACRO_HTML_OUTPUT_DEFINED_

#include "jar.h"

namespace NIMacro {

	class DI_IMacro HtmlOutput {
	public:
		/// Creates the file
		HtmlOutput(Nbaseutil::OutputStream& ostream, const Nbaseutil::safestring& title);

		/// Appends to an existing file
		HtmlOutput(Nbaseutil::OutputStream& ostream);
		
		~HtmlOutput();
		
		/// Adds the document termination tags
		void Finalize();

		// All output methods assume that any HTML special symbols have been 
		// quoted properly. You can prepare this with the Quote() method.
		static safestring Quote(const Nbaseutil::safestring& text);

		// reverse
		static void Unquote(Nbaseutil::safestring& text);

		void OutRaw(const Nbaseutil::safestring& text);	// raw text output
		void OutRaw(const char* text) {OutRaw(Nbaseutil::safestring(text));}
		void OutPara(const Nbaseutil::safestring& text);	// begins a new paragraph and outputs the text
		void OutImage(const PImage img, const Nbaseutil::safestring& caption, const char* writeimageargs="palette=\"truecolor\"", bool bmp=true, bool invert=false); // output and image

		// A general dispatch to different IMacro classes:
		void OutTable(const PMemBlock& tbl, const Nbaseutil::safestring& caption, bool uselocale);

		void OutTable(const PVector tbl, const Nbaseutil::safestring& caption, bool uselocale); // output an table
		void OutTable(const PContainer tbl, const Nbaseutil::safestring& caption, bool uselocale); // output an table
		//void OutTable(const CEvoGrid& tbl, const Nbaseutil::safestring& caption, bool transpose, bool uselocale); // output an table
		void OutTableCells(const PJar tbl, const Nbaseutil::safestring& caption, bool uselocale);
		void OutTable(const PTable tbl, const Nbaseutil::safestring& caption, bool uselocale);

		std::set<Nbaseutil::safestring> usedfiles_;
		Nbaseutil::OutputStream& GetStream() {return f_;}
	private:
		Nbaseutil::OutputStream& f_;
		Nbaseutil::safestring filename_pref_;
		int nimages_;	// number of output images
		int ntables_;	// number of output tables
	};

}


#endif
