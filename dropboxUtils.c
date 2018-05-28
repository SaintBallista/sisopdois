#ifndef DROPBOXUTILS_C
#define DROPBOXUTILS_C

#include"dropboxUtils.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <asm/errno.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pwd.h>

#define TRUE 1
#define FALSE 0

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

void removeBlank(char * filename){
	char aux[200]; int i=0; int j=0;

	const char *homedir;

	if ((homedir = getenv("HOME")) == NULL) {
	    homedir = getpwuid(getuid())->pw_dir;
	}

	strcpy(aux,homedir);
	while(aux[j]!='\0')
		j++;

	while((filename[i]!='\0')&&(filename[i]!='\n')){
		if ((filename[i]!=' ')&&(filename[i]!='~')){
			aux[j] = filename[i];
			j++;
		}
		i++;
	}
	aux[j]= '\0';
	strcpy(filename,aux);
}

int create_home_dir(char *userID){
	//cria diretório com nome do user no HOME do user, chamado pelo cliente
	char *path = malloc((strlen(userID)+15)*sizeof(char));
	//strcpy(dir, "mkdir ~/");
	strcpy(path, "~/sync_dir_");
	strcat(path, userID);    //forma o path utilizando o user id

	int ret=0;

	char *syscmd = malloc((((strlen(userID)+15)*2)+50)*sizeof(char));

	strcpy(syscmd, "if [ ! -d ");   //monta o comando em bash para cria o dir caso ele não excista
	strcat(syscmd, path);
	strcat(syscmd, " ];then\n mkdir ");
	strcat(syscmd, path);
	strcat(syscmd, "\nfi");

	ret = system(syscmd);  //chama o comando
	free(syscmd);

	free(path);
	return ret;
}

int create_home_dir_server(char *userID){
	//cria diretório com nome do user no HOME do user, chamado pelo cliente
	char *path = malloc((strlen(userID)+30)*sizeof(char));
	//strcpy(dir, "mkdir ~/");
	strcpy(path, "~/dropboxserver/sync_dir_");
	strcat(path, userID);

	int ret=0;

	char *syscmd = malloc((((strlen(userID)+23)*2)+50)*sizeof(char));

	strcpy(syscmd, "if [ ! -d ");   //monta o comando em bash para cria o dir caso ele não excista
	strcat(syscmd, path);
	strcat(syscmd, " ];then\n mkdir ");
	strcat(syscmd, path);
	strcat(syscmd, "\nfi");
	ret = system(syscmd);

	free(syscmd);

	free(path);
	return ret;
}

char * getArgument(char* command){
	char* argument;
	int i=0; int j=0;
	argument = (char*) malloc(sizeof(char)*100);
	while(command[i]!=' ')
		i++;
	while(command[i]==' ')
		i++;
	while((command[i]!=' ')&&(command[i]!='\0')){
		argument[j]= command[i];
		i++;
		j++;
	}
	return argument;
}

int create_server_root(){
	//cria diretório raiz do servidor na home da máquina, caso não exista ainda
	char *path = malloc(15*sizeof(char));
	strcpy(path, "~/dropboxserver");
	DIR *dir = opendir(path);
	int ret=0;

	char *syscmd = malloc(65*sizeof(char));

	strcpy(syscmd, "if [ ! -d ");   //monta o comando em bash para cria o dir caso ele não excista
	strcat(syscmd, path);
	strcat(syscmd, " ];then\n mkdir ");
	strcat(syscmd, path);
	strcat(syscmd, "\nfi");
	ret = system(syscmd);

	free(path);
	return ret;
}

int create_server_userdir(char *userID){
	//cria diretório com nome do user no na raiz do servidor
	char *path = malloc((strlen(userID)+17)*sizeof(char));
	strcpy(path, "~/dropboxserver/");
	strcat(path, userID);

	int ret=0;

	char *syscmd = malloc((((strlen(userID)+16)*2)+50)*sizeof(char));

	strcpy(syscmd, "if [ ! -d ");   //monta o comando em bash para cria o dir caso ele não excista
	strcat(syscmd, path);
	strcat(syscmd, " ];then\n mkdir ");
	strcat(syscmd, path);
	strcat(syscmd, "\nfi");
	ret = system(syscmd);

	free(path);
	return ret;
}

int receive_int_from(int socket){
	//retorn número de op recebido no socket socket, ou -1(op inválida) caso erro
	int n;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
	clilen = sizeof(struct sockaddr_in);

	//char *buf = malloc(sizeof(char)); //recebe um char apenas representando um número de op no intervalo [1...algo]
	int receivedInt = 0;

	n = recvfrom(socket,(char*) &receivedInt, sizeof(int), 0, (struct sockaddr *) &cli_addr, &clilen);
	if (n < 0)
		return -1;

	//retorna o ack
	n = sendto(socket, "ACK", 3, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
	if (n  < 0)
		return -1;

	//int ret = atoi(buf);
	//free(buf);
	return ntohl(receivedInt);
}

int send_int_to(int socket, int op){
	//envia uma op como um inteiro, -1 se falha e  0  se sucesso
	int n;
	struct sockaddr_in serv_addr, from;
	//char *buf = malloc(sizeof(char)); //op tem apenas um char/dígito
	//sprintf(buf, "%d", op); //conversão de op para a string buf

	int sendInt = htonl(op);

	n = sendto(socket, (char*) &sendInt, sizeof(int), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
	if (n < 0)
		return -1;

	//free(buf);
	char *buf = malloc(sizeof(char)*3); //para receber o ack

	unsigned int length = sizeof(struct sockaddr_in);
	n = recvfrom(socket, buf, 3*sizeof(char), 0, (struct sockaddr *) &from, &length);
	if (n < 0)
		return -1;

	free(buf);

	return 0;
}

char* receive_string_from(int socket){
	int str_size = receive_int_from(socket); //recebe o tamamho do string a ser lido

	char *buf = malloc(sizeof(char)*str_size);

	int n;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
	clilen = sizeof(struct sockaddr_in);

	n = recvfrom(socket,buf, str_size, 0, (struct sockaddr *) &cli_addr, &clilen);
	if (n < 0)
		return NULL;

	//retorna o ack
	n = sendto(socket, "ACK", 3, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
	if (n  < 0)
		return NULL;

	return buf;
}

int send_string_to(int socket, char* str){
	int str_size = strlen(str);

	send_int_to(socket, str_size); //envia o tam do string a ser lido pro server

	int n;
	struct sockaddr_in serv_addr, from;

	n = sendto(socket, str, str_size, 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
	if (n < 0)
		return -1;

	char *buf = malloc(sizeof(char)*3); //para receber o ack

	unsigned int length = sizeof(struct sockaddr_in);
	n = recvfrom(socket, buf, 3*sizeof(char), 0, (struct sockaddr *) &from, &length);
	if (n < 0)
		return -1;

	free(buf);

	return 0;

}

int receive_file_from(int socket, char* file_name, struct sockaddr sender){
	int n, file;
	socklen_t clilen;
	char buf[CHUNK+10]; //chunk de arquivo + header
	clilen = sizeof(struct sockaddr_in);
	int recebeutudo = FALSE;
	char *bufACK = malloc(sizeof(char)*3); //para receber o ack
	int counter = 0;
	char mensagemdeconfirmacao[100];
	char mensagemdeconfirmacaoanterior[100];
	char mensagemesperada[100];
	struct sockaddr_in serv_addr, from;
	char bufferitoa[100];
	char bufferitoAnt[100];
	int k =0;

	removeBlank(file_name);

	file = open(file_name, O_RDWR | O_CREAT, 0666);
	if (file == -1){
		printf("Houve erro\n");
		printf("path: %s ---\n\n\n\n\n",file_name);
	}

	int endof = FALSE;

	while(!recebeutudo){
		strcpy(mensagemesperada,"");
		strcpy(bufferitoa,"");
		strcat(mensagemesperada,"packet");
		sprintf(bufferitoa,"%d",counter);
		strcat(mensagemesperada,bufferitoa); //mensagem de confirmacao é ACKpacket<numerodopacote>
		strcpy(mensagemdeconfirmacao,"");
		strcat(mensagemdeconfirmacao,"ACKpacket");
		strcat(mensagemdeconfirmacao,bufferitoa); //mensagem de confirmacao é ACKpacket<numerodopacote>
		strcpy(mensagemdeconfirmacaoanterior,"");
		strcat(mensagemdeconfirmacaoanterior,"ACKpacket");
		//strcpy(bufferitoa,"");
		sprintf(bufferitoAnt,"%d",(counter-1));
		strcat(mensagemdeconfirmacaoanterior,bufferitoAnt); //mensagem de confirmacao é ACKpacket<numerodopacote>
		memset(buf,0,CHUNK+10);
		n = recvfrom(socket, buf, CHUNK+50, 0, (struct sockaddr *) &sender, &clilen);


		//printf("\nCounter: %d\tBufITOA: %s\tsize: %d", counter,bufferitoa, strlen(bufferitoa));

		//if(counter==27 || counter ==28)printf("buf: %s\n\n\n",buf);

		//printf("recebemos o pacote: %s \n",buf);

		k=0;
		int offset = (counter==0?1:strlen(bufferitoa))+strlen("packet"); //tam do header do packet
		//printf("\noffset: %d", offset);
		//printf("sizeitoa: %d\tsize packet: %d\n",(counter%10)+1,strlen("packet"));
		while ((k<CHUNK)&&(strncmp(&buf[offset + k],"endoffile",sizeof("endoffile"))!=0) && (endof==FALSE) && (buf[offset + k]!='\0')){
			//write(file,buf+10+k,1);
			write(file,buf+offset+k,1);
			//printf("%s\n",buf+offset+k);
			k++;
		}
		printf("\nk: %d", k);
		if (strncmp(&buf[offset + k],"endoffile",sizeof("endoffile"))==0){
			//printf("Achou end of file!\n");
			printf("%s\n",buf);
			endof = TRUE;
		}

		//fprintf(stderr, "\nchegou aqui wtf\n");

		if(strcmp(buf, "xxxCABOOARQUIVOxxx")==0){ //se recebeu pacote de fiim de arquivo
			recebeutudo = TRUE;
			sendto(socket, "xxxCABOOARQUIVOxxx", sizeof("xxxCABOOARQUIVOxxx"), 0, (const struct sockaddr *) &sender, sizeof(struct sockaddr_in));
			//printf("Recemos a mensagem de fim de arquivo!\n");
		}
		else
			//printf("Msg de ack eh: %s\n", mensagemdeconfirmacao);
			sendto(socket, mensagemdeconfirmacao, sizeof(mensagemdeconfirmacao), 0, (const struct sockaddr *) &sender, sizeof(struct sockaddr_in));
		counter++;
	}
	if (close(file) == -1) {
			 printf("erro no fechamento de arquivo\n");
	 }
	printf("Chegou aqui \n");
	return 0;

}

int send_file_to(int socket, char* file_name, struct sockaddr destination){
	int n, file,counter;
	char buf[CHUNK];
	char bufTrue[CHUNK+OPCODE+20];
	char mensagemdeconfirmacao[100];
	struct sockaddr_in serv_addr, from;
	char bufACK[100]; //para receber o ack
	counter = 0;
	unsigned int length = sizeof(struct sockaddr_in);
	file_name[strlen(file_name)-1]='\0'; // /n que tava vindo de graça

	file = open(file_name, O_RDONLY);;
	char bufferitoa[100];

	printf("filename is %s\n",file_name);
	n=read(file, buf, CHUNK);
//printf("bufread: .%s. size: %d\n\n\n",buf,n);
	while(n>0){
		memset(bufTrue,0,CHUNK+OPCODE+10);
		strcpy(bufTrue,"");
		strcpy(bufferitoa,"");
		strcat(bufTrue,"packet");
		sprintf(bufferitoa,"%d",counter);
		strcat(bufTrue,bufferitoa); //pacote é packet<numerodopacote><DATACHUNK>
		strncat(bufTrue,buf,n);
		strcpy(mensagemdeconfirmacao,"");
		strcat(mensagemdeconfirmacao,"ACKpacket");
		strcat(mensagemdeconfirmacao,bufferitoa); //mensagem de confirmacao é ACKpacket<numerodopacote>
		if (n<1240){
			strcat(bufTrue,"endoffile");
		}
		//printf("buftrue: %s\n\n\n",bufTrue);

		//if(counter==27 || counter ==28)printf("buftrue: %s\n\n\n",bufTrue);


		//int offset = (counter%10)+1/*tamanho do indicador de pacote*/+strlen("packet"); //tam do header do packet

		int offset = (counter==0?1:strlen(bufferitoa))+strlen("packet"); //tam do header do packet

		while(strcmp(bufACK,mensagemdeconfirmacao)){ //enquanto nao forem iguais
			sendto(socket, bufTrue, CHUNK+offset, 0, (const struct sockaddr *) &destination, sizeof(struct sockaddr_in));
			//printf("enviamos o pacote: %s \n",bufTrue);
			n = recvfrom(socket, bufACK, 20, 0, (struct sockaddr *) &from, &length);
			//printf("Recebemos Ack: %s\n",bufACK);
			//usleep(250000);
		}
		n=read(file, buf, CHUNK);
		//printf("tamanho que resta: %d\n",n);
		counter++;
	}
	//envia o sinal de final do arquivo, de tal forma que não precisa indicar tam do arquivo
	sendto(socket, "xxxCABOOARQUIVOxxx", CHUNK, 0, (const struct sockaddr *) &destination, sizeof(struct sockaddr_in));
	printf("Arquivo foi enviado...\n");

	n = recvfrom(socket, bufACK, 3*sizeof(char), 0, (struct sockaddr *) &from, &length);
	if (n < 0)
		return -1;

	//free(bufACK);

	close(file);

	return 0;
}

#endif
