#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GuiFormData.h"
#include "GuiRequestTranslator.h"
#include "Executor_Module/ConversionExecutor.h"
#include "Task_Module/ConversionTaskBuilder.h"
#include "Task_Module/ConversionTask.h"
#include "Universal_Module/CommonEnum.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QIntValidator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lineEdit_threadNum->setValidator(new QIntValidator(1,16,this));
    ui->lineEdit_threadNum->setPlaceholderText("預設自動(可以不填)");
}

GuiFormData MainWindow::collectFormData() const{
    GuiFormData form;
    form.inputPath = selectedInputPath_;
    form.outputDir = selectedOutputPath_;
    form.formatText = ui->comboFormat->currentText();
    form.recursive = ui->recursiveCheckBox->isChecked();
    form.dirPolicy = ui->comboBox_dirPolicy->currentText();
    form.threadtext = ui->lineEdit_threadNum->text();
    form.preserveRelativeStructure = ui->comboBox_preserveRelativeStructure->currentText();
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
    debugPrintRequest(req);

    // 測試用的停止指令
    return;

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

void MainWindow::debugPrintRequest(const NormalizedConversionRequest& req){
    qDebug() << "inputPath =" << QString::fromStdWString(req.inputPath.wstring());
    if(!req.outputDir){
      qDebug() << "outputDir = std::nullopt";
    }else{
      qDebug() << "outputDir =" << QString::fromStdWString(req.outputDir.value().wstring());
    }
    qDebug() << "recursive =" << req.recursive;

    QString formatText = "<unknown>";
    if(!req.format){
       formatText = "std::nullopt";
       qDebug() << "format =" << formatText;
    }else{
        switch (req.format.value()) {
        case Common::OutputFormat::Txt:  formatText = "txt"; break;
        case Common::OutputFormat::Md:   formatText = "md"; break;
        case Common::OutputFormat::Html: formatText = "html"; break;
        }
       qDebug() << "format =" << formatText;
    }


    QString policyText = "<unknown>";
    switch (req.dirPolicy) {
    case Common::ExistingDirPolicy::Reject:           policyText = "reject"; break;
    case Common::ExistingDirPolicy::Overwrite:        policyText = "overwrite"; break;
    case Common::ExistingDirPolicy::RenameWithSuffix: policyText = "renamewithsuffix"; break;
    }
    qDebug() << "dirPolicy =" << policyText;

    if (req.threadCount.has_value()) {
        qDebug() << "threadCount =" << static_cast<qulonglong>(*req.threadCount);
    } else {
        qDebug() << "threadCount = <auto>";
    }

    if (!req.preserveRelativeStructure.has_value()) {
        qDebug() << "preserveRelativeStructure = <std::nullopt>";
    } else if (*req.preserveRelativeStructure) {
        qDebug() << "preserveRelativeStructure = true";
    } else {
        qDebug() << "preserveRelativeStructure = false";
    }
}