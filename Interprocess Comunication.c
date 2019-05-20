#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

unsigned char buff[1];
char *s, *sharedMem, *sharedFile;
char size;
int shmId, memSize, fdResp, fdReq, fd;
off_t fileSize;

int convert(unsigned char *buff, int bytes){
	int number = 0;
	for(int i = 0; i < bytes; i++){
		number = number | buff[i] << 8*i;
	}
	return number;
}

int nextMul(int point, int mul){
	while(point % mul != 0){
		point ++;
	}
	return point;
}

void convertInt(int number, char* buffer){
	buffer[3] = (number >> 24) & 0xFF;
	buffer[2] = (number >> 16) & 0xFF;
	buffer[1] = (number >> 8) & 0xFF;
	buffer[0] = number & 0xFF;
}

void success(char *msg){
	size = strlen(msg);
	write(fdResp, &size, 1);
	write(fdResp, msg, size);
	size = 7;
	write(fdResp, &size, 1);
	write(fdResp, "SUCCESS", size);
}

void error(char *msg){
	size = strlen(msg);
	write(fdResp, &size, 1);
	write(fdResp, msg, size);
	size = 5;
	write(fdResp, &size, 1);
	write(fdResp, "ERROR", size);
}

void init(){
	if(mkfifo("RESP_PIPE_84672", 0664) < 0){
		perror("ERROR\ncannot create response pipe");
		exit(1);
	}
	fdResp = open("RESP_PIPE_84672", O_RDWR);
	fdReq = open("REQ_PIPE_84672", O_RDWR);
	if(fdResp < 0){
		perror("ERROR\ncannot open the response pipe");
		exit(1);
	}
	if(fdReq < 0){
		perror("ERROR\ncannot open the request pipe");
		exit(1);
	}
}

void ping(){
	size = 4;
	write(fdResp, &size, 1);
	write(fdResp, "PING", 4);
	write(fdResp, &size, 1);
	write(fdResp, "PONG", 4);
	unsigned int number = 84672;
	write(fdResp, &number, 5);
}

int create_shm(){
	read(fdReq, s, 10);
	memSize = convert((unsigned char*)s, strlen(s));
	shmId = shmget(15616, memSize, IPC_CREAT | 0644);
	if(shmId < 0) {
		error("CREATE_SHM");
		return -1;
	}
	sharedMem = (char*)malloc(memSize * sizeof(char));
	sharedMem = (char*)shmat(shmId, 0, 0);
	if(sharedMem == (void*)-1){
		error("CREATE_SHM");
		return -1;
	}
	else{
		success("CREATE_SHM");
	}
	return 1;
}

int write_shm(){
	unsigned int offset;
	read(fdReq, s, sizeof(unsigned int));
	offset = (int)convert((unsigned char*)s, strlen(s));
	read(fdReq, s, sizeof(unsigned int));
	if(offset > 0 && offset + sizeof(convert((unsigned char*)s, strlen(s))) < memSize){
		sprintf((sharedMem + offset), "%s", s);
		success("WRITE_TO_SHM");
		return 1;
	}
	else{
		error("WRITE_TO_SHM");
		return -1;
	}
}

int map_file(){
	//int fd;
	read(fdReq, buff, 1);
	read(fdReq, s, buff[0]);
	s[buff[0]] = '\0';
	if((fd = open(s, O_RDWR)) < 0){
		error("MAP_FILE");
		return -1;
	}
	fileSize = lseek(fd, 0, SEEK_END);
	sharedFile = (char*)malloc(fileSize);
	lseek(fd, 0, SEEK_SET);
	sharedFile = (char*)mmap(sharedFile, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
	if(sharedFile == (void*)-1){
		error("MAP_FILE");
		return -1;
	}
	success("MAP_FILE");
	return 1;
}

int read_offset(){
	int offset, nrBytes;
	read(fdReq, s, sizeof(unsigned int));
	offset = convert((unsigned char*)s, strlen(s));
	read(fdReq, s, sizeof(unsigned int));
	nrBytes = convert((unsigned char*)s, strlen(s));
	if(offset > 0 && offset + nrBytes < fileSize){
		snprintf(sharedMem, nrBytes + 1, "%s", sharedFile + offset);
		success("READ_FROM_FILE_OFFSET");
		return 1;
	}
	else{
		error("READ_FROM_FILE_OFFSET");
		return -1;
	}
}

int read_section(){
	int offset, nrBytes, secNr, secOff, oux;
	read(fdReq, s, sizeof(unsigned int));
	secNr = convert((unsigned char*)s, strlen(s));
	read(fdReq, s, sizeof(unsigned int));
	offset = convert((unsigned char*)s, strlen(s));
	read(fdReq, s, sizeof(unsigned int));
	nrBytes = convert((unsigned char*)s, strlen(s));

	char *sections;
	sections = (char*)malloc(sizeof(char) * 4);
	snprintf(sections, 2, "%s", sharedFile + 8);
	oux = convert((unsigned char*)sections, 1);
	if(secNr > oux){
		error("READ_FROM_FILE_SECTION");
		return -1;
	}

	snprintf(sections, 4, "%s", sharedFile + 29 + (28*(secNr-1)));
	secOff = convert((unsigned char*)sections, 4);
	snprintf(sharedMem, nrBytes+1, "%s", sharedFile + secOff + offset);
	success("READ_FROM_FILE_SECTION");	
	free(sections);
	return 1;
}

int read_logic(){
	char *memory;
	int offset, nrBytes, nrSec, secOff, point, size;
	read(fdReq, s, sizeof(unsigned int));
	offset = convert((unsigned char*)s, strlen(s));
	read(fdReq, s, sizeof(unsigned int));
	nrBytes = convert((unsigned char*)s, strlen(s));

	char *sections;
	sections = (char*)malloc(sizeof(char) * 4);
	snprintf(sections, 3, "%s", sharedFile + 8);
	nrSec = convert((unsigned char*)sections, 1);
	memory = (char*)malloc(fileSize * 4100);
	point = 0;

	for(int i = 1; i <= nrSec; i++){
		snprintf(sections, 5, "%s", sharedFile + 29 + (28*(i-1)));
		secOff = convert((unsigned char*)sections, 4);
		snprintf(sections, 5, "%s", sharedFile + 33 + (28*(i-1)));
		size = convert((unsigned char*)sections, 4);
		point = nextMul(point, 4096);
		snprintf(memory + point, size + 1, "%s", sharedFile + secOff);
		point += size;
	}

	if(offset < point){
		snprintf(sharedMem, nrBytes + 1, "%s", memory + offset);
		success("READ_FROM_LOGICAL_SPACE_OFFSET");
		return 1;	
	}
	else{
		error("READ_FROM_LOGICAL_SPACE_OFFSET");
		return -1;
	}
}

int main(int argc, char** argv){
	init();
	size = 7;
	write(fdResp, &size, 1);
	if(write(fdResp, "CONNECT", 7) < 0){
		perror("ERROR\ncannot write to pipe");
		exit(1);
	}
	printf("SUCCESS\n");
	if(read(fdReq, buff, 1) < 0){
		perror("ERROR\ncannot read from pipe");
		exit(1);
	}
	s = (char*)malloc(sizeof(char) * buff[0]);
	read(fdReq, s, buff[0]);
	while(strcmp(s, "EXIT") != 0){
		if(strcmp(s,"PING") == 0){
			ping();
		}
		if(strcmp(s, "CREATE_SHM") == 0){
			create_shm();
		}
		if(strcmp(s, "WRITE_TO_SHM") == 0){
			write_shm();
		}
		if(strcmp(s, "MAP_FILE") == 0){
			map_file();
		}
		if(strcmp(s, "READ_FROM_FILE_OFFSET") == 0){
			read_offset();
		}
		if(strcmp(s, "READ_FROM_FILE_SECTION") == 0){
			read_section();
		}
		if(strcmp(s, "READ_FROM_LOGICAL_SPACE_OFFSET") == 0){
			read_logic();

		}
		read(fdReq, buff, 1);
		read(fdReq, s, buff[0]);
		s[buff[0]] = '\0';
	}
}
