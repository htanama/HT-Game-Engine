#pragma once

enum class EditorState { Editor, Playing };
extern EditorState gameState;

class Engine{
public:
    static bool isRunning;

    static void SetIsRunning(bool state){
        isRunning = state;
    }
};
