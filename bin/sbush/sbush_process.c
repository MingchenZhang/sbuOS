#include<stdio.h>
#include<stdlib.h>
#include "include/sbush_process.h"
#include "include/sbush_exec.h"

#define NUMBER_OF_TOKENS_BY_PIPE 20
#define NUMBER_OF_TOKENS_BY_SPACE 20
#define NUMBER_OF_CHAR_EACH_TOKEN 50

int selfdefine_stringlen(char* string);
int isBackground(char**save_tokens, int* number_of_tokens);
void deal_Ampercent(char** save_tokens,int token_index);
char** ParseBySpace(char* token, int*argc);
int lastCharIsAmpercent(char* string);
int self_strComp(char*str1, char* str2);
void makeEveryElementNUll(char array[],int size);
void self_strCpy(char* des,char*src);
void self_memCpy(void* des, void* src, int size);
void stripExtraSpace(char* token);
void removeSpaceOnTheEnd(char* token);
int ParseCommand(char*command, char**save_tokens, int*index);
int ProcessCommand (char* command);


/*int main (int argc, char**argv){
	ProcessCommand("abc def | ghi jkl mno | qqq www&");
	return 1;
}*/



int ProcessCommand (char* command){
	char**save_tokens= malloc(sizeof(char*)*NUMBER_OF_TOKENS_BY_PIPE); 
	int i;
	for(i=0;i<NUMBER_OF_TOKENS_BY_PIPE;i++){
		char* sample=malloc(sizeof(char)*NUMBER_OF_CHAR_EACH_TOKEN);  
		save_tokens[i]=sample;
	}
	int number_of_tokens=0; // tokenize with |
	ParseCommand(command, save_tokens,&number_of_tokens); 
	Job job;
	if (number_of_tokens==0) { 
		job.first_process=NULL; 
		int k;	
		for(k=0;k<NUMBER_OF_TOKENS_BY_PIPE;k++){ 	  	
			free(*(save_tokens+k)); // free all the stuff in line 
		 }
		free(save_tokens);
		return 0;
	}	
	int isItBackground=isBackground(save_tokens, &number_of_tokens);
	Process* iteratorProcess=NULL;
	Process*pro_pointer_add_holder[NUMBER_OF_TOKENS_BY_PIPE];
	int argc;
	int j;	
	for(j=0;j<number_of_tokens;j++){
		removeSpaceOnTheEnd(*(save_tokens+j)); 
		stripExtraSpace(*(save_tokens+j)); 
	    char**argv=ParseBySpace(*(save_tokens+j),&argc);
		Process * process = malloc(sizeof(Process));
		pro_pointer_add_holder[j]=process;
		process->next=NULL;
		process->path=argv[0];
		process->argv=argv;
		process->argc=argc;
		process->fd_in=-1;
		process->fd_out=-1;
		process->process_id=-1;
		if(iteratorProcess!=NULL){
			iteratorProcess->next=process;
		}
		if(j==0){ // first process 
			job.first_process=process;
			if(isItBackground) job.isBackground=1;
			else if(!isItBackground) job.isBackground=0;
			iteratorProcess=process;
		}
		
		if(j!=0) iteratorProcess=process;
	}
	execute(job);
	/*Process* test_iterator=job.first_process;
	printf("the job is background value %d\n", job.isBackground);
	while(test_iterator!=NULL){
		printf("the process path is %s\n",test_iterator->path);
		int argv_i=0;
		while(test_iterator->argv[argv_i]!=0){
			printf("the %d argv is %s\n",argv_i, test_iterator->argv[argv_i]);
			argv_i++;
		}
		test_iterator=test_iterator->next;
	
	}*/
	//free all the stuff
	int a;
	for(a=0;a<number_of_tokens;a++){
		int b;
		for(b=0;b<pro_pointer_add_holder[a]->argc;b++){
			free( pro_pointer_add_holder[a]->argv[b]);
	}
		free(pro_pointer_add_holder[a]->argv); 
	}
	int k;	
	for(k=0;k<NUMBER_OF_TOKENS_BY_PIPE;k++){ 	  	
		free(*(save_tokens+k)); 
	}
	free(save_tokens);
	return 1;
}








int selfdefine_stringlen(char* string){
	int i=0;
	while(*(string+i)!='\0'){
		i++;
	}
 	return i;
}
int isBackground(char**save_tokens, int * number_of_tokens){	
	if(  save_tokens[*number_of_tokens-1][selfdefine_stringlen(save_tokens[*number_of_tokens-1])-1]=='&'){
	    return 1;
	}
	else
		return 0;
}


char** ParseBySpace(char* token, int*argc){
	char**argv= malloc(sizeof(char*)*NUMBER_OF_TOKENS_BY_SPACE); //20 argv
	/*int i;
	for(i=0;i<NUMBER_OF_TOKENS_BY_SPACE;i++){
		char* sample=malloc(sizeof(char)*NUMBER_OF_CHAR_EACH_TOKEN);  
		argv[i]=sample;
	}	*/
	int tokenStringIndex=0;
	char save_Char[NUMBER_OF_CHAR_EACH_TOKEN];
	makeEveryElementNUll(save_Char,NUMBER_OF_CHAR_EACH_TOKEN);
	int save_CharIndex=0;
	int argv_index=0;
	while(*(token+tokenStringIndex)!='\0'){
		if(*(token+tokenStringIndex) !=' ' ){		 
			save_Char[save_CharIndex]=*(token+tokenStringIndex);
			tokenStringIndex++;
			save_CharIndex++;
		}
		else if(*(token+tokenStringIndex)==' '){
		     argv[argv_index]=malloc(sizeof(char)*NUMBER_OF_CHAR_EACH_TOKEN);
			 self_strCpy(* (argv+argv_index),save_Char);
			 argv_index++;
			 save_CharIndex=0;
			 tokenStringIndex++;
			 makeEveryElementNUll(save_Char,NUMBER_OF_CHAR_EACH_TOKEN);	         
			 continue;
		}
		
	}	
	argv[argv_index]=malloc(sizeof(char)*NUMBER_OF_CHAR_EACH_TOKEN);
	self_strCpy(* (argv+argv_index),save_Char);
	//case1 last token is & eg: ls & 
	if(  self_strComp(*(argv+argv_index),"&")==0  ){
		free(argv[argv_index]);
		argv[argv_index]=0;
		*argc=argv_index;
		return argv;
	}
	//case2 last character of last token is & eg: ls&	
	else if(lastCharIsAmpercent(*(argv+argv_index))){
		int Amp_position= selfdefine_stringlen(*(argv+argv_index))-1;
		argv[argv_index][Amp_position]='\0';
		argv_index++;
		argv[argv_index]=0;
		*argc=argv_index;
		return argv;
	}
	//case3 None of the above cases 
	argv_index++;
	argv[argv_index]=0;
	*argc=argv_index;
	return argv;
}


int lastCharIsAmpercent(char* string){
	int length=selfdefine_stringlen(string);
	if(string[length-1]=='&')
		return 1;
	return 0;
}
int self_strComp(char*str1, char* str2){ //if s1 > < or =s2
	while (*str1 && *str2 && *str1 == *str2) {
		++str1; ++str2; 
	}
    return (unsigned char)(*str1) - (unsigned char)(*str2);
}


void self_strCpy(char* des,char*src){
	int Length=selfdefine_stringlen(src);
	int i;
	for(i=0;i<Length;i++){
		des[i]=src[i];
	}
	des[Length]='\0';
}
void self_memCpy(void* des, void* src, int size){
	int i;
	for(i=0;i<size;i++){
		((char*)des)[i]=((char*)src )[i];
	}
}

void stripExtraSpace(char* token){
	int index;
	int new_index;
	for(index=new_index=0;token[index];index++)
			if(token[index]!=' '||(index>0 && token[index-1]!=' '))
				token[new_index++]=token[index];
	token[new_index]='\0';
}
void removeSpaceOnTheEnd(char* token){
	//remove sapce in the end
	int tokenLength=selfdefine_stringlen(token);
	int back_index=tokenLength-1;
	while(token[back_index]==' '){
		token[back_index]='\0';
		back_index--;
	}
	//remove space in the front
	tokenLength=selfdefine_stringlen(token);
	int front_index=0;
	while(front_index<tokenLength){
		if(token[front_index]==' '){
		 	front_index++;
		}
		else if (token[front_index]!=' '){
			break;
		}
	}
	char template[100];
	self_strCpy(template, token+front_index);
	self_strCpy(token,template);
}


int ParseCommand(char*command, char**save_tokens, int*index){
	char*backup = malloc(selfdefine_stringlen(command)*sizeof(char));
	self_strCpy(backup,command);
	removeSpaceOnTheEnd(backup);
	if (selfdefine_stringlen(backup)==0){
		free(backup);
		//printf("it is a command with all spaces\n");
		return -1;
	}
	int backup_stringIndex=0;
	char save_tokenChar[NUMBER_OF_CHAR_EACH_TOKEN];
	makeEveryElementNUll(save_tokenChar,NUMBER_OF_CHAR_EACH_TOKEN);
	int save_tokenCharIndex=0;
	int save_tokens_index=0;
	while(*(backup+backup_stringIndex)!='\0'){
		if(*(backup+backup_stringIndex) !='|' ){		 
			save_tokenChar[save_tokenCharIndex]=*(backup+backup_stringIndex);
			backup_stringIndex++;
			save_tokenCharIndex++;
		}
		else if(*(backup+backup_stringIndex)=='|'){
		     self_strCpy(* (save_tokens+save_tokens_index),save_tokenChar);
			 save_tokens_index++;
			 save_tokenCharIndex=0;
			 backup_stringIndex++;
			 makeEveryElementNUll(save_tokenChar,NUMBER_OF_CHAR_EACH_TOKEN);	         
			 continue;
		}
		
	}	
	self_strCpy(* (save_tokens+save_tokens_index),save_tokenChar);
	save_tokens_index++;
	*index=save_tokens_index;
	free (backup);
	return 1;
}

void makeEveryElementNUll(char array[],int size){
	int i;
	for(i=0;i<size;i++)
	array[i]='\0';
}