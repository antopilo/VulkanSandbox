#include "Logger.h"

#include <iostream>

void Logger::Log(const std::string& log)
{
	std::cout << log << std::endl;
}