#include "mainwindow.h"


ImageViewer::ImageViewer()
{
    imageLabel = new QLabel;
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(true);

    scrollArea = new QScrollArea;
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    setCentralWidget(scrollArea);

    createActions();
    createMenus();

    setWindowTitle(tr("Image Viewer"));
    resize(500, 400);

}

void ImageViewer::enableMouseTrack(bool enable)
{
    imageLabel->setMouseTracking(enable);
    scrollArea->setMouseTracking(enable);
    setMouseTracking(enable);
}

void ImageViewer::redraw()
{
    QPen paintpen(Qt::red);
    paintpen.setWidth(5);
    painter->setPen(paintpen);
    for (int i = 0; i < points.size(); i++)
    {
        painter->drawPoint(points[i]);
    }
    paintpen.setWidth(0);
    painter->setPen(paintpen);
    for (int i = 0; i < points.size() - 1; i++)
    {
        painter->drawLine(points[i], points[i + 1]);
    }
}

void ImageViewer::open()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                    tr("Open File"), QDir::currentPath());
    if (!fileName.isEmpty()) {
        qImage = new QImage(fileName);
        originImage = new QImage(fileName);
        confirmedImage = new QImage(fileName);
        if (qImage->isNull()) {
            QMessageBox::information(this, tr("Image Viewer"),
                                     tr("Cannot load %1.").arg(fileName));
            return;
        }
        painter = new QPainter(qImage);
        imageLabel->setPixmap(QPixmap::fromImage(*qImage));
        scaleFactor = 1.0;

        printAct->setEnabled(true);
        saveAct->setEnabled(true);
        fitToWindowAct->setEnabled(true);
        deselectAct->setEnabled(true);
        undoAct->setEnabled(true);
        updateActions();

        if (!fitToWindowAct->isChecked())
            imageLabel->adjustSize();

        imageFile = fileName.toStdString();
        imageMat = qimage_to_mat_cpy(*qImage, CV_8UC1);
        for (int x = 0; x < imageMat.cols; x++)
        {
            for (int y = 0; y < imageMat.rows; y++)
            {
                cv::Vec3b intensity = imageMat.at<cv::Vec3b>(y, x);
//                qDebug() << x << ", " << y << ": " << intensity[0] << ", " << intensity[1] << ", " << intensity[2];
            }
        }

        enableMouseTrack(true);

        iplImage = cvLoadImage(imageFile.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
        heapNode = new HeapNode[imageMat.rows * imageMat.cols + 1];
        if (!heapNode)
        {
            qDebug() << "Fail to alloct memory to heapnode.";
            return;
        }
    }
}

void ImageViewer::print()
{
    Q_ASSERT(imageLabel->pixmap());
#ifndef QT_NO_PRINTER
    QPrintDialog dialog(&printer, this);
    if (dialog.exec()) {
        QRect rect = painter->viewport();
        QSize size = imageLabel->pixmap()->size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter->setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter->setWindow(imageLabel->pixmap()->rect());
        painter->drawPixmap(0, 0, *imageLabel->pixmap());
    }
#endif
}

void ImageViewer::save()
{
    if (!qImage->isNull())
    {
        QString fileName = QFileDialog::getSaveFileName(this,
                                        tr("Save as ..."), QDir::currentPath(), tr("Images (*.png *.xpm *.jpg)"));
        if(!qImage->save(fileName))
        {
            qDebug() << "Fail to save " << fileName << "\n";
        }
    }
}

void ImageViewer::zoomIn()
{
    scaleImage(1.25);
}

void ImageViewer::zoomOut()
{
    scaleImage(0.8);
}

void ImageViewer::normalSize()
{
    imageLabel->adjustSize();
    scaleFactor = 1.0;
}

void ImageViewer::fitToWindow()
{
    bool fitToWindow = fitToWindowAct->isChecked();
    scrollArea->setWidgetResizable(fitToWindow);
    if (!fitToWindow) {
        normalSize();
    }
    updateActions();
}

void ImageViewer::about()
{
    QMessageBox::about(this, tr("About Image Viewer"),
            tr("<p>The <b>Image Viewer</b> example shows how to combine QLabel "
               "and QScrollArea to display an image. QLabel is typically used "
               "for displaying a text, but it can also display an image. "
               "QScrollArea provides a scrolling view around another widget. "
               "If the child widget exceeds the size of the frame, QScrollArea "
               "automatically provides scroll bars. </p><p>The example "
               "demonstrates how QLabel's ability to scale its contents "
               "(QLabel::scaledContents), and QScrollArea's ability to "
               "automatically resize its contents "
               "(QScrollArea::widgetResizable), can be used to implement "
               "zooming and scaling features. </p><p>In addition the example "
               "shows how to use QPainter to print an image.</p>"));
}

void ImageViewer::createActions()
{
    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcut(tr("Ctrl+O"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    printAct = new QAction(tr("&Print..."), this);
    printAct->setShortcut(tr("Ctrl+P"));
    printAct->setEnabled(false);
    connect(printAct, SIGNAL(triggered()), this, SLOT(print()));

    saveAct = new QAction(tr("&Save..."), this);
    saveAct->setShortcut(tr("Ctrl+S"));
    saveAct->setEnabled(false);
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    zoomInAct = new QAction(tr("Zoom &In (25%)"), this);
    zoomInAct->setShortcut(tr("Ctrl++"));
    zoomInAct->setEnabled(false);
    connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));

    zoomOutAct = new QAction(tr("Zoom &Out (25%)"), this);
    zoomOutAct->setShortcut(tr("Ctrl+-"));
    zoomOutAct->setEnabled(false);
    connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));

    normalSizeAct = new QAction(tr("&Normal Size"), this);
    normalSizeAct->setEnabled(false);
    connect(normalSizeAct, SIGNAL(triggered()), this, SLOT(normalSize()));

    fitToWindowAct = new QAction(tr("&Fit to Window"), this);
    fitToWindowAct->setEnabled(false);
    fitToWindowAct->setCheckable(true);
    fitToWindowAct->setShortcut(tr("Ctrl+F"));
    connect(fitToWindowAct, SIGNAL(triggered()), this, SLOT(fitToWindow()));

    deselectAct = new QAction(tr("&Deselect"), this);
    deselectAct->setShortcut(tr("Ctrl+D"));
    deselectAct->setEnabled(false);
    connect(deselectAct, SIGNAL(triggered()), this, SLOT(deselect()));

    undoAct = new QAction(tr("&Undo"), this);
    undoAct->setShortcut(tr("Ctrl+Z"));
    undoAct->setEnabled(false);
    connect(undoAct, SIGNAL(triggered()), this, SLOT(undo()));

    aboutAct = new QAction(tr("&About"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}

void ImageViewer::createMenus()
{
    fileMenu = new QMenu(tr("&File"), this);
    fileMenu->addAction(openAct);
    fileMenu->addAction(printAct);
    fileMenu->addAction(saveAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    viewMenu = new QMenu(tr("&View"), this);
    viewMenu->addAction(zoomInAct);
    viewMenu->addAction(zoomOutAct);
    viewMenu->addAction(normalSizeAct);
    viewMenu->addSeparator();
    viewMenu->addAction(fitToWindowAct);

    toolMenu = new QMenu(tr("&Tool"), this);
    toolMenu->addAction(deselectAct);
    toolMenu->addAction(undoAct);

    helpMenu = new QMenu(tr("&Help"), this);
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(viewMenu);
    menuBar()->addMenu(toolMenu);
    menuBar()->addMenu(helpMenu);
}

void ImageViewer::updateActions()
{
    zoomInAct->setEnabled(!fitToWindowAct->isChecked());
    zoomOutAct->setEnabled(!fitToWindowAct->isChecked());
    normalSizeAct->setEnabled(!fitToWindowAct->isChecked());
}

void ImageViewer::scaleImage(double factor)
{
    Q_ASSERT(imageLabel->pixmap());
    scaleFactor *= factor;
    imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());

    adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(scrollArea->verticalScrollBar(), factor);

    zoomInAct->setEnabled(scaleFactor < 3.0);
    zoomOutAct->setEnabled(scaleFactor > 0.333);
}

void ImageViewer::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
    scrollBar->setValue(int(factor * scrollBar->value()
                            + ((factor - 1) * scrollBar->pageStep()/2)));
}

bool ImageViewer::mouseOnImage(QPoint & p, int x, int y)
{
    p.setX((x + scrollArea->horizontalScrollBar()->value()) / scaleFactor);
    p.setY(((y - menuBar()->size().height()) + scrollArea->verticalScrollBar()->value()) / scaleFactor);
    if (p.x() > qImage->width() | p.y() > qImage->height())
    {
        return false;
    }
    return true;
}

void ImageViewer:: mousePressEvent(QMouseEvent *e)
{
    if(qImage == NULL || imageFile.length() == 0 || closed)
    {
        return;
    }

    if(e->button() == Qt::LeftButton)
    {
        QPoint p1;

        if (!mouseOnImage(p1, e->x(), e->y()))
        {
            qDebug() << "press at " << p1.x() << ", " << p1.y();
            return;
        }
        QPen paintpen(Qt::red);
        paintpen.setWidth(5);
        painter->setPen(paintpen);
        painter->drawPoint(p1);
//        if (!points.empty())
//        {
//            QPoint p0 = points.back();
//            paintpen.setWidth(0);
//            painter->setPen(paintpen);
//            painter->drawLine(p0, p1);;

//        }
        points.push_back(p1);
        calGraph(iplImage, heapNode, p1.y(), p1.x());
        drawPath(p1.x(), p1.y());
        if (confirmedImage)
        {
            free(confirmedImage);
        }
        confirmedImage = new QImage(*qImage);

        imageLabel->setPixmap(QPixmap::fromImage(*qImage));
    }
    else if(e->button() == Qt::RightButton)
    {
        if (points.size() > 2)
        {
            enableMouseTrack(false);
            QPoint first = points[0];
            QPoint last = points.back();
            clearPainting();
            calGraph(iplImage, heapNode, first.y(), first.x());
            drawPath(last.x(), last.y());
            imageLabel->setPixmap(QPixmap::fromImage(*qImage));
            closed = true;
        }
    }
}

void ImageViewer::mouseMoveEvent(QMouseEvent * e)
{
    if (closed)
    {
        return;
    }
//    qDebug() << "In mouseMoveEvent.";
//    qDebug() << "point size: " << points.size();
    if(points.size() > 0)
    {
        QPoint last = points.back();
        QPoint p1;
        if (!mouseOnImage(p1, e->x(), e->y()))
        {
            return;
        }
//        qDebug() << "ex = " << e->x() << ", ey = " << e->y();
        qDebug() << "x = " << p1.x() << ", y = " << p1.y() << ", index = " << p1.x() + p1.y() * qImage->width() << ", total = " << qImage->width() * qImage->height();
        clearPainting();
//        redraw();
//        QPen paintpen(Qt::red);
//        paintpen.setWidth(0);
//        painter->setPen(paintpen);
//        painter->drawLine(last, p1);
        drawPath(p1.x(), p1.y());
        imageLabel->setPixmap(QPixmap::fromImage(*qImage));
    }
}

void ImageViewer::mouseDoubleClickEvent(QMouseEvent * e)
{
    if ( e->button() == Qt::LeftButton )
    {

    }
}

void ImageViewer::clearPainting()
{
    free(qImage);
    free(painter);
    qImage = new QImage(*confirmedImage);
    painter  = new QPainter(qImage);
}

void ImageViewer::deselect()
{
    free(qImage);
    free(painter);
    qImage = new QImage(*originImage);
    painter  = new QPainter(qImage);
    points.clear();
    imageLabel->setPixmap(QPixmap::fromImage(*qImage));
    enableMouseTrack(true);
    closed = false;
}

void ImageViewer::undo()
{
    if (!points.empty())
    {
        points.pop_back();
    }
    qDebug() << "point size: " << points.size();
    clearPainting();
    if (points.empty())
    {
        imageLabel->setPixmap(QPixmap::fromImage(*qImage));
        return;
    }
    redraw();
    imageLabel->setPixmap(QPixmap::fromImage(*qImage));
}

cv::Mat ImageViewer::qimage_to_mat_cpy(QImage const &img, int format)
{
    return cv::Mat(img.height(), img.width(), format,
                   const_cast<uchar*>(img.bits()),
                   img.bytesPerLine()).clone();
}

void ImageViewer::drawPath(int x, int y)
{
    QPen paintpen(Qt::red);
    paintpen.setWidth(0);
    painter->setPen(paintpen);

    x = std::min(x, qImage->width() - 1);
    y = std::min(y, qImage->height() - 1);
    qDebug() << "In drawPath: x = " << x << ", y = " << y << ", index = " << x + y * qImage->width() << ", total = " << qImage->width() * qImage->height();
    int finalIndex = y * qImage->width() + x;
    HeapNode * tempNode = &heapNode[finalIndex];
    while (tempNode->GetPreNode())
    {
        HeapNode * parent = tempNode->GetPreNode();
        QPoint p1(parent->GetColumn(), parent->GetRow());
        QPoint p2(tempNode->GetColumn(), tempNode->GetRow());
        painter->drawLine(p1, p2);
        tempNode = parent;
    }
}
