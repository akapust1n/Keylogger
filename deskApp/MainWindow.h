#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QByteArray>
#include <QDebug>
#include <QMainWindow>
#include <QObject>
#include <QTcpSocket>
#include <QtNetwork>

namespace Ui {
class MainWindow;
}

class QTcpServer;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

signals:
    void dataReceived(QByteArray);

private slots:
    void newConnection();
    void disconnected();
    void readyRead();
    void newData(QByteArray data);

private:
    Ui::MainWindow* ui;
    QTcpServer* server;
    QHash<QTcpSocket*, QByteArray*> buffers; //We need a buffer to store data until block has completely received
    QHash<QTcpSocket*, qint32*> sizes; //We need to store the size to verify if a block has received completely
private:
    qint32 ArrayToInt(QByteArray source);
};

#endif // MAINWINDOW_H
