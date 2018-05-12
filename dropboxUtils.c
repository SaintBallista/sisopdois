//#ifndef DROPBOXUTILS_HEADER
//#define DROPBOXUTILS_HEADER
#include"dropboxUtils.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>


struct file_info{
	char name[MAXNAME];
	char extension[MAXNAME];
	char last_modified[MAXNAME];
	int size;
};

struct client{
	int devices[2];
	char userid[MAXNAME];
	struct file_info fi[MAXFILES];
	int logged_in;
};

int create_home_dir(char *userID){
	char *dir = malloc((strlen(userID)+10)*sizeof(char));	
	strcpy(dir, "mkdir ~/");
	strcat(dir, userID);
	int ret = system(dir);
	free(dir);
	return ret;
}


int receive_int_from(int socket){
	//retorn número de op recebido no socket socket, ou -1(op inválida) caso erro
	int n;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
	clilen = sizeof(struct sockaddr_in);

	char *buf = malloc(sizeof(char)); //recebe um char apenas representando um número de op no intervalo [1...algo]
	
	n = recvfrom(socket, buf, sizeof(char), 0, (struct sockaddr *) &cli_addr, &clilen);
	if (n < 0) 
		return -1;
	
	//retorna o ack
	n = sendto(socket, "ACK", 3, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
	if (n  < 0) 
		return -1;

	int ret = atoi(buf);
	free(buf);
	return ret;
}

int send_int_to(int socket, int op){
	//envia uma op como um inteiro, -1 se falha e  0  se sucesso
	int n;	
	struct sockaddr_in serv_addr, from;
	char *buf = malloc(sizeof(char)); //op tem apenas um char/dígito
	sprintf(buf, "%d", op); //conversão de op para a string buf  
	
	n = sendto(socket, buf, sizeof(char), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
	if (n < 0) 
		return -1;

	free(buf);
	buf = malloc(sizeof(char)*3); //para receber o ack
	
	unsigned int length = sizeof(struct sockaddr_in);
	n = recvfrom(socket, buf, 3*sizeof(char), 0, (struct sockaddr *) &from, &length);
	if (n < 0)
		return -1;

	free(buf);

	return 0;
}


//#endif
