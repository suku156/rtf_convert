#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GuiFormData.h"
#include "GuiRequestTranslator.h"
#include "Executor_Module/ConversionExecutor.h"
#include "Task_Module/ConversionTaskBuilder.h"
#include "Task_Module/ConversionTask.h"
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

GuiFormData MainWindow::collectFormData() const{
    GuiFormData form;
    form.inputPath = selectedInputPath_;
    form.outputDir = selectedOutputPath_;
    form.formatText = ui->comboFormat->currentText();
    return form;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnConvert_clicked(){
    GuiFormData form = collectFormData();
    GuiRequestTranslator request;
    auto result = request.translate(form);
    if(!result.ok){
        QMessageBox::warning(
            this,
            QStringLiteral("錯誤"),
            QString::fromStdWString(result.message));
        return;
    }

    auto& req = result.Normalizedrequest;

     // 呼叫並連結核心功能
    App::ConversionEngine conversionengine;
    taskBuilder::ConversionTaskBuilder builder;
    BuildResult BDresult = builder.build(req);
    App::AppExitCode resultCode = conversionengine.run(BDresult);
    qDebug() << "AppExitCode:"
             << static_cast<int>(resultCode);
}

void MainWindow::on_btnSelectInput_clicked(){
    QString file = QFileDialog::getOpenFileName(
      this,
      "選擇輸入檔案",
      "",
      "RTF Files (*.rtf);;All Files (*)"
    );
    if(file.isEmpty()) return;

    selectedInputPath_ = file;

    ui->InputInfo->setText(file);
    ui->InputInfo->adjustSize();

}

void MainWindow::on_btnSelectOutput_clicked(){
    QString outputdir = QFileDialog::getExistingDirectory(
        this,
        "選擇指定輸出資料夾",
        ""
    );

    if(outputdir.isEmpty()) return;

    selectedOutputPath_ = outputdir;

    ui->OutputInfo->setText(outputdir);
    ui->OutputInfo->adjustSize();

}