#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#define MAXLINE 512
#define MAXMEM 10
#define NAMELEN 20
#define SERV_PORT 8080
#define LISTENQ 5

int listenfd,connfd[MAXMEM];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char user[MAXMEM][NAMELEN];
char win;
void Quit();
void rcv_snd(int n);

int main()	{

	pthread_t thread;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t length;
	char buff[MAXLINE];

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	if(listenfd < 0) {
		printf("Socket created failed.\n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("Bind failed.\n");
		return -1;
	}

	printf("listening...\n");
	listen(listenfd, LISTENQ);

	pthread_create(&thread, NULL, (void*)(&Quit), NULL);

	int i=0;
	for(i=0; i<MAXMEM; i++) {
		connfd[i]=-1;
	}
	memset(user, '\0', sizeof(user));
	printf("initialize...\n");

	while(1) {

		length = sizeof(cli_addr);

		for(i=0; i<MAXMEM; i++) {

			if(connfd[i]==-1) {
				break;
			}
		}

		printf("receiving...\n");
		connfd[i] = accept(listenfd, (struct sockaddr*)&cli_addr, &length);

		pthread_create(malloc(sizeof(pthread_t)), NULL, (void*)(&rcv_snd), (void*)i);
	}

	return 0;
}

void Quit()	{

	char msg[10];
	while(1) {

		scanf("%s", msg);

		if(strcmp("/quit",msg)==0) {

			printf("Bye~\n");
			close(listenfd);
			exit(0);
		}
	}
}

void rcv_snd(int n)	{

	char notify[1024];
	char recvbuf[1024];
	char sendbuf[1024];
	char who[1024];
	char name[NAMELEN];
	char msg[1024];
	char wresult[] = "YOU WIN!\n";
	char lresult[] = "YOU LOSE!\n";
	char even[] = "EVEN\n";

	int i = 0;
	int retval;

	//獲得client的名字
	int length;
	length = recv(connfd[n], name, 6, 0);
	if(length > 0) {

		name[length] = 0;
		strcpy(user[n], name);
	}

	//告知所有人有新client加入
	memset(notify, '\0', sizeof(notify));
	strcpy(notify, name);
	strcat(notify, " join\n");
	for(i = 0; i < MAXMEM; i++) {

		if(connfd[i] != -1)
			send(connfd[i], notify, strlen(notify), 0);
	}

	//接收某client的訊息並轉發
	while(1) {

		memset(recvbuf, '\0', sizeof(recvbuf));
		memset(sendbuf, '\0', sizeof(sendbuf));

		if((length = recv(connfd[n], recvbuf, 1024, 0)) > 0) {

			recvbuf[length]=0;

			if(strncmp("/quit", recvbuf, 5) == 0) {

				close(connfd[n]);
				connfd[n]=-1;
				pthread_exit(&retval);
			}

			//顯示目前在線
			else if(strncmp("/list", recvbuf, 5) == 0) {

				strcpy(sendbuf, "<SERVER> Online:");

				for(i = 0; i < MAXMEM; i++) {

					if(connfd[i] != -1) {
						strcat(sendbuf, user[i]);
						strcat(sendbuf, " ");
					}
				}

				strcat(sendbuf, "\n");
				send(connfd[n], sendbuf, strlen(sendbuf), 0);
			}

			else if(strncmp("/game", recvbuf, 5) == 0)	{

				strcpy(msg, "CHOOSE YOUR OPPONENT: \n");
				send(connfd[n], msg, strlen(msg), 0);

				recv(connfd[n], recvbuf, 1024, 0);

				for(i = 0; i < 5; i++)	{
					if(strncmp(user[i], recvbuf, 5) == 0)
						break;
				}

				memset(msg, 0, sizeof(msg));
				memset(msg, 0, sizeof(recvbuf));

				strcpy(msg, "YES or NO\n");
				send(connfd[i], msg, strlen(msg), 0);
				recv(connfd[i], recvbuf, 1024, 0);

				if(strncmp("YES", recvbuf, 3) == 0)	{

					int p1 = n; //O
					int p2 = i; //X

					int turn = 1;

					strcpy(sendbuf, "START, You are p2(X)\n");
					send(connfd[p2], sendbuf, strlen(sendbuf)+1, 0);

					memset(sendbuf, 0, sizeof(sendbuf));

					strcpy(sendbuf, "START, You are p1(O)\n");
					send(connfd[p1], sendbuf, strlen(sendbuf)+1, 0);

					char bd[3][3];
					int flag[10] = {0};
					int j, k;
					for(j = 0; j < 3; j++)	{
						for(k = 0; k < 3; k++)
							bd[j][k] = '*';
					}

					int cnt = 0;
					int x1, y1, x2, y2;
					char cur;
					while(cnt++ < 9)	{

						if(turn == 1)	{

							cur = 'O';
							memset(recvbuf, 0, sizeof(recvbuf));
							memset(msg, 0, sizeof(msg));

							strcpy(msg, "YOUR TURN\n");

							while(1)	{
								if(send(connfd[p1], msg, strlen(msg), 0) > 0)
									break;
							}

							while(1)	{
								if(recv(connfd[p1], recvbuf, 1024, 0) > 0)
									break;
							}

							int r = atoi(recvbuf);
							x1 = r / 10;
							y1 = r % 10;

							bd[x1][y1] = 'O';
							turn = 2;
						}

						else if(turn == 2)	{

							cur = 'X';
							memset(recvbuf, 0, sizeof(recvbuf));

							while(1)	{
								if(send(connfd[p2], msg, strlen(msg), 0) > 0)
									break;
							}

							while(1)	{
								if(recv(connfd[p2], recvbuf, 1024, 0)>0)
									break;
							}

							int r = atoi(recvbuf);
							x2 = r / 10;
							y2 = r % 10;

							bd[x2][y2] = 'X';
							turn = 1;
						}

						if(bd[0][0] == cur && bd[1][1] == cur && bd[2][2] == cur)
							win = cur;

						if(bd[0][2] == cur && bd[1][1] == cur && bd[2][0] == cur)
							win = cur;

						int row;
						for(row = 0; row < 3; row++)	{

							if(bd[0][row] == cur && bd[1][row] == cur && bd[2][row] == cur)	{
								win = cur;
								break;
							}
						}

						int col;
						for(col = 0; col < 3; col++)	{

							if(bd[col][0] == cur && bd[col][1] == cur && bd[col][2] == cur)	{
								win = cur;
								break;
							}
						}

						for(j = 0; j < 3; j++)	{
							for(k = 0; k < 3; k++)
								printf("%c ", bd[j][k]);
							printf("\n");
						}

						printf("\n");

						if(win == 'O')	{

							send(connfd[p1], wresult, strlen(wresult), 0);
							send(connfd[p2], lresult, strlen(lresult), 0);
							break;
						}

						else if(win == 'X')	{

							send(connfd[p1], wresult, strlen(wresult), 0);
							send(connfd[p2], lresult, strlen(lresult), 0);
							break;
						}

						else if(cnt == 9)	{

							send(connfd[p1], even, strlen(even), 0);
							send(connfd[p2], even, strlen(even), 0);
							break;
						}
					}
				}
			}

			//直接傳給每個人
			else {
				strcpy(sendbuf, name);
				strcat(sendbuf,": ");
				strcat(sendbuf, recvbuf);

				for(i = 0; i < MAXMEM; i++) {

					if(connfd[i] != -1) {

						if(strcmp(name, user[i])==0)
							continue;

						else
							send(connfd[i], sendbuf, strlen(sendbuf), 0);
					}
				}
			}
		}
	}
}
