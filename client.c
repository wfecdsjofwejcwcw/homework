#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<errno.h>

#define MESSAGE_BUFF 500
#define USERNAME_BUFF 10

typedef struct {
	char* prompt;
	int socket;

}thread_data;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char name[5][10];
char pwd[5][10];

void init()	{

	strcpy(name[0], "user0\0");
	strcpy(name[1], "user1\0");
	strcpy(name[2], "user2\0");
	strcpy(name[3], "user3\0");

	strcpy(pwd[0], "000\0");
	strcpy(pwd[1], "111\0");
	strcpy(pwd[2], "222\0");
	strcpy(pwd[3], "333\0");
}

//打字、傳送訊息
void* send_message(char prompt[USERNAME_BUFF+4], int socket_fd, struct sockaddr_in *address, char username[])	{

	char message[MESSAGE_BUFF];
	char buff[MESSAGE_BUFF];
	char notice[]="/send";

	memset(message, '\0', sizeof(message));
	memset(buff, '\0', sizeof(buff));

	send(socket_fd, username, strlen(username), 0);

	while(fgets(message, MESSAGE_BUFF, stdin)!=NULL) {

		printf("%s",prompt);

		if(strncmp(message, "/quit", 5)==0) {
			send(socket_fd, message, strlen(message), 0);

			printf("Close connection...\n");
			exit(0);
		}

		send(socket_fd, message, strlen(message), 0);
		memset(message, '\0', sizeof(message));
	}
}

void* receive(void* threadData)	{

	int socket_fd, response;
	char message[MESSAGE_BUFF];
	thread_data* pData = (thread_data*)threadData;
	socket_fd = pData->socket;
	char* prompt = pData->prompt;

	char buff[MESSAGE_BUFF];

	//接收訊息
	while(1) {

		memset(message, '\0', MESSAGE_BUFF);
		memset(buff, '\0', sizeof(buff));

		fflush(stdout);

		response = recv(socket_fd, message, MESSAGE_BUFF, 0);
		if (response == -1) {

			fprintf(stderr, "recv() failed: %s\n", strerror(errno));
			break;
		}

		else if (response == 0) {

			printf("\nPeer disconnected\n");
			break;
		}

		printf("%s", message);
		printf("%s", prompt);

		fflush(stdout);
	}
}

int main()	{

	init();

	struct sockaddr_in address;
	int socket_fd, response;
	char prompt[USERNAME_BUFF+4];
	char username[USERNAME_BUFF];
	char buf[100];
	pthread_t thread;

	//連線設定
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(8080);
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	if(connect(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0)	{
		perror("CONNECT");
		exit(1);
	}

	printf("CONNECTED\n");

	//進入大廳
	printf("Enter your name: ");
	fgets(username, USERNAME_BUFF, stdin);
	username[strlen(username) - 1] = 0;

	int i;
	for(i = 0; i < 5; i++)	{

		if(strncmp(name[i], username, 5) == 0)
			break;
	}

	printf("PASSWORD: ");
	
	memset(buf, 0, sizeof(buf));	
	fgets(buf, sizeof(buf), stdin);
	
	if(strncmp(buf, pwd[i], 3) == 0)	{
		printf("LOGIN SUCCESSFULLY\n");
		strcpy(prompt, username);
		strcat(prompt, "> ");
	}

	else if(strncmp(buf, pwd[i], 3) != 0)	{
		printf("LOGIN FAILURE\n");
		exit(1);
	}

	//建thread data
	thread_data data;
	data.prompt = prompt;
	data.socket = socket_fd;

	//建thread接收訊息
	pthread_create(&thread, NULL, (void *)receive, (void *)&data);

	//傳送訊息
	send_message(prompt, socket_fd, &address,username);

	//關閉
	close(socket_fd);
	pthread_exit(NULL);
	return 0;
}
