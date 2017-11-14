#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QObject>

class NetworkManager : public QObject {
    Q_OBJECT
public:
    NetworkManager(QObject* parent = 0);
    bool sendDiff(QString diff);
private slots:
    void onPostAnswer(QNetworkReply* reply);

private:
    QByteArray stringToJson(QString diff);
    QNetworkAccessManager* networkManager;
    bool writeData(QByteArray data);
    bool connectToHost(QString host);
    QByteArray IntToArray(qint32 source);

private:
    QString serviceUrl = "localhost";
    QTcpSocket* socket;
};

#endif // NETWORKMANAGER_H
