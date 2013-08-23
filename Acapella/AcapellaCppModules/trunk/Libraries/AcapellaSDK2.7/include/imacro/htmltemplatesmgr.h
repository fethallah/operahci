#ifndef x_ACAPELLA_IMACRO_HTMPLTEMPLATESMGR_H_INCLUDED_
#define x_ACAPELLA_IMACRO_HTMPLTEMPLATESMGR_H_INCLUDED_

#include "scriptfile.h"

namespace NIMacro {

/**
* A class for manipulating templates in a StdLib text file.
* The templates are marked in the file by lines of the form "--- TEMPLATENAME"
* The template continues until the next line beginning with "---"
*/
class DI_IMacro HtmlTemplatesMgr {
public:
	/**
	* Construct the object to manage specified file. 
	* @param filename Filename without path. It is searched in StdLib directories. If not found, then
	* one will get error messages later.
	* @param immediate_load Try to load the file immediately.
	*/ 
	HtmlTemplatesMgr(const Nbaseutil::safestring& filename, bool immediate_load=false);

	~HtmlTemplatesMgr();

	/**
	* Finds specified template in the file. Returns true if found.
	* @param templatename The name of the template to look for.
	* @param text Output parameter, the template text is stored here.
	*/
	bool GetHtmlTemplate(const Nbaseutil::safestring& templatename, Nbaseutil::safestring& text) const;

	/**
	* Rereads the templates from the file. Returns true if there were any changes.
	*/
	bool ReloadTemplates() const;

	/**
	* To be called when the focus is returned back to this program. Reloads templates
	* from all changed template files. Returns true if there were any changes detected.
	*/
	static bool NotifyAppActivated();
private:
	Nbaseutil::safestring save_filename_;	// initial filename
	mutable Nbaseutil::safestring html_templates_file_;	// full pathname
	mutable NIMacro::FileTime html_templates_timestamp_;
	typedef std::map<Nbaseutil::safestring, Nbaseutil::safestring> html_templates_t;
	mutable html_templates_t html_templates_;	// html templates loaded from templates file.

	mutable NIMacro::Mutex mx_;
	static NIMacro::Mutex smx_;
	static std::set<const HtmlTemplatesMgr*> registry_;
};

/// Check the file timestamp and reload it in the content parameter, if timestamp doesn't match. Return true and update the timestamp if file was reloaded.
DI_IMacro bool ReloadTextFileIfChanged(const Nbaseutil::safestring& filename, FileTime& timestamp, Nbaseutil::safestring& content);





} // namespace
#endif
