#pragma once
#include <vector>
#include <string>

class Logger {
private:
    static std::vector<std::string> logMessages;

public:
    static void Log(const std::string& message){
        logMessages.push_back(message);

        // keep only the last 100 messages to save memory
        if(logMessages.size() > 100)
            logMessages.erase(logMessages.begin());
    }   

    static const std::vector<std::string>& GetLogMessages(){
        return logMessages;
    }

};

std::vector<std::string> Logger::logMessages = {};