/*
    File: script_manager.hpp
*/

#pragma once

// Script
#include "script.hpp"
#include "ioutput.hpp"

// C++
#undef __in
#undef __out
#include <unordered_map>
#include <sstream>
#include <string>

// Windows
#include <winsock2.h>
#include <windows.h>
#undef __in
#undef __out

/*
    Class: ScriptManager
        This is the class the manages all the scripts by loading, receiving update requests by
        the daemon ( running in a separate thread ).
*/
class ScriptManager
{
public :
    static const unsigned int default_port { 6666 };
    static const size_t       max_message_size { 256 };

    enum class Mode
    {
        Release,
        Test
    };

public :
    /*
        Constructor: ScriptManager
            Builds a new ScriptManager instance initializing all data structures
    */
    ScriptManager();

    /*
        Destructor: ~ScriptManager
            Destroys the ScriptManager terminating the daemon process
    */
    ~ScriptManager();

    void set_output_channel(IOutput& output_channel) { m_output_channel = &output_channel; }

    IOutput const* get_output_channel()const { return m_output_channel; }

    Mode get_mode()const { return m_mode; }

    bool initialize_connection();
    bool connect_update_server(const char* address, unsigned int port);
    bool connect_stream_server(const char* address, unsigned int port);
    bool run_daemon(const char* daemon_directory, const char* root_directory, Mode mode, unsigned int port);

    void stop();

    /*
        Function: update
            Runs the scriptmanager

        Note:
            This does *not* call the update() method on all the scripts
    */
    void update();

    // Retard compiler is giving me error when using register, i know it's a reserved keyword, but it should work anyway
    void query_update(Script& script);

protected :
    void log();
	
	template <typename ...values>
	void report_error(values&& ...codes);

	template <typename ...values>
	void report_warning(values&& ...codes);

	template <typename ...values>
	void report_info(values&& ...codes);

    bool     m_termination_received;
    Mode     m_mode;

    IOutput* m_output_channel;

    bool    m_winsock_initialized;
    SOCKET  m_update_socket;
    SOCKET  m_stream_socket;

    struct UpdateNotification
    {
        enum class Option: unsigned short
        {
            Default = 0,
            ForceInit = 1,
            LastInQueue = 2,
        };

        char     name[Script::max_name_length];
        Option   option;
    };

    size_t             m_notification_buffer_read;
    UpdateNotification m_notification_buffer;

    int	   m_stream_buffer_read;
    int    m_stream_buffer_expected;
    int    m_header_read;
    int    m_header_buffer;
    char   m_stream_buffer[max_message_size];

    void* m_daemon_process;
    void* m_daemon_thread;

    std::string m_bin_directory;

    struct ScriptData
    {
        Script::init_callback       init;
        Script::update_callback     update;
        Script::destroy_callback    destroy;
        HMODULE                     dll_handle;
    };

    std::unordered_map<std::string, ScriptData> m_scripts;

private :
    void load_library(const std::string& dll_name, ScriptData& script_data);
    void update_script();
};

template <typename value>
std::ostream& format(std::ostream& stream, value&& string)
{
	return stream << string;
}

template <typename value, typename ...values>
std::ostream& format(std::ostream& stream, value&& first, values&& ...strings)
{
	stream << first;
	return format(stream, std::forward<values>(strings)...);
}

template <typename ...values>
void ScriptManager::report_error(values&& ...codes)
{
	if (m_output_channel == nullptr)
		return;

	std::stringstream message_stream;
	format(message_stream, "[C][Error]", std::forward<values>(codes)...);
	m_output_channel->log_error(message_stream.str().c_str());
}

template <typename ...values>
void ScriptManager::report_warning(values&& ...codes)
{
	if (m_output_channel == nullptr)
		return;

	std::stringstream message_stream;
	format(message_stream, "[C][Warning]", std::forward<values>(codes)...);
	m_output_channel->log_warning(message_stream.str().c_str());
}

template <typename ...values>
void ScriptManager::report_info(values&& ...codes)
{
	if (m_output_channel == nullptr)
		return;

	std::stringstream message_stream;
	format(message_stream, "[C][Info]", std::forward<values>(codes)...);
	m_output_channel->log_info(message_stream.str().c_str());
}