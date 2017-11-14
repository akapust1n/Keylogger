#ifndef OBSERVER_H
#define OBSERVER_H
#include <NetworkManager.h>
#include <QObject>
#include <fstream>
class Observer : public QObject {
    Q_OBJECT
public:
    Observer(QObject* parent = 0);

private slots:
    void check();

private:
    std::string s2;
    NetworkManager manager;
};

#endif // OBSERVER_H
