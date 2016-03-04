#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#/****************************************************************************
    **
    ** Copyright (C) 2015 The Qt Company Ltd.
    ** Contact: http://www.qt.io/licensing/
    **
    ** This file is part of the examples of the Qt Toolkit.
    **
    ** $QT_BEGIN_LICENSE:BSD$
    ** You may use this file under the terms of the BSD license as follows:
    **
    ** "Redistribution and use in source and binary forms, with or without
    ** modification, are permitted provided that the following conditions are
    ** met:
    **   * Redistributions of source code must retain the above copyright
    **     notice, this list of conditions and the following disclaimer.
    **   * Redistributions in binary form must reproduce the above copyright
    **     notice, this list of conditions and the following disclaimer in
    **     the documentation and/or other materials provided with the
    **     distribution.
    **   * Neither the name of The Qt Company Ltd nor the names of its
    **     contributors may be used to endorse or promote products derived
    **     from this software without specific prior written permission.
    **
    **
    ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    ** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    ** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    ** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    ** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    ** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    ** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    ** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
    **
    ** $QT_END_LICENSE$
    **
    ****************************************************************************/

#include <QMainWindow>
#include <QtGui>
#include <QPainter>
#include <QPrinter>
#include "opencv2/opencv.hpp"
#include "fibheap.h"

class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;


class HeapNode : public FibHeapNode
{
    double LinkCost[8];
    int State;
    double Key;
    HeapNode *PrevNode;
    int column, row;


public:

      HeapNode() : FibHeapNode() { Key = 0; };

      virtual void operator =(FibHeapNode& RHS);
      virtual int  operator ==(FibHeapNode& RHS);
      virtual int  operator <(FibHeapNode& RHS);

      virtual void operator =(float NewKeyVal);
      virtual void Print();
      double GetKeyValue() { return Key; };
      int GetRow(){return row;}
      int GetColumn() {return column;}
      int GetState() {return State;}
      HeapNode * GetPreNode(){ return PrevNode;}
      double GetLinkCost(int pos) { return LinkCost[pos]; }

      void SetStateValue(int instate){ State = instate; }
      void SetLinkCost(double *ls)
      {
          for (int i = 0; i < 8; i++)
            LinkCost[i] = ls[i];
      }
      void SetLinkCost_i(int pos, double val)
      {
            LinkCost[pos] = val;
      }
      void SetKeyValue(double inkey) { Key = inkey; };
      void SetRowColumn(int inrow, int incolumn){ row = inrow; column = incolumn;}
      void SetPreNode(HeapNode * innode){ PrevNode = innode; }
};


class ImageViewer : public QMainWindow
{
    Q_OBJECT

public:
    ImageViewer();

private slots:
    void open();
    void print();
    void zoomIn();
    void zoomOut();
    void normalSize();
    void fitToWindow();
    void about();
    void save();
    void deselect();
    void undo();
    void saveMask();

private:
    void createActions();
    void createMenus();
    void updateActions();
    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar *scrollBar, double factor);
    void mousePressEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent * e);
    bool mouseOnImage(QPoint & pt, int x, int y);
    void mouseMoveEvent(QMouseEvent * e);
    void clearPainting();
    void enableMouseTrack(bool enable);
    void redraw();
    cv::Mat qimage_to_mat_cpy(QImage const &img, int format);
    void drawPath(int x, int y);
    void addContour(int x, int y, std::vector<QPoint> & cont);
    IplImage * QImageToIplImage(const QImage *qImage);


    QLabel *imageLabel;
    QScrollArea *scrollArea;
    double scaleFactor;

#ifndef QT_NO_PRINTER
    QPrinter printer;
#endif

    QAction *openAct;
    QAction *printAct;
    QAction *saveAct;
    QAction *saveMaskAct;
    QAction *exitAct;
    QAction *zoomInAct;
    QAction *zoomOutAct;
    QAction *normalSizeAct;
    QAction *fitToWindowAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *deselectAct;
    QAction *undoAct;

    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *toolMenu;
    QMenu *helpMenu;

    std::string imageFile;
    cv::Mat imageMat;
    QImage *qImage;
    QImage *originImage;
    QImage *confirmedImage;
    QImage *mask;
    bool closed = false;
    IplImage * iplImage;
    HeapNode * heapNode;
    std::vector<QPoint> points;

    QPainter *painter;
    std::vector<std::vector<QPoint> > contour;
};

class MyQPoint : QPoint
{
public:
    MyQPoint(int x, int y, int width) : QPoint(x, y) {this->width = width;}

    bool operator < (const MyQPoint & point) const
    {
        return (this->y() * width + this->x()) < (point.y() * point.width + point.x());
    }

    int width;
};

int calGraph(IplImage * img, HeapNode * A, int seed_x, int seed_y);

#endif // MAINWINDOW_H
