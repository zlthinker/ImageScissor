//***************************************************************************
// FIBTEST.CPP
//
// Test program for the F-heap implementation.
// Copyright (c) 1996 by John Boyer.
// See header file for free usage information.
//***************************************************************************

#include <stdlib.h>
#include <iostream>
#include <stdio.h>
// #include <conio.h>
#include <ctype.h>
#include <opencv2/core/core.hpp>  
#include <opencv2/highgui/highgui.hpp>
//#include <highgui.h>
// #include <mem.h>
#include <string.h>
#include <time.h>
#include "mainwindow.h"

#include "fibheap.h"
#define infinity 999999999.9
using namespace std;

//#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")  



void HeapNode::Print()
{
     FibHeapNode::Print();
     cout << Key;
}

void HeapNode::operator =(float NewKeyVal)
{
HeapNode Temp;

     Temp.Key = Key = NewKeyVal;
     FHN_Assign(Temp);
}

void HeapNode::operator =(FibHeapNode& RHS)
{
     FHN_Assign(RHS);
     Key = ((HeapNode&) RHS).Key;
}

int  HeapNode::operator ==(FibHeapNode& RHS)
{
     if (FHN_Cmp(RHS)) return 0;
     return Key == ((HeapNode&) RHS).Key ? 1 : 0;
}

int  HeapNode::operator <(FibHeapNode& RHS)
{
int X;

     if ((X=FHN_Cmp(RHS)) != 0)
	  return X < 0 ? 1 : 0;

     return Key < ((HeapNode&) RHS).Key ? 1 : 0;
};


bool is_column_legal(int this_column,int column)
{
	if (this_column < column && this_column >= 0)
		return true;
	else
		return false;
}
bool is_row_legal(int this_row, int row)
{
	if (this_row < row && this_row >= 0)
		return true;
	else 
		return false;
}


double Calc_d_Link(IplImage *img,int this_row, int this_column, double *ls)
{
    if (img == NULL) {cout << "image is empty";exit(1);}
    int length = img->width;
    int width = img->height;
    int n_channel = img->nChannels;
    double ls_rgb[3][8];
    memset(ls_rgb, 0, 3*8*sizeof(double));
    uchar *data = (uchar *)img->imageData;
    int step = img->widthStep/sizeof(uchar);
    double max = 0.0;
    double sum = 0.0;
    if (is_column_legal(this_column-1,length) && is_row_legal(this_row-1,width))
    {
        sum = 0;
        for ( int k = 0; k < 3; k++ )
        {
            ls_rgb[k][0] = abs(data[this_row*step+n_channel*(this_column-1)+k]-data[(this_row-1)*step+n_channel*this_column+k])/sqrt(2);
            sum += 	ls_rgb[k][0]*ls_rgb[k][0];
        }
        ls[0] = sqrt(sum/3.0);
        max = (max < ls[0])?ls[0]:max;
    }

    if (is_column_legal(this_column-1,length) && is_row_legal(this_row+1,width) && is_row_legal(this_row-1,width))
    {
        sum = 0;
        for ( int k = 0; k < 3; k++ )
        {
            ls_rgb[k][1] = abs(data[(this_row-1)*step+n_channel*(this_column-1)+k]+data[(this_row-1)*step+n_channel*this_column+k]-data[(this_row+1)*step+n_channel*(this_column-1)+k]-data[(this_row+1)*step+n_channel*this_column+k])/4.0;
            sum += ls_rgb[k][1]*ls_rgb[k][1];
        }
        ls[1] = sqrt(sum/3.0);
        max = (max < ls[1])?ls[1]:max;
    }
    if (is_column_legal(this_column-1,length) && is_row_legal(this_row+1,width))
    {
        sum = 0;
        for ( int k = 0; k < 3; k++ )
        {
            ls_rgb[k][2] = abs(data[this_row*step+n_channel*(this_column-1)+k]-data[(this_row+1)*step+n_channel*this_column+k])/sqrt(2);
            sum += ls_rgb[k][2]*ls_rgb[k][2];
        }
        ls[2] = sqrt(sum/3.0);
        max = (max < ls[2])?ls[2]:max;
    }

    if (is_column_legal(this_column-1,length) && is_column_legal(this_column+1,length) && is_row_legal(this_row+1,width))
    {
        sum = 0;
        for ( int k = 0; k < 3; k++ )
        {
            ls_rgb[k][3] = abs(data[(this_row+1)*step+n_channel*(this_column-1)+k]+data[this_row*step+n_channel*(this_column-1)+k]-data[(this_row+1)*step+n_channel*(this_column+1)+k]-data[this_row*step+n_channel*(this_column+1)+k])/4.0;
            sum += ls_rgb[k][3]*ls_rgb[k][3];
        }
        ls[3] = sqrt(sum/3.0);
        max = (max < ls[3])?ls[3]:max;
    }

    if (is_column_legal(this_column+1,length) && is_row_legal(this_row+1,width))
    {
        sum = 0;
        for ( int k = 0; k < 3; k++ )
        {
            ls_rgb[k][4] = abs(data[this_row*step+n_channel*(this_column+1)+k]-data[(this_row+1)*step+n_channel*this_column+k])/sqrt(2);
            sum += ls_rgb[k][4]*ls_rgb[k][4];
        }
        ls[4] = sqrt(sum/3.0);
        max = (max < ls[4])?ls[4]:max;
    }
    if (is_column_legal(this_column+1,length) && is_row_legal(this_row+1,width) && is_row_legal(this_row-1,width))
    {
        sum = 0;
        for ( int k = 0; k < 3; k++ )
        {
            ls_rgb[k][5] = abs(data[(this_row-1)*step+n_channel*(this_column+1)+k]+data[(this_row-1)*step+n_channel*this_column+k]-data[(this_row+1)*step+n_channel*(this_column+1)+k]-data[(this_row+1)*step+n_channel*this_column+k])/4.0;
            sum += ls_rgb[k][5]*ls_rgb[k][5];
        }
        ls[5] = sqrt(sum/3.0);

        max = (max < ls[5])?ls[5]:max;
    }
    if (is_column_legal(this_column+1,length) && is_row_legal(this_row-1,width))
    {
        sum = 0;
        for ( int k = 0; k < 3; k++ )
        {
            ls_rgb[k][6] = abs(data[this_row*step+n_channel*(this_column+1)+k]-data[(this_row-1)*step+n_channel*this_column+k])/sqrt(2);
            sum += ls_rgb[k][6]*ls_rgb[k][6];
        }
        ls[6] = sqrt(sum/3.0);

        max = (max < ls[6])?ls[6]:max;
    }

    if (is_column_legal(this_column-1,length) && is_column_legal(this_column+1,length) && is_row_legal(this_row-1,width))
    {
        sum = 0;
        for ( int k = 0; k < 3; k++ )
        {
            ls_rgb[k][7] = abs(data[(this_row-1)*step+n_channel*(this_column-1)+k]+data[this_row*step+n_channel*(this_column-1)+k]-data[(this_row-1)*step+n_channel*(this_column+1)+k]-data[this_row*step+n_channel*(this_column+1)+k])/4.0;
            sum += ls_rgb[k][7]*ls_rgb[k][7];
        }
        ls[7] = sqrt(sum/3.0);
        max = (max < ls[7])?ls[7]:max;
    }
    return max;
}



/*
int IntCmp(const void *pA, const void *pB)
{
int A, B;

    A = *((const int *) pA);
    B = *((const int *) pB);
    if (A < B) return -1;
    if (A == B) return 0;
    return 1; 
}
*/
int calGraph(IplImage * img, HeapNode * A, int seed_x, int seed_y)
{
    if (img == NULL) { cout << "image open error"; return 1;}
    int length = img->width;
    int width = img->height;


    FibHeap *theHeap = NULL;
    int Max=length*width;
	
// Setup for the Fibonacci heap

     if ((theHeap = new FibHeap) == NULL)
     {
     cout << "Memory allocation failed-- ABORTING.\n";
         exit(-1);
     }

//     theHeap->ClearHeapOwnership();
     int i;
     //initialize ls to be infinity
     double ls[8];
     for (i=0;i<8;i++)
        ls[i] = infinity;

    double max_d = 0.0;
    double tmp;
     //initial HeapNodes with INITIAL state, column and row numbers, linkcost
     for (i=0;i<Max;i++)
     {
         A[i].SetStateValue(0);
         A[i].SetRowColumn(i/length,i-i/length*length);
         // calculate Dlink
         tmp = Calc_d_Link(img,i/length,i-i/length*length,ls);
         max_d = (max_d < tmp)?tmp:max_d;
         A[i].SetLinkCost(ls);
     }
     //set linkcost
     for (i=0;i<Max;i++)
     {
        for (int j = 0;j < 8; j++)
        {
            tmp = A[i].GetLinkCost(j);
            if (abs(tmp - infinity) < 0.001)
                continue;
            if (j-(j/2)*2 == 0)
            {
                tmp = (max_d - tmp)*sqrt(2);
                A[i].SetLinkCost_i(j,tmp);
            }
            else
                A[i].SetLinkCost_i(j,max_d - tmp);
		
        }
     }

     //get seed point.
     A[seed_x*length+seed_y].SetKeyValue(0);
     A[seed_x*length+seed_y].SetPreNode(NULL);
     theHeap->Insert(&A[seed_x*length+seed_y]);
	 
     HeapNode * min = NULL;
     int this_row = 0,this_column = 0;
     int index = 0;
     int count = 0;
     while( theHeap->GetNumNodes() > 0)
     {
         count++;
        min = (HeapNode *)theHeap->ExtractMin();
        min->SetStateValue(2);//mark min as expanded
        this_row = min->GetRow();
        this_column = min->GetColumn();

        if ( this_row-1 >= 0 && this_column-1 >= 0)
        {
            index = (this_row-1)*length+this_column-1;
            if ( A[index].GetState() == 0 )
            {
                A[index].SetPreNode(min);
                A[index].SetKeyValue(min->GetKeyValue() + min->GetLinkCost(0));
                A[index].SetStateValue(1);
                theHeap->Insert(&A[index]);
            }
            if ( A[index].GetState() == 1 )
                if ( min->GetKeyValue() + min->GetLinkCost(0) < A[index].GetKeyValue() )
                {
                    A[index].SetPreNode(min);
                    HeapNode tmp = A[index];
                    tmp.SetKeyValue(min->GetKeyValue() + min->GetLinkCost(0));
                    theHeap->DecreaseKey(&A[index],tmp);
                    //for test
                    HeapNode * test = A[index].GetPreNode();

                }
        }
        if ( this_column-1 >= 0 )
        {
            index = (this_row)*length+this_column-1;
            if ( A[index].GetState() == 0 )
            {
                A[index].SetPreNode(min);
                A[index].SetKeyValue(min->GetKeyValue() + min->GetLinkCost(1));
                A[index].SetStateValue(1);
                theHeap->Insert(&A[index]);
            }
            if ( A[index].GetState() == 1 )
                if ( min->GetKeyValue() + min->GetLinkCost(1) < A[index].GetKeyValue() )
                {
                    A[index].SetPreNode(min);
                    HeapNode tmp = A[index];
                    tmp.SetKeyValue(min->GetKeyValue() + min->GetLinkCost(1));
                    theHeap->DecreaseKey(&A[index],tmp);
                }
        }
        if ( this_row+1 < width && this_column-1 >= 0)
        {
            index = (this_row+1)*length+this_column-1;
            if ( A[index].GetState() == 0 )
            {
                A[index].SetPreNode(min);
                A[index].SetKeyValue(min->GetKeyValue() + min->GetLinkCost(2));
                A[index].SetStateValue(1);
                theHeap->Insert(&A[index]);
            }
            if ( A[index].GetState() == 1 )
                if ( min->GetKeyValue() + min->GetLinkCost(2) < A[index].GetKeyValue() )
                {
                    A[index].SetPreNode(min);
                    HeapNode tmp = A[index];
                    tmp.SetKeyValue(min->GetKeyValue() + min->GetLinkCost(2));
                    theHeap->DecreaseKey(&A[index],tmp);
                }
        }
        if ( this_row+1 < width)
        {
            index = (this_row+1)*length+this_column;
            if ( A[index].GetState() == 0 )
            {
                A[index].SetPreNode(min);
                A[index].SetKeyValue(min->GetKeyValue() + min->GetLinkCost(3));
                A[index].SetStateValue(1);
                theHeap->Insert(&A[index]);
            }
            if ( A[index].GetState() == 1 )
                if ( min->GetKeyValue() + min->GetLinkCost(3) < A[index].GetKeyValue() )
                {
                    A[index].SetPreNode(min);
                    HeapNode tmp = A[index];
                    tmp.SetKeyValue(min->GetKeyValue() + min->GetLinkCost(3));
                    theHeap->DecreaseKey(&A[index],tmp);
                }
        }
        if ( this_row+1 < width && this_column+1 < length)
        {
            index = (this_row+1)*length+this_column+1;
            if ( A[index].GetState() == 0 )
            {
                A[index].SetPreNode(min);
                A[index].SetKeyValue(min->GetKeyValue() + min->GetLinkCost(4));
                A[index].SetStateValue(1);
                theHeap->Insert(&A[index]);
            }
            if ( A[index].GetState() == 1 )
                if ( min->GetKeyValue() + min->GetLinkCost(4) < A[index].GetKeyValue() )
                {
                    A[index].SetPreNode(min);
                    HeapNode tmp = A[index];
                    tmp.SetKeyValue(min->GetKeyValue() + min->GetLinkCost(4));
                    theHeap->DecreaseKey(&A[index],tmp);
                }
        }
        if ( this_column+1 < length)
        {
            index = (this_row)*length+this_column+1;
            if ( A[index].GetState() == 0 )
            {
                A[index].SetPreNode(min);
                A[index].SetKeyValue(min->GetKeyValue() + min->GetLinkCost(5));
                A[index].SetStateValue(1);
                theHeap->Insert(&A[index]);
            }
            if ( A[index].GetState() == 1 )
                if ( min->GetKeyValue() + min->GetLinkCost(5) < A[index].GetKeyValue() )
                {
                    A[index].SetPreNode(min);
                    HeapNode tmp = A[index];
                    tmp.SetKeyValue(min->GetKeyValue() + min->GetLinkCost(5));
                    theHeap->DecreaseKey(&A[index],tmp);
                }
        }
        if ( this_row-1 >= 0 && this_column+1 < length)
        {
            index = (this_row-1)*length+this_column+1;
            if ( A[index].GetState() == 0 )
            {
                A[index].SetPreNode(min);
                A[index].SetKeyValue(min->GetKeyValue() + min->GetLinkCost(6));
                A[index].SetStateValue(1);
                theHeap->Insert(&A[index]);
            }
            if ( A[index].GetState() == 1 )
                if ( min->GetKeyValue() + min->GetLinkCost(6) < A[index].GetKeyValue() )
                {
                    A[index].SetPreNode(min);
                    HeapNode tmp = A[index];
                    tmp.SetKeyValue(min->GetKeyValue() + min->GetLinkCost(6));
                    theHeap->DecreaseKey(&A[index],tmp);
                }
        }
        if ( this_row-1 >= 0)
        {
            index = (this_row-1)*length+this_column;
            if ( A[index].GetState() == 0 )
            {
                A[index].SetPreNode(min);
                A[index].SetKeyValue(min->GetKeyValue() + min->GetLinkCost(7));
                A[index].SetStateValue(1);
                theHeap->Insert(&A[index]);
            }
            if ( A[index].GetState() == 1 )
                if ( min->GetKeyValue() + min->GetLinkCost(7) < A[index].GetKeyValue() )
                {
                    A[index].SetPreNode(min);
                    HeapNode tmp = A[index];
                    tmp.SetKeyValue(min->GetKeyValue() + min->GetLinkCost(7));
                    theHeap->DecreaseKey(&A[index],tmp);
                }
        }
     }


/*	 for (i=Max-1;i>=0;i--)
         theHeap->Insert(&A[i]);
	
     cout << "before #nodes = " << theHeap->GetNumNodes() << endl;

     for (i=0;i<Max;i++) {
         if ((Min = (HeapNode*) theHeap->ExtractMin ()) != NULL) {
            float key = -Min->GetKeyValue();
            cout << key << ' ';
         }
     }
     cout << endl;

     for (i=0;i<Max;i++)
         cout << A[i].GetKeyValue() << ' ';

     cout << "after #nodes = " << theHeap->GetNumNodes() << endl;
// Cleanup; test heap ownership flag settings
*/

     delete theHeap;
}
