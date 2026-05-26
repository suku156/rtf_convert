#pragma once
#include <string>
#include <cstddef>


enum class ProgressEventType {
    Info,
    Error,
    Warning,
    Fail,
    Start,
    Finish,
    BatchStart,
    BatchFinish,
    UnitDone,
    Detail
};

struct ProgressEvent {
    ProgressEventType type = ProgressEventType::Info;
    std::wstring message;
    size_t done  = 0;
    size_t total = 0;
};