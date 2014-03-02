#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFERSIZE		256
#define ERROR_NOT_FOUND		"NO"
#define OK			"Ok"
#define BYE			"Bye"

int main(int args, char *argv[]) {

	u_int port;
	int server;
	int client;
	int localerror;
	struct sockaddr_in myAddr;
	struct sockaddr_in cliente_addr;
	socklen_t clienteLen;	
	int status;
	char *filePath;
	int file;
	char byte;
	int fileSize;
	struct stat buf;
	int fileLength;
	char *buffer;
	int readBytes;
	int writeBytes;
	char *message = (char*) calloc(100, sizeof(char));

	//Validamos los Arguemntos
	if(args < 2) {
		fprintf(stderr,"Error: Missing Arguments\n");
		fprintf(stderr,"\tUSE: %s [PORT]\n",argv[0]);
		return 1;
	}

	port = atoi(argv[1]);
	if(port < 1 || port > 65535) {
		fprintf(stderr,"Port %i out of range (1-65535)\n",port);
		return 1;
	}

	//Iniciamos la apertura del Socket
	server = socket(PF_INET,SOCK_STREAM,0);
	if(server == -1) {
		localerror = errno;
		fprintf(stderr, "Error: %s\n",strerror(localerror));
		return 1;
	}

	//Nos adjudicamos el Puerto.
	bzero(&myAddr,sizeof(myAddr));
	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myAddr.sin_port = htons(port);

	status = bind(server, (struct sockaddr *)&myAddr, sizeof(myAddr));
	if(status != 0) {
		localerror = errno;
		fprintf(stderr,"Can't Bind Port: %s\n",strerror(localerror));
		close(server);
		return 1;
	}

	//Nos ponemos el modo de Escucha
	status = listen(server,5);
	if(status == -1) {
		localerror = errno;
		fprintf(stderr,"Can't listen on socket(%s)\n",strerror(localerror));
		close(server);
		return 1;
	}	

	//Esperamos una Conexión
	while(1) {
		bzero(&cliente_addr,sizeof(cliente_addr));
		client = accept(server,(struct sockaddr *)&cliente_addr,&clienteLen);
		if(client == -1) {
			localerror = errno;
			fprintf(stderr,"Error acepting conection %s\n",strerror(localerror));
			close(server);
			return 1;
		}

		//Inicia el protocolo...
		buffer = (char *) calloc(1, BUFFERSIZE);
		//recive nombre de archivo a enviar
		filePath = (char *) calloc(252, sizeof(char));
		char ret = 0;
		char new = 0;
		readBytes = 0;
		int length = 0;

		//printf("READING FILE NAME FROM CLIENT\n");
		while(ret != '\r' && new != '\n' && (readBytes = read(client, buffer, 1)) > 0){
			ret = new;
			new = buffer[0];
			//printf("READ %c FROM CLIENT\n",new);
			if(new != '\r' && new != '\n')
				filePath[length] = new;
			length++;
		}

		//printf("FILENAME READ: %s\n", filePath);

		//verifica existencia de archivo y su tamano
		file = open(filePath, O_RDONLY);

		if(file == -1){
			//envia file not found
			printf("COULDNT OPEN FILE\n");
			writeBytes = 0;
			length = strlen(ERROR_NOT_FOUND);
			printf("Enviando %s al cliente\n", ERROR_NOT_FOUND);
			while(writeBytes < length){
				writeBytes = write(client, ERROR_NOT_FOUND + writeBytes, length - writeBytes);
				printf("Se escribieron %i bytes de %i al cliente\n", writeBytes, length);
			}
		}
		else{
			//Envia OK
			writeBytes = 0;
			length = strlen(OK);
			printf("Enviando %s al cliente\n", OK);
			while(writeBytes < length){
				writeBytes = write(client, OK + writeBytes, length - writeBytes);
				printf("Se escribieron %i bytes de %i al cliente\n", writeBytes, length);
			}
			//get the file size
			fstat(file, &buf);
			fileSize = buf.st_size;
			printf("File Size: %i\n", fileSize);
			//envia filesize
			writeBytes = 0;
			length = 10;
			snprintf(message, 100, "%i", fileSize);
			printf("Enviando %s al cliente\n", message);
			while(writeBytes < length){
				writeBytes = write(client, message + writeBytes, length - writeBytes);
				printf("Se escribieron %i bytes de %i al cliente\n", writeBytes, length);
			}
			//recibe confirmacion
			free(buffer);
			printf("Esperando confirmacion de cliente...\n");
			buffer = (char *) calloc(3, sizeof(char));
			status = read(client, buffer, 2);
			printf("Ciente ha confirmado: %s\n", buffer);
			//envia archivo
			printf("Enviando %s al cliente\n", filePath);
			readBytes = 0;
			writeBytes = 0;
			buffer = (char *) calloc(1,BUFFERSIZE);
			while((readBytes = read(file, buffer, BUFFERSIZE)) > 0){
				printf("Se escribiran %i bytes al cliente\n", readBytes);
				writeBytes = 0;
				while(writeBytes < readBytes){
					writeBytes = write(client, buffer + writeBytes, readBytes - writeBytes);
					printf("Se escribieron %i bytes de %i al cliente\n", writeBytes, readBytes);
				}
			}
			printf("Archivo enviado al cliente\n");
			//recibe BYE
			free(buffer);
			printf("Esperando despedida de cliente...\n");
			buffer = (char *) calloc(4, sizeof(char));
			status = read(client, buffer, 3);
			printf("Ciente se ha despedido: %s\n", buffer);

			//envia BYE
			printf("Despidiendome del cliente...\n");
			status = write(client, BYE, strlen(BYE));
			//termina el protocolo
			printf("Protocolo terminado con exito\n");
		}
		//ciclo para leer el archivo. buffer size
		close(client);
	}

	free(buffer);
	free(message);
	free(filePath);
	close(file);
	return 0;
}
