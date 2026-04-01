// =====================================================
//  gui 線得到的會用到的資訊打包而成的結構
// =====================================================
#ifndef GUIFORMDATA_H
#define GUIFORMDATA_H

#endif // GUIFORMDATA_H
#pragma once
#include <QString>

struct GuiFormData{
    QString inputPath;
    QString outputDir;
    QString formatText;
    QString dirPolicy;
    QString threadtext;
    QString preserveRelativeStructure;
    bool recursive;
};
