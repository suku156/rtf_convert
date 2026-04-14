#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GuiFormData.h"
#include "GuiRequestTranslator.h"
#include "Executor_Module/ConversionExecutor.h"
#include "Task_Module/ConversionTaskBuilder.h"
#include "Task_Module/ConversionTask.h"
#include "Universal_Module/CommonEnum.h"
#include "GuiObserver.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QIntValidator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    guiObserver_ = new GuiObserver(this);
    convertWatcher_ = new QFutureWatcher<ConvertRunResult>(this);
    finishResetTimer_ = new QTimer(this);
    finishResetTimer_ ->setSingleShot(true);

    connect(guiObserver_, &GuiObserver::logMessage,this,
            &MainWindow::observerAppendLog,Qt::QueuedConnection);

    connect(guiObserver_, &GuiObserver::progressChanged,this,
            &MainWindow::observerUpdateProgressBar,Qt::QueuedConnection);

    connect(convertWatcher_, &QFutureWatcher<ConvertRunResult>::finished,
            this, &MainWindow::onConvertFinished);

    connect(finishResetTimer_, &QTimer::timeout,this,&MainWindow::resetToIdle);

    updateOutputDisplay();
    ui->lineEdit_threadNum->setValidator(new QIntValidator(1,16,this));
    ui->lineEdit_threadNum->setPlaceholderText("預設自動(可以不填)");


    // 詳細資訊欄顯示
    ui->plainTextEdit_log->setReadOnly(true);
    ui->plainTextEdit_log->setLineWrapMode(QPlainTextEdit::NoWrap);

    // 設定狀態欄根進度條為待機
    resetToIdle();
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
        appendLog("轉譯失敗: " + QString::fromStdWString(result.message));
        QMessageBox::warning(
            this,
            QStringLiteral("錯誤"),
            QString::fromStdWString(result.message));
        return;
    }

    auto& req = result.Normalizedrequest;
    taskBuilder::ConversionTaskBuilder builder;
    BuildResult BDresult = builder.build(req);
    appendLogToBuildResult(BDresult);
    if(!BDresult.ok){
        QMessageBox::warning(this, "建立任務失敗", QString::fromStdWString(BDresult.message));
        return;
    }

    lastBuildResult_ = BDresult;
    lastReq_ = req;

    // 使用轉換開始狀態函式
    enterRunningState();

    // 複製核心會用到的參數做使用
    auto bdresult = BDresult;
    auto observer = guiObserver_;

    // 將核心功能放到背景執行緒
    QFuture<ConvertRunResult> future = QtConcurrent::run([bdresult, observer]() -> ConvertRunResult {
       App::ConversionEngine conversionengine(observer);
       ConvertRunResult runResult;
       runResult.exitCode = conversionengine.run(bdresult);
       return runResult;
    });

    convertWatcher_->setFuture(future);
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

    inputTargetType_ = InputTargetType::File;
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
    inputTargetType_ = InputTargetType::Directory;
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

void MainWindow::appendLog(const QString& text){
    QString line = QTime::currentTime().toString("[HH:mm:ss] ") + text;
    ui->plainTextEdit_log->appendPlainText(line);
}

void MainWindow::on_btnCleanLog_clicked(){
    ui->plainTextEdit_log->clear();
}

void MainWindow::appendLogToBuildResult(const BuildResult& buildresult){
    appendLog("[GUI] 本次轉換設定");
    if(inputTargetType_ == InputTargetType::File){
        appendLog("[GUI] 輸入模式 : 單檔");
    }
    else if(inputTargetType_ == InputTargetType::Directory){
        appendLog("[GUI] 輸入模式 : 資料夾");
    }
    appendLog("[GUI] 輸入路徑 : " + QString::fromStdWString(buildresult.task.inputPath.wstring()));
    appendLog("[GUI] 預計輸出目錄 : " + QString::fromStdWString(buildresult.task.outputDir.wstring()));

    switch(buildresult.task.format){
      case Common::OutputFormat::Txt :
        appendLog("[GUI] 輸出格式 : txt");
        break;
      case Common::OutputFormat::Md  :
        appendLog("[GUI] 輸出格式 : md");
        break;
      case Common::OutputFormat::Html:
        appendLog("[GUI] 輸出格式 : html");
        break;
    }

    if(inputTargetType_ == InputTargetType::Directory){
        switch(buildresult.task.dirPolicy){
        case Common::ExistingDirPolicy::Reject :
            appendLog("[GUI] 輸出資料夾同名處理策略 : 安全模式");
            break;
        case Common::ExistingDirPolicy::Overwrite :
            appendLog("[GUI] 輸出資料夾同名處理策略 : 覆蓋模式");
            break;
        case Common::ExistingDirPolicy::RenameWithSuffix :
            appendLog("[GUI] 輸出資料夾同名處理策略 : 重新命名模式");
            break;
        }

        if(buildresult.task.recursive){
            appendLog("[GUI] 遞迴模式 : 開啟");
            if(buildresult.task.preserveRelativeStructure){
                appendLog("[GUI] 保留中間資料夾路徑 : 保留");
            }else{
                appendLog("[GUI] 保留中間資料夾路徑 : 不保留");
            }
        }else{
            appendLog("[GUI] 遞迴模式 : 關閉");
        }
    }

    ui->plainTextEdit_log->appendPlainText("");
}

void MainWindow::observerAppendLog(QString text){
    appendLog(text);
}

void MainWindow::observerUpdateProgressBar(int done,int total){
    if (total <= 0) {
        ui->progressBar->setMinimum(0);
        ui->progressBar->setMaximum(1);
        ui->progressBar->setValue(0);
        return;
    }

    if(done < 0) done = 0;
    if(done > total) done = total;

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(done);
}

void MainWindow::onConvertFinished(){
    const ConvertRunResult runResult = convertWatcher_->result();

    // 結束後執行的狀態函式
    QString finishshowtext = "完成";
    showFinishedStateAndDelayReset(finishshowtext);

    if (!lastReq_) {
        QMessageBox::warning(this, "未知狀態", "缺少請求資訊");
        return;
    }

    const auto& req = *lastReq_;

    // 依據回傳結果做出回饋
    switch (runResult.exitCode) {
    case App::AppExitCode::Success :
    {

        QString pathresult = QString::fromStdWString(req.inputPath.filename().wstring());
        QString successresult = "轉換成功,已處理目標: " + pathresult ;
        QMessageBox::information(this, "完成",successresult);
        break;
    }
    case App::AppExitCode::Fail :
    case App::AppExitCode::RunTimeError:
    {

        QMessageBox::critical(this, "轉換失敗", "轉換過程發生錯誤");
        break;
    }
    case App::AppExitCode::PartialSuccess:
    {

        QMessageBox::warning(this, "部分完成", "部分檔案轉換成功，部分失敗");
        break;
    }
    default:
    {

        QMessageBox::warning(this, "未知狀態", "發生未知錯誤");
        break;
    }
    }

    ui->plainTextEdit_log->appendPlainText("");
}

void MainWindow::enterRunningState(){
    // 停掉舊的完成延遲
    if(finishResetTimer_->isActive()){
        finishResetTimer_->stop();
    }

    // 手動上鎖
    ui->btnConvert->setEnabled(false);
    ui->label_status->setText("處裡中...");
    ui->progressBar->setVisible(true);


    if(inputTargetType_ == InputTargetType::File){
        ui->progressBar->setRange(0,0);
    }
    else if(inputTargetType_ == InputTargetType::Directory){
        ui->progressBar->setMinimum(0);
        ui->progressBar->setMaximum(1);
        ui->progressBar->setValue(0);
    }
    else if(inputTargetType_ == InputTargetType::None){
        ui->progressBar->setRange(0,0);
    }
}

void MainWindow::showFinishedStateAndDelayReset(const QString& text){
    ui ->label_status->setText(text);

    // 顯示 100%
    if (ui->progressBar->maximum() == 0) {
        ui->progressBar->setRange(0, 1);
        ui->progressBar->setValue(1);
    } else {
        ui->progressBar->setValue(ui->progressBar->maximum());
    }

    ui->btnConvert->setEnabled(true);

    // 計時1秒後回到待機
    finishResetTimer_-> start(1250);
}

void MainWindow::resetToIdle(){
   ui->label_status->setText("準備中");
    ui->progressBar->setVisible(false);
}

