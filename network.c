#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "matrix.h"
#include <strings.h>

#undef max
#define max(x,y) ((x) > (y) ? (x) : (y))

int if_connection(int sock, int us);
int rounding(double a);
void broadcast(in_port_t port);

//Функция отправляет широковещательный IP-пакет на порт № MATRIXPORT
void broadcast(in_port_t port)
{
	register int s;
	register int bytes;
	struct sockaddr_in sa;
	unsigned char buffer[BUFSIZ+1];
	unsigned char str[20] = "";
	int i = 1;

	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("broadcast:socket");
		exit(EXIT_FAILURE);
	}

	setsockopt(s, SOL_SOCKET, SO_BROADCAST, &i, sizeof(i));
	bzero(&sa, sizeof sa);

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr("255.255.255.255");
	sa.sin_port = htons(MATRIXPORT);

	strcpy(str, KEYSTRING);
	str[9] = port & 0xff;
	str[10] = (port>>8) & 0xff;

	if( sendto(s, str, 11, 0, (struct sockaddr*)&sa, sizeof sa) < 0){
		perror("broadcast:sendto");
		exit(EXIT_FAILURE);
	}

	close(s);
}

//Функция определяет IP-адреса серверов и кол-во процессорных ядер на них
//Возвращает число серверов
//В каждый элемент массива srvrs(начиная с первого, а не с нулевого) пишет
//число ядер на сервере и сокет, соответствующий соединению с этим сервером
//В *maxcores пишет суммарное число процессорных ядер
int servers_info(int backlog, int* maxcores, SERVER_T* srvrs, int us, int maxserv)
{
	register int s, c;
	int b;
	struct sockaddr_in sa, ca;
	time_t t;
	struct tm *tm;
	FILE *server;

	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("servers_info:socket()");
		exit(EXIT_FAILURE);
	}

	bzero(&sa, sizeof sa);

	sa.sin_family = AF_INET;
//	sa.sin_port   = htons(MATRIXPORT);

	if (INADDR_ANY)
		sa.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, (struct sockaddr *)&sa, sizeof sa) < 0) {
		perror("servers_info:bind()");
		close(s);
		exit(EXIT_FAILURE);
	}
	int addrlen = sizeof(sa);
	getsockname(s, (struct sockaddr *)&sa, &addrlen);
	printf("unused port:%d\n", sa.sin_port);
	broadcast(sa.sin_port);
	listen(s, backlog);

	int i = 0;	//Суммарное количество ядер на серверах
	int n;
	int j;		//Номер сервера
	char keystr[10] = "";

	for(j = 1; (i < *maxcores);) {
		ca = sa;
		b = sizeof ca;

		if( !if_connection(s, us) ){
			*maxcores = i;
			return j - 1;
		}

		if ((c = accept(s, (struct sockaddr *)&ca, &b)) < 0) {
			perror("servers_info:accept");
			close(s);
			exit(EXIT_FAILURE);
		}

		read(c, keystr, 9);
		if( strcmp(keystr, KEYSTRING) ){
			close(c);
			printf("Соединение отклонено...\n");
			continue;
		}
		read(c, &n, sizeof(n));
		i += n;
		srvrs[j].cores	= n;
		srvrs[j].socket	= c;
		j++;
	}
}

//Возвращает 1, если есть входящее соединение, и вызов accept() не будет блокирующим, и
//0 в противном случае. Каждое соединение ожидается в течение us микросекунд.
int if_connection(int sock, int us)
{
	fd_set rfds;
	struct timeval tv;
	int retval;

	/* Watch stdin (fd 0) to see when it has input. */
	FD_ZERO(&rfds);
	FD_SET(sock, &rfds);

	/* Wait up to five seconds. */
	tv.tv_sec  = 0;
	tv.tv_usec = us;

	retval = select(sock + 1, &rfds, NULL, NULL, &tv);
	/* Don't rely on the value of tv now! */

	if (retval == -1){
		perror("if_connection:select()");
		exit(EXIT_FAILURE);
	}
	else if (retval)
		return 1;
	/* FD_ISSET(0, &rfds) will be true. */
	else
		return 0;
}

int rounding(double a)
{
	int ia;
	ia = (int)a;
	if( (a-ia) < (ia+1-a) )
		return ia;
	else
		return ia + 1;
}
//Отправляет всем серверам матрицу и номера миноров, с которыми они должны работать
//sn - число серверов
//ncores - число потоков на серверах и клиенте
//serv[1],..,serv[sn] - параметры серверов
void m_send(MATRIX_T* a, SERVER_T* serv, int sn, int ncores)
{
	int i;
	int sj;			//номер сервера
	int col1, col2;	//i-ому потоку достаются миноры № col1,..,col2
	int clcol2;
	int bytes = 0;
	char str[100] = "";

	col1 = 0;
	col2 = rounding((double)a->cols/ncores*serv[0].cores) - 1;
	clcol2 = col2;
	printf("client:minors %d %d\n", col1, col2);

	for(i = 1; i <= sn; i++){
		col1 = col2 + 1;
		if(i == sn)
			col2 = a->cols - 1;
		else
			col2 = rounding((double)col1 + (double)a->cols/ncores*serv[i].cores) - 1;
		printf("server #%d: minors %d %d\n", i, col1, col2);

		if( (bytes = write(serv[i].socket, &col1, sizeof(col1)) ) <= 0){	//Раздаем серверам номера миноров
			perror("m_send:write col1");
			exit(EXIT_FAILURE);
		}

		if( (bytes = write(serv[i].socket, &col2, sizeof(col2)) ) <= 0){
			perror("m_send:write col2");
			exit(EXIT_FAILURE);
		}
		if( (bytes = write(serv[i].socket, &(a->cols), sizeof(a->cols)) ) < 0){ //Отправляем порядок матрицы
			perror("m_send:write mtrx order");
			exit(EXIT_FAILURE);
		}
	}
	m_val_send(a, serv, sn);
}

//Сетевая версия функции m_det
//nth - кол-во потоков на клиенте
long double m_network_det(MATRIX_T* a, int nth, int maxserv)
{
	int sn;		//Число серверов
	int snth;	//Число ядер на серверах
	char *endptr;
	long double det = 0.0, minorsum = 0.0;
	int i, bytes;
	int clcol1, clcol2;		//Номера миноров, скоторыми работает клиент
	SERVER_T serv[MAXORDER + 1];

	snth = a->cols;
	sn = servers_info(100, &snth, serv, USECONDS, maxserv);
	printf("cores on servers: %d\ncores on client: %d\n", snth, nth);
	serv[0].cores	= nth;
	serv[0].socket = 0;

	m_send(a, serv, sn, snth + nth);

	clcol1 = 0;
	clcol2 = rounding((double)a->cols/(snth + nth)*serv[0].cores) - 1;
	det = m_mt_minorsSum(a, nth, clcol1, clcol2);	//Клиент не сидит без дела, а считает свои миноры

//	printf("%.10Le\n", det);
	m_free(a);
	for(i = 1; i <= sn; i++){
		if( (bytes = read(serv[i].socket, &minorsum, sizeof(minorsum) )) < 0){
			perror("m_network_det:read");
			exit(EXIT_FAILURE);
		}
		if(!bytes){
			printf("Соединение с сервером потеряно\n");
			exit(EXIT_FAILURE);
		}
		det += minorsum;
		if(close(serv[i].socket) < 0)
			perror("m_network_det:close()");
	}
	return det;
}

//Функция рассылает серверам содержимое a->val
//sn - число серверов
void m_val_send(MATRIX_T* a, SERVER_T* serv, int sn)
{
	int i, j, bytes;
	int retval;
	int nfds = 0;
	int* curpos;		//массив номеров последних отправленных элемментов
	int completed = 0;	//число завершенных передач
	fd_set wd;
	int nn = a->cols*a->rows;	//кол-во чисел в матрице

	curpos = malloc( (1+sn)*sizeof(int) );
	for(i = 0; i <= sn; i++)
		curpos[i] = 0;

	printf("Matrix sending begun...\n");
	while(completed < sn){
		FD_ZERO(&wd);
		for(i = 1; i <= sn; i++){
			FD_SET(serv[i].socket, &wd);
			nfds = max(nfds, serv[i].socket);
		}
		retval = select(nfds + 1, 0, &wd, 0, NULL);
		for(i = 1; i <= sn; i++)
			if( (curpos[i] < nn) && FD_ISSET(serv[i].socket, &wd)){
				if( (bytes = write(serv[i].socket, a->val+curpos[i], sizeof(double)*(nn-curpos[i]) ))<0 ) {
					perror("m_val_send:write");
					fprintf(stderr, "curpos=%d\n", curpos[i]);
					for(j = 1; j <= sn; j++)
						close(serv[j].socket);
					exit(EXIT_FAILURE);
				}
				if(bytes == 0){
					for(j = 1; j <= sn; j++)
						close(serv[j].socket);
					printf("Потеряно соединение с одним из серверов\n");
					exit(EXIT_FAILURE);
				}
				curpos[i] += bytes/sizeof(double);
				if(nn == curpos[i])
					completed++;
			}
		
	}
	printf("Matrix sent...\n");
}