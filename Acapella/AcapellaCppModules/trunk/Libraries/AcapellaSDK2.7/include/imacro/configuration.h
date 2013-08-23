#ifndef x_PKI_TLN_ACAPELLA_IMACRO_CONFGFILE_H_INCLUDED
#define x_PKI_TLN_ACAPELLA_IMACRO_CONFGFILE_H_INCLUDED

namespace YAML {
	inline bool Convert(const std::string& input, Nbaseutil::safestring& output) {
		output.assign(input.data(), input.size());
		return true;
	}
} // namespace YAML


#include <yaml-cpp/yaml.h>
#ifdef ACAPELLA_PLATFORM_WINDOWS
#ifdef _DEBUG
	#pragma comment(linker, "/defaultLib:yaml-cppD.lib")
#else
	#pragma comment(linker, "/defaultLib:yaml-cppR.lib")
#endif
#endif

namespace NIMacro {

	/**
	* A singleton class for managing Acapella processwide settings.
	*/
	class DI_IMacro Configuration: private boost::noncopyable {
	public: // typedefs

		/// A typedef for settings inside the subsystem section.
		typedef YAML::Node setting_t;

	public: // static interface

		/// Returns a reference to the singleton
		static Configuration& Instance();

	public: // interface for Acapella library clients

		/**
		* Load and apply a configuration from a disk file. The name of the disk file is remembered and it
		* can be later reloaded and reapplied by calling Reload().
		*
		* There is no default configuration file name applied by the IMacro library. The library clients like
		* runmacro console interface executable may use one.
		* @param filename A name of an existing disk file in YAML format.
		*/
		void LoadFromFile(const safestring& filename);

		/**
		* Parse and apply the passed configuration. If Reload() is called later, the same
		* settings are re-applied. This config file name set by LoadFromFile() is cleared by calling this method.
		* @param content The configuration file content in YAML format.
		*/
		void LoadFromMemory(const safestring& content);

		/**
		* Parse and apply the passed settings. These are not remembered and not reapplied when calling Reload().
		* @param content The configuration file content in YAML format.
		*/
		void Change(const safestring& content);

		/**
		* Reparses and reapplies the configuration settings set by LoadFromFile() or LoadFromMemory().
		* If LoadFromFile() was called last, then the corresponding file is re-read from disk.
		* The changes applied by Change() are not reapplied.
		*/
		void Reload();

		/// Reapplies the last settings for the specified subsystem, without reloading the config file.
		void Reapply(const safestring& subsystem_name);

	public: // interface for subsystems

		/**
		* An abstract base class interface for deriving subsystems for configuration setting.
		* A module library can define one or more subsystems, for example.
		*/
		class SubSystemBase {
		public:
			virtual ~SubSystemBase() {}
			/**
			* This is called for applying a single setting from a section in the config file corresponding
			* to this subsystem, according to the registration name. If the setting is unknown, then it is up
			* to the subsystem whether to ignore it or throw an error. In the latter case a level 2 log message
			* is logged with [acapella/config] topic, this does not affect processing other setting for this or other
			* subsystems.
			*
			* During the call it is not allowed to call any Configuration class methods.
			* @param aca Acapella instance for which the settings are applied.
			* @param name Setting name, as appearing in the configuration file.
			* @param value Setting value, as parsed from the configuration file.
			*/
			virtual void ApplySetting(const safestring& name, const setting_t& value)=0;
		};

		/**
		* Register a subsystem by the config file managament. If a configuration has been loaded,
		* the \a subsystem object will be immediately called for applying the corresponding settings.
		* If a subsystem with the same name has been already registered, it is replaced.
		*/
		void RegisterSubSystem(const safestring& subsystem_name, SubSystemBase& subsystem);

		/**
		* Unregister a subsystem explicitly, should not be needed usually.
		*/
		void UnregisterSubSystem(const safestring& subsystem_name, SubSystemBase& subsystem);

	private: // implementation
		Configuration();
		std::string LoadAndCheck(const safestring& filename);
		void ProcessConfig(const safestring& config_filename, const std::string& content, const safestring& subsystem_filter="");
		void ProcessYamlDocument(const YAML::Node& doc, const safestring& subsystem_filter="");
		bool ProcessSubSystem(const safestring& subsystem_name, const YAML::Node& node);
		bool LoadSubSystem(const safestring& subsystem_name);

	private: // data
		boost_mutex subsystem_map_mx_;
		typedef std::map<safestring, SubSystemBase*> subsystem_map_t;
		subsystem_map_t subsystem_map_;

		boost_mutex mx_;
		safestring config_filename_;
		std::string config_content_;
	};

	// utility functions for processing YAML data

	/// A stream operator for fetching a string value from a YAML scalar node to a safestring.
	DI_IMacro const YAML::Node& operator>>(const YAML::Node& node, safestring& x);

	/// Convert a scalar YAML node into the requsted type and return its value.
	template<typename T>
	inline T ExtractYAML(const YAML::Node& node) {
		T x;
		node >> x;
		return x;
	}

	/// Find a subkey in the YAML node, case-insensitive, and return its value or the default.
	template<typename T>
	inline T ExtractYAML(const YAML::Node& node, const safestring& key, const T& defaultvalue) {
		for(YAML::Iterator it=node.begin();it!=node.end();++it) {
			safestring s;
			it.first() >> s;
			if (s==key) {
				// gcc 4.0.1 on Mac chokes on it.second().to<T>(), try another way
				T x;
				it.second() >> x;
				return x;
			}
		}
		return defaultvalue;
	}

} // namespace NIMacro


#endif
