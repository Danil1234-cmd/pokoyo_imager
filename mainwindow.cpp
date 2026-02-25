#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <libudev.h>
#include <blkid/blkid.h>
#include <sys/wait.h>
#include <vector>
#include <string.h>
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
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <linux/loop.h>
#include <sys/stat.h>
#include <errno.h>
#include <cstring>
#include <QDirIterator>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <sys/sysmacros.h>

static int runCommand(const QStringList &args)
{
    if (args.isEmpty()) return -1;

    // prepare argv
    std::vector<char*> argv;
    for (const QString &s : args) {
        QByteArray b = s.toLocal8Bit();
        char *c = (char*)malloc(b.size()+1);
        memcpy(c, b.constData(), b.size()+1);
        argv.push_back(c);
    }
    argv.push_back(nullptr);

    // find executable path if argv[0] is not absolute
    QString exe = QString::fromLocal8Bit(argv[0]);
    if (!QFile::exists(exe)) {
        QByteArray pathEnv = qgetenv("PATH");
        QStringList parts = QString(pathEnv).split(':');
        parts << "/sbin" << "/usr/sbin" << "/usr/bin" << "/bin";
        bool found = false;
        for (const QString &p : parts) {
            QString candidate = p + "/" + exe;
            if (QFile::exists(candidate) && QFileInfo(candidate).isExecutable()) {
                free(argv[0]);
                QByteArray cb = candidate.toLocal8Bit();
                char *c = (char*)malloc(cb.size()+1);
                memcpy(c, cb.constData(), cb.size()+1);
                argv[0] = c;
                found = true;
                break;
            }
        }
        if (!found) {
            // keep original argv[0]
        }
    }

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        // fallback to no capture
        pid_t pid = fork();
        if (pid == 0) {
            execvp(argv[0], argv.data());
            _exit(127);
        }
        int status = -1;
        if (pid > 0) waitpid(pid, &status, 0);
        for (char *c : argv) if (c) free(c);
        if (WIFEXITED(status)) return WEXITSTATUS(status);
        return -1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // child: redirect stdout and stderr to pipe
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execvp(argv[0], argv.data());
        _exit(127);
    }

    // parent
    close(pipefd[1]);
    QByteArray output;
    char buf[4096];
    ssize_t n;
    // read until EOF
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
        output.append(buf, n);
    }
    close(pipefd[0]);

    int status = -1;
    if (pid > 0) waitpid(pid, &status, 0);

    for (char *c : argv) if (c) free(c);

    if (!output.isEmpty()) {
        qDebug().noquote() << output;
    }

    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}


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

static bool getBlockDeviceSizeBytes(const QString &device, unsigned long long &outSize)
{
    int fd = open(device.toLocal8Bit().constData(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;
    unsigned long long size = 0;
    if (ioctl(fd, BLKGETSIZE64, &size) != 0) {
        close(fd);
        return false;
    }
    close(fd);
    outSize = size;
    return true;
}

static bool tryUmount(const QString &target)
{
    const char *p = target.toLocal8Bit().constData();
    // MNT_DETACH == lazy unmount
    if (umount2(p, MNT_DETACH) != 0) {
        // try regular umount
        if (umount(p) != 0) {
            return false;
        }
    }
    return true;
}

// Setup loop device for ISO and mount it read-only. Returns loop device path on success.
static QString setupLoopAndMount(const QString &isoPath, const QString &mountPoint, QString &outLoopDev)
{
    int isoFd = open(isoPath.toLocal8Bit().constData(), O_RDONLY | O_CLOEXEC);
    if (isoFd < 0) return QString();

    int ctl = open("/dev/loop-control", O_RDWR | O_CLOEXEC);
    if (ctl < 0) { close(isoFd); return QString(); }

    int loopNum = ioctl(ctl, LOOP_CTL_GET_FREE);
    close(ctl);
    if (loopNum < 0) { close(isoFd); return QString(); }

    QString loopDev = QString("/dev/loop%1").arg(loopNum);
    int loopFd = open(loopDev.toLocal8Bit().constData(), O_RDWR | O_CLOEXEC);
    if (loopFd < 0) {
        qDebug() << "failed to open" << loopDev << ":" << strerror(errno);
        // try create node fallback; determine major from /dev/loop0 if available
        dev_t devnum = makedev(7, loopNum);
        struct stat st0;
        if (stat("/dev/loop0", &st0) == 0) {
            dev_t base = st0.st_rdev;
            unsigned major_nr = major(base);
            devnum = makedev(major_nr, loopNum);
        }
        if (mknod(loopDev.toLocal8Bit().constData(), S_IFBLK | 0660, devnum) == 0) {
            qDebug() << "created loop device node" << loopDev;
            loopFd = open(loopDev.toLocal8Bit().constData(), O_RDWR | O_CLOEXEC);
        } else {
            qDebug() << "mknod failed for" << loopDev << ":" << strerror(errno);
        }

        if (loopFd < 0) {
            close(isoFd);
            return QString();
        }
    }

    if (ioctl(loopFd, LOOP_SET_FD, isoFd) < 0) {
        qDebug() << "LOOP_SET_FD failed:" << strerror(errno);
        close(loopFd);
        close(isoFd);
        return QString();
    }

    struct loop_info64 li;
    memset(&li, 0, sizeof(li));
    li.lo_flags = LO_FLAGS_READ_ONLY;
    if (ioctl(loopFd, LOOP_SET_STATUS64, &li) < 0) {
        // ignore non-fatal
    }

    close(loopFd);
    close(isoFd);

    // Try mounting with common ISO filesystem types, then fallback to auto-detect.
    const char *typesToTry[] = {"iso9660", "udf", nullptr};
    bool mounted = false;
    for (const char **t = typesToTry; *t != nullptr; ++t) {
        if (mount(loopDev.toLocal8Bit().constData(), mountPoint.toLocal8Bit().constData(), *t, MS_RDONLY, nullptr) == 0) {
            mounted = true;
            break;
        } else {
            qDebug() << "mount(" << loopDev << "," << mountPoint << ", type=" << *t << ") failed:" << strerror(errno);
        }
    }
    if (!mounted) {
        // try auto-detect
        if (mount(loopDev.toLocal8Bit().constData(), mountPoint.toLocal8Bit().constData(), nullptr, MS_RDONLY, nullptr) == 0) {
            mounted = true;
        } else {
            qDebug() << "mount(auto) failed for" << loopDev << "->" << mountPoint << ":" << strerror(errno);
        }
    }

    if (!mounted) {
        int lfd = open(loopDev.toLocal8Bit().constData(), O_RDWR | O_CLOEXEC);
        if (lfd >= 0) {
            ioctl(lfd, LOOP_CLR_FD);
            close(lfd);
        }
        return QString();
    }

    outLoopDev = loopDev;
    return loopDev;
}

static bool mountDeviceNode(const QString &devNode, const QString &mountPoint, QString &errOut)
{
    const char *node = devNode.toLocal8Bit().constData();
    const char *mp = mountPoint.toLocal8Bit().constData();

    QStringList types = {"vfat", "ntfs", "exfat", "iso9660", "udf", ""};
    QList<QString> errors;

    for (const QString &t : types) {
        int flags = 0;
        const char *type = nullptr;
        if (!t.isEmpty()) type = t.toLocal8Bit().constData();
        if (mount(node, mp, type, flags, nullptr) == 0) {
            return true;
        } else {
            int e = errno;
            QString msg = QString("mount(%1, %2, type=%3) failed: %4 (%5)")
                    .arg(devNode).arg(mountPoint).arg(t.isEmpty() ? QString("auto") : t)
                    .arg(QString::fromLocal8Bit(strerror(e))).arg(e);
            qDebug() << msg;
            errors.append(msg);
        }
    }

    errOut = errors.join("\n");
    return false;
}

static quint64 computeDirSize(const QString &dirPath)
{
    quint64 total = 0;
    QDirIterator it(dirPath, QDir::Files | QDir::NoSymLinks | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();
        if (fi.isFile()) total += fi.size();
    }
    return total;
}

static bool copyWithProgress(const QString &srcDir, const QString &dstDir, std::function<void(int)> progressCb)
{
    quint64 total = computeDirSize(srcDir);
    quint64 copied = 0;
    if (!QDir(dstDir).exists()) QDir().mkpath(dstDir);

    QDirIterator it(srcDir, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString srcPath = it.next();
        QFileInfo fi(srcPath);
        QString rel = QDir(srcDir).relativeFilePath(srcPath);
        QString destPath = QDir(dstDir).filePath(rel);

        if (fi.isDir()) {
            QDir().mkpath(destPath);
            continue;
        }

        QFile in(srcPath);
        QFile out(destPath);
        if (!in.open(QIODevice::ReadOnly)) return false;
        if (!out.open(QIODevice::WriteOnly)) { in.close(); return false; }

        const qint64 bufSize = 1 << 20; // 1MB
        QByteArray buf;
        buf.resize(bufSize);
        while (!in.atEnd()) {
            qint64 r = in.read(buf.data(), bufSize);
            if (r <= 0) break;
            qint64 w = out.write(buf.constData(), r);
            if (w != r) { in.close(); out.close(); return false; }
            copied += r;
            if (total > 0 && progressCb) {
                int p = int((copied * 100) / total);
                progressCb(p);
            }
        }

        in.close();
        out.close();
        QFile::setPermissions(destPath, fi.permissions());
        struct stat st;
        if (stat(srcPath.toLocal8Bit().constData(), &st) == 0) {
            struct timespec times[2];
            times[0] = st.st_atim;
            times[1] = st.st_mtim;
            utimensat(AT_FDCWD, destPath.toLocal8Bit().constData(), times, 0);
        }
    }

    if (progressCb) progressCb(100);
    return true;
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

    unsigned long long deviceSizeULL = 0;
    if (!getBlockDeviceSizeBytes(devicePath, deviceSizeULL)) {
        QMessageBox::critical(this, "Error", "Failed to get device size.");
        return;
    }
    qint64 deviceSize = (qint64)deviceSizeULL;

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
    tryUmount(devicePath);
    // note: wipefs is not invoked here; if you need to clear signatures,
    // consider using libparted or libblkid APIs instead of calling wipefs.

    // Partition table
    QString mklabel = (partitionScheme == "GPT") ? "gpt" : "msdos";
    if (runCommand({"parted", "-s", devicePath, "mklabel", mklabel}) != 0)
        return;

    if (runCommand({"parted", "-s", devicePath, "mkpart", "primary", "0%", "100%"}) != 0)
        return;

    runCommand({"partprobe", devicePath});
    QThread::msleep(500);

    QString partitionPath = devicePath + "1";

    // Format
    if (fsType == "FAT32")
        runCommand({"mkfs.vfat", "-F", "32", partitionPath});
    else if (fsType == "NTFS")
        runCommand({"mkfs.ntfs", "-F", partitionPath});
    else if (fsType == "exFAT")
        runCommand({"mkfs.exfat", partitionPath});

    // Mount paths
    QString tmpBase = QDir::tempPath() + "/pokoyo_imager";
    QString mountIsoDir = tmpBase + "/iso_mount";
    QString mountTargetDir = tmpBase + "/target_mount";

    tryUmount(mountIsoDir);
    tryUmount(mountTargetDir);

    QDir().mkpath(mountIsoDir);
    QDir().mkpath(mountTargetDir);

    QString loopDev;
    if (setupLoopAndMount(isoPath, mountIsoDir, loopDev).isEmpty()) {
        QMessageBox::critical(this, "Error", "ISO mount failed.");
        return;
    }

    QString mountErr;
    if (!mountDeviceNode(partitionPath, mountTargetDir, mountErr)) {
        ui->textLog->append("Target mount failed:\n" + mountErr);
        QMessageBox::critical(this, "Error", "Target mount failed:\n" + mountErr);
        // try to unmount ISO
        tryUmount(mountIsoDir);
        QDir(tmpBase).removeRecursively();
        return;
    }

    log("Starting copy...");

    // Copy in background and update progress via callback
    QFuture<bool> future = QtConcurrent::run([=]() {
        return copyWithProgress(mountIsoDir, mountTargetDir, [&](int p){
            QMetaObject::invokeMethod(this, [=]() {
                ui->progressBar->setValue(p);
            });
        });
    });

    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [=]() {
        bool ok = watcher->result();
        if (!ok) {
            QMessageBox::critical(this, "Error", "Copy failed.");
        } else {
            log("Copy complete.");

            if (partitionScheme == "MBR" && fsType == "FAT32") {
                log("Installing SYSLINUX...");
                runCommand({"syslinux", partitionPath});
                runCommand({"dd", "if=/usr/lib/syslinux/mbr/mbr.bin", QString("of=%1").arg(devicePath), "bs=440", "count=1"});
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

        sync();
        tryUmount(mountIsoDir);
        tryUmount(mountTargetDir);
        QDir(tmpBase).removeRecursively();

        ui->btnStart->setEnabled(true);
        watcher->deleteLater();
    });

    watcher->setFuture(future);
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

    struct udev *udev = udev_new();
    if (!udev) return;

    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    udev_list_entry_foreach(entry, devices) {
        const char *path = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev, path);
        if (!dev) continue;

        const char *devtype = udev_device_get_devtype(dev);
        if (!devtype || strcmp(devtype, "disk") != 0) {
            udev_device_unref(dev);
            continue;
        }

        struct udev_device *parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if (!parent) {
            udev_device_unref(dev);
            continue;
        }

        const char *devnode = udev_device_get_devnode(dev);
        if (!devnode) { udev_device_unref(dev); continue; }

        QString name = QFileInfo(QString(devnode)).fileName();
        unsigned long long sizeBytes = 0;
        getBlockDeviceSizeBytes(QString(devnode), sizeBytes);

        const char *model = udev_device_get_sysattr_value(parent, "product");
        if (!model) model = udev_device_get_property_value(dev, "ID_MODEL");

        UsbDevice device;
        device.name = name;
        device.size = QString::number(sizeBytes);
        device.model = model ? QString(model) : QString("USB Disk");

        usbDevices.append(device);

        QString displayText = device.model + " (" + device.size + ")  [/dev/" + device.name + "]";
        ui->comboDevice->addItem(displayText);

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);

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

