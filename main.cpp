#include "jsonlineseditor.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    JsonLinesEditor w;
    w.show();
    return a.exec();
}
