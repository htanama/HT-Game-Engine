#pragma once

class Engine{
public:
    static bool isRunning;

    static void SetIsRunning(bool state){
        isRunning = state;
    }
};