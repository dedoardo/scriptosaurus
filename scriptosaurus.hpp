/*
	MIT License

	Copyright (c) 2016 Edoardo 'sparkon' Dominici

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#pragma once

#if !defined(scriptosaurus_32bit) && !defined(scriptosaurus_64bit)
#	define scriptosaurus_32bit
#endif

// C++ STL
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <initializer_list>
#include <atomic>
#include <thread>
#include <functional>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <regex>
#include <atomic>
#include <iostream>
#include <mutex>
#include <iomanip>

// Platform includes
#if defined(scriptosaurus_win32)
#	define WIN32_LEAN_AND_MEAN
#	include <WinSock2.h>
#	include <Windows.h>
#	include <shellapi.h>
#endif

namespace scriptosaurus
{
	// Base Types
	using ScriptName = std::string;
	using ScriptRoutine = void(*)(void*);

	// Some constants, feel free to modify them if it better suits your needs.
	const char kScriptDirSeparator = '@';
	const char kCompilerLogFile[] = ".scriptosaurus_log";
	const char kBinDirName[] = ".bin";
	const char kScriptExtension[] = ".cpp";
	const unsigned int kDaemonSleepTimeMs = 100u;

	const char* kStartTag = ":scriptosaurus";
	const char* kEndTag = ":end";
	const char* kDebugCmd = "debug";

	// True if you want a warning to be generated when scriptosaurus: is found but not the 
	// closing end: or viceversa
	const bool kWarnOnOpenBlocks = true;

	// True if you want a warning to be generated when a file fails to compile
	// It's ok to leave this to false, as pressing Ctrl+S mid-typing will trigger
	// the warning
	const bool kWarnOnCompilerErrors = true;

	namespace internal
	{
		void write_out(std::ostream& ostream, std::initializer_list<const char*> msgs);
		struct ScriptMetadata
		{
			enum class Optimization
			{
				O1, O2, O3
			};

			bool		 generate_debug;
			Optimization optimization;

			std::vector<std::pair<std::string, std::string>> defines;
		};
	}

	// Error reporting, simply define your callback
	// Locking not really needed since most of the logging is done in the daemon 
	// thread. BUT if the user doesn't want any problem he should overwrite 
	// the defaults callbacks
#if	!defined(scriptosaurus_on_info)
#	define scriptosaurus_on_info(...) internal::write_out(std::cout, {__VA_ARGS__});
#endif

#if	!defined(scriptosaurus_on_warning)
#	define scriptosaurus_on_warning(...) internal::write_out(std::cout, {__VA_ARGS__});
#endif

#if	!defined(scriptosaurus_on_error)
#	define scriptosaurus_on_error(...) internal::write_out(std::cerr, {__VA_ARGS__});
#endif

	/*
		Platform specific code is contained here. Currently on win32 is supported.
	*/
	namespace platform
	{
#if defined(scriptosaurus_win32)
		using Lock = CRITICAL_SECTION;
		using Library = HMODULE;
		const HMODULE kInvalidLibrary = nullptr;
		const char kDynamicLibraryExtension[] = ".dll";

		// Locking
		// ====================================================================
		inline void init(Lock* lock)
		{
			InitializeCriticalSectionAndSpinCount(lock, 0x00000800);
		}

		inline void destroy(Lock* lock)
		{
			DeleteCriticalSection(lock);
		}

		inline void lock(Lock* lock)
		{
			EnterCriticalSection(lock);
		}

		inline void unlock(Lock* lock)
		{
			LeaveCriticalSection(lock);
		}

		// DLL
		// ====================================================================
		inline Library load(const char* filename)
		{
			return LoadLibraryA(filename);
		}

		inline void unload(Library& library)
		{
			if (library != kInvalidLibrary)
				FreeLibrary(library);
		}

		template <typename FuncType>
		inline FuncType get_func_ptr(Library& library, const char* func_name)
		{
			return (FuncType)(GetProcAddress(library, func_name));
		}

		// OS
		// ====================================================================
		inline int execute_command(const char* cmd)
		{
			scriptosaurus_on_info("Executing command: ", cmd);
			return system(cmd);
		}

		inline bool create_directory(const char* filename)
		{
			return CreateDirectoryA(filename, nullptr) == TRUE;
		}

		inline bool delete_file(const char* filename)
		{
			return DeleteFileA(filename) == TRUE;
		}

		inline void iter_dir(const std::string& root_dir, std::function<void(const std::string& dir, const std::string& filename)> callback)
		{
			WIN32_FIND_DATAA fd;
			HANDLE hfind = FindFirstFileA((root_dir + "\\*").c_str(), &fd);
			if (hfind != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (std::strcmp(fd.cFileName, ".") == 0 || std::strcmp(fd.cFileName, "..") == 0 || fd.cFileName[0] == '.')
						continue;

					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						iter_dir(root_dir + "/" + fd.cFileName, callback);
						continue;
					}

					callback(root_dir, fd.cFileName);
				} while (FindNextFileA(hfind, &fd));
				FindClose(hfind);
			}
		}
#endif
	}

	/*
	Compiler code, not related to the platform. Currently only msvc is supported.
	the macro itself ( scriptosaurus_msvc ) should also contain the path of
	the executable.

	Note on VS2013. Debugging probably doesn't work (https://connect.microsoft.com/VisualStudio/feedback/details/1026184/visual-studio-keeps-locking-the-pdb-file-even-after-the-dll-is-released-using-freelibrary)
	because when calling FreeLibrary() the .pdb file is locked in. Even if
	you don't start debugging if the .pdb file is produced it will be locked
	and following modifications will fail to link.
	*/
	namespace compiler
	{
		struct CompilerInput
		{
			std::string input_file;
			std::string out_file;
			std::string dump_file;
		};

		struct LinkerInput
		{
			std::vector<std::string> obj_files;
			std::string out_file;
		};

		bool compile(const ScriptName& name, const CompilerInput& input, const internal::ScriptMetadata& metadata);
		bool link(const ScriptName& name, const LinkerInput& input, const internal::ScriptMetadata& metadata);

#if defined(scriptosaurus_msvc)
		const char* kMSVCPath = scriptosaurus_msvc;
		const char* kMSVCCompiler = "cl.exe";
		const char* kMSVCLinker = "link.exe";

		inline bool compile(const ScriptName& name, const CompilerInput& input, const internal::ScriptMetadata& metadata)
		{
			const char kCompilerBat[] = ".scriptosaurus_msvc_compile.bat";
#if defined(scriptosaurus_32bit)
			const std::string vcvars = std::string(kMSVCPath) + "/vcvars32.bat";
#elif defined(scriptosaurus_64bit)
			const std::string vcvars = std::string(kMSVCPath) + "/vcvars64.bat";
#else
#	error Please define architecture for MSVC compiler
#endif
			const std::string cl = std::string(kMSVCPath) + "/" + kMSVCCompiler;

			std::stringstream bat_ss;
			bat_ss << "@ECHO OFF" << std::endl;
			bat_ss << std::quoted(vcvars) << " && "; // setup environment
			bat_ss << std::quoted(cl) << " "; // cl.exe
			bat_ss << (metadata.generate_debug ? "/Zi " : " ") << input.input_file; // Debug info

			// preprocessor
			for (const auto& kv : metadata.defines)
				bat_ss << " /D" << kv.first << "=" << kv.second << " ";

			bat_ss << " /c /EHsc /MT /Fo" << std::quoted(input.out_file) << " ";

			std::ofstream bat_stream(kCompilerBat);
			bat_stream << bat_ss.str();
			bat_stream.close();

			int res = platform::execute_command(kCompilerBat);

			platform::delete_file(kCompilerBat);

			return res == ERROR_SUCCESS;
		}

		inline bool link(const ScriptName& name, const LinkerInput& input, const internal::ScriptMetadata& metadata)
		{
			const char kLinkerBat[] = ".scriptosaurus_msvc_link.bat"; 
#if defined(scriptosaurus_32bit)
				const std::string vcvars = std::string(kMSVCPath) + "/vcvars32.bat";
#elif defined(scriptosaurus_64bit)
			const std::string vcvars = std::string(kMSVCPath) + "/vcvars64.bat";
#else
#	error Please define architecture for MSVC compiler
#endif
			const std::string link = std::string(kMSVCPath) + "/" + kMSVCLinker;

			std::stringstream bat_ss;
			bat_ss << "@ECHO OFF" << std::endl;
			bat_ss << std::quoted(vcvars) << " && "; // setup environment
			bat_ss << std::quoted(link); // link.exe
			bat_ss << " /DLL "; // DLL output
			bat_ss << (metadata.generate_debug ? "/DEBUG " : "");
			bat_ss << " /OUT:" << std::quoted(input.out_file) << " "; // output file
			for (const auto& obj : input.obj_files)
				bat_ss << std::quoted(obj) << " "; // list of obj files to input

			std::ofstream bat_stream(kLinkerBat);
			bat_stream << bat_ss.str();
			bat_stream.close();

			int res = platform::execute_command(kLinkerBat);

			platform::delete_file(kLinkerBat);
			
			return res == ERROR_SUCCESS;
		}
#elif defined(scriptosaurus_clang_cl)
		const char* kMSVCPath = scriptosaurus_clang_cl;
		const char* kClangCompiler = "clang-cl.exe";
		const char* kMSVCLinker = "link.exe";

		inline bool compile(const ScriptName& name, const CompilerInput& input, const internal::ScriptMetadata& metadata)
		{
			// TODO: No clang path supported ( add option )
			const std::string clang = kClangCompiler;

			std::stringstream clang_ss;
			clang_ss << clang << " ";
			clang_ss << (metadata.generate_debug ? "/Zi " : " ") << input.input_file; // Debug info

																					// preprocessor
			for (const auto& kv : metadata.defines)
				clang_ss << " /D" << kv.first << "=" << kv.second << " ";

			clang_ss << " /c /EHsc /MT /Fo" << std::quoted(input.out_file) << " ";

			int res = platform::execute_command(clang_ss.str().c_str());

			return res == ERROR_SUCCESS;
		}

		inline bool link(const ScriptName& name, const LinkerInput& input, const internal::ScriptMetadata& metadata)
		{
			const char kLinkerBat[] = ".scriptosaurus_msvc_link.bat";
#if defined(scriptosaurus_32bit)
			const std::string vcvars = std::string(kMSVCPath) + "/vcvars32.bat";
#elif defined(scriptosaurus_64bit)
			const std::string vcvars = std::string(kMSVCPath) + "/vcvars64.bat";
#else
#	error Please define architecture for MSVC compiler
#endif
			const std::string link = std::string(kMSVCPath) + "/" + kMSVCLinker;

			std::stringstream bat_ss;
			bat_ss << "@ECHO OFF" << std::endl;
			bat_ss << std::quoted(vcvars) << " && "; // setup environment
			bat_ss << std::quoted(link); // link.exe
			bat_ss << " /DLL "; // DLL output
			bat_ss << (metadata.generate_debug ? "/DEBUG " : "");
			bat_ss << " /OUT:" << std::quoted(input.out_file) << " "; // output file
			for (const auto& obj : input.obj_files)
				bat_ss << std::quoted(obj) << " "; // list of obj files to input

			std::ofstream bat_stream(kLinkerBat);
			bat_stream << bat_ss.str();
			bat_stream.close();

			int res = platform::execute_command(kLinkerBat);

			platform::delete_file(kLinkerBat);

			return res == ERROR_SUCCESS;
		}
#endif
	}

	struct Script;
	namespace internal
	{
		template <typename KeyType, typename ValueType>
		using Map = std::unordered_map<KeyType, ValueType>;

		struct SwappableRoutine
		{
			void* address;
			platform::Lock* lock_address;
		};

		// Param for initializer list to be converted to SwappableRoutine
		struct SwappableRoutineParam
		{
			const char* name;
			void* base_address;
		};

		struct ReloadableScript
		{
			Script* script = nullptr;
			Map<std::string, SwappableRoutine> routines;
			platform::Library current_library = platform::kInvalidLibrary;
			unsigned int reload_counter = 0u;
		};

		using ScriptHash = std::size_t;
		struct PerLoopData
		{
			bool seen;
			ScriptHash hash;
		};
		using UpdateMap = Map<ScriptName, PerLoopData>;

		struct DaemonData
		{
			std::string root_directory;
			std::string bin_directory;
			platform::Lock* scripts_lock;
			Map<ScriptName, ReloadableScript>* scripts;
		};

		void write_out(std::ostream& ostream, std::initializer_list<const char*> msgs)
		{
			static std::mutex mutex;

			std::unique_lock<std::mutex> lock(mutex);
			for (const auto& msg : msgs)
				ostream << msg;
			ostream << std::endl;
		}

		// Converts a path+filename to the corresponding scriptname
		inline ScriptName filename_to_name(const std::string& filename)
		{
			// If absolute path, skipping volume
			const char* filep = filename.data();
			if (filename[1] == ':')
				filep += 2;

			std::string file(filep);

			// Removing root directory
			file = file.substr(file.find_first_of('/') + 1);

			// Removing trailing extension
			file = file.substr(0, file.find_last_of('.'));

			// Substituting // \ with separator
			std::replace(file.begin(), file.end(), '\\', kScriptDirSeparator);
			std::replace(file.begin(), file.end(), '/', kScriptDirSeparator);
			return file;
		}

		// Computes the hash for the file by loading it from the specified path.
		inline ScriptHash compute_hash(const std::string& filename)
		{
			std::ifstream file_stream(filename.c_str());
			if (!file_stream.is_open())
			{
				scriptosaurus_on_error("Failed to open file for hashing: ", filename.c_str());
				return std::hash<std::string>{}("");
			}

			std::stringstream ss;
			ss << file_stream.rdbuf();
			return std::hash<std::string>{}(ss.str());
		}

		// Parses a single command in a metadata block
		inline void parse_command(const std::string& cmd_name, std::string& cmd_params, ScriptMetadata& metadata)
		{
			if (cmd_name.compare(kDebugCmd) == 0)
			{
				std::transform(cmd_params.begin(), cmd_params.end(), cmd_params.begin(), ::tolower);
				if (cmd_params.compare("true") == 0)
					metadata.generate_debug = true;
				else if (cmd_params.compare("false") == 0)
					metadata.generate_debug = false;
				else
					// Report error;
					metadata.generate_debug = false;
			}
		}

		// Extracts the script metadata from the file
		// Currently only supports one block
		inline void extract_metadata(const ScriptName& script_name, const char* file_buffer, ScriptMetadata& metadata)
		{
			std::string buffer = file_buffer;
			auto block_start = buffer.find(kStartTag);
			auto block_end = buffer.find(kEndTag);

			// No metadata found
			if (block_start == std::string::npos || block_end == std::string::npos)
			{
				if (kWarnOnOpenBlocks)
					scriptosaurus_on_warning("Found unmatched block for: ", script_name.c_str());
				return;
			}

			std::string block = buffer.substr(block_start, block_end - block_start);

			std::istringstream iss(block);
			for (std::string line; std::getline(iss, line); )
			{
				std::regex command_regex("([a-zA-Z]+)\\s*:\\s*([/:a-zA-Z0-9_ ,]+)");
				std::smatch command_matches;

				if (std::regex_search(line, command_matches, command_regex) &&
					command_matches.size() == 3)
				{
					std::string cmd_name = command_matches.str(1);
					std::string cmd_params = command_matches.str(2);
					parse_command(cmd_name, cmd_params, metadata);
				}
			}

			return;
		}

		// Processes diffs for a single file. This is where the magic happens.
		inline void process_diff(DaemonData& daemon_data, const ScriptName& name, const std::string& filename)
		{
			// Extracting metadata
			std::ifstream file_stream(filename.c_str());
			if (!file_stream.is_open())
			{
				scriptosaurus_on_error("Failed to open file for diff processing: ", filename.c_str());
				return;
			}

			std::stringstream ss;
			ss << file_stream.rdbuf();

			// Extracting metadata
			ScriptMetadata metadata;
			extract_metadata(name, ss.str().c_str(), metadata);

#if defined(scriptosaurus_live)
			metadata.defines.push_back({ "scriptosaurus_live", "1" });
#endif

			// Retrieving counter for library double buffering
			platform::lock(daemon_data.scripts_lock);
			unsigned int counter = (*daemon_data.scripts)[name].reload_counter++;
			platform::unlock(daemon_data.scripts_lock);

			// Generating some names
			const std::string dll_name = "/" + name + kScriptDirSeparator + std::to_string(counter % 2) + platform::kDynamicLibraryExtension;
			const std::string old_dll_name = "/" + name + kScriptDirSeparator + std::to_string((counter + 1) % 2) + platform::kDynamicLibraryExtension;
			const std::string out_name = daemon_data.bin_directory + dll_name;
			const std::string old_out_name = daemon_data.bin_directory + old_dll_name;
			
			compiler::CompilerInput cinput;
			cinput.input_file = filename;
			cinput.out_file = daemon_data.bin_directory + "/tmp.obj";
			cinput.dump_file = daemon_data.root_directory + "/" + kCompilerLogFile;

			if (!compiler::compile(name, cinput, metadata))
			{
				if (kWarnOnCompilerErrors)
					scriptosaurus_on_warning("Failed to compile: ", name.c_str());
				return;
			}

			compiler::LinkerInput linput;
			linput.obj_files.push_back(cinput.out_file);
			linput.out_file = out_name;

			if (!compiler::link(name, linput, metadata))
			{
				scriptosaurus_on_error("Failed to link: ", name.c_str());
				return;
			}

			// Here comes the tricky part, now in out_name we have our dll ready to be swapped in
			// The function pointers for this diff script are loaded at
			// out_name@0.dll we then load out_name@1.dll in the address space
			platform::Library library = platform::load(out_name.c_str());
			if (library == nullptr)
			{
				scriptosaurus_on_error("Failed to load library: ", out_name.c_str());
				auto err= GetLastError();
				return;
			}

			// Updating pointers
			platform::lock(daemon_data.scripts_lock);

			std::unordered_map<ScriptName, SwappableRoutine>& importables = (*daemon_data.scripts)[name].routines;

			// We need to set all pointers to nullptr just to make sure we don't 
			// load any reference
			for (auto& importable : importables)
			{
				void* routine_addr = importable.second.address;
				platform::Lock* lock_addr = importable.second.lock_address;

				void* new_routine_addr = platform::get_func_ptr<ScriptRoutine>(library, importable.first.c_str());
				if (new_routine_addr == nullptr)
				{
					// Log error:
					// SampleScript wants to import "foo" but "foo" was not found in the compiled library
					// We continue anyway because we need to set it to null, we are unloading previous library
				}

				platform::lock(lock_addr);

				// Writing new function pointer
				*((ScriptRoutine*)routine_addr) = (ScriptRoutine)new_routine_addr;
				
				platform::unlock(lock_addr);
			}

			// Time to free previous library ( if any )
			auto& script = (*daemon_data.scripts)[name];
			if (script.current_library != platform::kInvalidLibrary)
			{
				platform::unload(script.current_library);
			}
			// And setting new one
			script.current_library = library;

			platform::unlock(daemon_data.scripts_lock);
		}

		// Recursively looks for all the files that need to be diffed and feeds them
		// to process_diff
		inline void rec_build_diffs(DaemonData& daemon_data, UpdateMap& map)
		{
			platform::iter_dir(daemon_data.root_directory, [&](const std::string& dir, const std::string& filename)
			{
				std::string full_name = std::string(dir) + "/" + filename;

				ScriptName name = filename_to_name(full_name.c_str());

				// New file
				if (map.find(name) == map.end())
				{
					PerLoopData data;
					data.hash = std::hash<std::string>{}("");
					map[name] = data;
				}

				// Processing file if hash is different than current one
				ScriptHash hash = compute_hash(full_name);

				if (hash != map[name].hash)
					process_diff(daemon_data, name, full_name);

				// Tagged as seen this iteration & updating hash
				map[name].seen = 1;
				map[name].hash = hash;
			});
		}
	}

	/*
		Public API
	*/
	// Declares a new bindable routine
#if defined(scriptosaurus_live)
#	define scriptosaurus_routine(name) scriptosaurus::ScriptRoutine name = nullptr; scriptosaurus::platform::Lock _##name;
#else
#	define scriptosaurus_routine(name) scriptosaurus::ScriptRoutine name = nullptr;
#endif

	// Helper to add routines to daemon
#define scriptosaurus_prepare(obj, routine) { #routine, &obj.routine }

	// Calls a previously declared routine
#if defined(scriptosaurus_live)
#	define scriptosaurus_call(obj, fun, args) { scriptosaurus::platform::lock(&obj._##fun); if (obj.fun != nullptr) obj.fun(args); scriptosaurus::platform::unlock(&obj._##fun); }
#else
#	define scriptosaurus_call(obj, fun, args) obj.fun(args);
#endif

	/*
		Struct: Script
			Single importable routine
	*/
	struct Script
	{
		Script(const char* name) : name(name) { } 
		const ScriptName name;
	};

	/*
		Class: ScriptDaemon
			Handles all the registered reloadablescripts.
			Uses the same interface for live and one time loading.
			You add all the scripts via add(...) then call link_all().
			link_all() doesn't do anything if scriptosarusus_live is defined. 
	*/
	class ScriptDaemon final
	{
	public:
		/*
			Constructor: ScriptDaemon
				Default initialization
		*/
		ScriptDaemon(const char* root_dir);

		/*
			Destructor: ~ScriptDaemon
				Destroys and frees the library that has been created internally
				(either for each script or for all of them)
		*/
		~ScriptDaemon();

		/*
			Function: add
				Adds a new script with the specified importable routines.
				use scriptosaurus_prepare(obj, fun) to create a SwappableRoutineParam.
				Two scripts with the same name cannot exist.
		*/
		bool add(Script& script, std::initializer_list<internal::SwappableRoutineParam> swappables);

		/*
			Function: link_all
				If scriptosaurus_live then the daemon thread is launched.

				If scriptosaurus_live is not defined then the daemon thread is terminated
				immediatly and all the function pointers are loaded into the previously
				added scripts. Should be used in test/release mode.

				If you really want to move everything offline, just do a run 
				with link_all(nullptr) and a dyanmic library will be create in
				<root_dir>/.bin. 
				Next time you run in release just link_all(<library_path>) will load
				all the function pointers.
		*/
		bool link_all(const char* library_path = nullptr);

	private:
		void _daemon_routine();
		std::string m_root_dir;
		std::thread		  m_daemon;
		std::atomic<bool> m_to_terminate;
	
		// Map contention
		platform::Lock m_scripts_lock;
		internal::Map<ScriptName, internal::ReloadableScript> m_scripts;

		platform::Library m_global_library;
	};

	ScriptDaemon::ScriptDaemon(const char* root_dir) :
		m_root_dir(root_dir),
		m_global_library(platform::kInvalidLibrary)
	{
		if (root_dir == nullptr)
		{
			scriptosaurus_on_error("Cannot start Daemon on null directory");
			return;
		}

		platform::init(&m_scripts_lock);
	}

	ScriptDaemon::~ScriptDaemon()
	{
		m_to_terminate.store(true);
		m_daemon.join();

		platform::destroy(&m_scripts_lock);

		// Time to release all the libraries
#if defined(scriptosaurus_live)
		platform::lock(&m_scripts_lock);
		for (auto& kv : m_scripts)
		{
			platform::unload(kv.second.current_library);
		}
		m_scripts.clear();

		platform::unlock(&m_scripts_lock);
#else
		platform::unload(m_global_library);
#endif
	}

	inline bool ScriptDaemon::add(Script& script, std::initializer_list<internal::SwappableRoutineParam> swappables)
	{
		// Hooking could be potentially done from multiple Threads.
		platform::lock(&m_scripts_lock);
		if (m_scripts.find(script.name) != m_scripts.end())
		{
			scriptosaurus_on_error("Failed to add script! ", script.name.c_str(), " already added");
			platform::unlock(&m_scripts_lock);
			return false;
		}

		m_scripts[script.name].script = &script;
		auto& loadable_script = m_scripts[script.name];
		for (const auto& swappable : swappables)
		{
			// Overwriting if same name
			platform::Lock* lock_address = (platform::Lock*)(((char*)swappable.base_address) + sizeof(ScriptRoutine));

			m_scripts[script.name].routines[swappable.name].address = swappable.base_address;

			// Lock only enabled in live mode
#if defined(scriptosaurus_live)
			m_scripts[script.name].routines[swappable.name].lock_address = lock_address;
			platform::init(lock_address);
#endif

			scriptosaurus_on_info("Hooking: ", script.name.c_str(), "::", swappable.name);
		}
		platform::unlock(&m_scripts_lock);
		
		scriptosaurus_on_info("Successfully added: ", script.name.c_str());

		return true;
	}

	inline bool ScriptDaemon::link_all(const char* library_path)
	{
		if (library_path != nullptr)
			scriptosaurus_on_warning("Custom library loading / Cross compilation not yet implemented");

#if defined(scriptosaurus_live)
		scriptosaurus_on_info("Starting daemon at: ", m_root_dir.c_str());

		m_to_terminate.store(false);
		m_daemon = std::thread(&ScriptDaemon::_daemon_routine, this);
		return true;
#else
		if (m_scripts.empty())
		{
			scriptosaurus_on_info("Linked 0 scripts");
			return true;
		}

		// This is how compilation works for multiple files. All .cpp are compiled 
		// into different objs then merged into single dll. But names for lookups are different and 
		// are in the format <script_name><scriptosaurus_separator><routine_name>. 
		const char kScriptosaurusSeparator[] = "$";

		compiler::LinkerInput linput;
		for (const auto& kv : m_scripts)
		{
			std::string file_path = kv.first + kScriptExtension;
			std::replace(file_path.begin(), file_path.end(), kScriptDirSeparator, '/');

			// kScriptDirSeparator may not be accepted as function name (e.g. '@')
			std::string script_name = kv.first;
			std::replace(script_name.begin(), script_name.end(), kScriptDirSeparator, '_');
			std::string filename = m_root_dir + "/" + file_path;

			std::ifstream file_stream(filename);
			if (!file_stream.is_open())
			{
				scriptosaurus_on_error("Failed to open: ", filename.c_str());
				return false;
			}

			std::stringstream ss;
			ss << file_stream.rdbuf();
		
			// Building metadata
			internal::ScriptMetadata metadata;
			internal::extract_metadata(kv.first, ss.str().c_str(), metadata);
			metadata.defines.push_back({"scriptosaurus_script_name", "\"" + script_name + "\""});
			metadata.defines.push_back({ "scriptosaurus_separator", std::string("\"") + kScriptosaurusSeparator + "\"" });
		
			// Compiling to obj
			compiler::CompilerInput cinput;
			cinput.input_file = filename;
			cinput.out_file = kv.first + ".obj";

			if (!compiler::compile(kv.first, cinput, metadata))
			{
				scriptosaurus_on_error("Failed to compile: ", kv.first.c_str(), " see log for more info");
				return false;
			}

			linput.obj_files.push_back(cinput.out_file);
		}

		// Linking them all together
		linput.out_file = "scriptosaurus.dll";
		
		internal::ScriptMetadata metadata;

		if (!compiler::link("", linput, metadata))
		{
			scriptosaurus_on_error("Failed to link all object files: ", std::to_string(linput.obj_files.size()).c_str());
			return false;
		}
	
		m_global_library = platform::load(linput.out_file.c_str());
		if (m_global_library == platform::kInvalidLibrary)
		{
			scriptosaurus_on_error("Failed to load: ", linput.out_file.c_str());
			return false;
		}

		// Loading routines
		for (auto& kv : m_scripts)
		{
			for (auto& rkv : kv.second.routines)
			{
				internal::SwappableRoutine& routine = rkv.second;

				std::string script_name = kv.first;
				std::replace(script_name.begin(), script_name.end(), kScriptDirSeparator, '_');
				std::string mangled_proc_name = script_name + kScriptosaurusSeparator + rkv.first;

				*((ScriptRoutine*)routine.address) = platform::get_func_ptr<ScriptRoutine>(m_global_library, mangled_proc_name.c_str());
			}
		}
#endif
		return true;
	}

	inline void ScriptDaemon::_daemon_routine()
	{
		const std::string bin_dir = std::string(m_root_dir) + "/" + kBinDirName;

		// Creating bin directory if does not exist
		platform::create_directory(bin_dir.c_str());
	
		internal::UpdateMap update_map;
		internal::DaemonData daemon_data;
		daemon_data.root_directory = m_root_dir;
		daemon_data.bin_directory = bin_dir;
		daemon_data.scripts = &m_scripts;
		daemon_data.scripts_lock = &m_scripts_lock;

		while (true)
		{
			if (m_to_terminate)
			{
				scriptosaurus_on_info("Terminating daemon: ", m_root_dir.c_str());
				return;
			}

			// Tagging everything as unseen
			for (auto& kv : update_map)
				kv.second.seen = false;

			// Processing items
			internal::rec_build_diffs(daemon_data, update_map);

			// Removing non-seen names
			auto iter = update_map.begin();
			while (iter != update_map.end())
			{
				if (iter->second.seen == false)
					iter = update_map.erase(iter);
				else
					++iter;
			}

			std::this_thread::sleep_for(std::chrono::duration<decltype(kDaemonSleepTimeMs), std::milli>(kDaemonSleepTimeMs));
		}
	}
}