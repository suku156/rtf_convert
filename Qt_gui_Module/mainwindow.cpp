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

class ConvertUiGuard{
private:
    QPushButton* button_;
    QLabel* label_;
public:
    ConvertUiGuard(QPushButton* button,QLabel* label)
        : button_(button),label_(label)
    {
        if(button_) button_->setEnabled(false);
        if(label_)  label_ ->setText("處裡中...");
        qApp->processEvents();
    }

    ~ConvertUiGuard()
    {
        if(button_) button_->setEnabled(true);
        if(label_)  label_ ->setText("準備中");
    }

    // 禁止複製
    ConvertUiGuard(const ConvertUiGuard&) = delete;
    ConvertUiGuard& operator=(const ConvertUiGuard&) = delete;
    // 禁止移動
    ConvertUiGuard(ConvertUiGuard&&) = delete;
    ConvertUiGuard& operator=(ConvertUiGuard&&) = delete;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    updateOutputDisplay();
    ui->lineEdit_threadNum->setValidator(new QIntValidator(1,16,this));
    ui->lineEdit_threadNum->setPlaceholderText("預設自動(可以不填)");
    ui->label_status->setText("準備中");
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

    ConvertUiGuard guard(ui->btnConvert,ui->label_status);

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

    // 依據回傳結果做出回饋
    switch (resultCode) {
    case App::AppExitCode::Success :
        QMessageBox::information(this, "完成", "轉換成功");
        break;
    case App::AppExitCode::Fail :
    case App::AppExitCode::RunTimeError:
        QMessageBox::critical(this, "轉換失敗", "轉換過程發生錯誤");
        break;
    case App::AppExitCode::PartialSuccess:
        QMessageBox::warning(this, "部分完成", "部分檔案轉換成功，部分失敗");
        break;
    defult:
        QMessageBox::warning(this, "未知狀態", "發生未知錯誤");
        break;
    }
}

void MainWindow::on_btnSelectInputFile_clicked(){
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

void MainWindow::on_btnSelectInputDir_clicked(){
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "選擇輸入資料夾",
        ""
        );

    if(dir.isEmpty()) return;

    selectedInputPath_ = dir;

    ui->InputInfo->setText(dir);
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
    updateOutputDisplay();
}

void MainWindow::on_btnCleanOutput_clicked(){
    selectedOutputPath_.clear();
    updateOutputDisplay();
}

void MainWindow::updateOutputDisplay(){
    if(selectedOutputPath_.isEmpty()){
        ui->OutputInfo->setText("未指定 (使用預設) ");
    }else{
        ui->OutputInfo->setText(selectedOutputPath_);
        ui->OutputInfo->adjustSize();
    }
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