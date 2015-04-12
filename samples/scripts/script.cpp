#include <iostream>
#include "scriptosaurus.hpp"

bool init(void* data)
{
    std::cout << "init called" << std::endl;
    return true;
}

void update(void* data)
{
    static int count { 0 };
    int ciao = 99999;
    if (count >= 100000)
    {
        std::cout << "Hello world24" << std::endl;
        count = 0;
    }
    ++count;
}

void destroy(void* data)
{
    std::cout << "destroy called" << std::endl;
}