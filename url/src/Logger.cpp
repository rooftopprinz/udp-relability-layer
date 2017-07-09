#include "Logger.hpp"

namespace urlsock
{

LoggerServer loggerServer;

namespace color {
    enum Code {
        FG_RED      = 31,
        FG_GREEN    = 32,
        FG_YELLOW   = 33,
        FG_BLUE     = 34,
        FG_DEFAULT  = 39,
        BG_RED      = 41,
        BG_GREEN    = 42,
        BG_BLUE     = 44,
        BG_DEFAULT  = 49,
        DEFAULT  = 0
    };
    class Modifier {
        Code code;
    public:
        Modifier(Code pCode) : code(pCode) {}
        friend std::ostream&
        operator<<(std::ostream& os, const Modifier& mod)
        {
            return os << "\033[" << mod.code << "m";
            // return os;
        }
    };
}


void LoggerServer::log(LogEntry logEntry)
{
    std::lock_guard<std::mutex> guard(toBeLoggedMutex);
    toBeLogged.push_back(logEntry);
    toBeLoggedCv.notify_one();
}

void LoggerServer::logProcessor()
{
    logProcessorRunning = true;
    std::cout << "LogProcessor Running!!" << std::endl;
    color::Modifier red(color::FG_RED);
    color::Modifier yellow(color::FG_YELLOW);
    color::Modifier green(color::FG_GREEN);
    color::Modifier def(color::DEFAULT);

    while (!killLogProcessor)
    {
        std::unique_lock<std::mutex> lock(toBeLoggedMutex);
        toBeLoggedCv.wait(lock, [this](){
            return this->toBeLogged.begin() != this->toBeLogged.end() || killLogProcessor;
        });

        if (toBeLogged.begin()!=toBeLogged.end())
        {
            auto& l = *(toBeLogged.begin());
            auto us = std::chrono::duration_cast<std::chrono::microseconds>
                (l.time.time_since_epoch());

            std::cout << def;
            std::cout << std::dec << us.count()%(86164000000ul) << "us ";
            
            std::cout << color::Modifier(static_cast<color::Code>((l.threadId&0x0F)+30));
            std::cout << "t" << std::dec << (l.threadId&0xFFF);
            std::cout << def;
            std::cout << " ";

            if (l.level == LOG_DEBUG)
            {
                std::cout << "DBG ";
                std::cout << l.name << " ";
                std::cout << green;
            }
            else if (l.level == LOG_INFO)
            {
                std::cout << "INF ";
                std::cout << l.name << " ";
                std::cout << green;
            }
            else if (l.level == LOG_WARNING)
            {
                std::cout << "WRN ";
                std::cout << l.name << " ";
                std::cout << yellow;
            }
            else if (l.level == LOG_ERROR)
            {
                std::cout << "ERR ";
                std::cout << l.name << " ";
                std::cout << red;
            }

            std::cout << l.text << std::endl;
            std::cout << def;
            toBeLogged.pop_front();
        }
    }
    std::cout << "LogProcessor will now end!!" << std::endl;
    logProcessorRunning = false;
}


void LoggerServer::waitEmpty()
{
    while(toBeLogged.begin()!=toBeLogged.end())
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1ms);
    }
}

std::string Logger::getName()
{
    return name;
}

LoggerStream operator << (Logger logger, ELogLevel logLevel)
{
    return LoggerStream(logger, logLevel);
}

} // namespace urlsock
