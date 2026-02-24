#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QFileDialog>
#include <QTimer>
#include <QRegularExpression>
#include <QTextStream>
#include <QMessageBox>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <unistd.h>
#include <QThread>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    connect(ui->comboDevice,
            QOverload<int>::of(&QComboBox::activated),
            this,
            &MainWindow::on_comboDevice_activated);

    connect(ui->comboTheme, &QComboBox::currentTextChanged,
            this, [=](const QString &theme)
            {
                if(theme == "Dark")
                    loadStyle("pokoyo_dark.qss");
                else
                    loadStyle("pokoyo_light.qss");
            });


    // COMBO BOXES

    // partition scheme
    ui->comboPartitionScheme->addItem("MBR");
    ui->comboPartitionScheme->addItem("GPT");

    //target system
    ui->comboTargetSystem->addItem("BIOS (or UEFI-CSM)");
    ui->comboTargetSystem->addItem("UEFI (non CSM)");

    //file system
    ui->comboFileSystem->addItem("FAT32");
    ui->comboFileSystem->addItem("NTFS");
    ui->comboFileSystem->addItem("exFAT");
    ui->comboFileSystem->setCurrentIndex(0);

    //cluster sizes
    QStringList clusterSizes = {"Default", "512 bytes", "1 KB", "2 KB",
                                "4 KB", "8 KB", "16 KB", "32 KB", "64 KB"};
    ui->comboClusterSize->addItems(clusterSizes);
    ui->comboClusterSize->setCurrentIndex(0); // Default
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadStyle(const QString &styleName)
{
    QString appDir = QCoreApplication::applicationDirPath();

    QString fullPath = appDir + "/" + styleName;

    QFile file(fullPath);

    if(!file.exists())
    {
        fullPath = appDir + "/../" + styleName;
        file.setFileName(fullPath);
    }

    if(file.open(QFile::ReadOnly))
    {
        QString styleSheet = QLatin1String(file.readAll());
        qApp->setStyleSheet(styleSheet);
        file.close();
    }
}



void MainWindow::on_btnStart_clicked()
{
    int index = ui->comboDevice->currentIndex();
    if (index < 0 || index >= usbDevices.size()) {
        QMessageBox::warning(this, "Error", "No USB device selected.");
        return;
    }

    QString isoPath;
    int bootIndex = ui->comboBootType->currentIndex();
    if (bootIndex >= 0)
        isoPath = ui->comboBootType->itemText(bootIndex);

    if (isoPath.isEmpty() || !QFile::exists(isoPath)) {
        QMessageBox::warning(this, "Error", "Invalid ISO file.");
        return;
    }

    if (geteuid() != 0) {
        QMessageBox::warning(this, "Root required",
                             "Run Pokoyo Imager as root.");
        return;
    }

    UsbDevice dev = usbDevices[index];
    QString devicePath = "/dev/" + dev.name;

    QFileInfo isoInfo(isoPath);
    qint64 isoSize = isoInfo.size();

    QProcess sizeProc;
    sizeProc.start("blockdev", QStringList() << "--getsize64" << devicePath);
    sizeProc.waitForFinished();
    qint64 deviceSize = sizeProc.readAllStandardOutput().trimmed().toLongLong();

    if (deviceSize < isoSize + 1024 * 1024) {
        QMessageBox::critical(this, "Error", "USB drive too small.");
        return;
    }

    QString partitionScheme = ui->comboPartitionScheme->currentText();
    QString fsType = ui->comboFileSystem->currentText();
    bool createExtended = ui->checkExtendedLabel->isChecked();

    if (QMessageBox::warning(this,
                             "WARNING",
                             "ALL DATA WILL BE ERASED!\nContinue?",
                             QMessageBox::Yes | QMessageBox::No)
        != QMessageBox::Yes)
        return;

    ui->btnStart->setEnabled(false);
    ui->progressBar->setValue(0);

    auto log = [&](QString s){
        ui->textLog->append(s);
        qDebug() << s;
    };

    log("Cleaning up old mounts...");
    QProcess::execute("umount", QStringList() << "-lf" << devicePath);
    QProcess::execute("wipefs", QStringList() << "-a" << devicePath);

    // Partition table
    QString mklabel = (partitionScheme == "GPT") ? "gpt" : "msdos";
    if (QProcess::execute("parted", {"-s", devicePath, "mklabel", mklabel}) != 0)
        return;

    if (QProcess::execute("parted", {"-s", devicePath, "mkpart", "primary", "0%", "100%"}) != 0)
        return;

    QProcess::execute("partprobe", {devicePath});
    QThread::msleep(500);

    QString partitionPath = devicePath + "1";

    // Format
    if (fsType == "FAT32")
        QProcess::execute("mkfs.vfat", {"-F", "32", partitionPath});
    else if (fsType == "NTFS")
        QProcess::execute("mkfs.ntfs", {"-F", partitionPath});
    else if (fsType == "exFAT")
        QProcess::execute("mkfs.exfat", {partitionPath});

    // Mount paths
    QString tmpBase = QDir::tempPath() + "/pokoyo_imager";
    QString mountIsoDir = tmpBase + "/iso_mount";
    QString mountTargetDir = tmpBase + "/target_mount";

    QProcess::execute("umount", {"-lf", mountIsoDir});
    QProcess::execute("umount", {"-lf", mountTargetDir});

    QDir().mkpath(mountIsoDir);
    QDir().mkpath(mountTargetDir);

    if (QProcess::execute("mount", {"-o", "loop,ro", isoPath, mountIsoDir}) != 0) {
        QMessageBox::critical(this, "Error", "ISO mount failed.");
        return;
    }

    if (QProcess::execute("mount", {partitionPath, mountTargetDir}) != 0) {
        QMessageBox::critical(this, "Error", "Target mount failed.");
        return;
    }

    log("Starting rsync...");

    QProcess *rsync = new QProcess(this);
    rsync->setProcessChannelMode(QProcess::MergedChannels);

    QStringList rsyncArgs;
    rsyncArgs << "-r"
              << "-t"
              << "--info=progress2"
              << "--no-perms"
              << "--no-owner"
              << "--no-group"
              << "--modify-window=1"
              << mountIsoDir + "/"
              << mountTargetDir + "/";

    connect(rsync, &QProcess::readyReadStandardOutput, this, [=]() {
        QString out = QString::fromLocal8Bit(rsync->readAll());
        ui->textLog->append(out);

        QRegularExpression re("(\\d+)%");
        auto match = re.match(out);
        if (match.hasMatch()) {
            int p = match.captured(1).toInt();
            if (p > 100) p = 100;
            ui->progressBar->setValue(p);
        }
    });

    connect(rsync, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int exitCode, QProcess::ExitStatus){

                if (exitCode != 0 && exitCode != 23) {
                    QMessageBox::critical(this, "Error", "Rsync failed.");
                } else {
                    log("Copy complete.");

                    // Install syslinux only if MBR + FAT32
                    if (partitionScheme == "MBR" && fsType == "FAT32") {
                        log("Installing SYSLINUX...");
                        QProcess::execute("syslinux", {partitionPath});
                        QProcess::execute("dd", {"if=/usr/lib/syslinux/mbr/mbr.bin",
                                                 "of=" + devicePath,
                                                 "bs=440", "count=1"});
                    }

                    if (createExtended) {
                        QFile autorun(mountTargetDir + "/autorun.inf");
                        if (autorun.open(QIODevice::WriteOnly)) {
                            QTextStream ts(&autorun);
                            ts << "[Autorun]\nLABEL=POKOYO\nICON=icon.ico\n";
                        }
                    }

                    QMessageBox::information(this, "Success",
                                             "ISO written successfully (ISO mode).");
                }

                QProcess::execute("sync");
                QProcess::execute("umount", {"-lf", mountIsoDir});
                QProcess::execute("umount", {"-lf", mountTargetDir});
                QDir(tmpBase).removeRecursively();

                ui->btnStart->setEnabled(true);
                rsync->deleteLater();
            });

    rsync->start("rsync", rsyncArgs);
}


void MainWindow::on_btnClose_clicked()
{
    this->close();
}


void MainWindow::on_comboDevice_activated(int index)
{
    if(index < 0 || index >= usbDevices.size())
        return;

    const UsbDevice &dev = usbDevices[index];

    qDebug() << "Selected:";
    qDebug() << "Path:" << "/dev/" + dev.name;
    qDebug() << "Size:" << dev.size;
    qDebug() << "Model:" << dev.model;
}


void MainWindow::loadUsbDevices()
{
    usbDevices.clear();
    ui->comboDevice->clear();

    QProcess process;
    process.start("lsblk", QStringList() << "-J" << "-o"
                                         << "NAME,SIZE,MODEL,RM,TRAN");
    process.waitForFinished();

    QByteArray data = process.readAllStandardOutput();

    qDebug() << "RAW JSON:";
    qDebug().noquote() << data;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject rootObj = doc.object();
    QJsonArray devices = rootObj["blockdevices"].toArray();

    for(const QJsonValue &val : devices)
    {
        QJsonObject obj = val.toObject();

        QString tran = obj["tran"].toString();
        QString name = obj["name"].toString();
        QString size = obj["size"].toString();
        QString model = obj["model"].toString();

        qDebug() << "Device:" << name
                 << "tran:" << tran;

        // filter USB
        if(tran != "usb")
            continue;

        UsbDevice device;
        device.name  = name;
        device.size  = size;
        device.model = model;

        usbDevices.append(device);

        QString displayText =
            model + " (" +
            size + ")  [/dev/" +
            name + "]";

        ui->comboDevice->addItem(displayText);
    }

    qDebug() << "Found USB devices:" << usbDevices.size();
}


void MainWindow::on_btnRefresh_clicked()
{
    loadUsbDevices();
}


void MainWindow::on_comboBootType_activated(int index)
{
    if(index < 0 || index >= ui->comboBootType->count())
        return;

    QString selectedFile = ui->comboBootType->itemText(index);
    ui->editImagePath->setText(selectedFile);

    qDebug() << "Selected ISO:" << selectedFile;
}


void MainWindow::on_btnSelectImage_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open ISO"), "/home/user/Downloads", tr("ISO (*.iso)"));
    ui->comboBootType->addItem(fileName);

}



void MainWindow::on_editImagePath_returnPressed()
{
    QString InputFileName = ui->editImagePath->text();
    ui->comboBootType->addItem(InputFileName);
}

