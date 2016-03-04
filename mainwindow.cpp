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
    debugMode = false;

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
        minPath = new QImage(originImage->width() * 3, originImage->height() * 3, QImage::Format_ARGB32);
        minPathCopy = new QImage(originImage->width() * 3, originImage->height() * 3, QImage::Format_ARGB32);

        if (qImage->isNull()) {
            QMessageBox::information(this, tr("Image Viewer"),
                                     tr("Cannot load %1.").arg(fileName));
            return;
        }
        painter = new QPainter(qImage);
        minPathPainter = new QPainter(minPath);
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
        heapNode = new HeapNode[qImage->width() * qImage->height() + 1];
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

void ImageScissor::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_1)
    {
        debugMode = false;
        pixNode = new QImage(originImage->width() * 3, originImage->height() * 3, QImage::Format_ARGB32);
        pixNode->fill(QColor(0, 0, 0, 255));
//        qDebug() << "width=" << minPath->width() << ", height=" << minPath->height();
        for ( int row = 0; row < originImage->height(); ++row )
        {
            for ( int col = 0; col < originImage->width(); ++col )
            {
                int index = row * originImage->width() + col;
                HeapNode *node = &heapNode[index - 1];
                QRgb rgb( originImage->pixel( col, row ) );
                pixNode->setPixel(col * 3 + 1, row * 3 + 1, rgb);
            }
        }

        imageLabel->setPixmap(QPixmap::fromImage(*pixNode));
//        QLabel *newLabel = new QLabel;
//        newLabel->setPixmap(QPixmap::fromImage(*pixNode));
//        newLabel->show();
    }

    else if(event->key() == Qt::Key_2)
    {
        debugMode = false;
        costGraph = new QImage(originImage->width() * 3, originImage->height() * 3, QImage::Format_ARGB32);
        costGraph->fill(QColor(0, 0, 0, 255));
        for ( int row = 0; row < originImage->height(); ++row )
        {
            for ( int col = 0; col < originImage->width(); ++col )
            {

                int index = row * originImage->width() + col;
//                qDebug() << "col=" << col << ", row=" << row << ", index=" << index;
                HeapNode *node = &heapNode[index];
                QRgb rgb( originImage->pixel( col, row ) );
                costGraph->setPixel(col * 3 + 1, row * 3 + 1, rgb);

//                qDebug() << "cost0: " << node->LinkCost[0];
//                qDebug() << "cost4: " << node->LinkCost[4];
                int intensity = std::min((int) (node->LinkCost[0] * 0.4), 255);
                QRgb rgb1(intensity);
                costGraph->setPixel(col * 3, row * 3, rgb1);
                intensity = std::min((int) (node->LinkCost[1] * 0.4), 255);
                QRgb rgb2(intensity);
                costGraph->setPixel(col * 3, row * 3 + 1, rgb2);
                intensity = std::min((int) (node->LinkCost[2] * 0.4), 255);
                QRgb rgb3(intensity);
                costGraph->setPixel(col * 3, row * 3 + 2, rgb3);
                intensity = std::min((int) (node->LinkCost[3] * 0.4), 255);
                QRgb rgb4(intensity);
                costGraph->setPixel(col * 3 + 1, row * 3 + 2, rgb4);
                intensity = std::min((int) (node->LinkCost[4] * 0.4), 255);
                QRgb rgb5(intensity);
                costGraph->setPixel(col * 3 + 2, row * 3 + 2, rgb5);
                intensity = std::min((int) (node->LinkCost[5] * 0.4), 255);
                QRgb rgb6(intensity);
                costGraph->setPixel(col * 3 + 2, row * 3 + 1, rgb6);
                intensity = std::min((int) (node->LinkCost[6] * 0.4), 255);
                QRgb rgb7(intensity);
                costGraph->setPixel(col * 3 + 2, row * 3, rgb7);
                intensity = std::min((int) (node->LinkCost[7] * 0.4), 255);
                QRgb rgb8(intensity);
                costGraph->setPixel(col * 3 + 1, row * 3, rgb8);

            }
        }

//        QLabel *newLabel = new QLabel;
//        newLabel->setPixmap(QPixmap::fromImage(*costGraph));
//        newLabel->show();
        imageLabel->setPixmap(QPixmap::fromImage(*costGraph));
    }

    else if(event->key() == Qt::Key_3)
    {
        debugMode = false;
        pathTree = new QImage(originImage->width() * 3, originImage->height() * 3, QImage::Format_ARGB32);
        pathTree->fill(QColor(0, 0, 0, 255));
        for ( int row = 0; row < originImage->height(); ++row )
        {
            for ( int col = 0; col < originImage->width(); ++col )
            {
                pathTree->setPixel(col * 3 + 1, row * 3 + 1, QColor(0, 0, 255, 255).rgb());
                int index = row * originImage->width() + col;
                HeapNode *node = &heapNode[index];
                HeapNode *parent = node->GetPreNode();
                if (parent == NULL) continue;
                int dx = parent->GetColumn() - node->GetColumn();
                int dy = parent->GetRow() - node->GetRow();
//                qDebug() << "col=" << col << ", row=" << row << ", index=" << index;
//                qDebug() << "dx=" << dx << ", dy=" << dy;
                pathTree->setPixel(col * 3 + 1 + dx, row * 3 + 1 + dy, QColor(255, 255, 128, 255).rgb());
                pathTree->setPixel(parent->GetColumn() * 3 + 1 - dx, parent->GetRow() * 3 + 1 - dy, QColor(255, 255, 0, 255).rgb());

            }
        }

//        QLabel *newLabel = new QLabel;
//        newLabel->setPixmap(QPixmap::fromImage(*pathTree));
//        newLabel->show();
        imageLabel->setPixmap(QPixmap::fromImage(*pathTree));
    }

    else if(event->key() == Qt::Key_4)
    {
        minPath = new QImage(originImage->width() * 3, originImage->height() * 3, QImage::Format_ARGB32);
        minPath->fill(QColor(0, 0, 0, 255));
        for ( int row = 0; row < originImage->height(); ++row )
        {
            for ( int col = 0; col < originImage->width(); ++col )
            {
                minPath->setPixel(col * 3 + 1, row * 3 + 1, QColor(0, 0, 255, 255).rgb());
                int index = row * originImage->width() + col;
                HeapNode *node = &heapNode[index];
                HeapNode *parent = node->GetPreNode();
                if (parent == NULL) continue;
                int dx = parent->GetColumn() - node->GetColumn();
                int dy = parent->GetRow() - node->GetRow();
//                qDebug() << "col=" << col << ", row=" << row << ", index=" << index;
//                qDebug() << "dx=" << dx << ", dy=" << dy;
                minPath->setPixel(col * 3 + 1 + dx, row * 3 + 1 + dy, QColor(255, 255, 128, 255).rgb());
                minPath->setPixel(parent->GetColumn() * 3 + 1 - dx, parent->GetRow() * 3 + 1 - dy, QColor(255, 255, 0, 255).rgb());

            }
        }

        debugMode = true;
        enableMouseTrack(true);

        minPathCopy = new QImage(*minPath);
        QPen paintpen(Qt::red);
        paintpen.setWidth(5);
        QPainter* temp_painter = new QPainter(minPath);
        temp_painter->setPen(paintpen);
        temp_painter->drawPoint(points.back().x() * 3 + 1, points.back().y() * 3 + 1);

//        QLabel *newLabel = new QLabel;
//        newLabel->setPixmap(QPixmap::fromImage(*pathTree));
//        newLabel->show();
        imageLabel->setPixmap(QPixmap::fromImage(*minPath));
        debugMode = true;
        enableMouseTrack(true);
    }
    else if(event->key() == Qt::Key_5)
    {
        imageLabel->setPixmap(QPixmap::fromImage(*qImage));
        debugMode = false;
        if (closed) {
            enableMouseTrack(false);
        }
    }
    else if(event->key() == Qt::Key_Escape)
    {
        enableMouseTrack(false);
        clearPainting();
        redraw();
        closed = true;
        if (!debugMode) {
            imageLabel->setPixmap(QPixmap::fromImage(*qImage));
        }
    }

}

void ImageScissor::mouseMoveEvent(QMouseEvent * e)
{
    if (closed && !debugMode)
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
        if (!debugMode)
        {
            redraw();
        }
//        QPen paintpen(Qt::red);
//        paintpen.setWidth(0);
//        painter->setPen(paintpen);
//        painter->drawLine(last, p1);
        drawPath(p1.x(), p1.y());
        if (!debugMode) {
            imageLabel->setPixmap(QPixmap::fromImage(*qImage));
        } else {
            imageLabel->setPixmap(QPixmap::fromImage(*minPath));
        }
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
    if (!debugMode)
    {
        free(qImage);
        free(painter);
        qImage = new QImage(*originImage);
        painter  = new QPainter(qImage);
    }
    else
    {
        free(minPath);
        free(minPathPainter);
        minPath = new QImage(*minPathCopy);
        minPathPainter  = new QPainter(minPath);
    }
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
    debugMode = false;
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
    minPathPainter->setPen(paintpen);


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
        if (!debugMode) {
            painter->drawLine(p1, p2);
        } else {
            QPoint p11(parent->GetColumn() * 3 + 1, parent->GetRow() * 3 + 1);
            QPoint p22(tempNode->GetColumn() * 3 + 1, tempNode->GetRow() * 3 + 1);
            minPathPainter->drawLine(p11, p22);
        }
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

