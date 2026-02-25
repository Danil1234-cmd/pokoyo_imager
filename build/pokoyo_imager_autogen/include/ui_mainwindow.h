/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
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
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *hboxLayout;
    QLabel *labelTheme;
    QComboBox *comboTheme;
    QSpacerItem *themeSpacer;
    QGroupBox *groupDevice;
    QHBoxLayout *hboxLayout1;
    QComboBox *comboDevice;
    QPushButton *btnRefresh;
    QGroupBox *groupBoot;
    QVBoxLayout *vboxLayout;
    QComboBox *comboBootType;
    QHBoxLayout *hboxLayout2;
    QLineEdit *editImagePath;
    QPushButton *btnSelectImage;
    QGroupBox *groupPartition;
    QFormLayout *formLayout;
    QLabel *label;
    QComboBox *comboPartitionScheme;
    QLabel *label1;
    QComboBox *comboTargetSystem;
    QGroupBox *groupFormat;
    QFormLayout *formLayout1;
    QLabel *label2;
    QComboBox *comboFileSystem;
    QLabel *label3;
    QComboBox *comboClusterSize;
    QLabel *label4;
    QLineEdit *editVolumeLabel;
    QCheckBox *checkQuickFormat;
    QCheckBox *checkExtendedLabel;
    QProgressBar *progressBar;
    QTextEdit *textLog;
    QHBoxLayout *hboxLayout3;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnStart;
    QPushButton *btnClose;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(520, 660);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
#ifndef Q_OS_MAC
        verticalLayout->setSpacing(6);
#endif
        verticalLayout->setContentsMargins(8, 8, 8, 8);
        verticalLayout->setObjectName("verticalLayout");
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName("hboxLayout");
        labelTheme = new QLabel(centralwidget);
        labelTheme->setObjectName("labelTheme");

        hboxLayout->addWidget(labelTheme);

        comboTheme = new QComboBox(centralwidget);
        comboTheme->addItem(QString());
        comboTheme->addItem(QString());
        comboTheme->setObjectName("comboTheme");

        hboxLayout->addWidget(comboTheme);

        themeSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        hboxLayout->addItem(themeSpacer);


        verticalLayout->addLayout(hboxLayout);

        groupDevice = new QGroupBox(centralwidget);
        groupDevice->setObjectName("groupDevice");
        hboxLayout1 = new QHBoxLayout(groupDevice);
        hboxLayout1->setObjectName("hboxLayout1");
        comboDevice = new QComboBox(groupDevice);
        comboDevice->setObjectName("comboDevice");

        hboxLayout1->addWidget(comboDevice);

        btnRefresh = new QPushButton(groupDevice);
        btnRefresh->setObjectName("btnRefresh");

        hboxLayout1->addWidget(btnRefresh);


        verticalLayout->addWidget(groupDevice);

        groupBoot = new QGroupBox(centralwidget);
        groupBoot->setObjectName("groupBoot");
        vboxLayout = new QVBoxLayout(groupBoot);
        vboxLayout->setObjectName("vboxLayout");
        comboBootType = new QComboBox(groupBoot);
        comboBootType->setObjectName("comboBootType");

        vboxLayout->addWidget(comboBootType);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setObjectName("hboxLayout2");
        editImagePath = new QLineEdit(groupBoot);
        editImagePath->setObjectName("editImagePath");

        hboxLayout2->addWidget(editImagePath);

        btnSelectImage = new QPushButton(groupBoot);
        btnSelectImage->setObjectName("btnSelectImage");

        hboxLayout2->addWidget(btnSelectImage);


        vboxLayout->addLayout(hboxLayout2);


        verticalLayout->addWidget(groupBoot);

        groupPartition = new QGroupBox(centralwidget);
        groupPartition->setObjectName("groupPartition");
        formLayout = new QFormLayout(groupPartition);
        formLayout->setObjectName("formLayout");
        formLayout->setLabelAlignment(Qt::AlignRight|Qt::AlignVCenter);
        label = new QLabel(groupPartition);
        label->setObjectName("label");

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        comboPartitionScheme = new QComboBox(groupPartition);
        comboPartitionScheme->setObjectName("comboPartitionScheme");

        formLayout->setWidget(0, QFormLayout::FieldRole, comboPartitionScheme);

        label1 = new QLabel(groupPartition);
        label1->setObjectName("label1");

        formLayout->setWidget(1, QFormLayout::LabelRole, label1);

        comboTargetSystem = new QComboBox(groupPartition);
        comboTargetSystem->setObjectName("comboTargetSystem");

        formLayout->setWidget(1, QFormLayout::FieldRole, comboTargetSystem);


        verticalLayout->addWidget(groupPartition);

        groupFormat = new QGroupBox(centralwidget);
        groupFormat->setObjectName("groupFormat");
        formLayout1 = new QFormLayout(groupFormat);
        formLayout1->setObjectName("formLayout1");
        formLayout1->setLabelAlignment(Qt::AlignRight|Qt::AlignVCenter);
        label2 = new QLabel(groupFormat);
        label2->setObjectName("label2");

        formLayout1->setWidget(0, QFormLayout::LabelRole, label2);

        comboFileSystem = new QComboBox(groupFormat);
        comboFileSystem->setObjectName("comboFileSystem");

        formLayout1->setWidget(0, QFormLayout::FieldRole, comboFileSystem);

        label3 = new QLabel(groupFormat);
        label3->setObjectName("label3");

        formLayout1->setWidget(1, QFormLayout::LabelRole, label3);

        comboClusterSize = new QComboBox(groupFormat);
        comboClusterSize->setObjectName("comboClusterSize");

        formLayout1->setWidget(1, QFormLayout::FieldRole, comboClusterSize);

        label4 = new QLabel(groupFormat);
        label4->setObjectName("label4");

        formLayout1->setWidget(2, QFormLayout::LabelRole, label4);

        editVolumeLabel = new QLineEdit(groupFormat);
        editVolumeLabel->setObjectName("editVolumeLabel");

        formLayout1->setWidget(2, QFormLayout::FieldRole, editVolumeLabel);

        checkQuickFormat = new QCheckBox(groupFormat);
        checkQuickFormat->setObjectName("checkQuickFormat");

        formLayout1->setWidget(3, QFormLayout::SpanningRole, checkQuickFormat);

        checkExtendedLabel = new QCheckBox(groupFormat);
        checkExtendedLabel->setObjectName("checkExtendedLabel");

        formLayout1->setWidget(4, QFormLayout::SpanningRole, checkExtendedLabel);


        verticalLayout->addWidget(groupFormat);

        progressBar = new QProgressBar(centralwidget);
        progressBar->setObjectName("progressBar");
        progressBar->setMinimumHeight(18);
        progressBar->setValue(0);

        verticalLayout->addWidget(progressBar);

        textLog = new QTextEdit(centralwidget);
        textLog->setObjectName("textLog");
        textLog->setMinimumHeight(120);
        textLog->setReadOnly(true);

        verticalLayout->addWidget(textLog);

        hboxLayout3 = new QHBoxLayout();
        hboxLayout3->setObjectName("hboxLayout3");
        horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        hboxLayout3->addItem(horizontalSpacer);

        btnStart = new QPushButton(centralwidget);
        btnStart->setObjectName("btnStart");
        btnStart->setMinimumWidth(110);

        hboxLayout3->addWidget(btnStart);

        btnClose = new QPushButton(centralwidget);
        btnClose->setObjectName("btnClose");
        btnClose->setMinimumWidth(80);

        hboxLayout3->addWidget(btnClose);


        verticalLayout->addLayout(hboxLayout3);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Pokoyo Imager 1.0", nullptr));
        labelTheme->setText(QCoreApplication::translate("MainWindow", "Theme:", nullptr));
        comboTheme->setItemText(0, QCoreApplication::translate("MainWindow", "Light", nullptr));
        comboTheme->setItemText(1, QCoreApplication::translate("MainWindow", "Dark", nullptr));

        groupDevice->setTitle(QCoreApplication::translate("MainWindow", "Device", nullptr));
        btnRefresh->setText(QCoreApplication::translate("MainWindow", "Refresh", nullptr));
        groupBoot->setTitle(QCoreApplication::translate("MainWindow", "Boot selection", nullptr));
        btnSelectImage->setText(QCoreApplication::translate("MainWindow", "Select", nullptr));
        groupPartition->setTitle(QCoreApplication::translate("MainWindow", "Partition scheme", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Scheme:", nullptr));
        label1->setText(QCoreApplication::translate("MainWindow", "Target system:", nullptr));
        groupFormat->setTitle(QCoreApplication::translate("MainWindow", "Format options", nullptr));
        label2->setText(QCoreApplication::translate("MainWindow", "File system:", nullptr));
        label3->setText(QCoreApplication::translate("MainWindow", "Cluster size:", nullptr));
        label4->setText(QCoreApplication::translate("MainWindow", "Volume label:", nullptr));
        checkQuickFormat->setText(QCoreApplication::translate("MainWindow", "Quick format", nullptr));
        checkExtendedLabel->setText(QCoreApplication::translate("MainWindow", "Create extended label and icon files", nullptr));
        btnStart->setText(QCoreApplication::translate("MainWindow", "START", nullptr));
        btnClose->setText(QCoreApplication::translate("MainWindow", "CLOSE", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
