#include <Observer.h>
#include <QCoreApplication>
#include <iostream>

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);
    std::cout << "Hello\n";
    Observer observer(&a);
    //
    return a.exec();
}
