/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.10.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout_2;
    QPushButton *btnAdvanced;
    QGroupBox *groupBox_advanced;
    QFormLayout *formLayout;
    QComboBox *comboFormat;
    QLabel *format_label;
    QComboBox *comboBox_dirPolicy;
    QLabel *dirPolicy_label;
    QComboBox *comboBox_preserveRelativeStructure;
    QLabel *preserveRelativeStructure_label;
    QLabel *thread_label;
    QCheckBox *recursiveCheckBox;
    QSpinBox *spinBox_threadNum;
    QWidget *widget_FeedBack;
    QGridLayout *gridLayout;
    QProgressBar *progressBar;
    QLabel *label_status;
    QPlainTextEdit *plainTextEdit_log;
    QPushButton *btnCleanLog;
    QGroupBox *groupBox_input;
    QFormLayout *formLayout_3;
    QLabel *label_showinput;
    QLabel *InputInfo;
    QPushButton *btnSelectInputFile;
    QPushButton *btnSelectInputDir;
    QGroupBox *groupBox_output;
    QFormLayout *formLayout_2;
    QLabel *label_showOutput;
    QLabel *OutputInfo;
    QPushButton *btnSelectOutput;
    QPushButton *btnCleanOutput;
    QPushButton *btnConvert;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 937);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        gridLayout_2 = new QGridLayout(centralwidget);
        gridLayout_2->setSpacing(15);
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout_2->setContentsMargins(12, 12, 12, 12);
        btnAdvanced = new QPushButton(centralwidget);
        btnAdvanced->setObjectName("btnAdvanced");

        gridLayout_2->addWidget(btnAdvanced, 2, 0, 1, 1);

        groupBox_advanced = new QGroupBox(centralwidget);
        groupBox_advanced->setObjectName("groupBox_advanced");
        groupBox_advanced->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
        groupBox_advanced->setAlignment(Qt::AlignmentFlag::AlignRight|Qt::AlignmentFlag::AlignTop|Qt::AlignmentFlag::AlignTrailing);
        formLayout = new QFormLayout(groupBox_advanced);
        formLayout->setObjectName("formLayout");
        formLayout->setHorizontalSpacing(8);
        formLayout->setVerticalSpacing(8);
        formLayout->setContentsMargins(10, 10, 10, 10);
        comboFormat = new QComboBox(groupBox_advanced);
        comboFormat->addItem(QString());
        comboFormat->addItem(QString());
        comboFormat->addItem(QString());
        comboFormat->addItem(QString());
        comboFormat->setObjectName("comboFormat");
        comboFormat->setMouseTracking(false);
        comboFormat->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
        comboFormat->setMaxVisibleItems(10);
        comboFormat->setMinimumContentsLength(0);

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, comboFormat);

        format_label = new QLabel(groupBox_advanced);
        format_label->setObjectName("format_label");
        QFont font;
        font.setPointSize(12);
        format_label->setFont(font);

        formLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, format_label);

        comboBox_dirPolicy = new QComboBox(groupBox_advanced);
        comboBox_dirPolicy->addItem(QString());
        comboBox_dirPolicy->addItem(QString());
        comboBox_dirPolicy->addItem(QString());
        comboBox_dirPolicy->setObjectName("comboBox_dirPolicy");
        comboBox_dirPolicy->setLayoutDirection(Qt::LayoutDirection::LeftToRight);

        formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, comboBox_dirPolicy);

        dirPolicy_label = new QLabel(groupBox_advanced);
        dirPolicy_label->setObjectName("dirPolicy_label");
        dirPolicy_label->setFont(font);

        formLayout->setWidget(1, QFormLayout::ItemRole::FieldRole, dirPolicy_label);

        comboBox_preserveRelativeStructure = new QComboBox(groupBox_advanced);
        comboBox_preserveRelativeStructure->addItem(QString());
        comboBox_preserveRelativeStructure->addItem(QString());
        comboBox_preserveRelativeStructure->addItem(QString());
        comboBox_preserveRelativeStructure->setObjectName("comboBox_preserveRelativeStructure");
        comboBox_preserveRelativeStructure->setLayoutDirection(Qt::LayoutDirection::LeftToRight);

        formLayout->setWidget(2, QFormLayout::ItemRole::LabelRole, comboBox_preserveRelativeStructure);

        preserveRelativeStructure_label = new QLabel(groupBox_advanced);
        preserveRelativeStructure_label->setObjectName("preserveRelativeStructure_label");
        QFont font1;
        font1.setPointSize(11);
        preserveRelativeStructure_label->setFont(font1);

        formLayout->setWidget(2, QFormLayout::ItemRole::FieldRole, preserveRelativeStructure_label);

        thread_label = new QLabel(groupBox_advanced);
        thread_label->setObjectName("thread_label");
        thread_label->setFont(font);

        formLayout->setWidget(3, QFormLayout::ItemRole::FieldRole, thread_label);

        recursiveCheckBox = new QCheckBox(groupBox_advanced);
        recursiveCheckBox->setObjectName("recursiveCheckBox");
        recursiveCheckBox->setFont(font);
        recursiveCheckBox->setLayoutDirection(Qt::LayoutDirection::LeftToRight);

        formLayout->setWidget(5, QFormLayout::ItemRole::FieldRole, recursiveCheckBox);

        spinBox_threadNum = new QSpinBox(groupBox_advanced);
        spinBox_threadNum->setObjectName("spinBox_threadNum");
        spinBox_threadNum->setMinimumSize(QSize(41, 24));

        formLayout->setWidget(4, QFormLayout::ItemRole::FieldRole, spinBox_threadNum);


        gridLayout_2->addWidget(groupBox_advanced, 3, 0, 1, 2);

        widget_FeedBack = new QWidget(centralwidget);
        widget_FeedBack->setObjectName("widget_FeedBack");
        gridLayout = new QGridLayout(widget_FeedBack);
        gridLayout->setSpacing(8);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setContentsMargins(10, 10, 10, 10);
        progressBar = new QProgressBar(widget_FeedBack);
        progressBar->setObjectName("progressBar");
        progressBar->setValue(0);

        gridLayout->addWidget(progressBar, 3, 0, 1, 1);

        label_status = new QLabel(widget_FeedBack);
        label_status->setObjectName("label_status");
        QFont font2;
        font2.setPointSize(20);
        label_status->setFont(font2);

        gridLayout->addWidget(label_status, 2, 0, 1, 1);

        plainTextEdit_log = new QPlainTextEdit(widget_FeedBack);
        plainTextEdit_log->setObjectName("plainTextEdit_log");

        gridLayout->addWidget(plainTextEdit_log, 6, 0, 1, 2);

        btnCleanLog = new QPushButton(widget_FeedBack);
        btnCleanLog->setObjectName("btnCleanLog");

        gridLayout->addWidget(btnCleanLog, 4, 0, 1, 2);


        gridLayout_2->addWidget(widget_FeedBack, 5, 0, 1, 2);

        groupBox_input = new QGroupBox(centralwidget);
        groupBox_input->setObjectName("groupBox_input");
        groupBox_input->setFont(font);
        formLayout_3 = new QFormLayout(groupBox_input);
        formLayout_3->setObjectName("formLayout_3");
        formLayout_3->setHorizontalSpacing(8);
        formLayout_3->setVerticalSpacing(8);
        formLayout_3->setContentsMargins(10, 10, 10, 10);
        label_showinput = new QLabel(groupBox_input);
        label_showinput->setObjectName("label_showinput");

        formLayout_3->setWidget(0, QFormLayout::ItemRole::LabelRole, label_showinput);

        InputInfo = new QLabel(groupBox_input);
        InputInfo->setObjectName("InputInfo");

        formLayout_3->setWidget(0, QFormLayout::ItemRole::FieldRole, InputInfo);

        btnSelectInputFile = new QPushButton(groupBox_input);
        btnSelectInputFile->setObjectName("btnSelectInputFile");

        formLayout_3->setWidget(2, QFormLayout::ItemRole::SpanningRole, btnSelectInputFile);

        btnSelectInputDir = new QPushButton(groupBox_input);
        btnSelectInputDir->setObjectName("btnSelectInputDir");

        formLayout_3->setWidget(3, QFormLayout::ItemRole::SpanningRole, btnSelectInputDir);


        gridLayout_2->addWidget(groupBox_input, 0, 0, 1, 2);

        groupBox_output = new QGroupBox(centralwidget);
        groupBox_output->setObjectName("groupBox_output");
        groupBox_output->setFont(font);
        formLayout_2 = new QFormLayout(groupBox_output);
        formLayout_2->setObjectName("formLayout_2");
        formLayout_2->setHorizontalSpacing(8);
        formLayout_2->setVerticalSpacing(8);
        formLayout_2->setContentsMargins(10, 10, 10, 10);
        label_showOutput = new QLabel(groupBox_output);
        label_showOutput->setObjectName("label_showOutput");
        QFont font3;
        font3.setPointSize(14);
        label_showOutput->setFont(font3);

        formLayout_2->setWidget(0, QFormLayout::ItemRole::LabelRole, label_showOutput);

        OutputInfo = new QLabel(groupBox_output);
        OutputInfo->setObjectName("OutputInfo");
        OutputInfo->setFont(font);

        formLayout_2->setWidget(0, QFormLayout::ItemRole::FieldRole, OutputInfo);

        btnSelectOutput = new QPushButton(groupBox_output);
        btnSelectOutput->setObjectName("btnSelectOutput");

        formLayout_2->setWidget(1, QFormLayout::ItemRole::SpanningRole, btnSelectOutput);

        btnCleanOutput = new QPushButton(groupBox_output);
        btnCleanOutput->setObjectName("btnCleanOutput");

        formLayout_2->setWidget(3, QFormLayout::ItemRole::SpanningRole, btnCleanOutput);


        gridLayout_2->addWidget(groupBox_output, 1, 0, 1, 2);

        btnConvert = new QPushButton(centralwidget);
        btnConvert->setObjectName("btnConvert");
        QFont font4;
        font4.setPointSize(28);
        btnConvert->setFont(font4);

        gridLayout_2->addWidget(btnConvert, 4, 0, 1, 1);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 20));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        btnAdvanced->setText(QCoreApplication::translate("MainWindow", "\351\241\257\347\244\272/\351\232\261\350\227\217 \351\200\262\351\232\216\350\250\255\345\256\232", nullptr));
        groupBox_advanced->setTitle(QCoreApplication::translate("MainWindow", "\351\200\262\351\232\216\350\250\255\345\256\232", nullptr));
        comboFormat->setItemText(0, QCoreApplication::translate("MainWindow", "default", nullptr));
        comboFormat->setItemText(1, QCoreApplication::translate("MainWindow", "txt", nullptr));
        comboFormat->setItemText(2, QCoreApplication::translate("MainWindow", "md", nullptr));
        comboFormat->setItemText(3, QCoreApplication::translate("MainWindow", "html", nullptr));

        format_label->setText(QCoreApplication::translate("MainWindow", "\351\201\270\346\223\207\350\274\270\345\207\272\346\240\274\345\274\217 : ", nullptr));
        comboBox_dirPolicy->setItemText(0, QCoreApplication::translate("MainWindow", "reject", nullptr));
        comboBox_dirPolicy->setItemText(1, QCoreApplication::translate("MainWindow", "overwrite", nullptr));
        comboBox_dirPolicy->setItemText(2, QCoreApplication::translate("MainWindow", "renamewithsuffix", nullptr));

        dirPolicy_label->setText(QCoreApplication::translate("MainWindow", "\351\201\207\345\210\260\345\220\214\345\220\215\350\263\207\346\226\231\345\244\276\347\232\204\345\256\211\345\205\250\347\255\226\347\225\245\351\201\270\346\223\207 : ", nullptr));
        comboBox_preserveRelativeStructure->setItemText(0, QCoreApplication::translate("MainWindow", "default", nullptr));
        comboBox_preserveRelativeStructure->setItemText(1, QCoreApplication::translate("MainWindow", "preserve-structure", nullptr));
        comboBox_preserveRelativeStructure->setItemText(2, QCoreApplication::translate("MainWindow", "flat-output", nullptr));

        preserveRelativeStructure_label->setText(QCoreApplication::translate("MainWindow", "\351\201\236\350\277\264\346\250\241\345\274\217\344\270\213\346\230\257\345\220\246\344\277\235\347\225\231\344\270\255\351\226\223\350\263\207\346\226\231\345\244\276\350\267\257\345\276\221\347\255\226\347\225\245\351\201\270\346\223\207 :", nullptr));
        thread_label->setText(QCoreApplication::translate("MainWindow", "\350\250\255\345\256\232\345\244\232\345\237\267\350\241\214\347\267\222\346\225\270\351\207\217(1~16) :", nullptr));
        recursiveCheckBox->setText(QCoreApplication::translate("MainWindow", "\345\225\237\347\224\250\351\201\236\350\277\264\346\250\241\345\274\217", nullptr));
        label_status->setText(QCoreApplication::translate("MainWindow", "\346\272\226\345\202\231\344\270\255", nullptr));
        btnCleanLog->setText(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244 log \345\215\200\346\226\207\345\255\227", nullptr));
        groupBox_input->setTitle(QCoreApplication::translate("MainWindow", "\350\274\270\345\205\245", nullptr));
        label_showinput->setText(QCoreApplication::translate("MainWindow", "\350\274\270\345\205\245 :", nullptr));
        InputInfo->setText(QCoreApplication::translate("MainWindow", "\346\234\252\346\214\207\345\256\232(\345\277\205\351\201\270)", nullptr));
        btnSelectInputFile->setText(QCoreApplication::translate("MainWindow", "\351\201\270\346\223\207\346\252\224\346\241\210", nullptr));
        btnSelectInputDir->setText(QCoreApplication::translate("MainWindow", "\351\201\270\346\223\207\350\263\207\346\226\231\345\244\276", nullptr));
        groupBox_output->setTitle(QCoreApplication::translate("MainWindow", "\350\274\270\345\207\272\344\275\215\347\275\256(\351\240\220\350\250\255:\345\226\256\346\252\224\347\233\256\346\250\231\346\224\276\345\234\250\346\252\224\346\241\210\345\220\214\350\263\207\346\226\231\345\244\276/\350\263\207\346\226\231\345\244\276\347\233\256\346\250\231\346\224\276\345\234\250\347\233\256\346\250\231\350\263\207\346\226\231\345\244\276\345\205\247)", nullptr));
        label_showOutput->setText(QCoreApplication::translate("MainWindow", "\350\274\270\345\207\272 :", nullptr));
        OutputInfo->setText(QCoreApplication::translate("MainWindow", "\346\234\252\346\214\207\345\256\232(\345\217\257\344\270\215\351\201\270\345\260\207\344\275\277\347\224\250\351\240\220\350\250\255)", nullptr));
        btnSelectOutput->setText(QCoreApplication::translate("MainWindow", "\351\201\270\346\223\207\344\275\215\347\275\256", nullptr));
        btnCleanOutput->setText(QCoreApplication::translate("MainWindow", "\346\270\205\347\251\272\344\275\215\347\275\256", nullptr));
        btnConvert->setText(QCoreApplication::translate("MainWindow", "\350\275\211\346\217\233\351\226\213\345\247\213", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
