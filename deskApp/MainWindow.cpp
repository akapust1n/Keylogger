#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <iostream>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    server = new QTcpServer(this);
    connect(server, SIGNAL(newConnection()), SLOT(newConnection()));
    connect(server, SIGNAL(dataReceived(QByteArray data)), SLOT(newData(QByteArray data)));
    qDebug() << "Listening:" << server->listen(QHostAddress::Any, 8080);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::newConnection()
{
    while (server->hasPendingConnections()) {
        QTcpSocket* socket = server->nextPendingConnection();
        connect(socket, SIGNAL(readyRead()), SLOT(readyRead()));
        connect(socket, SIGNAL(disconnected()), SLOT(disconnected()));
        QByteArray* buffer = new QByteArray();
        qint32* s = new qint32(0);
        buffers.insert(socket, buffer);
        sizes.insert(socket, s);
    }
}

void MainWindow::disconnected()
{
    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
    QByteArray* buffer = buffers.value(socket);
    qint32* s = sizes.value(socket);
    socket->deleteLater();
    delete buffer;
    delete s;
}

void MainWindow::readyRead()
{
    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
    QByteArray* buffer = buffers.value(socket);
    qint32* s = sizes.value(socket);
    qint32 size = *s;
    while (socket->bytesAvailable() > 0) {
        buffer->append(socket->readAll());
        while ((size == 0 && buffer->size() >= 4) || (size > 0 && buffer->size() >= size)) //While can process data, process it
        {
            if (size == 0 && buffer->size() >= 4) //if size of data has received completely, then store it on our global variable
            {
                size = ArrayToInt(buffer->mid(0, 4));
                *s = size;
                buffer->remove(0, 4);
            }
            if (size > 0 && buffer->size() >= size) // If data has received completely, then emit our SIGNAL with the data
            {
                QByteArray data = buffer->mid(0, size);
                buffer->remove(0, size);
                size = 0;
                *s = size;
                emit dataReceived(data);
            }
        }
    }
}

void MainWindow::newData(QByteArray data)
{
    std::cout << data.toStdString();
}
qint32 MainWindow::ArrayToInt(QByteArray source)
{
    qint32 temp;
    QDataStream data(&source, QIODevice::ReadWrite);
    data >> temp;
    return temp;
}

//void MainWindow::slotReadClient()
//{
//    QTcpSocket* clientSocket = (QTcpSocket*)sender();
//    int idusersocs = clientSocket->socketDescriptor();
//    QTextStream os(clientSocket);
//    os.setAutoDetectUnicode(true);
//    os << "HTTP/1.0 200 Ok\r\n"
//          "Content-Type: text/html; charset=\"utf-8\"\r\n"
//          "\r\n"
//          "<h1>Ok</h1>\n"
//       << QDateTime::currentDateTime().toString() << "\n";
//    clientSocket->waitForReadyRead();
//    auto dd = clientSocket->readAll();
//    qDebug() << QString::fromStdString(dd.toStdString());
//    // Если нужно закрыть сокет
//    clientSocket->close();
//    SClients.remove(idusersocs);
//}
