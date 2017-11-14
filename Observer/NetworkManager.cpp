#include "NetworkManager.h"
#include <QDataStream>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
{

    socket = new QTcpSocket(this);
}

bool NetworkManager::sendDiff(QString diff)
{
    auto pp = connectToHost(serviceUrl);

    QByteArray body = stringToJson(diff);
    //std::cout << "RESULT" << ;
    bool dd = writeData(body);
    std::cout << "RESULT: " << dd << std::endl;
    //std::cout << body.toStdString();
    // std::cout << "bodysize " << body.size() << std::endl;
    //  std::cout << diff.toStdString();
    //networkManager->post(networkRequest, QByteArray::fromStdString("ddddddddddddddddddddd"));
}

void NetworkManager::onPostAnswer(QNetworkReply* reply)
{
}

QByteArray NetworkManager::stringToJson(QString diff)
{
    //std::cout << diff.toStdString() << std::endl;
    QStringList list1 = diff.split('\n');
    std::cout << "size: " << list1.size() << std::endl;
    QJsonArray array = QJsonArray::fromStringList(list1);

    QJsonObject object;
    object.insert("keys", array);
    QJsonDocument doc(object);
    QByteArray result = doc.toJson();
    return result;
}
bool NetworkManager::connectToHost(QString host)
{
    socket->connectToHost(host, 8080);
    return socket->waitForConnected();
}

QByteArray NetworkManager::IntToArray(qint32 source) //Use qint32 to ensure that the number have 4 bytes
{
    //Avoid use of cast, this is the Qt way to serialize objects
    QByteArray temp;
    QDataStream data(&temp, QIODevice::ReadWrite);
    data << source;
    return temp;
}
bool NetworkManager::writeData(QByteArray data)
{

    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(IntToArray(data.size())); //write size of data
        socket->write(data); //write the data itself
        return socket->waitForBytesWritten();
    } else
        return false;
}
