#include "mytcpserver.h"
#include <iostream>
MyTcpServer::MyTcpServer(QObject* parent)
    : QObject(parent)
{
    server = new QTcpServer(this);

    // whenever a user connects, it will emit signal
    connect(server, SIGNAL(newConnection()),
        this, SLOT(newConnection()));

    if (!server->listen(QHostAddress::Any, 2325)) {
        qDebug() << "Server could not start";
    } else {
        qDebug() << "Server started!";
    }
}

void MyTcpServer::newConnection()
{

    // need to grab the socket
    QTcpSocket* socket = server->nextPendingConnection();
    std::cout << "CONNECTION" << std::endl;

    socket->write("Hello client\r\n");
    socket->flush();

    socket->waitForBytesWritten(3000);

    socket->close();
}
