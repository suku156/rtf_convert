#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GuiFormData.h"
#include "GuiRequestTranslator.h"
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->conboFormat->addItem("txt");
    ui->conboFormat->addItem("md");
    ui->conboFormat->addItem("html");

}

GuiFormData MainWindow::collectFormData() const{
    GuiFormData form;
    form.inputPath = ui->InputInfo->text();
    form.outputDir = ui->OutputInfo->text();
    form.formatText = ui->conboFormat->currentText();
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
}

void MainWindow::on_btnSelectInput_clicked(){
    QString file = QFileDialog::getOpenFileName(
      this,
      "選擇輸入檔案",
      "",
      "RTF Files (*.rtf);;All Files (*)"
    );

    if(!file.isEmpty()){
        ui->InputInfo->setText(file);
        ui->InputInfo->adjustSize();
    }
}

void MainWindow::on_btnSelectOutput_clicked(){
    QString outputdir = QFileDialog::getExistingDirectory(
        this,
        "選擇指定輸出資料夾",
        ""
    );

    if(!outputdir.isEmpty()){
        ui->OutputInfo->setText(outputdir);
        ui->OutputInfo->adjustSize();
    }
}