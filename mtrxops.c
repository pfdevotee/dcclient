#include "matrix.h"
#include <math.h>

//Размещение новой матрицы
MATRIX_T* m_new(int nrows, int ncols, int* err)
{
	double *temp;
	MATRIX_T *m = NULL;
	if( (temp = (double*)malloc(nrows*ncols*sizeof(double)))	== NULL ) {
		return NULL;
		*err = ALLOCFAIL;
	}
	if( (m = (MATRIX_T*)malloc(sizeof(MATRIX_T))) == NULL ) {
		*err = ALLOCFAIL;
		free(temp);
		return NULL;
	}
	m->cols = ncols;
	m->rows = nrows;
	m->val  = temp;
	return m;
}

//Освобождение матрицы
void m_free(MATRIX_T* m)
{
	if(m == NULL) return;
	free(m->val);
	free(m);
}

//Создает новую квадратную матрицу и инициализирует её из файла
MATRIX_T* m_fsqnew(FILE *fp, int *err)
{
	int n, i, j, index;
	MATRIX_T* mtrx;

	if( fscanf(fp, "%d", &n) == EOF ){
		*err = EMPTYFILE;
		return NULL;
	}

	if( (mtrx = m_new(n, n, err)) == NULL ){
		return NULL;
	}

	for(i = 0; i < n; i++){
		for(j = 0; j < n; j++){
			if( (fscanf(fp, "%lf", mtrx->val+mdx(mtrx, i, j))) == EOF ){
				*err = FWRONGFORMAT;
				m_free(mtrx);
				return NULL;
			}
		}
	}
	*err = 0;
	return mtrx;
}

//Рассчитывает определитель матрицы по методу Гаусса
//с частичной максимизацией ведущего элемента
long double m_det(MATRIX_T* a, double epsilon)
{
	int	row, col, swap, sign, err;
	double pivot, e_norm;
	long double det;
	MATRIX_T* t;

	det  = 1.0;
	sign = 1;

//Разместить "черновую" матрицу для работы
	if( !(t = m_new(a->rows, a->cols, &err)) ) return 0.0;
	memcpy((void*)t->val, (void*)a->val, a->rows*a->cols*sizeof(double));
	//Для каждой строки
	for(row = 0; row < t->rows; row++){;
		//Найти в столбце под диагональю наибольший элемент
		swap = maxelementrow(t, row);
		//Переместить строки для помещения наибольшего элемента на место ведущего
		if(swap != row) {
			sign = -sign;
			swaprows(t, row, swap);
		}
		//Умножить текущее значение определителя на ведущий элемент
		pivot = t->val[mdx(t, row, row)];
		det *= pivot;
/*		if( fabs(det)/e_norm < epsilon )
			return 0;*/
		col = row;
		set_low_zero(t, col);
	}
	m_free(t);
	if(sign < 0)
		det = -det;
	return det;
}

void set_low_zero(MATRIX_T* t, int col)
{
	int i, j;
	double pivot, factor;
	pivot = t->val[mdx(t, col, col)];
	for(i = col+1; i < t->rows; i++){
		factor = t->val[mdx(t, i, col)] / pivot;
		for(j = col; j < t->cols; j++){
			t->val[mdx(t, i, j)] -=
					factor * t->val[mdx(t, col, j)];
		}
	}
}

void swaprows(MATRIX_T* t, int row, int swap)
{
	int j;
	double temp;
	for(j = 0; j < t->cols; j++){
		temp = t->val[mdx(t, row, j)];
		t->val[mdx(t, row, j)]  = t->val[mdx(t, swap, j)];
		t->val[mdx(t, swap, j)] = temp;
	}
}

int maxelementrow(MATRIX_T* a, int col)
{
	int maxi, i;
	double max;

	max  = a->val[col];
	maxi = col;
	for(i = col+1;	i < a->rows; i++)
		if(a->val[mdx(a, i, col)] > max){
			max = a->val[mdx(a, i, col)];
			maxi = i;
		}
	return maxi;
}

int mdx(MATRIX_T* a, int i, int j)
{
	return i*a->cols + j;
}

void m_fprint(FILE* fp, MATRIX_T* a)
{
	int i, j;

	for(i = 0; i < a->rows; i++){
		for(j = 0; j < a->cols; j++)
			fprintf(fp, "%.0f ", a->val[mdx(a, i, j)]);
		fprintf(fp, "\n");
	}
	fprintf(fp, "\n");
}