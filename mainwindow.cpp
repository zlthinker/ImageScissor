#include "mainwindow.h"


ImageScissor::ImageScissor()
{
    imageLabel = new QLabel;
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(true);

    scrollArea = new QScrollArea;
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    setCentralWidget(scrollArea);

    qImage = NULL;
    originImage = NULL;
    mask = NULL;
    iplImage = NULL;
    heapNode = NULL;
    painter = NULL;

    createActions();
    createMenus();

    setWindowTitle(tr("Image Viewer"));
    resize(500, 400);

}

void ImageScissor::enableMouseTrack(bool enable)
{
    imageLabel->setMouseTracking(enable);
    scrollArea->setMouseTracking(enable);
    setMouseTracking(enable);
}

void ImageScissor::redraw()
{
    QPen paintpen(Qt::red);
    paintpen.setWidth(5);
    painter->setPen(paintpen);
    for (int i = 0; i < points.size(); i++)
    {
        painter->drawPoint(points[i]);
    }
    paintpen.setWidth(1);
    painter->setPen(paintpen);
    for (int i = 0; i < contour.size(); i++)
    {
        std::vector<QPoint> cont = contour[i];
        for(int j = 0; j < cont.size() - 1; j++)
        {
            painter->drawLine(cont[j], cont[j + 1]);
        }
    }
}

void ImageScissor::open()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                    tr("Open File"), QDir::currentPath());
    if (!fileName.isEmpty()) {
        if (qImage != NULL)    free(qImage);
        if (originImage != NULL)    free(originImage);
        if (mask != NULL)    free(mask);
        if (painter != NULL)    free(painter);

        qImage = new QImage(fileName);
        originImage = new QImage(fileName);

        if (qImage->isNull()) {
            QMessageBox::information(this, tr("Image Viewer"),
                                     tr("Cannot load %1.").arg(fileName));
            return;
        }
        painter = new QPainter(qImage);
        deselect();
        imageLabel->setPixmap(QPixmap::fromImage(*qImage));
        scaleFactor = 1.0;
        closed = false;

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
//        for (int x = 0; x < imageMat.cols; x++)
//        {
//            for (int y = 0; y < imageMat.rows; y++)
//            {
//                cv::Vec3b intensity = imageMat.at<cv::Vec3b>(y, x);
//                qDebug() << x << ", " << y << ": " << intensity[0] << ", " << intensity[1] << ", " << intensity[2];
//            }
//        }

        enableMouseTrack(true);

//        iplImage = cvLoadImage(imageFile.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
        iplImage = QImageToIplImage(qImage);
        if (!iplImage)
        {
            qDebug() << "IplImage is NULL.";
        }
        heapNode = new HeapNode[imageMat.rows * imageMat.cols + 1];
        if (!heapNode)
        {
            qDebug() << "Fail to alloct memory to heapnode.";
            return;
        }
    }
}

void ImageScissor::print()
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

void ImageScissor::save()
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

void ImageScissor::saveMask()
{
    QPainterPath path;
    path.moveTo(points[0].x(), points[0].y());
    for (int c = 0; c < contour.size(); c++)
    {
        std::vector<QPoint> cont = contour[c];
        for (int p = 0; p < cont.size(); p++)
        {
            path.lineTo(cont[p].x(), cont[p].y());
        }
    }
    path.lineTo(points[0].x(), points[0].y());
    path.lineTo(points[0].x(), points[0].y());

    mask = new QImage(originImage->width(), originImage->height(), QImage::Format_ARGB32);
    mask->fill(QColor(255, 255, 255, 0));
    QPainter *maskPainter = new QPainter(mask);
    maskPainter->fillPath (path, QBrush (Qt::black));


    QImage* saveImage = new QImage(*originImage);
    *saveImage = saveImage->convertToFormat(QImage::Format_ARGB32);
    for ( int row = 0; row < mask->height(); ++row )
    {
        for ( int col = 0; col < mask->width(); ++col )
        {
            QRgb rgb( mask->pixel( col, row ) );
//            qDebug() << "alpha: " << clr.alpha() << ", R:" << clr.red() << ", G:" << clr.green() << ", B:" << clr.blue();
            if (qAlpha(rgb) < 128)
            {
                saveImage->setPixel(col, row, QColor(255, 255, 255, 0).rgba());
            }
        }
    }
    imageLabel->setPixmap(QPixmap::fromImage(*saveImage));

    if (!saveImage->isNull())
    {
        QString fileName = QFileDialog::getSaveFileName(this,
                                        tr("Save as ..."), QDir::currentPath(), tr("Images (*.png *.xpm *.jpg)"));
        if(!saveImage->save(fileName))
        {
            qDebug() << "Fail to save " << fileName << "\n";
        }
    }
    free(saveImage);
}

void ImageScissor::zoomIn()
{
    scaleImage(1.25);
}

void ImageScissor::zoomOut()
{
    scaleImage(0.8);
}

void ImageScissor::normalSize()
{
    imageLabel->adjustSize();
    scaleFactor = 1.0;
}

void ImageScissor::fitToWindow()
{
    bool fitToWindow = fitToWindowAct->isChecked();
    scrollArea->setWidgetResizable(fitToWindow);
    if (!fitToWindow) {
        normalSize();
    }
    updateActions();
}

void ImageScissor::about()
{
    QMessageBox::about(this, tr("About Image Scissor"),
            tr("<p>The <b>Image Scissor</b> is a light-weight "
               "edge manipulation software done by Lei ZHOU and Peng XU. </p>"
                ));
}

void ImageScissor::createActions()
{
    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcut(tr("Ctrl+O"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    printAct = new QAction(tr("&Print..."), this);
    printAct->setShortcut(tr("Ctrl+P"));
    printAct->setEnabled(false);
    connect(printAct, SIGNAL(triggered()), this, SLOT(print()));

    saveAct = new QAction(tr("&Save Contour..."), this);
    saveAct->setShortcut(tr("Ctrl+S"));
    saveAct->setEnabled(false);
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveMaskAct = new QAction(tr("&Save Mask..."), this);
    saveMaskAct->setEnabled(false);
    connect(saveMaskAct, SIGNAL(triggered()), this, SLOT(saveMask()));

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

void ImageScissor::createMenus()
{
    fileMenu = new QMenu(tr("&File"), this);
    fileMenu->addAction(openAct);
    fileMenu->addAction(printAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveMaskAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    viewMenu = new QMenu(tr("&View"), this);
    viewMenu->addAction(zoomInAct);
    viewMenu->addAction(zoomOutAct);
    viewMenu->addAction(normalSizeAct);
//    viewMenu->addSeparator();
//    viewMenu->addAction(fitToWindowAct);

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

void ImageScissor::updateActions()
{
    zoomInAct->setEnabled(!fitToWindowAct->isChecked());
    zoomOutAct->setEnabled(!fitToWindowAct->isChecked());
    normalSizeAct->setEnabled(!fitToWindowAct->isChecked());
}

void ImageScissor::scaleImage(double factor)
{
    Q_ASSERT(imageLabel->pixmap());
    scaleFactor *= factor;
    imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());

    adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(scrollArea->verticalScrollBar(), factor);

    zoomInAct->setEnabled(scaleFactor < 3.0);
    zoomOutAct->setEnabled(scaleFactor > 0.333);
}

void ImageScissor::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
    scrollBar->setValue(int(factor * scrollBar->value()
                            + ((factor - 1) * scrollBar->pageStep()/2)));
}

bool ImageScissor::mouseOnImage(QPoint & p, int x, int y)
{
    p.setX((x + scrollArea->horizontalScrollBar()->value()) / scaleFactor);
    p.setY(((y - menuBar()->size().height()) + scrollArea->verticalScrollBar()->value()) / scaleFactor);
    if (p.x() > qImage->width() | p.y() > qImage->height())
    {
        return false;
    }
    return true;
}

void ImageScissor:: mousePressEvent(QMouseEvent *e)
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
//        QPen paintpen(Qt::red);
//        paintpen.setWidth(5);
//        painter->setPen(paintpen);
//        painter->drawPoint(p1);
//        if (!points.empty())
//        {
//            QPoint p0 = points.back();
//            paintpen.setWidth(0);
//            painter->setPen(paintpen);
//            painter->drawLine(p0, p1);;

//        }

        qDebug() << "point size is " << points.size();
        qDebug() << "press at " << p1.x() << ", " << p1.y();
        if (points.size() > 0)
        {
            std::vector<QPoint> cont;
            addContour(p1.x(), p1.y(), cont);
            contour.push_back(cont);
        }

        points.push_back(p1);
        calGraph(iplImage, heapNode, p1.y(), p1.x());

        redraw();

//        if (confirmedImage)
//        {
//            free(confirmedImage);
//        }
//        confirmedImage = new QImage(*qImage);

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
            std::vector<QPoint> cont;
            addContour(first.x(), first.y(), cont);
            contour.push_back(cont);
            redraw();
            imageLabel->setPixmap(QPixmap::fromImage(*qImage));
            closed = true;
            undoAct->setEnabled(false);
            saveMaskAct->setEnabled(true);
            qDebug() << "Contour is closed, point size is " << points.size() << ", contour size is " << contour.size();
        }
    }
}

void ImageScissor::mouseMoveEvent(QMouseEvent * e)
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
//        qDebug() << "x = " << p1.x() << ", y = " << p1.y() << ", index = " << p1.x() + p1.y() * qImage->width() << ", total = " << qImage->width() * qImage->height();
        clearPainting();
        redraw();
//        QPen paintpen(Qt::red);
//        paintpen.setWidth(0);
//        painter->setPen(paintpen);
//        painter->drawLine(last, p1);
        drawPath(p1.x(), p1.y());
        imageLabel->setPixmap(QPixmap::fromImage(*qImage));
    }
}

void ImageScissor::mouseDoubleClickEvent(QMouseEvent * e)
{
    if ( e->button() == Qt::LeftButton )
    {

    }
}

void ImageScissor::clearPainting()
{
    free(qImage);
    free(painter);
    qImage = new QImage(*originImage);
    painter  = new QPainter(qImage);
}

void ImageScissor::deselect()
{
    free(qImage);
    free(painter);
    qImage = new QImage(*originImage);
    painter  = new QPainter(qImage);
    points.clear();
    contour.clear();
    qDebug() << "contour size is " << contour.size();
    imageLabel->setPixmap(QPixmap::fromImage(*qImage));
    enableMouseTrack(true);
    closed = false;
    undoAct->setEnabled(true);
}

void ImageScissor::undo()
{
    if (!points.empty())
    {
        points.pop_back();
    }
    if (!contour.empty())
    {
        contour.pop_back();
    }
    qDebug() << "point size: " << points.size();
    clearPainting();

    if (points.empty())
    {
        imageLabel->setPixmap(QPixmap::fromImage(*qImage));
        return;
    }
    QPoint last = points.back();
    calGraph(iplImage, heapNode, last.y(), last.x());
    redraw();
    imageLabel->setPixmap(QPixmap::fromImage(*qImage));
}

cv::Mat ImageScissor::qimage_to_mat_cpy(QImage const &img, int format)
{
    return cv::Mat(img.height(), img.width(), format,
                   const_cast<uchar*>(img.bits()),
                   img.bytesPerLine()).clone();
}

void ImageScissor::drawPath(int x, int y)
{
    QPen paintpen(Qt::red);
    paintpen.setWidth(1);
    painter->setPen(paintpen);

    x = std::min(x, qImage->width() - 1);
    y = std::min(y, qImage->height() - 1);
//    qDebug() << "In drawPath: x = " << x << ", y = " << y << ", index = " << x + y * qImage->width() << ", total = " << qImage->width() * qImage->height();
    int finalIndex = y * qImage->width() + x;
    HeapNode * tempNode = &heapNode[finalIndex];
    if (!tempNode->GetPreNode())
    {
        qDebug() << "The parent is NULL.";
    }
    while (tempNode->GetPreNode())
    {
        HeapNode * parent = tempNode->GetPreNode();
        QPoint p1(parent->GetColumn(), parent->GetRow());
        QPoint p2(tempNode->GetColumn(), tempNode->GetRow());
        painter->drawLine(p1, p2);
        tempNode = parent;
    }
}

void ImageScissor::addContour(int x, int y, std::vector<QPoint> &cont)
{
    x = std::min(x, qImage->width() - 1);
    y = std::min(y, qImage->height() - 1);
    int finalIndex = y * qImage->width() + x;
    HeapNode * tempNode = &heapNode[finalIndex];
    if (!tempNode->GetPreNode())
    {
        qDebug() << "In addContour: The parent is NULL.";
    }
    while (tempNode->GetPreNode())
    {
        HeapNode * parent = tempNode->GetPreNode();
        QPoint p1(parent->GetColumn(), parent->GetRow());
        cont.push_back(p1);
        tempNode = parent;
    }
    std::reverse(cont.begin(), cont.end());
//    QPoint first = cont[0];
//    QPoint last = cont.back();
//    qDebug() << "first is " << first.x() << ", " << first.y();
//    qDebug() << "last is " << last.x() << ", " << last.y();
}

IplImage * ImageScissor::QImageToIplImage(const QImage *qImage)
{
    int width = qImage->width();
    int height = qImage->height();
    CvSize size;//(width,height);
    size.width = width;
    size.height = height;
    IplImage *iplImage = cvCreateImage(size, IPL_DEPTH_8U, 3);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            QRgb rgb = qImage->pixel(x, y);
            cvSet2D(iplImage, y, x, CV_RGB(qRed(rgb), qGreen(rgb), qBlue(rgb)));
        }
    }
    return iplImage;
}
