/*
	Author: Samuel Steinberg
	October-November 2018
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <signal.h>
#include <fcntl.h>
#include "fields.h"
#include "jval.h"
#include "dllist.h"
#include "jrb.h"

/* This function will set my temporary line to NULL, which I then fill in along the way in my program */
void null_line(char **command, int size)
{
	memset(command, 0, size*sizeof(command[0]));
}
/*Function copies the stream line in case I have need of it*/
void copy_field(char **command, IS is)
{
	int i;
	for (i = 0; i < is->NF; i++)
	{
		command[i] = is->fields[i];
	}
}
/*This function is called whenever the < redirect is called, dup2 will create a copy of the file descriptor 0, aka stdin */
void redirect_input(int fd)
{
	if (dup2(fd, 0) != 0)
	{
		perror("Error Input");
		exit(1);
	}
	close(fd);
}
/*This function is called whenever the > or >> redirect is called, dup2 will create a copy of the file descriptor 1, aka stdout */
void redirect_output(int fd2)
{ 
	if (dup2(fd2, 1) != 1)
	{
		perror("Error Output");
		exit(1);
	}
	close(fd2);
}
/*This function error checks the file descriptor after the open() call. If it cannot be opened/an error occurred -1 is returned
  hence the less than 0 check. */
void bad_file_check(int filed, IS is, int i)
{
	if (filed < 0)
	{
		perror(is->fields[i+1]);
		exit(1);
	}
}
/* This function is called when there is NO ampersand specified. wait() is called, and if the id is found then the node is deleted.
   While thetree is not empty, do this until all processes are found and deleted. This ensures there will be no zombie processes. */
void process_wait(int pid, int status, JRB temp, JRB jrb)
{
	pid = wait(&status);
	temp = jrb_find_int(jrb, pid);
	if (temp != NULL)
	{
		jrb_delete_node(temp);
	}

	while( !(jrb_empty(jrb)))
	{
		pid = wait(&status);
		temp = jrb_find_int(jrb, pid);
		if (temp != NULL)
		{
			jrb_delete_node(temp);
		}
	}
}
/*This function handles the forking off of the process when a | is read */
void pipe_fork(int fork_val1, int fd, int pipefd[2])
{
	if (fd != -1)
	{
		redirect_input(fd);
		fd = -1;
	}
	/*dup2 called here to make a copy*/
	if (dup2(pipefd[1], 1) != 1)
	{
		perror("pipefd[1]");
		exit(1);
	}
}
/* This function handles general redirect forking */
void general_fork(int fd, int fd2)
{
	if (fd != -1)
	{
		redirect_input(fd);
	}
	if (fd2 != -1)
	{
		redirect_output(fd2);
	}
}
/*This function processes normal shell commands, redirects, and pipes */
void process_line(IS is, JRB jrb, int wait_flag)
{
	int fd2 = -1;
	int fd = -1;
	int i;
	int pid;
	int fork_val1;
	int fork_val2;
	int status;
	int pipefd[2];
	int prev_pipe;
	int pipe_val;
	int argument_count = 0;
	JRB temp;
	int size = is->NF +1;
	char **command = (char **)malloc(sizeof(char*)*size);
	null_line(command, size);
	/*Traverse through the fields...*/
	for (i = 0; i < is->NF; i++)
	{
		/*If the > direct is found, open the file with permissions to create, read/write, or truncate.
		  Then check to see if it is a bad file, and if not NULL out the commands as you go to make sure there
		  is nothing left in the stream at these locations. */
		if (strcmp(is->fields[i], ">") == 0)
		{
			
			fd2 = open(is->fields[i+1], O_CREAT | O_WRONLY | O_TRUNC, 0644); 			
			bad_file_check(fd2, is, i);
			is->fields[i] = (char*)NULL;
			i++;
			is->fields[i] = (char*)NULL;
		}
		/*Or else, if the < redirect is given open the file with permissions to read from the file. Then check if its a bad
		  file and NULL out the fields. */
		else
			if (strcmp(is->fields[i], "<") == 0)
			{
				fd = open(is->fields[i+1], O_RDONLY, 0644);
				bad_file_check(fd, is, i);
				is->fields[i] = (char*)NULL;
				i++;
				is->fields[i] = (char*)NULL;
			}
		/*Or else, if the >> redirect is given open the file with permission to create, read/write, or append to the file.
		  Then check to see if the file is bad and NULL out the stream locations. */
		else
			if (strcmp(is->fields[i], ">>") == 0)
			{
				
				fd2 = open(is->fields[i+1], O_CREAT | O_WRONLY | O_APPEND, 0644);
				bad_file_check(fd2, is, i);
				is->fields[i] = (char*)NULL;
				i++;
				is->fields[i] = (char*)NULL;
			}
		/* Or else if a pipe is given...*/
		else
			if (strcmp(is->fields[i], "|") == 0)
			{
				/*Using a char** to make sure I dont mess up the IS fields*/
				command[argument_count] = (char*)NULL;
				argument_count = 0;

				/* Call pipe to CONNECT the two processes...if less than 0 it is a bad pipe */
				if (pipe(pipefd) < 0)
				{
					perror("Pipe error");
					exit(1);
				}
				
				/*Fork and insert the id onto the tree */
				fork_val1 = fork();
				jrb_insert_int(jrb, fork_val1, new_jval_i(fork_val1));
				/*Check file descriptors and redirect the input on successful fork*/
				if (fork_val1 == 0)
				{
					pipe_fork(fork_val1, fd, pipefd);
					/*Close the pipes and call exec on the command */
					close(pipefd[0]);
					close(pipefd[1]);
					execvp(command[0], command);
					perror(command[0]);
					exit(1);
				}
				/*or else, close the file, close the write end of the pipe but SAVE the read end of the pipe for use in next pipe*/
				else
				{
					if (fd != -1)
					{
						close(fd);
					}
					fd = pipefd[0];
					close(pipefd[1]);
				}
			}
		/*Handling of 'normal' processes, copy the command and increase the argument count for this facet */
		else
		{
			command[argument_count] = is->fields[i];
			argument_count++;
		}
	}
	/*After the handling of the IS stream, null out the last char in command to set the end*/
	command[argument_count] = (char*)NULL;
	
	/*Insert forked process to tree */
	fork_val2 = fork();
	jrb_insert_int(jrb, fork_val2, new_jval_i(fork_val2));
	/*Same process as above, redirect the input and output and execvp them*/
	if (fork_val2 == 0)
	{
		general_fork(fd, fd2);
		execvp(command[0], command);
		perror(command[0]);
		exit(1);
	}
	/*Or else, if there is no ampersand speficied process the jrb and kill the processes one by one.*/
	else
	{
		if (wait_flag == 0)
		{
			process_wait(pid, status, temp, jrb);
		}
		/*Close files*/
		close(fd);
		close(fd2);
	}
	/*Free malloced command*/
	free(command);

}
/*This function will parse the arguments given by user and will handle the stream */
void parse_arguments(int argc, char *argv[])
{
	IS is;
	is = new_inputstruct(NULL);
	char *user_input;
	JRB jrb;
	/*If user does not specify a command after running the program jsh3 will automatically be the prompt*/
	if (argc == 1)  
	{
		user_input = "jsh3";
	}
	/*Or else, it will be the user specification will be the argument given*/
	else if (argc == 2)
	{
		user_input = argv[1];
	}
	/*More than 1 user argument and an error is flagged. */
	else
	{
		fprintf(stderr, "Incorrect number of arguments given (None or a single one required)\n");
		exit(1);
	}

	/*This while loop is infinite, until ctrl-d or exit is specified */
	while(1)
	{
		/* Planks gradescript runs with a "-"...*/
		if (strcmp(user_input, "-"))
		{
			printf("%s: ", user_input);
		}
		/*if getline is greater than 0...aka its a valid stream and not EOF */
		if (get_line(is) >= 0)
		{
			jrb = make_jrb();
			int wait_flag = 0;
			/*This handles the 'enter' key pressed by user --> just continues to next line without performing an action*/
			if (is->fields[0] == NULL)
			{
				continue;
			}
			/*Break out if exit is given -- Ends program */
			if (strcmp(is->fields[0], "exit") == 0)
			{
				break;
			}
			/*If the last character in the stream line is an ampersand, NULL out the position, decrement the number of fields
			  accordingly, and then setting the NULL char...I then set the wait_flag to 1, which symbolizes and ampersand
			  was in the line and to handle wait() accordingly */
			if (is->text1[strlen(is->text1) - 2] == '&')
			{
				is->text1[strlen(is->text1)-2] = '\0';
				is->NF -= 1;
				is->fields[is->NF] = (char*)NULL;
				wait_flag = 1;
			}
			/*or else, the wait_flag is 0 and the last field is the NULL */
			else
			{
				wait_flag = 0;
				is->fields[is->NF] = (char*)NULL;
			}
			/*Call process_line, pass the stream, tree, and wait flag */
			process_line(is, jrb, wait_flag);	
		}
		/*Break at EOF */
		else
		{
			break;
		}
		jrb_free_tree(jrb);
	}
	jettison_inputstruct(is);
}
/*Main just calls parse_arguments which will do the heavy lifting*/
int main(int argc, char *argv[])
{
	parse_arguments(argc, argv);
	return 0;
}
