#pragma once

#include "LogLevel.h"
#include "Errors.h"

#include <fstream>
#include <string>
#include <unordered_map>
#include <iostream>
#include <typeinfo>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>


class Logger
{
public:
    Logger() = delete;

    static bool init(std::string logFilePath);
    static void terminate();

    template <typename T>
    static void LogError(ERROR_CODES errorCode, LOG_LEVEL logLevel = LOG_LEVEL::ERROR, bool sendToCerr = true)
    {
        std::string errorMessage = Errors::GetErrorMessage(errorCode);

        if (sendToCerr)
        {
            std::cerr << errorMessage << std::endl;
        }

        AddToQueue(typeid(T).name(), errorMessage, logLevel);
    }

    template <typename T>
    static void LogMessage(std::string message, LOG_LEVEL logLevel = LOG_LEVEL::INFO, bool sendToCout = true)
    {
        if (sendToCout)
        {
            std::cout << message << std::endl;
        }

        AddToQueue(typeid(T).name(), message, logLevel);
    }

private:
    struct LogEntry
    {
        std::chrono::system_clock::time_point Timestamp;
        std::string Caller;
        std::string Message;
        LOG_LEVEL LogLevel;
    };

    inline static std::atomic<bool> _running;
    inline static std::atomic<bool> _initialized = false;
    
    static std::ofstream _logFile;
    static const std::unordered_map<LOG_LEVEL, std::string> logLevelMessages;

    inline static std::mutex _queueMutex;
    inline static std::condition_variable _cvQueue;
    inline static std::queue<LogEntry> _messagesToWrite;
    inline static std::thread _worker;

    static void AddToQueue(std::string caller, std::string message, LOG_LEVEL logLevel);
    static void LogLoop();
    static void Log(LogEntry logEntry);

};
