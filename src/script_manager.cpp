/*
    File: script_manager.cpp
*/

// Header
#include "script_manager.hpp"

// C++ STD
#undef __in
#undef __out
#include <cstring>
#include <iostream> // TODO REMOVE

ScriptManager::ScriptManager() :
    m_termination_received { false },
    m_mode { Mode::Test },
    m_output_channel { nullptr },

    m_winsock_initialized { false },
    m_update_socket { INVALID_SOCKET },
    m_stream_socket { INVALID_SOCKET },

    m_notification_buffer_read { 0 },
    m_stream_buffer_read { 0 },
    m_stream_buffer_expected { 0 },
    m_header_read { 0 },
    m_header_buffer { 0 },

    m_daemon_process { nullptr },
    m_daemon_thread { nullptr }
{

}

ScriptManager::~ScriptManager()
{
    stop();
}

bool ScriptManager::initialize_connection()
{
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
        return false;

	return true;
}

bool ScriptManager::connect_update_server(const char* address, unsigned int port)
{
    // Creating main socket
    m_update_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_update_socket == INVALID_SOCKET)
    {
        stop();
        return false;
    }

    // Setting socket mode to non-blocking
    u_long mode = 1;

    // By default it works on localhost, but can be easility configured to run in remote
	// i feel bad to use gethostbyname instead of getaddrinfo, but minimum Windows version required is 8.1
    struct hostent* host = gethostbyname(address);

    if (host == nullptr)
    {
        stop();
        return false;
    }

    SOCKADDR_IN sockaddr_in;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(static_cast<unsigned short>(port));
    sockaddr_in.sin_addr.s_addr = *((unsigned long*)host->h_addr);

    if (connect(m_update_socket, (SOCKADDR*)(&sockaddr_in), sizeof(sockaddr_in)) != 0)
    {
        stop();
        return false;
    }

    ioctlsocket(m_update_socket, FIONBIO, &mode);

    return true;
}

bool ScriptManager::connect_stream_server(const char* address, unsigned int port)
{
    // Creating main socket
    m_stream_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_stream_socket == INVALID_SOCKET)
    {
        stop();
        return false;
    }

    // Setting socket mode to non-blocking
    u_long mode = 1;

    // By default it works on localhost, but can be easility configured to run in remote
    struct hostent* host = gethostbyname(address);
    if (host == nullptr)
    {
        stop();
        return false;
    }

    SOCKADDR_IN sockaddr_in;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(static_cast<unsigned short>(port));
    sockaddr_in.sin_addr.s_addr = *((unsigned long*)host->h_addr);

    if (connect(m_stream_socket, (SOCKADDR*)(&sockaddr_in), sizeof(sockaddr_in)) != 0)
    {
        stop();
        return false;
    }

    ioctlsocket(m_stream_socket, FIONBIO, &mode);

    return true;
}

bool ScriptManager::run_daemon(const char* daemon_directory, const char* root_directory, Mode mode, unsigned int port)
{
    // Starting daemon process
    STARTUPINFO startup_info = { sizeof(startup_info) };
    PROCESS_INFORMATION process_information;

    std::stringstream ss_command;
    ss_command << "C:/Python34/python ";            // python
    ss_command << daemon_directory << "/daemon.py ";  // daemon.py
    if (mode == Mode::Test)
        ss_command << "Test ";
    else if (mode == Mode::Release)
        ss_command << "Release ";
    else
        return false;

    ss_command << root_directory << " ";
    ss_command << std::to_string(port) << " ";
    ss_command << std::to_string(port) << " ";

    auto command = ss_command.str();
    // This api is bullshit
	auto result = CreateProcess(nullptr, const_cast<char*>(command.c_str()), nullptr, nullptr,
		false, 0, nullptr, nullptr, &startup_info, &process_information);
    if (result ==  FALSE)
    {
		std::cout << result << std::endl;
		report_error("Failed to create daemon process with error code :", GetLastError());
        return false;
    }

    initialize_connection();

    if (!connect_update_server("localhost", port))
    {
		report_error("Failed to connect to update server at localhost : ", port);
        stop();
        return false;
    }

	if (!connect_stream_server("localhost", port + 1))
	{
		report_error("Failed to connect to stream server at localhost : ", port);
        stop();
        return false;
    }

    // Updating bin directory
    m_bin_directory = root_directory;
    m_bin_directory.append("\\.bin");

    if (mode == Mode::Release)
    {
        m_mode = Mode::Release;
        while (!m_termination_received)
        {
            update();
        }
        WaitForSingleObject(m_daemon_thread, INFINITE);
        WaitForSingleObject(m_daemon_process, INFINITE);
        stop();

		report_info("Finished loading scripts");
    }

    return true;
}

void ScriptManager::stop()
{
    //! Terminating daemon process if started
    if (m_daemon_process != nullptr)
    {
        CloseHandle(m_daemon_process);
        m_daemon_process = nullptr;
    }

    if (m_daemon_thread != nullptr)
    {
        CloseHandle(m_daemon_thread);
        m_daemon_thread = nullptr;
    }

    if (m_update_socket != INVALID_SOCKET)
    {
        shutdown(m_update_socket, SD_BOTH);
        closesocket(m_update_socket);
		m_update_socket = INVALID_SOCKET;
    }

    if (m_stream_socket != INVALID_SOCKET)
    {
        shutdown(m_stream_socket, SD_BOTH);
        closesocket(m_stream_socket);
		m_stream_socket = INVALID_SOCKET;
    }

    if (m_winsock_initialized)
    {
        WSACleanup();
        m_winsock_initialized = false;
    }
}

void ScriptManager::update()
{
    if (m_update_socket != INVALID_SOCKET)
    {
        int result = recv(m_update_socket, reinterpret_cast<char*>(&m_notification_buffer) + m_notification_buffer_read, static_cast<int>(sizeof(UpdateNotification) - m_notification_buffer_read), 0);
        if (result == 0)
        {
			report_error("Update server disconnected, please restart it and restart client again");
			stop();
			return;
        }
        else if (result > 0)
        {
            // Notification read successfully
            m_notification_buffer_read += result;
            if (m_notification_buffer_read >= sizeof(UpdateNotification))
            {
                m_notification_buffer.option = static_cast<UpdateNotification::Option>(ntohs(static_cast<short>(m_notification_buffer.option)));
                update_script();
                m_notification_buffer_read = 0;
            }
        }
		auto error_code = WSAGetLastError();

		if (result == SOCKET_ERROR && error_code != WSAEWOULDBLOCK)
        {
			report_error("Error receiving data from update server with code[", error_code, "]. Please restart it and restart client again");
			stop();
			return;
		}
    }
    if (m_stream_socket != INVALID_SOCKET)
    {
        // It means that we finished processing the previous message
        if (m_stream_buffer_expected == 0)
        {
            int result = recv(m_stream_socket, reinterpret_cast<char*>(&m_header_buffer) + m_header_read, sizeof(m_header_buffer) - m_header_read, 0);
            if (result == 0)
            {
				report_error("Stream server disconnected, please restart it and restart client again");
				stop();
				return;
            }
            else if (result > 0)
            {
                m_header_read += result;

                // Notification read successfully
                if (m_header_read >= static_cast<int>(sizeof(m_header_buffer)))
                {
                    m_stream_buffer_expected = m_header_buffer;
                    m_header_read = 0;
                }
            }

			auto error_code = WSAGetLastError();

			if (result == SOCKET_ERROR && error_code != WSAEWOULDBLOCK)
            {
				report_error("Error receiving data from stream server with code[", error_code, "]. Please restart it and restart client again");
				stop();
				return;
            }
        }
        else
        {
            int result = recv(m_stream_socket, &m_stream_buffer[0] + m_stream_buffer_read, m_stream_buffer_expected - m_stream_buffer_read, 0);
            if (result == 0)
            {
				report_error("Stream server disconnected, please restart it and restart client again");
				stop();
				return;
            }
            else if (result > 0)
            {
                m_stream_buffer_read += result;

                // Notification read successfully
                if (m_stream_buffer_read >= m_stream_buffer_expected)
                {
                    log();
                    std::memset(&m_stream_buffer[0], 0, max_message_size * sizeof(m_stream_buffer[0]));
                    m_stream_buffer_expected = 0;
                    m_stream_buffer_read = 0;
                }
            }
			auto error_code = WSAGetLastError();

			if (result == SOCKET_ERROR && error_code != WSAEWOULDBLOCK)
            {
				report_error("Error receiving data from stream server with code[", error_code, "]. Please restart it and restart client again");
				stop();
				return;
            }
        }
    }
}

void ScriptManager::load_library(const std::string& dll_name, ScriptData& script_data)
{
    script_data.dll_handle = LoadLibraryA(dll_name.c_str());
    if (script_data.dll_handle == nullptr)
    {
		report_error("Failed to load DLL \"", dll_name, "\", with error : ", GetLastError());
		return;
	}

    // Retrieving function pointers
    script_data.init = reinterpret_cast<Script::init_callback>(GetProcAddress(script_data.dll_handle, "init"));
    if (script_data.init == nullptr)
		report_warning("Failed to get \"init\" procedure, with error : ", GetLastError());

    script_data.update = reinterpret_cast<Script::update_callback>(GetProcAddress(script_data.dll_handle, "update"));
    if (script_data.update == nullptr)
		report_warning("Failed to get \"update\" procedure, with error : ", GetLastError());

    script_data.destroy = reinterpret_cast<Script::destroy_callback>(GetProcAddress(script_data.dll_handle, "destroy"));
    if (script_data.destroy == nullptr)
		report_warning("Failed to get \"destroy\" procedure, with error : ", GetLastError());

	report_info("Successfully loaded \"", dll_name, "\"");
}

void ScriptManager::update_script()
{
    // We know string operations are slow, but we are running in debug and this method is called sometimes,
    // not that often, optimization here would be ridicolous
    std::string dll_name = m_bin_directory;
    dll_name.append("\\");
    dll_name.append(m_notification_buffer.name);\

    // If in release mode and we finished processing the bunch of files
    if (m_mode == Mode::Release && static_cast<short>(m_notification_buffer.option) & static_cast<short>(UpdateNotification::Option::LastInQueue))
    {
        m_termination_received = true;
    }

    std::string dll_name_tmp = dll_name;
    dll_name.append(".dll");
    dll_name_tmp.append("_tmp.dll");

    ScriptData updated_script {
            nullptr,
            nullptr,
            nullptr,
            nullptr
    };

    auto ret = m_scripts.find(m_notification_buffer.name);
    if (ret != m_scripts.end())
    {
        // Freeing library from user address space
        FreeLibrary(ret->second.dll_handle);
    }

    if (!CopyFile(dll_name_tmp.c_str(), dll_name.c_str(), false))
    {
		report_error("Failed to copy dll file to tmp location with error : ", GetLastError());
        return;
    }

    load_library(dll_name, updated_script);
    m_scripts[std::string(m_notification_buffer.name)] = updated_script;
}

void ScriptManager::query_update(Script& script)
{
    auto ret = m_scripts.find(std::string(script.m_name));
    if (ret != m_scripts.end())
    {
        script.m_init = ret->second.init;
        script.m_update = ret->second.update;
        script.m_destroy = ret->second.destroy;
    }
    else
    {
        script.m_init = nullptr;
        script.m_update = nullptr;
        script.m_destroy = nullptr;
    }
}

void ScriptManager::log()
{
    if (m_output_channel != nullptr)
    {
        // The message is expected to have this format
        // [D][Info|Error|Warning]Message
        // We are cheap with controls and we are just going to check if the 4th
        // character is an I or E or W
        const char leading_char = m_stream_buffer[4];
        switch(leading_char)
        {
            case 'I':
                m_output_channel->log_info(&m_stream_buffer[0]);
                break;
            case 'W':
                m_output_channel->log_warning(&m_stream_buffer[0]);
                break;
            case 'E':
                m_output_channel->log_error(&m_stream_buffer[0]);
                break;
        }
    }
}