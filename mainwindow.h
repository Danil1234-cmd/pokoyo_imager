#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

struct UsbDevice
{
    QString name;   // sdb
    QString size;   // 16G
    QString model;  // SanDisk
};


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnStart_clicked();

    void on_btnClose_clicked();

    void on_comboDevice_activated(int index);

    void on_btnRefresh_clicked();

    void on_btnSelectImage_clicked();

    void on_comboBootType_activated(int index);

    void on_editImagePath_returnPressed();

private:
    Ui::MainWindow *ui;

    void loadStyle(const QString &styleName);

    QVector<UsbDevice> usbDevices;
    void loadUsbDevices();

};

#endif
