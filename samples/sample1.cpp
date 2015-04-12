#include "script_manager.hpp"
#include <iostream>

const char* g_daemon_path { "path_to_daemon"};
const char* g_scripts_path { "path_to_scripts"};
const char* g_script_name{ "script" };
const unsigned int g_daemon_port { 6661 };

class StdOutput : public IOutput
{
public:
    void log_info(const char* message)override
    {
        std::cout << message << std::endl;
    }

    void log_warning(const char* message)override
    {
        std::cout << message << std::endl;
    }

    void log_error(const char* message)override
    {
        std::cout << message << std::endl;
    }
};

int main()
{
    StdOutput output;

    ScriptManager script_manager;
    script_manager.set_output_channel(output);
    if (!script_manager.run_daemon(g_daemon_path, g_scripts_path, ScriptManager::Mode::Test, g_daemon_port))
		while (true) {}

    Script sample_script(g_script_name, script_manager);

    sample_script.init(nullptr);

    auto quit = false;
    while (!quit)
    {
        script_manager.update();
        sample_script.update(nullptr);
    }

    sample_script.destroy(nullptr);

	while (true) { } 
}
