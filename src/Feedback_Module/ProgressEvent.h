#pragma once
#include <string>


enum class ProgressEventType {
    Info,
    Error,
    Warning,
    Start,
    Finish
};

struct ProgressEvent {
    ProgressEventType type = ProgressEventType::Info;
    std::wstring message;
};