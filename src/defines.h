#pragma once

// version define, only changes when a new release is put out
#define VERSION 0.1

// logging macros for faster development, interchangeability and readability
#define LOG(logline, module) Logger::getInstance().log(##logline, Logger::facility::info, ##module)
#define LOG_WARNING(logline, module) Logger::getInstance().log(##logline, Logger::facility::warning, ##module)
#define LOG_DEBUG(logline, module) Logger::getInstance().log(##logline, Logger::facility::debug, ##module)
#define LOG_ERROR(logline, module) Logger::getInstance().log(##logline, Logger::facility::error, ##module)