#ifndef x_IMACRO_DATASET_H_INCLUED_
#define x_IMACRO_DATASET_H_INCLUED_

#include "script.h"
#include "executive.h"



namespace NIMacro {

class SourcedataProvider;
typedef ThreadSharablePointer<SourcedataProvider> PSourcedataProvider;
class SourcedataManager;

/**
* Base class for sourcedata providers.
*/
	class DI_IMacro SourcedataProvider: public ThreadSharable {
	public: // interface
		SourcedataProvider();

		/**
		* Return a new object of the same type as the current object.
		* This function is called by Acapella when the user creates new datasets.
		* The object will be used for serving data for the dataset specified by the parameter.
		* @param dataset_name Dataset name, like "Samples", "Positive control", etc. 
		*		This is an informational parameter. The name is usually chosen by the end user or script.
		*/
		PSourcedataProvider Clone(const Nbaseutil::safestring& dataset_name) const;

		/// Dtor. This may be called by Acapella as soon as the object is not needed any more.
		virtual ~SourcedataProvider();

		/// Return the type of the provider class.
		Nbaseutil::safestring GetType() const;


		/**
		* Store the current object state in the output parameters init1, init2, init3.
		* The object must be able to restore itself in the next program runs when the same info is passed to Init().
		* The content of the init variables depends on the provider type, as reported by GetType(). 
		*/
		void GetState(const Nbaseutil::safestring& dataset_name, Nbaseutil::safestring& init1, Nbaseutil::safestring& init2, int& init3, bool for_registry_storing) const;

		/**
		* Initialize the object. The initialization is assumed to restore/replace the full state of the object.
		* The Init() must cope with empty or corrupt init values.
		*
		* @param dataset_name Informal parameter - the user-given dataset name.
		* @param init1 Initialization string 1 as returned from GetState().
		* @param init2 Initialization string 2 as returned from GetState().
		* @param init3 Initialization integer as returned from GetState().
		*/
		void Init(const Nbaseutil::safestring& dataset_name, const Nbaseutil::safestring& init1, const Nbaseutil::safestring& init2, int init3);


		/**
		* Get a piece of HTML code which can be displayed on the HtmlView page in order to 
		* interact with the object.
		*
		* The text typically includes \<INPUT\> elements. The names of the elements should begin
		* with &%i; - these occurences will be replaced by unique placeholders.
		*
		* @param ctx The current execution context.
		* @param buffer The output buffer to store the HTML or PHP text.
		* @param dataset_name The name of the dataset, to be displayed in the generated HTML.
		* @param data_index The currently selected well index.
		*/
		void GetHtml(ExecutionContext& ctx, Nbaseutil::safestring& buffer, const Nbaseutil::safestring& dataset_name, int data_index) const; 

		/**
		* After the user has submitted the HTML page, this function may be called to pass
		* the user-chosen settings to the object. The object is supposed to update it's content
		* accordingly. The object may display interactive dialogues in this function if needed.
		*
		* @param ctx The current execution context.
		* @param settings Container of the input element values, which were marked as &%i; in the GetHtml() text.
		* @param hwnd A window handle (HWND on Windows) to display the possible dialogues in, or NULL.
		*/
		void PostHtmlData(ExecutionContext& ctx, const PContainer settings, void* hwnd); 

		/// Returns the number of currently selected wells, or -1 if the number cannot be determined.
		int GetCount() const;

		int GetCurrentPos() const;

		Nbaseutil::safestring GetLabel(int data_index) const;

		/**
		* Move to another well. Throws if not possible.
		* @param data_index Well index. This is usually in range 0 .. GetCount()-1.
		* @param data_description Output parameter, the object should store here a short description of the currently selected well, e.g. "R3C5".
		*/
		void Jump(int data_index, Nbaseutil::safestring& data_description);

		/**
		* Return the available data for the currently selected well. The container should include the necessary 
		* output parameters of the SingleWell() module, in particular SourceData table and SourceDataProp container. 
		* The images should be returned in the column "sourcedata.image".
		* Any filenames should be returned in the column "sourcedata.filename".
		*/
		NIMacro::PContainer GetSourceData() const;

		/// Same as GetSourceData(void), but allows for well index override. The "current" dataset pos set by Jump() is not modified.
		NIMacro::PContainer GetSourceData(int datasetpos_override) const;


		Nbaseutil::safestring GetProviderName() const {return provider_name_;}

		/// Return true if the provider is enabled by software and hardware configuration and can be used.
		bool IsEnabled(Nbaseutil::safestring& message) const;

	protected: // Notification interface

		void NotifySelectionChanged(int number_of_wells, const Nbaseutil::safestring& dataset_description);
		virtual void NotifyJump(int data_index) {}

	protected: // virtual functions
		/// The implementation of Clone(), see Clone() description.
		using ThreadSharable::DoClone;
		virtual PSourcedataProvider DoClone(const Nbaseutil::safestring& dataset_name) const=0;
		virtual Nbaseutil::safestring DoGetType() const=0;
		virtual void DoGetState(const Nbaseutil::safestring& dataset_name, Nbaseutil::safestring& init1, Nbaseutil::safestring& init2, int& init3, bool for_registry_storing) const=0;
		virtual void DoInit(const Nbaseutil::safestring& dataset_name, const Nbaseutil::safestring& init1, const Nbaseutil::safestring& init2, int init3)=0;
		virtual void DoGetHtml(ExecutionContext& ctx, Nbaseutil::safestring& buffer, const Nbaseutil::safestring& dataset_name, int data_index) const {} 
		virtual void DoPostHtmlData(ExecutionContext& ctx, const PContainer settings, void* hwnd) {}
		virtual int DoGetCount() const {return -1;}
		virtual NIMacro::PContainer DoGetSourceData(int data_index) const {return NULL;}
		virtual Nbaseutil::safestring DoGetLabel(int data_index) const {return "";}
		virtual bool DoIsEnabled(Nbaseutil::safestring& message) const {return true;}

	private: // data
		friend class SourcedataManager;
		int data_index_;
		Nbaseutil::safestring provider_name_;
	};

	class DI_IMacro SourcedataManager: public mb_malloced {
	public: // interface
		static SourcedataManager& Instance(bool alive=true);

		void RegisterProvider(const Nbaseutil::safestring& provider_name, PSourcedataProvider provider);

		void RemoveProvider(const Nbaseutil::safestring& provider_name);

		PVector ListProviders() const;

		PVector ListDatasets() const;

		PSourcedataProvider GetDataset(const Nbaseutil::safestring& dataset_name);

		void SetDataset(const Nbaseutil::safestring& dataset_name, const Nbaseutil::safestring& provider_name);

		void RemoveDataset(const Nbaseutil::safestring& dataset_name);

		Nbaseutil::safestring GetActiveDataset() const;

		bool SetActiveDataset(const Nbaseutil::safestring& dataset_name); 

		bool IsProviderEnabled(const Nbaseutil::safestring& provider_name, Nbaseutil::safestring& message); 

	private: // implementation
		SourcedataManager();

		~SourcedataManager();

		void RememberSettings(PSourcedataProvider provider, const Nbaseutil::safestring& dataset_name);

		void RecallSettings(PSourcedataProvider provider, const Nbaseutil::safestring& dataset_name);

	private: // data
		typedef std::map<Nbaseutil::safestring, PSourcedataProvider> providers_t;
		providers_t providers_;
		mutable Mutex providers_mx_;

		typedef std::map<Nbaseutil::safestring, PSourcedataProvider> datasets_t;
		datasets_t datasets_;
		mutable Mutex datasets_mx_;

		Nbaseutil::safestring active_dataset_;
		mutable Mutex active_dataset_mx_;
	};


/** A class providing some support for representing disk directories as IMacro datasets used by SingleWell() and ForeachWell() modules.
* This class is responsible for delivering images to the Singlewell() module
* if the image source in the EvoShell plugin PLayer window is switched to "disk files",
* and is also used for implementing the -dataset option of the command-line interface.
* The reason it's in the imacro.dll is exactly the need to support different interfaces.
*
* One DiskDataSet object manages a collection of files in a single directory. Files are
* mapped to well indices by complicated rules, or by custom scripts. For one well up
* to 8 image files are supported, containing single channel images for up to 8 channels,
* or one multi-image file (tiff, pev) per well containing arbitrary number of channels.
*/
class DiskDataSet: public mb_malloced {
public:
	// Bugfix 25.11.2003: put ctor/dtor in .cpp in order to enable debug/release mix
	DI_IMacro DiskDataSet();
	DI_IMacro ~DiskDataSet();
	DI_IMacro DiskDataSet(const DiskDataSet&); // copy ctor needed for imacro.exe project.
	// End bugfix 25.11.2003

	/** Set the disk directory for the DiskDataSet object.
	* Reset() must be called after this function.
	* 
	* If the directory contains a file named 'index.script', then this is read
	* and the procedures "filename_to_wellindex" and "wellindex_to_filenames"
	* are extracted from this file and corresponding Script objects are created.
	* Also, the 'readimage.script' in this directory is read in and parsed similarly,
	* if present.
	* If the created scripts contain syntax errors, these are reported via the 
	* SendAlarmIMacro() function which in the EvoShell enviroment relays 
	* to the SendAlarm mechanims of EvoShell, with notices popping up in the 
	* Protocol pane of EvoShell program. Default implementations will be used for 
	* any failing or missing script.
	*/
	DI_IMacro void SetPath(const safestring& path);

	/** Set the well selecting specification for the DiskDataSet object.
	* Reset() must be called after this function.
	*
	* No analysis or matching to the actual filenames is performed.
	* The mask may be a wildcarded file mask, or a comma-separated sequence
	* of well numbers or well number diapasons, e.g. "1-12, 15-19".
	* The well numbers are mapped to file names via the 
	* filename_to_wellindex() and wellindex_to_filenames() member functions.
	*/
	DI_IMacro void SetWellMask(const safestring & wellmask);

	/** Analyze wellmask, (re)read the current content of the disk directory,
	* and prepare the list of wells to be analyzed. If reset_current_pos==true,
	* then flush the internal well position to the beginning of the list.
	* This function usually calls filename_to_wellindex() repeatedly.
	*
	* Reset() must be called after SetPath() and/or SetWellMask(), and before 
	* GetWell() or Jump().
	*/
	DI_IMacro safestring Reset(bool reset_current_pos=true);

	/// Clear the error flag and error message, if these have been raised.
	DI_IMacro void ClearErrors();

	/** 
	* Extracts well index from image filename. If the current disk directory contains
	* index.script and a parsable procedure "filename_to_wellindex" therein,
	* this is called. Otherwise, a default implementation (default_filename_to_wellindex() function)
	* is called.
	*/
	DI_IMacro safestring filename_to_wellindex(const safestring& filename) const;

	/**
	* Finds out the image filename(s) corresponding to the given well index.
	* If the current disk directory contains
	* index.script and a parsable procedure "filename_to_wellindex" therein,
	* this is called. Otherwise, a default implementation (default_wellindex_to_filenames() function)
	* is called.
	*/
	DI_IMacro void wellindex_to_filenames(const safestring& wellindex, std::vector<safestring>& filenames, safestring type, safestring& available_type) const;

	/**
	* Return well data for the specified index. The returned container contains a SourceData table and a SourceDataProp container.
	* If there is no good data available for the specified index, NULL is returned.
	* If well index is out of bounds, an exception is thrown.
	*/
	PContainer GetSourceData(int wellindex);

	/** 
	* Change the position of 'current well' in the list of wells prepared by Reset().
	* Reset() must be called before this function.
	* @param wellpos: 0=first well, 1=next well, 2=previous well.
	* @param caller The calling module, derived from IO_module.
	* @param pszDataset The dataset name.
	* @param channelcount Requested number of image frames.
	* @param available_channelcount Available number of image frames.
	* @param type Obsolete, not used.
	* @param available_type Obsolete, not used.
	*/
	DI_IMacro Nbaseutil::safestring Jump(int wellpos, IO_module* caller, const char* pszDataset, int channelcount, int& available_channelcount, Executive::imagetype_t type, Executive::imagetype_t& available_type);

	/// Return the current disk directory.
	DI_IMacro Nbaseutil::safestring GetPath() const {return path_;}

	DI_IMacro Nbaseutil::safestring GetWellMask() const {return wellmask_;}

	DI_IMacro int GetWellCount() const {return int(array_.size());}

	DI_IMacro Nbaseutil::safestring GetWellLabel(int wellindex) const;

private: // implementation

	/** 
	Depending on 'step' parameter (+1/-1), move forward or backward in the list of wells until a suitable one is found.
	* The meaning of most parameters is the same as for GetWell().
	* @param fn An output parameter, receives the names for files for up to 8 channels.
	*/
	bool FindGoodWell(std::vector<safestring>& filenames, IO_module* caller, int channelcount, Executive::imagetype_t type, Executive::imagetype_t& available_type, int step=+1);

private: // data
	/// Current disk directory
	safestring path_;
	/// Current well mask
	safestring wellmask_;
	/// Current position in the well list, and the one used before that.
	int k_, last_k_;
	/// Parsed custom scripts, if present in the path_, otherwise NULL's.
	PScript filename_to_wellindex_script_, wellindex_to_filenames_script_, readimage_script_;
	/// Reused Instance for running the custom scripts.
	mutable PInstance I_;
	/// List of wells, prepared in Reset().
	std::vector<safestring, mb_allocator_typedef<safestring>::allocator > array_;
	/// If some well does not contain any or all requested channels, then it is safe to skip it.
	bool can_skip_;	
	/// We are currently working with .flex files
	bool in_flex_dir_;
	/// The length of the well index part of the filename, determined heuristically.
	int wellindexlength_;	
	/// If Reset() throws an exception, we store it in errcode_ and errmsg_. If errcode_ is nonzero, we throw an Exception from most of other functions.
	unsigned int errcode_;	
	/// See errcode_
	safestring errmsg_;
};

/**
* An utility function, converts a settings script of provideinput() module calls into a Container
* @param settings A parameter file content, presumably containing ProvideInput() lines.
* @param ctx The parameter file is run under the specified execution context.
* @param caller Pointer to the calling module object or NULL. If present, it is used for logging potential warnings.
* @return The container containing ProvideInput() data: PROMPT->EXPRESSION (as written in the ProvideInput() call).
*/
DI_IMacro PContainer ProvideInput2Container(const Nbaseutil::safestring& settings, ExecutionContext& ctx, Mod* caller=NULL);

/// An utility function, converts a container into a corresponding parameter settings script.
DI_IMacro void Container2ProvideInput(const PContainer inputs, Nbaseutil::safestring& settings_string);

DI_IMacro void AppendPhpVar(safestring& php, const safestring& name, const safestring& value);


} // namespace

#endif
