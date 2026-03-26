#ifndef GUIREQUESTTRANSLATOR_H
#define GUIREQUESTTRANSLATOR_H

#endif // GUIREQUESTTRANSLATOR_H
#pragma once
#include "Task_Module/NormalizedConversionRequest.h"
#include <string>

struct guirequest{
    bool ok = false;
    std::wstring  message;
    NormalizedConversionRequest request;
};

class GuiRequestTranslator{
public:

private:

};
