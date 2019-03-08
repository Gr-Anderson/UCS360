/* 
	Author: Samuel Steinberg
	September 2018
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdint.h>
#include "fields.h"
#include "jval.h"
#include "dllist.h"
#include "jrb.h"

/* This function merely cleans up the read_in. I pass the list and the struct and append the file names to their respective lists */
void append_to_list(Dllist list, IS is)
{
	int i;
	for (i = 1; i < is->NF; i++) //read all args in line
	{
		dll_append(list, new_jval_s(strdup(is->fields[i])));
	}
}
/*read-in populates the lists and the E file. It pass the struct created in main (did this for deallocation  purposes)
 and then error check the file name.Then, getline and the fields library are used to populate 
 (help from append_to_list used here)...e file comments below */ 
char* read_in(IS is, char *file_name, char *efiles, Dllist cfiles, Dllist hfiles, Dllist lfiles, Dllist ffiles)
{

	efiles = NULL;
	if (is == NULL)
	{
		perror(file_name);
		exit(1);
	}
	while(get_line(is) >= 0)
	{
		if (is->NF <= 1)
		{
			continue;
		}
		else if (strcmp(is->fields[0], "C") == 0)
		{
			append_to_list(cfiles, is);
		}
		else if (strcmp(is->fields[0], "H") == 0)
		{
			append_to_list(hfiles, is);
		}
		else if (strcmp(is->fields[0], "L") == 0)
		{
			append_to_list(lfiles, is);
		}
		else if (strcmp(is->fields[0], "F") == 0)
		{
			append_to_list(ffiles, is);
		}
		/* If there is already an E line the program returns an error. Or else, the E file is copied from the first field */
		else if (strcmp(is->fields[0], "E") == 0)
		{
			if (efiles != NULL)
			{
				fprintf(stderr, "fmakefile (%i) cannot have more than one E line\n", is->line);
				exit(1);
			}
			efiles = strdup(is->fields[1]);
		}
		/*Error message for bad input */
		else
		{
			fprintf(stderr, "fakemake (%d): Lines must start with C, H, E, F or L\n", is->line);
			exit(1);
		}

		if (is->NF == 0) { }
	}
		/* If there is no executable, give an error. Or else, return the efile */
		if (efiles == NULL)
		{
			fprintf(stderr, "No executable specified\n");
			exit(1);
		}
		return efiles;

}
/*This function traverses the header file list and calls stat to get the latest version*/
long process_header_val(Dllist hlist)
{
	Dllist temp;
	struct stat buf;
	int time_check;
	long max_time = 0;

	dll_traverse(temp,hlist)
	{
		time_check = stat(temp->val.s, &buf);
		/*If the file does exist in the directory give an error*/
		if (time_check < 0)
		{
			fprintf(stderr, "fmakefile: %s: No such file or directory\n", temp->val.s);
			exit(1);
		}
		/*Or else, update the max time and return it */
		else
		{
			if (buf.st_mtime > max_time)
			{
				max_time = buf.st_mtime;
			}
		}
	}
	return max_time;
}
/* This function is really only used to clean up the processing of the c files.
   The compiling line is passed and the flags are initially copied onto it, along with a space. Then come the files.
   The line is then printed */
void append_prefix(char *compile_line, char *flags, Dllist temp)
{
	strcpy(compile_line, flags);
	strcat(compile_line, " ");
	strcat(compile_line, temp->val.s);
	printf("%s\n", compile_line);
}
/*When the system call is a negative, it is an error for a bad call. system() invokes an operating system command*/
void check_system(char *compile_line)
{
	int system_call;
	system_call = system(compile_line);
	
	if (system_call != 0) //return 0 on good call
	{
		fprintf(stderr, "Command failed.  Exiting\n");
		exit(1);
	}

}
/* System call used when o file exists or is more recent than c */
void check_fsystem(char *compile_line)
{
	int system_call;
	system_call = system(compile_line);
	
	if (system_call != 0) //return 0 on good call
	{
		fprintf(stderr, "Command failed.  Fakemake exiting\n");
		exit(1);
	}
}
/*This function gets the name of the executable. First i set the limit of the counter to be the length of the 
 input file. I then convert it to a .o file by copying the string before it hits a '.' which would signal the end
 of the name i want (i.e. if i read in mysort.c i really only care about the 'mysort' part) and then append a .o and return it */
char* get_exec_name(char *input)
{
	int input_length = strlen(input);
	int i = 0;
	char *exec_name = malloc(sizeof(char)*20);

	while (i < input_length)
	{
		if (input[i] == '.')
		{
			break;
		}
		else
		{
			exec_name[i] = input[i];
		}
		i++;
	}
	strcat(exec_name, ".o");
	return exec_name;
}
/*Again, this is just a clean up function. traverses the lists needed for printing and will concatenate
 the string (execstring acts as the line im wanting to print) to the list value (be it a library or remade file or what not) */
void cat_lists(char *execstring, Dllist list)
{
	Dllist temp;
	dll_traverse(temp, list)
	{
		strcat(execstring, " ");
		strcat(execstring, temp->val.s);
	}
}
/* More clean up. Called in processing my o file which is where i also print, this function will merely be the beginning of the output
 and will include the executable name */
void main_prefix(char* execstring, char *compile_o, char *efile)
{
	strcpy(execstring, compile_o);
	strcat(execstring, " ");
	strcat(execstring, efile);
}
/* This is where the bulk of my processing is done, where i call all the previously described functions. I made all the previous functions to
   clean this one up. I first determine whether I need to make the executable after calling stat() in main -- I pass the buf and result of the call--
   and then add all the data for the flags, remade files, and libraries.*/
void process_o_file(char *execstring, char *compile_o, char *efile, int exists, Dllist ffiles, Dllist remade_files, Dllist lfiles, int ofile_size, int made, struct stat buf)
{
	if (exists < 0)
	{
		main_prefix(execstring, compile_o, efile);
		cat_lists(execstring, ffiles);
		cat_lists(execstring, remade_files);
		cat_lists(execstring, lfiles);
		printf("%s\n", execstring);
		check_system(execstring);
	}
	/*Or else if the file exists, same as above, but 'made' represents an executable that has been remade. */
	else
	{
		if (ofile_size > buf.st_mtime || made == 1)
		{
			main_prefix(execstring, compile_o, efile);
			cat_lists(execstring, ffiles);
			cat_lists(execstring, remade_files);
			cat_lists(execstring, lfiles);
			printf("%s\n", execstring);
			check_fsystem(execstring);
		}
		/*If the executable is already made but up to date, return that message and exit so the program doesnt
		  try to compile */
		else
		{
			printf("%s up to date\n", efile);
			free(efile);
			exit(1);
		}
	}

}

/* This function will process each c file and will eventually return the max st_mtime of the .o files if they exists.
   The function accepts the list of c files, the max header st_mtime, the flags, and most importantly an integer
   pointer that allows me to communicate whether the .o has been made to the rest of the program */
long process_cfile(Dllist clist, long h_max, char *flags, int *made)
{
	struct stat buf;
	struct stat buf2;
	int exists; 
	int oexists;
	long max;
	char *ofile;
	char *compile_line = malloc(sizeof(char)*100);
	Dllist temp;
	max = 0;
	exists = 0;
	oexists = 0;
	
	dll_traverse(temp, clist)
	{
		/*exists returns a negative when the file cant be found in a directory, and if so give an error */
		exists = stat(temp->val.s, &buf);
		if (exists < 0)
		{
			fprintf(stderr, "fmakefile: %s: No such file or directory\n", temp->val.s);
			exit(1);
		}

		/*If it passes the above test, get the executable name by calling its function and call stat() on it. */
		ofile = get_exec_name(temp->val.s);
		oexists = stat(ofile, &buf2);

		/*If the file doesnt exist, make it. Flag it as made with the pointer. */
		if (oexists < 0)
		{
			append_prefix(compile_line, flags, temp);
			check_system(compile_line);
			*made = 1;
		}
		/*If it does exist check to see whether it is less recent than the C file or the max header st_mtime.
		  If so, make the file again like above. Then keep updating the max and return it when the traversal ends */
		else
		{
			if (buf2.st_mtime < buf.st_mtime || buf2.st_mtime < h_max)
			{
				append_prefix(compile_line, flags, temp);
				check_system(compile_line);
				*made = 1;
			}
			if (buf2.st_mtime > max)
			{
				max = buf2.st_mtime;
			}
		}
	}
	/*free malloced memory here because im done using it */
	free(ofile);
	free(compile_line);
	return max;

}

/* Function call to free the memory from the lists if they exist, and then free the dllist */
void free_list_mem(Dllist list)
{
	Dllist temp;
	dll_traverse(temp, list)
	{
		free(temp->val.s);
	}

	free_dllist(list);
}
/*This function will open the directory to search for the fmakefile. If found flag it and return it 
 after copying the name. Then error check. Detailed below. */
char* check_input(char *com_line, int argc)
{
	int find_make = 0;
	DIR *d;
	struct dirent *de;
	/*Error checking, open the directory and if it NULL give an error. */
	if (argc == 1)
	{
		d = opendir(".");
		if (d == NULL)
		{
			fprintf(stderr, "error: couldn't open \".\"\n");
			exit(1);
		}
		/* If it is not NULL read from the directory and if "fmakefile" is found flag it and set it to be argv[1] */
		for (de = readdir(d); de != NULL; de = readdir(d))
		{
			if (strcmp(de->d_name, "fmakefile") == 0)
			{
				find_make = 1;
				com_line = strdup("fmakefile");	
			}
		}
		/*If "fmakefile" isnt found give an error. */
		if (find_make == 0)
		{
			fprintf(stderr, "error: no makefile found\n");
			exit(1);
		}
	}
	/* If the incorrect arg count is given flag an error */
	else if (argc != 2)
	{
		fprintf(stderr, "usage: \"fakemake [ description-file ]\"\n");
		exit(1);
	}

	return com_line;
}
/*Main manages the core processing of the fucntions. Explained below. */
int main(int argc, char *argv[])
{
	long h_max;
	struct stat buf;
	int exists;
	int ofile_size;
	int made;
	Dllist temp;
	
	/*call function to check directory and set argv[1] */
	argv[1] = check_input(argv[1], argc);
	/*Memory allocation */
	char *efile;
	char *compile_c = malloc(sizeof(char)*100);
	char *compile_o = malloc(sizeof(char)*100);
	char *execstring = malloc(sizeof(char)*100);
	Dllist cfiles = new_dllist();
	Dllist hfiles = new_dllist();
	Dllist lfiles = new_dllist();
	Dllist ffiles = new_dllist();
	Dllist remade_files = new_dllist();
	
	/*read-in function called: populates the lists and gets the executable name */
	IS is;
	is = new_inputstruct(argv[1]);
	efile = read_in(is, argv[1], efile, cfiles, hfiles, lfiles, ffiles);		
	
	/* function call to get the max value of the header */
	h_max = process_header_val(hfiles);
	
	/* Creation of "gcc -c" and "gcc -o" strings */
	strcpy(compile_c, "gcc -c");
	strcpy(compile_o, "gcc -o");

	/*Traverse the flags and append to the compiling line */
	dll_traverse(temp, ffiles)
	{
		strcat(compile_c, " ");
		strcat(compile_c, strdup(temp->val.s));
	}
	/* Populate the remade files list by calling the get_exec_name function -- gets the executable name -- and populate */
	dll_traverse(temp, cfiles)
	{
		dll_append(remade_files, new_jval_s(get_exec_name(temp->val.s)));
	}
	/* Get the max st_mtime of the executable from the process_cfile function */
	ofile_size = process_cfile(cfiles, h_max, compile_c, &made);
	/*Call stat on the executable file and pass these values to the process_o_file for processing.
	  Like I explained above, this function will build the output line */
	exists = stat(efile, &buf);
	process_o_file(execstring, compile_o, efile, exists, ffiles, remade_files, lfiles, ofile_size, made, buf);
	/*Deallocate and return 0 */
	jettison_inputstruct(is);	
	free(compile_c);
	free(compile_o);
	free(execstring);
	free_list_mem(remade_files);
	free_list_mem(ffiles);
	free_list_mem(cfiles);
	free_list_mem(hfiles);
	free_list_mem(lfiles);
	return 0;
}
