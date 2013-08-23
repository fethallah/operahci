#ifndef _IMACRO_UIMACRo_MRULIST_H_INCLUDED_
#define _IMACRO_UIMACRo_MRULIST_H_INCLUDED_

namespace NIMacro {

#ifndef _MSC_VER
#undef HKEY_CURRENT_USER
#undef HKEY_LOCAL_MACHINE
 namespace {
	 const HKEY HKEY_CURRENT_USER = ((HKEY) 0x80000001 );
	 const HKEY HKEY_LOCAL_MACHINE = ((HKEY) 0x80000002 );
 }
#endif

/** A template for managing MRU lists of items of type T.
* Type T must be:
*		- comparable and assignable and default-constructible, so that it can be held in std::list<>.
*		- have a method: const char* c_str() for converting it into string representation.
*		- have a constructor from const char* for converting it from string representation.
*/
template<class T>
class MruList {
	typedef std::list<T> cont_t;
	cont_t cont_;
	mutable bool dirty_;	// needs saving in the registry
	HKEY iRegistry_;
	Nbaseutil::safestring sRegKey_, s_last_val_name_;
	T last_val_;
public:

	/**
	* @brief Construct the mru list and assign the location in registry.
	* @param sRegKey Name of the item, e.g. "ZoomMRU". This wil become a key name in the Windows registry. Can optionally contain colon followed by the last value registry name.
	* @param sRegPath Registry path where to create the key.
	* @param iRegistry Registry branch - HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE or 0 (for no storage in registry).
	*/
	MruList<T>(const Nbaseutil::safestring& sRegKey, const Nbaseutil::safestring& sRegPath=GetAcapellaRegBranch() + "\\IMacro\\", const HKEY iRegistry=HKEY_CURRENT_USER):
		dirty_(false), iRegistry_(iRegistry), sRegKey_(sRegPath)
	{
		Init(sRegKey, sRegPath, iRegistry);
		ReadFromRegistry();
	}
	MruList<T>():
	  dirty_(false), iRegistry_(0) {
	}

    /// Writes current content in the registry, if needed/possible, changes the location and reads the new content in.
	void ChangeRegistryLocation(const Nbaseutil::safestring& sRegKey, const Nbaseutil::safestring& sRegPath=GetAcapellaRegBranch() + "\\IMacro\\", const HKEY iRegistry=HKEY_CURRENT_USER) {
		WriteToRegistry();
		Init(sRegKey, sRegPath, iRegistry);
		ReadFromRegistry();
	}

	/// Calls WriteToRegistry().
	~MruList<T>() {
		WriteToRegistry();
	}

	T GetLastVal() const {
		return last_val_;
	}

	/// Reads the content from registry. This is done automatically in the ctor.
	void ReadFromRegistry() {
		if (!iRegistry_) {
			return; // not connected to registry
		}
		cont_.clear();
		for (int i=0; i<=9; ++i) {
			Nbaseutil::safestring s = GetRegKey(iRegistry_, sRegKey_.c_str(), NIMacro::str(i).c_str());
			if (!s.empty()) {
				cont_.push_back(s.c_str());
			}
		}
		if (!s_last_val_name_.empty()) {
			Nbaseutil::safestring s = GetRegKey(iRegistry_, sRegKey_.c_str(), s_last_val_name_.c_str());
			if (s.empty()) {
				last_val_ = Get(0);
			} else {
				last_val_ = T(s.c_str());
			}
		} else {
			last_val_ = Get(0);
		}
		dirty_ = false;
	}

	/// Write content into registry, if needed. This is called automatically from dtor.
	void WriteToRegistry() {
		if (!dirty_) {
			return;		// no need to save
		}
		if (!iRegistry_) {
			return; // not connected to registry
		}
		// Enh 17.08.2005: merge the info more wisely, for the case two apps are running in parallel
		cont_t new_entries;
		new_entries.swap(cont_);
		// Bugfix 31.08.2007: do not lose the current last setting:
		T last_val(last_val_);
		ReadFromRegistry();
		last_val_ = last_val;
		for (typename cont_t::const_reverse_iterator it=new_entries.rbegin(), itend=new_entries.rend(); it!=itend; ++it) {
			PushOnly(*it);
		}
		int k=0;
		for (const_iterator it=begin(), itend=end(); it!=itend; ++it, ++k) {
			if (k>9) {
				break;	// support only 10 values in registry
			}
			Nbaseutil::safestring s(it->c_str());
			SetRegKey(iRegistry_, sRegKey_, NIMacro::str(k), s);
		}
		while (k<=9) {
			SetRegKey(iRegistry_, sRegKey_, NIMacro::str(k), "");
			++k;
		}
		if (!s_last_val_name_.empty()) {
			Nbaseutil::safestring s(last_val_.c_str());
			SetRegKey(iRegistry_, sRegKey_, s_last_val_name_, s);
		}
		dirty_ = false;	// saved
	}

	/// Iterator type for iterating over the MRU list.
	typedef typename cont_t::const_iterator const_iterator;

	/// Return startpoint for iterating.
	const_iterator begin() const { return cont_.begin(); }

	/// Return endpoint for iterating.
	const_iterator end() const { return cont_.end(); }

	/// Pushes an item in the front of MRU list. If list size exceeds 10, the old items are forgotten. Returns true if any changes have been made which should be written in the registry.
	bool PushOnly(const T& t) {
		if (!(last_val_ == t)) {
			last_val_ = t;
			dirty_ = true;
		}
		typename cont_t::iterator it = std::find(cont_.begin(), cont_.end(), t);
		if (it!=cont_.end()) {
			if (it==cont_.begin()) {
				return dirty_;	// ok
			}
			cont_.erase(it);
		}
		cont_.push_front(t);
		if (cont_.size()>10) {
			typename cont_t::iterator p = cont_.end();
			--p;
			cont_.erase(p);
		}
		return true;
	}
	/// Pushes an item in the front of MRU list and writes the list in the registry if needed. If list size exceeds 10, the old items are forgotten.
	void Push(const T& t) {
		if (PushOnly(t)) {
			// Bugfix 17.08.2005 (Mantis #3842): write the changes in the registry immediately. The dtor is not called in Release build. 
			WriteToRegistry();
		}
	}

	/// Return k-th item, k==0 is the most recently used item.
	T Get(int k) const {
		int i=0;
		for (const_iterator it=begin(), itend=end(); it!=itend; ++it, ++i) {
			if (i==k) {
				return *it;
			}
		}
		return T();
	}
#ifdef __AFX_H__
	/// Load the MRU list in a CComboBox.
	void LoadToCombo(CComboBox& combo) const {
		combo.ResetContent();
		for (const_iterator it = begin(), itend=end(); it!=itend; ++it) {
			combo.AddString(it->c_str());
		}
		combo.SetCurSel(0);
		combo.SetEditSel(-1,-1);
	}
#endif

	/**
	* @brief Compose an HTML piece for displaying the MRU list.
	* @param tmplt A template for each item line. Should contain %o placeholder for item name and %s placeholder for text "SELECTED" for the first item.
	*             An example: "<option %s>%o</option>\n".
	* @param html Substituted item lines are appended to this output parameter.
	*/
	void LoadToHtmlTemplate(const Nbaseutil::safestring& tmplt, Nbaseutil::safestring& html);

private:
	void Init(const Nbaseutil::safestring& sRegKey, const Nbaseutil::safestring& sRegPath, const HKEY iRegistry) {
		iRegistry_ = iRegistry;
		sRegKey_ = sRegPath;
		if (!sRegKey_.empty() && sRegKey_.find_last_of("/\\")!=sRegKey_.length()-1) {
			sRegKey_ += "\\";
		}
		strlen_t k = sRegKey.find(':');
		if (k!=sRegKey.npos) {
			s_last_val_name_ = sRegKey.substr(k+1);
			sRegKey_ += sRegKey.substr(0, k);
		} else {
			s_last_val_name_.erase();
			sRegKey_ += sRegKey;
		}
	}
};

} // namespace
#endif
