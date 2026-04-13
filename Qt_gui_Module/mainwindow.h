// =====================================================
// Module  : mainwindow
// Author  : suku156
// Purpose : 負責gui運行的主要模組
// Layer   : gui
//
// Depend  :
//  GuiRequestTranslator
//  GuiFormData
//
// Used by :
//   main
//
// Notes :
//   gui 中主要負責執行視窗相關所有指令的模組
// =====================================================

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "GuiFormData.h"
#include "GuiObserver.h"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <optional>
#include "Task_Module/NormalizedConversionRequest.h"
#include "Task_Module/ConversionTask.h"

namespace App {
enum class AppExitCode : int;
}

struct ConvertRunResult {
    App::AppExitCode exitCode;
};

enum class InputTargetType{
   None,
   File,
   Directory
};

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    GuiFormData collectFormData() const;


private slots:
    void on_btnSelectInputFile_clicked();
    void on_btnSelectInputDir_clicked();
    void on_btnSelectOutput_clicked();
    void on_btnConvert_clicked();
    void on_btnCleanOutput_clicked();
    void on_btnCleanLog_clicked();
    void observerAppendLog(QString text);
    void observerUpdateProgressBar(int done,int total);
    void onConvertFinished();

private:
    Ui::MainWindow *ui;
    QString selectedInputPath_;
    QString selectedOutputPath_;
    InputTargetType inputTargetType_ = InputTargetType::None;
    GuiObserver* guiObserver_ =  nullptr;
    QFutureWatcher<ConvertRunResult>* convertWatcher_ = nullptr;
    std::optional<BuildResult> lastBuildResult_;
    std::optional<NormalizedConversionRequest> lastReq_;


private:
    void debugPrintRequest(const NormalizedConversionRequest& req);
    void updateOutputDisplay();
    void appendLogToBuildResult(const BuildResult& buildresult);
    void appendLog(const QString& text);


};
#endif // MAINWINDOW_H
