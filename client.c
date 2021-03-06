/**
*
**/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#define BUFFERSIZE 256

int main(int args, char *argv[]) {

	u_int port;
	int server;
	int client;
	int localerror;
	struct sockaddr_in server_addr;
	socklen_t clienteLen;	
	int status;
	char *filePath;
	char *buffer;
	char *sizeOfFile;
	char *nombre;
	#define ok		 "Ok"
	int size = 0;
	int tamano = 0;
	int fileSize = 0;
	int fd;
	int readBytes = 0;
	int writeBytes = 0;

	//Validamos los Argumentos
	if(args < 4){
		fprintf(stderr,"Error: Missing Arguments\n");
		fprintf(stderr,"\tUSE: %s [ADDR] [PORT] [FILENAME]\n",argv[0]);
		return 1;
	}

	//Iniciamos la apertura del Socket
	server = socket(PF_INET,SOCK_STREAM,0);
	if(server == -1) {
		localerror = errno;
		fprintf(stderr, "Error: %s\n",strerror(localerror));
		return 1;
	}

	port = atoi(argv[2]);

	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;	
	status = inet_pton(AF_INET,argv[1],&server_addr.sin_addr.s_addr);
	server_addr.sin_port = htons(port);

	status = connect(server,(struct sockaddr *)&server_addr,sizeof(server_addr));

	if(status != 0) {
		localerror = errno;
		printf("Conection error (%s)\n",strerror(localerror));
		return 1;
	}
	buffer = (char *) calloc(1,BUFFERSIZE);

	//Pedir un archivo
	filePath = (char*) malloc(sizeof(char) * strlen(argv[3]));
	strcpy(filePath, argv[3]);
	fd = open(filePath,O_RDONLY);
	fileSize = strlen(filePath);
	writeBytes = 0;
	filePath[fileSize] = '\r';
	filePath[fileSize+1] = '\n';
	while(writeBytes < fileSize+2){
		writeBytes = write(server, filePath + writeBytes, fileSize+2 - writeBytes);
	}

	//Enviando estado de la conexion
	status = write(server, "Conection established\n", 25);

	//Recibiendo estado del archivo
	status = read(server,buffer,2);
	printf("File state: %s\n",buffer);
	if(strcmp(ok,buffer)!=0){
		return 1;
	}

	//Recibiendo tamaño del archivo
	free(buffer);
	buffer = (char*) calloc(1, BUFFERSIZE);
	status = read(server,buffer, 10);
	printf("Size: %s\n",buffer);
	int totalFileSize = atoi(buffer);

	//Enviando Ok
	status = write(server, "Ok", 2);

	// Leer el archivo:
	readBytes = 0;
	writeBytes = 0;
	if ((fd = open("SendedFile.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))==-1){
		printf("Error openning file\n");
		return 1;
	}
	int totalReadBytes = 0;
	while(totalReadBytes < totalFileSize && (readBytes = read(server, buffer, BUFFERSIZE)) > 0){	
		writeBytes = 0;
		while(writeBytes < readBytes){
			writeBytes = write(fd, buffer + writeBytes, readBytes - writeBytes);
		}
		printf("Readed %i bytes of %i from server\n", readBytes, totalFileSize);
		totalReadBytes += readBytes;	
	}

	//Enviando despedida
	status = write(server, "Bye", 3);
	
	//Recibiendo despedida
	free(buffer);
	buffer = (char*) calloc(4, sizeof(char));
	status = read(server, buffer, 3);
	printf("Server sends: %s\n", buffer);

	close(fd);
	free(buffer);
	close(server);
}
