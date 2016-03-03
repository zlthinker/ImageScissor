#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ImageViewer imageViewer;
#if defined(Q_OS_SYMBIAN)
    imageViewer.showMaximized();
#else
    imageViewer.show();
#endif
    return app.exec();
}
