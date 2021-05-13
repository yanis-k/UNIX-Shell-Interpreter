#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


#define TOKEN_NUM 32        // number of tokens of command
#define CMDs_PERLINE 15     // max number of commands on a line split by ";"
#define CMD_BUFSIZE 600     // max character of command

char *read_cmd(){
	char *buffer;
	buffer=(char *) malloc(CMD_BUFSIZE*sizeof(char));
	gets(buffer);
	buffer = (char *)realloc(buffer,strlen(buffer)*sizeof(char));
	return buffer; 
}


char **split_line(char *line,int *counter){
	int bufsize=CMDs_PERLINE;
	int position=0;
	char **tokens,*token_buf;

	tokens=(char **)malloc(bufsize*sizeof(char *));

	if (!tokens) {
 		  fprintf(stderr, "! buffer : allocation error !\n");
	          exit(EXIT_FAILURE);
   }					  

	
	/* tokens is a 3-cell array of *char , every cell points to a command seperated by " ; " */
	for(token_buf=strtok(line,";");token_buf!=NULL;token_buf=strtok(NULL,";")){
		
		tokens[position]=token_buf; // token_buf points to next token at every loop
					    // always points at a system static buffer 
		(*counter)++;
		position++;
		
		if(position>bufsize){
 		 
			bufsize++;
			tokens=(char **)realloc(tokens,bufsize*sizeof(char *));
		}
	}	
	
	return tokens;
}



char **split_cmd(char *cmd){
	int bufsize=TOKEN_NUM;
	int position=0;
	char **tokens,*token_buf;

	tokens=(char **)malloc(bufsize*sizeof(char *));

	if (!tokens) {
    		fprintf(stderr, "! buffer : allocation error !\n");
    		exit(EXIT_FAILURE);
   }					  

	
/* tokens is a 32-cell array of *char , every cell points to a word of the command seperated by "-, ,\n" */
	for(token_buf=strtok(cmd,"\n\r ");token_buf!=NULL;token_buf=strtok(NULL,"\n\r ")){
	/* tokens[0] points to command name */	
		tokens[position]=token_buf; // token_buf points to next token at every loop always points at a system static buffer  			 
		position++;

		if(position>bufsize){
			fprintf(stderr,"\n ! ERROR : command of more than 32 characters !\n");
			exit(EXIT_FAILURE);
		}
	}	

	return tokens;
}

void execute(char **args){
	if (strcmp(args[0],"quit")){
  		//printf("executing command [%s] from proccess: %d\n", args[0], getpid()); // execvp() ignores "quit" command
		if (execvp(*args,args)==-1) {
			printf("\nUnable to execute command << %s >>\n", args[0]);
		}
	}
	else system("echo Quiting upon completion ");
}

/*-------------------------INTERACTIVE-------------------------------*/

void interactive_mode(int *pid_array, int status){

  char *cmd;
  char **cmd_args;
  char **line_cmds; 
  int cmds_counter;
  int pid,m_pid;  

  m_pid=getpid();

  do{
    cmds_counter=0;
    printf("yanis@prompt>");
    cmd = read_cmd();
    
    line_cmds=split_line(cmd,&cmds_counter);				//cmds_counter : number of commands on one line
    
	for(int i=0 ; i<cmds_counter; i++ ){
		if(strstr(line_cmds[i],"quit")!=NULL) status=1;  		//if "quit" is typed, exit prompt when everything is complete.
	}
	/*
	printf("\n\nline splitted :");
	for (int i=0;i<cmds_counter;i++) printf("( %s ) ",line_cmds[i]); 

	printf("\ncommands used: %d\n-------------------------\n",cmds_counter);	
	*/
	pid_array=(int *) malloc(cmds_counter*sizeof(int));	
		
	for(int i=0 ; i<cmds_counter; i++ ){
		if(getpid()==m_pid){
			pid=fork();
		}
		if(pid==0){	
			pid_array[i]=getpid();		
			cmd_args = split_cmd(line_cmds[i]);
			execute(cmd_args);
		}
		else if(pid<0){ 					//error fork
			printf("\nfork() FAILED, programm terminated\n");
			exit(1);
		}
		
	}
	
	
	for(int i=0 ; i<cmds_counter; i++ ){ //mother waits children to finish their work, in order to prevent "zombie" proccesses
		if(getpid()==m_pid)
			wait(&pid_array[i]);
	 }
 }while(status==0);

}

/*----------------------------BATCH MODE-----------------------------*/


void batch_mode(const char *batch,int *pid_array, int status){
	
  char *cmd;
  char **cmd_args;
  char **line_cmds; 
  int cmds_counter;
  int pid,m_pid;  

  FILE *fp=NULL; 
  if (fp==NULL) printf("\n... channel ok\n");
  fp = fopen(batch, "r"); 
  if (fp!=NULL) printf("... file ok\n");
  
  m_pid=getpid();
  
  cmd=(char *) malloc(CMD_BUFSIZE*sizeof(int));	
  do{
    cmds_counter=0;
        
    fgets(cmd, CMD_BUFSIZE, (FILE *)fp); 
    
    line_cmds=split_line(cmd,&cmds_counter);				//cmds_counter : number of commands on one line
    
	for(int i=0 ; i<cmds_counter; i++ ){
		if(strstr(line_cmds[i],"quit")!=NULL) status=1;  		//if "quit" is typed, exit prompt when everything is complete.
	}
	/*
	printf("\n\nline splitted :");
	for (int i=0;i<cmds_counter;i++) printf("( %s ) ",line_cmds[i]); 

	printf("\ncommands used: %d\n-------------------------\n",cmds_counter);	
	*/
	pid_array=(int *) malloc(cmds_counter*sizeof(int));	
		
	for(int i=0 ; i<cmds_counter; i++ ){
		if(getpid()==m_pid){
			pid=fork();
		}
		if(pid==0){	
			pid_array[i]=getpid();		
			cmd_args = split_cmd(line_cmds[i]);
			execute(cmd_args);
		}
		else if(pid<0){ 					//error fork
			printf("\nfork() FAILED, programm terminated\n");
			exit(1);
		}
		
	}
	
	
	for(int i=0 ; i<cmds_counter; i++ ){ //mother waits children to finish their work, in order to prevent "zombie" proccesses
		if(getpid()==m_pid)
			wait(&pid_array[i]);
	 }
  }while(status==0);
fclose(fp);
if (status==0) exit(1); //Exit program even if no "quit" is typed
}


int main(int argc, char **argv)
{
 
  int *pid_array;
  int status;
  if(argc==1) interactive_mode(pid_array,status);
  else{
    
    	const char *batch_name=argv[1];					// checking for batch's proper extension
    	if(strstr(batch_name,".txt")!=NULL) batch_mode(batch_name,pid_array,status);	// Run batch loop. 
			
    	else {
		fprintf(stderr,"\n! ERROR : Bad file extension, must be .txt!\n");
        	exit(EXIT_FAILURE);
  		}
	}	return 0;
}
