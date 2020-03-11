#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define debug(s,n) (n==0?fprintf(stderr, "%s\n",s):fprintf(stderr, "%s%d\n", s,n))
#define prompt "tarang@ubuntu:"
#define clear_screen() printf("\033[H\033[J")
#define debug(s,n) (n==0?fprintf(stderr, "%s\n",s):fprintf(stderr, "%s%d\n", s,n))
#define MAX_NO_OF_COMMANDS 50
#define MAX_COMMAND_LENGTH 1024

int my_cd(char **args);
int my_clear(char **args);
int my_exit(char **args);
int my_pwd(char **args);
void shell_prompt();
char* read_command();
void parse_command1(char* read_line);
//char** parse_command1(char* read_line);
char** parse_command2(char* read_line);
int execute_command(char** subcommands);
int execute_commandset(char command_str[]);
int launch_command(char **args);
void batch_execute(char *filename);
int execute_pipe(char** subcommands, int i);
int execute_out(char** subcommands, int i);
int execute_in(char** subcommands, int i);

char delimiters[MAX_NO_OF_COMMANDS];
char command_subsets[MAX_NO_OF_COMMANDS][MAX_COMMAND_LENGTH];
int row=0,col=0;


/*string array for built-in functions*/
char *my_builtin_str[] = {
  "cd",
  "clear",
  "exit",
  "pwd"
};

/*array of function pointers each of which take char** as parameter and return int*/
int (*my_builtin_func[]) (char **) = {
  &my_cd,
  &my_clear,
  &my_exit,
  &my_pwd
};

/*no of built-in functions, in order to traverse the array*/
int no_of_builtins() {
  return sizeof(my_builtin_str) / sizeof(char *);
}

/*internal pwd command implementation*/
int my_pwd(char **args)
{
	char* pwd=getcwd(NULL,0);
	if(pwd==NULL)
	{
		perror("Error in pwd(): ");
		return 0;
	}
	fprintf(stderr, "%s\n",pwd);
	return 1;
}
/*internal cd command implementation*/
int my_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "No argument to \"cd\"\n");
    return 0;
  } else {
  	
    if (chdir(args[1]) != 0) {
      perror("Error in my_cd():");
      return 0;
    }
  }
  fprintf(stderr, "\'cd\' successful.\n");
  return 1;
}
/*internal clear command implementation*/
int my_clear(char **args)
{
	clear_screen();
	return 1;
}
/*internal exit command implementation*/
int my_exit(char **args)
{
  exit(0);
}

int main(int argc, char* argv[])
{
	
	if(argc==1)
	{
		clear_screen();
		shell_prompt();
	}
	//else if(argc==2)
		//batch_execute(argv[1]);
	else
		printf("Wrong format.\n");

	return 0;
}

void shell_prompt()
{
	int i,count;
	char *read_line;
	char **subcommands1;
	char **subcommands2;
	int status=1;
	while(1)
	{
		count=0;
		row=0;
		col=0;
		memset(delimiters,'\0',MAX_NO_OF_COMMANDS);
		memset(command_subsets, '\0', MAX_NO_OF_COMMANDS*MAX_COMMAND_LENGTH);
		char* pwd=getcwd(NULL,0);
		if(pwd==NULL)
		{
			perror("Error in pwd(): ");
			break;
		}
		printf("%s%s$ ",prompt,pwd);
		read_line=read_command();
		if(read_line[0]=='\0')
			continue;
		parse_command1(read_line);
		for(i=0;i<=row;i++)
		{
			status=execute_commandset(command_subsets[i]);
			if(i!=row)
			{
				if(delimiters[count]==';')
				{
					continue;
				}
				else if(delimiters[count]=='&')
				{
					if(status==0)
						break;
					else
						continue;
				}
				else if(delimiters[count]=='|')
				{
					if(status==1)
						break;
					else
						continue;
				}	
			}		
		} 
	}
}

int execute_commandset(char command_str[])
{
	debug("0",0);
	int i=0,len,count=0,status;
	len=strlen(command_str);
	char* copy=(char*)malloc((len+1)*sizeof(char));
	strcpy(copy, command_str);
	debug(copy,0);
	char delims[MAX_COMMAND_LENGTH];
	char** subcommands=(char**)malloc(MAX_NO_OF_COMMANDS*sizeof(char*));
	char* token=strtok(copy,"|<>");
	while(token!=NULL)
	{
		subcommands[i]=token;
		token=strtok(NULL,"|<>");
		i++;
	}
	subcommands[i]=NULL;
	if(i==1)
	{
		status=execute_command(parse_command2(subcommands[0]));
		return status;
	}
	else
	{
		debug("len: ",len);
		for(i=0;i<len;i++)
		{
			if(command_str[i]=='|'||command_str[i]=='<'||command_str[i]=='>')
				delims[count++]=command_str[i];
		}
		if(count==1)
		{
			if(delims[0]=='|')
			{
				status=execute_pipe(subcommands, 0);
				return status;
			}
			else if(delims[0]=='<')
			{
				status=execute_in(subcommands, 0);
				return status;
			}
			else if(delims[0]=='>')
			{
				status=execute_out(subcommands, 0);
				return status;
			}
		}
		else if(count==2)
		{
			if(delims[0]=='|'&& delims[1]=='>')
			{
				debug("0",0);
				int fd;
				char* filename=subcommands[2];
				debug(filename, 0);
				fd=open(filename, O_WRONLY|O_CREAT, S_IRWXU);
				debug("fd: ",fd);
				if(fd==-1)
				{
					perror("Error in fd():");
					return 0;
				}
				char** subcommands1=parse_command2(subcommands[0]);
				char** subcommands2=parse_command2(subcommands[1]);
				debug("1",0);
				int status;
				int pipefd[2];
				pid_t p1,p2,wpid;

				if(pipe(pipefd)<0)
				{
					perror("Error in pipe(): ");
					return 0;
				}

				p1=fork();
				if(p1<0)
				{
					perror("Error in fork(): ");
					return 0;		
				}

				if(p1==0)
				{
					//child process
					close(pipefd[0]);
					dup2(pipefd[1],STDOUT_FILENO);
					close(pipefd[1]);

					if(execvp(subcommands1[0], subcommands1)<0)
						perror("Error in execvp():");

					exit(0);
				}
				else
				{
					//parent process
					p2=fork();

					if(p2<0)
					{
						perror("Error in fork():");
						return 0;
					}

					if(p2==0)
					{
						//child process
						close(pipefd[1]);
						dup2(pipefd[0], STDIN_FILENO);
						dup2(fd, STDOUT_FILENO);
						close(pipefd[0]);
						if(execvp(subcommands2[0], subcommands2)<0)
							perror("Error in execvp");

						exit(0);
					}
					else
					{
						close(pipefd[0]);
						close(pipefd[1]);
						//parent process
						debug("in1",0);
					    do {
					      wpid = waitpid(p2, &status, WUNTRACED);
					    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
						debug("in2",0);

						do {
					      wpid = waitpid(p2, &status, WUNTRACED);
					    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
						debug("in3",0);
					}	 

				}

			}
			else if(delims[0]=='|'&& delims[1]=='|')
			{
				char** subcommands1=parse_command2(subcommands[0]);
				char** subcommands2=parse_command2(subcommands[1]);
				char** subcommands3=parse_command2(subcommands[2]);
				debug("1",0);
				int status;
				int pipefd[2];
				int pipefd2[2];
				pid_t p1,p2,p3,wpid;

				if(pipe(pipefd)<0)
				{
					perror("Error in pipe(): ");
					return 0;
				}

				p1=fork();
				if(p1<0)
				{
					perror("Error in fork(): ");
					return 0;		
				}

				if(p1==0)
				{
					//child process
					close(pipefd[0]);
					dup2(pipefd[1],STDOUT_FILENO);
					close(pipefd[1]);

					if(execvp(subcommands1[0], subcommands1)<0)
						perror("Error in execvp():");

					exit(0);
				}
				else
				{
					//parent process
					if(pipe(pipefd2)<0)
					{
						perror("Error in pipe(): ");
						return 0;
					}
					p2=fork();

					if(p2<0)
					{
						perror("Error in fork():");
						return 0;
					}

					if(p2==0)
					{
						//child process
						dup2(pipefd[0], STDIN_FILENO);
						dup2(pipefd2[1], STDOUT_FILENO);
						close(pipefd[0]);
						close(pipefd[1]);
						close(pipefd2[0]);
						close(pipefd2[1]);
						if(execvp(subcommands2[0], subcommands2)<0)
							perror("Error in execvp");

						exit(0);
					}
					else
					{
						p3=fork();
						if(p3<0)
						{
							perror("Error in fork():");
							return 0;
						}

						if(p3==0)
						{
							//child process
							dup2(pipefd2[0], STDIN_FILENO);							
							close(pipefd[0]);
							close(pipefd[1]);
							close(pipefd2[0]);
							close(pipefd2[1]);
							if(execvp(subcommands3[0], subcommands3)<0)
								perror("Error in execvp");

							exit(0);							
						}
						else
						{
							//parent process

							close(pipefd[0]);
							close(pipefd[1]);
							close(pipefd2[0]);
							close(pipefd2[1]);
							//parent process
							debug("in1",0);
						    do {
						      wpid = waitpid(p1, &status, WUNTRACED);
						    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
							debug("in2",0);

							do {
						      wpid = waitpid(p2, &status, WUNTRACED);
						    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
							debug("in3",0);

							do {
						      wpid = waitpid(p3, &status, WUNTRACED);
						    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
							debug("in4",0);							
						}
					}	 

				}

			}
			else if(delims[0]=='<'&& delims[1]=='>')
			{
				pid_t pid,wpid;
				int fd_in, fd_out;
				char* filename_in=subcommands[1];
				char* filename_out=subcommands[2];
				char** subcommands1=parse_command2(subcommands[0]);

				fd_in=open(filename_in, O_RDONLY, S_IRWXU);
				fd_out=open(filename_out, O_WRONLY|O_CREAT, S_IRWXU);

				dup2(fd_in, STDIN_FILENO);
				dup2(fd_out, STDOUT_FILENO);

				pid=fork();
				if(pid<0)
				{
					perror("Error in fork(): ");
					exit(0);
				}
				if(pid==0)
				{
					//child process
				    if (execvp(subcommands1[0], subcommands1) == -1) {
				      perror("Error in execvp(): ");
				    }
				    exit(0);					
				}
				else
				{
				    do {
				      wpid = waitpid(pid, &status, WUNTRACED);
				    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
				}
				close(fd_in);
				close(fd_out);

			}

		}
	}
	return 1;

}

char* read_command()
{
	char* read_line=(char*)malloc(MAX_COMMAND_LENGTH*sizeof(char));
	int i=0;
	char c;
	while(1)
	{
		c=getchar();
		if(c=='\n')
		{
			read_line[i]='\0';
			return read_line;
		}
		else
			read_line[i]=c;

		i++;
	}

}

void parse_command1(char* read_line)
{
	int i,len,flag=0,count=0;
	len=strlen(read_line);
	for(i=0;i<len;i++)
	{
		if(read_line[i]==';')
		{
			delimiters[count++]=read_line[i];
			command_subsets[row][col]='\0';
			col=0;
			row++;
			continue;
		}
		else if(read_line[i]=='&' && read_line[i+1]=='&')
		{
			delimiters[count++]=read_line[i];
			command_subsets[row][col]='\0';
			col=0;
			row++;
			i++;
			continue;			
		}
		else if(read_line[i]=='|' && read_line[i+1]=='|')
		{
			delimiters[count++]=read_line[i];
			command_subsets[row][col]='\0';
			col=0;
			row++;
			i++;
			continue;
		}
		command_subsets[row][col++]=read_line[i];
	}
	command_subsets[row][col]='\0';
	return;

}

char** parse_command2(char* read_line)
{
	char** subcommands=(char**)malloc(MAX_NO_OF_COMMANDS*sizeof(char*));
	int i=0;
	char* token=strtok(read_line," ");
	while(token!=NULL)
	{
		subcommands[i]=token;
		token=strtok(NULL," ");
		i++;
	}
	subcommands[i]=NULL;
	return subcommands;

}

int execute_command(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < no_of_builtins(); i++) {
    if (strcmp(args[0], my_builtin_str[i]) == 0) {
      return (*my_builtin_func[i])(args);
    }
  }

  return launch_command(args);
}

int launch_command(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("Error in execvp(): ");
    }
    exit(0);
  } else if (pid < 0) {
    perror("Error in fork(): ");
    exit(0);
  } else {
    // Parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}


/*void batch_execute(char *filename)
{
	char* read_line=(char*)malloc(MAX_COMMAND_LENGTH*sizeof(char));

	int i,count;
	int status=1;
	FILE *fp;
	fp=fopen(filename,"r");


	while(fscanf(fp,"%[^\n]",read_line)!=EOF)
	{
		count=0;

		fgetc(fp);
		memset(delimiters,'\0', sizeof(delimiters));

		//char **subcommands1=parse_command1(read_line);
		i=0;
		while(subcommands1[i]!=NULL)
		{
			char **subcommands2=parse_command2(subcommands1[i]);
			status=execute_command(subcommands2);
			free(subcommands2);
			if(delimiters[count]==';')
			{
				i++;
				count++;
				continue;
			}
			else if(delimiters[count]=='&')
			{
				if(status==1)
				{
					i++;
					count+=2;
					continue;
				}
				else if(status==0)
					break;
			}
			else if(delimiters[count]=='|')
			{
				if(delimiters[count+1]=='|')
				{
					if(status==0)
					{
						i++;
						count+=2;
						continue;
					}
					else if(status==1)
						break;
				}
				else
				{

				}
			}
			else
			{
				break;
			}
		}
	}	

	free(read_line);
	fclose(fp);

}*/

int execute_pipe(char** subcommands, int i)
{
	char** subcommands1=parse_command2(subcommands[i]);
	char** subcommands2=parse_command2(subcommands[i+1]);
	debug("1",0);
	int status;
	int pipefd[2];
	pid_t p1,p2,wpid;

	if(pipe(pipefd)<0)
	{
		perror("Error in pipe(): ");
		return 0;
	}

	p1=fork();
	if(p1<0)
	{
		perror("Error in fork(): ");
		return 0;		
	}

	if(p1==0)
	{
		//child process
		close(pipefd[0]);
		dup2(pipefd[1],STDOUT_FILENO);
		close(pipefd[1]);

		if(execvp(subcommands1[0], subcommands1)<0)
			perror("Error in execvp():");

		exit(0);
	}
	else
	{
		//parent process
		p2=fork();

		if(p2<0)
		{
			perror("Error in fork():");
			return 0;
		}

		if(p2==0)
		{
			//child process
			close(pipefd[1]);
			dup2(pipefd[0], STDIN_FILENO);
			close(pipefd[0]);
			if(execvp(subcommands2[0], subcommands2)<0)
				perror("Error in execvp");

			exit(0);
		}
		else
		{
			close(pipefd[0]);
			close(pipefd[1]);
			//parent process
			debug("in1",0);
		    do {
		      wpid = waitpid(p1, &status, WUNTRACED);
		    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
			debug("in2",0);

			do {
		      wpid = waitpid(p2, &status, WUNTRACED);
		    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
			debug("in3",0);
		}	 

	}
	debug("out1",0);
	return 1;
	
}

int execute_out(char** subcommands, int i)
{
	debug(subcommands[i],0);
	debug(subcommands[i+1],0);
	char** subcommands1=parse_command2(subcommands[i]);
	char* filename=subcommands[i+1];
	debug("1",0);
	int status;
	int fd;
	pid_t p1,wpid;

	fd=open(filename, O_WRONLY|O_CREAT, S_IRWXU);
	debug("fd1: ",fd);
	if(fd==-1)
	{
		perror("Error in fd():");
		return 0;
	}

	p1=fork();
	if(p1<0)
	{
		perror("Error in fork(): ");
		return 0;		
	}

	if(p1==0)
	{
		//child process
		dup2(fd,STDOUT_FILENO);

		if(execvp(subcommands1[0], subcommands1)<0)
			perror("Error in execvp():");

		exit(0);
	}
	else
	{
	    do {
	      wpid = waitpid(p1, &status, WUNTRACED);
	    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
	    	
	}
	close(fd);
	debug("out1",0);
	return 1;
}


int execute_in(char** subcommands, int i)
{
	debug(subcommands[i],0);
	debug(subcommands[i+1],0);
	char** subcommands1=parse_command2(subcommands[i]);
	char* filename=subcommands[i+1];
	debug("1",0);
	int status;
	int fd;
	pid_t p1,wpid;

	fd=open(filename, O_RDONLY|O_CREAT, S_IRWXU);
	debug("fd: ",fd);
	if(fd==-1)
	{
		perror("Error in fd():");
		return 0;
	}

	p1=fork();
	if(p1<0)
	{
		perror("Error in fork(): ");
		return 0;		
	}

	if(p1==0)
	{
		//child process
		dup2(fd,STDIN_FILENO);

		if(execvp(subcommands1[0], subcommands1)<0)
			perror("Error in execvp():");

		exit(0);
	}
	else
	{
	    do {
	      wpid = waitpid(p1, &status, WUNTRACED);
	    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
	    	
	}
	close(fd);
	debug("out1",0);
	return 1;
}
