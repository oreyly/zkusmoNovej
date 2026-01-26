#include "Logger.h"

#include "Utils.h"

#include <ctime>
#include <iomanip>
#include <filesystem>

std::ofstream Logger::_logFile;

const std::unordered_map<LOG_LEVEL, std::string> Logger::logLevelMessages = {
    { LOG_LEVEL::INFO, "INFO" },
    { LOG_LEVEL::ERROR, "ERROR" },
    { LOG_LEVEL::WARNING, "WARNING" },
};

bool Logger::init(std::string logFilePath)
{
    if (_initialized.load(std::memory_order_acquire))
    {
        return false;
    }

    try
    {
        _logFile.open(logFilePath, std::ios_base::out | std::ios_base::app);

        if (!_logFile.is_open())
        {
            throw std::runtime_error(Errors::GetErrorMessage(ERROR_CODES::CANNOT_CREATE_LOG_FILE));
        }
    }
    catch (const std::exception& e)
    {
        std::string msg = Errors::GetErrorMessage(ERROR_CODES::CANNOT_CREATE_LOG_FILE);
        msg += "\nChyba: ";
        msg += e.what();
        perror(msg.c_str());
        return false;
    }

    _running.store(true, std::memory_order_release);
    _initialized.store(true, std::memory_order_release);

    _worker = std::thread(&Logger::LogLoop);

    _logFile << std::string(10,'=') + "Zacatek relace" + std::string(10, '=') << std::endl;

    std::string initMessage = "Logger uspesne inicializovan. vsechny logy lze nalezt v souboru: ";
    initMessage += std::filesystem::absolute(std::filesystem::path(logFilePath));

    Logger::LogMessage<Logger>(initMessage);
    return true;
}

void Logger::terminate()
{
    if (!_initialized.load(std::memory_order_acquire))
    {
        return;
    }

    _initialized.store(false, std::memory_order_release);
    _running.store(false, std::memory_order_release);

    if (_worker.joinable())
    {
        _worker.join();
    }

    Logger::LogMessage<Logger>("Konec logovani­");
    _logFile << std::string(10, '=') + "Konec relace" + std::string(10, '=') << std::endl;
    _logFile.close();
}

void Logger::AddToQueue(std::string caller, std::string message, LOG_LEVEL logLevel)
{
    if (!_running.load(std::memory_order_acquire))
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(_queueMutex);

        LogEntry entry {std::chrono::system_clock::now(), std::move(caller), std::move(message), logLevel};
        _messagesToWrite.push(entry);
    }

    _cvQueue.notify_one();
}

void Logger::LogLoop()
{
    while (_running.load(std::memory_order_acquire))
    {
        LogEntry logEntry;
        {
            std::unique_lock<std::mutex> lock(_queueMutex);

            _cvQueue.wait(lock, [] ()
                {
                    return !_messagesToWrite.empty() || !_running.load(std::memory_order_acquire);
                });

            if (!_running.load(std::memory_order_acquire))
            {
                break;
            }

            logEntry = std::move(_messagesToWrite.front());
            _messagesToWrite.pop();
        }

        Log(logEntry);
    }

    while (true)
    {
        LogEntry logEntry;

        {
            std::lock_guard<std::mutex> lock(_queueMutex);

            if (_messagesToWrite.empty())
            {
                return;
            }

            logEntry = std::move(_messagesToWrite.front());
            _messagesToWrite.pop();
        }

        Log(logEntry);
    }
}

void Logger::Log(LogEntry logEntry)
{
    if (!_initialized.load(std::memory_order_acquire)) return;

    std::time_t tt = std::chrono::system_clock::to_time_t(logEntry.Timestamp);
    std::tm time;
#ifdef _WIN32
    localtime_s(&time, &tt);
#else
    localtime_r(&tt, &time);
#endif

    _logFile << "[" << logLevelMessages.at(logEntry.LogLevel) << "] ["
        << std::setfill('0') << std::setw(2) << time.tm_hour << ":"
        << std::setfill('0') << std::setw(2) << time.tm_min << ":"
        << std::setfill('0') << std::setw(2) << time.tm_sec
        << "] [" << Utils::Demangle(logEntry.Caller) << "] " << logEntry.Message << std::endl;
}
