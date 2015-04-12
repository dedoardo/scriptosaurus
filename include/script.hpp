/*
    File: script.hpp
    Author: Edoardo 'sparkon' Dominici
    License: Mozilla Public License 2.0
*/

#pragma once

// C++ STD
#include <cstdlib>
#include <type_traits>

// Forward declarations
class ScriptManager;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4512)
#endif

/*
    Class: Script
        Represents a single C++ script to be compiled and executed
*/
class Script
{
    friend class ScriptManager;
public :
    using init_callback     = std::add_pointer<bool(void*)>::type;
    using update_callback   = std::add_pointer<void(void*)>::type;
    using destroy_callback  = std::add_pointer<void(void*)>::type;

    const static size_t max_name_length { 62 };

public :
    /*
        Constructor: Script
            Creates a new script instance linked to the ScriptManager, that is the one that will re-load
            and update the script
    */
    Script(const char name[], ScriptManager& script_manager);

    /*
        Destructor: ~Script
            Destroys te script and removes from memory all its resources
    */
    ~Script();

    /*
        Function: init
            This is the first of the three methods that are implemented inside the script

        Parameters:
            data - Pointer to custom data
    */
    bool init(void* data);

    /*
        Function: update
            Calls the script's update method, if it has been compiled correctly

        Parameters:
            data - Pointer to custom data
    */
    void update(void* data);

    /*
        Function: destroy
            Calls the script's destroy method, if it has been compiled correctly

        Parameters:
            data - Pointer to custom data
    */
    void destroy(void* data);

    /*
        Function: get_name
            Returns the name of the script, this is the same of the DLL name that is loaded with it
    */
    const char* get_name()const { return m_name; }

protected :
    char                m_name[max_name_length];
    ScriptManager&      m_script_manager;

    init_callback       m_init;
    update_callback     m_update;
    destroy_callback    m_destroy;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif