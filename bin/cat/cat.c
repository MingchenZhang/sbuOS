#include<stdio.h>
#include<unistd.h>
#include <sys/defs.h>
#define O_RDWR 0X0002

void setUp_fd(int argc , char**argv, int fd_array[20], int* isStdin_cat, int* number_of_fd);
int stdin_cat();
void write_string(char*buf,int fd);

int main(int argc, char**argv){
	int fd_array[20];
	int number_of_fd=0;
	int isStdin_cat=-1;
	setUp_fd(argc,argv,fd_array, &isStdin_cat, &number_of_fd);
	if (isStdin_cat){  // if it is cat from STDIN 
		return stdin_cat();
	}
	else if (!isStdin_cat){	
		int file_index;
		for(file_index=0;file_index<number_of_fd;file_index++){	
		char buf[4096];
		int buf_index=0;
		while(1){
			if (fd_array[file_index]==-1){
				puts("unable to read from file");
				puts(argv[file_index+1]);
				file_index=file_index+1;
				if(file_index>=number_of_fd){
					return 1;
				}
				else {
					continue;
				}
			}			
			int read_return = read(fd_array[file_index],buf+buf_index,1);
			if(file_index+1>number_of_fd){
				return 1;
			}
			else if( read_return==0 && file_index+1<number_of_fd ){
				buf[buf_index++]='\n';
				buf[buf_index++]='\0';
				write_string(buf,1);
				buf_index=0;
				file_index=file_index+1;
				continue;
			}
			else if(read_return==0 && file_index+1>=number_of_fd){
				buf[buf_index++]='\n';
				buf[buf_index++]='\0';
				write_string(buf,1);
				buf_index=0;
				return 1;							
			}
			buf_index++;
		
		}
	return 1;
	}		
	}	
}

void setUp_fd(int argc , char**argv, int fd_array[20], int* isStdin_cat, int* number_of_fd){
	if(argc==1){
		*isStdin_cat=1;
		*number_of_fd=1;
		fd_array[0]=0;
	}
	else{
		*isStdin_cat=0;
		int i;
		for(i=0;i<argc-1;i++){
			fd_array[i]=open(argv[i+1],O_RDWR);
			*number_of_fd=*number_of_fd+1;
		}
	}
}

int stdin_cat(){
	char buf[128];
	int buf_index=0;
	while(1){
			int read_return = read(0,buf+buf_index,1);
			if(read_return<0){
				puts("unable to read from stdin, error\n");
			}
			else if( buf[buf_index]=='\n'){
				buf[buf_index++]='\n';
				buf[buf_index++]='\0';
				write_string(buf,1);//stdout_fd=1
				buf_index=0;
				continue;
			}
		 	else if(read_return==0){
		 		break;
		 	}
			buf_index++;
		}
	return 1;
}

void write_string(char*buf,int fd){
	int index=0;
	while(buf[index]!='\0'){
		write(fd,buf+index,1);
		index++;
	}
	
}