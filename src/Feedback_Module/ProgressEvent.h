#pragma once
#include <string>


enum class ProgressEventType {
    Info,
    Start,
    Finish
};

struct ProgressEvent {
    ProgressEventType type = ProgressEventType::Info;
    std::wstring message;
};