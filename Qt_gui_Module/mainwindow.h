#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "GuiFormData.h"

struct NormalizedConversionRequest;
struct BuildResult;
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

private:
    Ui::MainWindow *ui;
    QString selectedInputPath_;
    QString selectedOutputPath_;
    InputTargetType inputTargetType_ = InputTargetType::None;

private:
    void debugPrintRequest(const NormalizedConversionRequest& req);
    void updateOutputDisplay();
    void appendLog(const QString& text);
    void appendLogToBuildResult(const BuildResult& buildresult);

};
#endif // MAINWINDOW_H
