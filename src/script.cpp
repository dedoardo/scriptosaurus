/*
    File: script.cpp
*/

// Header
#include "script.hpp"
#include "script_manager.hpp"

// C++ STD
#include <cassert>
#include <cstring>
#include <algorithm>

Script::Script(const char name[], ScriptManager& script_manager) :
    m_script_manager { script_manager },

    m_init { nullptr },
    m_update { nullptr },
    m_destroy { nullptr }
{
    assert(name != nullptr);
    assert(strlen(name) < max_name_length);
    std::memcpy(m_name, name, strlen(name) + 1);
    std::replace(&m_name[0], &m_name[0] + strlen(m_name), ':', '_');
}

Script::~Script()
{
    m_init = nullptr;
    m_update = nullptr;
    m_destroy = nullptr;
}

bool Script::init(void* data)
{
    if (m_script_manager.get_mode() != ScriptManager::Mode::Release || m_init == nullptr)
        m_script_manager.query_update(*this);

    if (m_init != nullptr)
        return m_init(data);
    return false;
}

void Script::update(void* data)
{
    if (m_script_manager.get_mode() != ScriptManager::Mode::Release || m_init == nullptr)
        m_script_manager.query_update(*this);
    if (m_update != nullptr)
        m_update(data);
}

void Script::destroy(void* data)
{
    if (m_script_manager.get_mode() != ScriptManager::Mode::Release || m_init == nullptr)
        m_script_manager.query_update(*this);
    if (m_destroy != nullptr)
        m_destroy(data);
}