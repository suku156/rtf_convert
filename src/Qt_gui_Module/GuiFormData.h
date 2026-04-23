// =====================================================
//  gui 線得到的會用到的資訊打包而成的結構
// =====================================================
#ifndef GUIFORMDATA_H
#define GUIFORMDATA_H

#endif // GUIFORMDATA_H
#pragma once
#include <QString>
#include <optional>
#include <cstddef>

struct GuiFormData{
    QString inputPath;
    QString outputDir;
    QString formatText;
    QString dirPolicy;
    std::optional<size_t> threadnum;
    QString preserveRelativeStructure;
    bool recursive;
};
