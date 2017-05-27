/* 
 * File:   main.c
 * Author: dmitry
 *
 * Created on 2 Май 2011 г., 20:56
 */

#include <stdlib.h>
#include <stdio.h>
#include "matrix.h"

int main(int argc, void** argv)
{
	SERVER_T srvs[MAXORDER + 1];
	FILE *fp;
	int err;
	MATRIX_T* mtrx;
	int nth;	//Число потоков на клиенте
	int sn;		//Число серверов
	int snth;	//Число ядер на серверах
	char *endptr;
	int maxserv = 100000000;
	long double det = 0.0;

	if(argc != 3){
		printf(
		"Аргументов должно быть два:\nпервый - число потоков на этой машине\nвторой - имя файла с матрицей.\n");
		return 0;
	}
	nth = strtol(argv[1], &endptr, 10);
	if(errno != 0){
		printf("Неверно указано число потоков\n");
		return 0;
	}
	fp = fopen(argv[2], "r");
	if(fp == NULL){
		printf("Не удалось открыть файл %s\n", (char*)argv[2]);
		return 0;
	}
	mtrx = m_fsqnew(fp, &err);
	if(err){
		printf("Не удалось прочитать матрицу\n");
		return 0;
	}
	if(mtrx->cols > MAXORDER){
		printf("Слишком большая матрица\n");
		return 0;
	}
//	maxserv = strtol(argv[3], &endptr, 10);
	det = m_network_det(mtrx, nth, maxserv);
	printf("\ndeterminant: %.10Le\n", det);
	return 0;
}


