/* 
 * File:   matrix.h
 * Author: dmitry
 *
 * Created on 29 Март 2011 г., 19:46
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef struct
{
	double *val;
	int rows;
        int cols;
} MATRIX_T;



#define LINELEN 10000      //Максимальная длина строки в файле с матрицей
#define MAXTHREADS 500     //Максимальное число потоков
#define MAXORDER 1000      //Максимальный порядок матрицы
#define ALLOCFAIL 1
#define FILEREADFAIL 2
#define EMPTYFILE 3
#define FWRONGFORMAT 4
#define MATRIXPORT 50000
#define KEYSTRING "9mdovgpw"
#define USECONDS 500000      //Время ожидания соединения

typedef struct
{
    int socket;
    int cores;          //Число процессорных ядер на сервере
} SERVER_T;

MATRIX_T* m_new(int nrows, int ncols, int* err);
void m_free(MATRIX_T* m);
MATRIX_T* m_fsqnew(FILE *fp, int *err);
long double m_det(MATRIX_T* a, double epsilon);
void set_low_zero(MATRIX_T* t, int col);
void swaprows(MATRIX_T* t, int row, int swap);
int maxelementrow(MATRIX_T* a, int row);
int mdx(MATRIX_T* a, int i, int j);
void m_fprint(FILE* fp, MATRIX_T* a);
long double m_mt_det(MATRIX_T* a, int nth);
int servers_info(int backlog, int* maxcores, SERVER_T* srvrs, int us, int maxserv);
void m_send(MATRIX_T* a, SERVER_T* serv, int sn, int ncores);
void m_val_send(MATRIX_T* a, SERVER_T* serv, int sn);
long double m_network_det(MATRIX_T* a, int nth, int maxserv);
long double m_mt_minorsSum(MATRIX_T* a, int nth, int col1, int col2);