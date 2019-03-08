/* 
   Author: Samuel Steinberg
   September-October 2018
*/
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdlib.h>
#include <utime.h>
#include <sys/types.h>
#include "fields.h"
#include "jval.h"
#include "dllist.h"
#include "jrb.h"

/* Need these definitions here because parse_commands calls them and is above them in the code */
void process_files(char *command, JRB jrb);
void x_option(JRB tree, JRB d);

/* Function parses command line arguments given from stdin */
void parse_commands(int argc, char *argv[])
{
	JRB j = make_jrb();
	JRB tree = make_jrb();
	JRB d = make_jrb();
	/* If there is less than 1 argument, or if there are less than 3 arguments with the c-option given
	   or if there are less than 2 arguments with the x-option given throw an error. */
	if ( (argc <= 1) || (argc != 3 && (argv[1] == "c")) || (argc != 2 && argv[1] == "x") )
	{
		fprintf(stderr, "usage: jtar [cxv] [files ...]\n");
		exit(1);
	}
	/*If the c option is given in stdin, call process_files on the second argument given after the executable and pass the tree as well */
	else if (strcmp(argv[1], "c") == 0)
	{
		process_files(argv[2], j);
		
	}
	/*If the x option is given in stdin, call x_option and pass in two trees*/
	else if (strcmp(argv[1], "x") == 0)
	{
		x_option(tree, d);
	}
	/* If the cv option is given, I first see if the given argument is a valid file. If so, check to see whether it is a 
	   directory or a file. I am completely aware this is incomplete, I was having too much trouble trying to implement them. */
	else if (strcmp(argv[1], "cv") == 0)
	{
		struct stat buf;
		int exists = lstat(argv[2], &buf);
		if (exists != 0) 
		{
			fprintf(stderr, "unable to read %s\n", argv[2]);
			exit(1);
		}
		else if (S_ISDIR(buf.st_mode) != 0)
		{
			fprintf(stderr, "Directory is: %s\n", argv[2]);
		}
		else if (S_ISREG(buf.st_mode) != 0)
		{
			fprintf(stderr, "File is: %s\n", argv[2]);
		}
		else
		{
			fprintf(stderr, "Unable to open command line argument: %s\n", argv[2]);
			exit(1);
		}
	}
	/* If the xv option is given, I first see if the given argument is a valid file. If so, check to see whether it is a 
	   directory or a file. I am completely aware this is incomplete, I was having too much trouble trying to implement them. */
	else if (strcmp(argv[1], "xv") == 0)
	{
		struct stat buf2;
		int exists = lstat(argv[2], &buf2);
		if (exists != 0) 
		{
			fprintf(stderr, "unable to open %s\n", argv[2]);
			exit(1);
		}
		else if (S_ISDIR(buf2.st_mode) != 0)
		{
			fprintf(stderr, "Directory is: %s\n", argv[2]);
		}
		else if (S_ISREG(buf2.st_mode) != 0)
		{
			fprintf(stderr, "File is: %s\n", argv[2]);
		}
		else
		{
			fprintf(stderr, "Unable to open command line argument: %s\n", argv[2]);
			exit(1);
		}
	}
	/*If none of these options hit then a bad flag was entered and an error is thrown */
	else
	{
		fprintf(stderr, "usage: jtar [cxv] [files ...]\n");
		exit(1);
	}

	/*Free the trees which had memory allocated */
	jrb_free_tree(j);
	jrb_free_tree(tree);
	jrb_free_tree(d);
}
/* This function is given a dllist, and then traverses that dllist and frees its content. Then once empty, the dllist itself is freed */
void free_list_mem(Dllist list)
{
	Dllist temp;
	dll_traverse(temp, list)
	{
		free(temp->val.s);
	}

	free_dllist(list);
}
/* Creates the tar file: everything is written to stdout. Info is grabbed from the size of the file, the name, and the buf and printed.
   The function is passed the stat buf and "file_name" which is actually just the directory or file passed from the parsing
   of the command line*/
void make_tarfile(struct stat buf, char *file_name)
{
	int name_size = strlen(file_name) + 1;

	fwrite(&name_size, sizeof(int), 1, stdout);
	fwrite(file_name, sizeof(char), name_size, stdout);
	fwrite(&buf, sizeof(struct stat), 1, stdout);
}
/*This function process a directory. It first makes the directory file calling make_tarfile function above, and then traverses the directory passed
  in from main. If "." or ".." is given, it is ignored. Or else, I malloc a char* to be my path and I first copy
  the directory into it, and then a "/", and then the name of what the dirent points to. This is then added to the Dllist file_path
  and the directory is closed before the function it is called in (process_files) recursively calls itself */
void process_directory(DIR *d, char *command, struct stat buf, Dllist file_path)
{
	struct dirent *de;
	make_tarfile(buf, command);
	for (de = readdir(d); de != NULL; de = readdir(d))
	{
		if ( (strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..") != 0) )
		{
			char *rpath = malloc(sizeof(char)*1000);
			strcpy(rpath, command);
			strcat(rpath, "/");
			strcat(rpath, de->d_name);
			dll_append(file_path, new_jval_s(strdup(rpath)));
		}	
	}
	closedir(d);
}
/* This function processes a file when the inode is not yet present in the tree. When the inode is not found, insert it into 
   the tree along with the name its attached to (file name in this case). Then open the file and...*/
void process_new_inode(struct stat buf, char *command, JRB jrb)
{
	char test;
	int i;
	FILE *f;
	jrb_insert_int(jrb, buf.st_ino, new_jval_s(strdup(command))); //insert it with the name of the file
	f = fopen(command, "rb");
	/* If NULL, throw an error */
	if (f == NULL)
	{
		perror(command);
	}
	/*Or else, tar the file and read/write its content. Then close the file. */
	else
	{
		make_tarfile(buf, command); //if file is not null make it
		fwrite(&buf.st_size, sizeof(int), 1, stdout);
		for(i = 0; i < buf.st_size; i++)
		{
			fread(&test, 1, 1, f);
			fwrite(&test, 1,1, stdout);
		}
		fclose(f);
	}
}
/* This function is called when the inode already exists. Tar up the file and write its content to stdout */
void inode_found(char *tmporary, struct stat buf, char *command)
{
	make_tarfile(buf,command);
	int fsize = strlen(tmporary) + 1;
	fwrite(&fsize, 1, sizeof(int), stdout);
	fwrite(tmporary, sizeof(char), fsize, stdout);
}
/* This function handles the c option given in stdin. */
void process_files(char *command, JRB jrb)
{
	struct stat buf;
	DIR *d;
	int exists = 0;
	Dllist temp;
	JRB temporary;
	int flag = 0;

	/* exists holds the value of lstat, if it is not 0 then that means it 'doesnt check out' and doesnt exist. Throw an error. */
	exists = lstat(command, &buf);
	if (exists != 0)
	{
		fprintf(stderr, "unable to read %s\n", command);
		exit(1);
	}
	/*Or else, if it is a directory open it. S_ISDIR returns a non-zero if the mode is a directory. st_mode deteremines that mode.
	  If the directory is not NULL (meaning it can open) then call the process_directory 
	  function explained above and then traverse the Dllist file_path created in the processing of the directory.
	  Recursively call the function on each string and then free the Dllist. The flag is set to 1 when the loop is entered to 
	  signify the command line argument could be opened. */
	else if (S_ISDIR(buf.st_mode) != 0)
	{
		flag = 1;
		Dllist file_path = new_dllist();
		d = opendir(command);
		if (d != NULL)
		{
			process_directory(d, command, buf, file_path);
			dll_traverse(temp, file_path)
			{
				process_files(temp->val.s, jrb);
			}
		}
		else
		{
			fprintf(stderr, "Could not open directory\n");
			exit(1);
		}
		free_list_mem(file_path);
	}
	/* Or else, buf holds a file or link this else/if is hit. S_ISREG and the st_mode work the same as for a directory.
	   temporary is a temporary node that holds the value of whether the inode of the buffer is found or not. If it is NOT
	   found then call the process_new_inode explained above. The flag also works the same as above. */
	else if (S_ISREG(buf.st_mode) != 0)
	{
		flag = 1;
		temporary = jrb_find_int(jrb, buf.st_ino);
		if (temporary == NULL)
		{
			process_new_inode(buf, command, jrb);
		}
		/* Or else if the inode is found, I malloc a char to hold the size of the string accompanying the found
		  inode. I then strdup it to hold its content. I then call the inode_found function explained above to process it.
		  I then free the memory. */
		else
		{
			char *tmporary = strdup(temporary->val.s);
			inode_found(tmporary, buf, command);
			free(tmporary);
		}
	}
	/* If the flag doesnt change to a one it cannot be opened. I am just now wondering why i didnt just make this an else...*/
	if (flag == 0)
	{
		fprintf(stderr, "Unable to open command line argument: %s\n", command);
	}

}
/* This function handles the x option's handling of regular files/links from stdin */
x_regfiles(struct stat buf, JRB tree, char *path, struct utimbuf ubuf)
{
	FILE *f;
	int file_length;
	int link_length;
	char *file = malloc(sizeof(char)*300);
	char *l = malloc(sizeof(char)*300);
	/*temp node holds the value of whether or not the inode is in the tree */
	JRB temp = jrb_find_int(tree, buf.st_ino);
	/*If it is not found, insert it along with the string accompanying it.*/
	if ( temp == NULL)
	{
		jrb_insert_int(tree, buf.st_ino, new_jval_s(path));
		/* Open the file and read it. store the length of the file into file_length. This is so we know how items to read. */
		f = fopen(path, "w");
		fread(&file_length, sizeof(int), 1, stdin);
		/*If file is NULL throw an error */
		if(f == NULL)
		{
			perror(path);
		}
		/* Or else, read into the file from stdin and then write the contents. */ 
		else
		{
			fread(file, 1, file_length, stdin);
			fwrite(file, 1, file_length, f);
			fclose(f);
		}
	}
	/* If it is found read into the length of the link and then into the link name itself. Then call link() to create
	  a new hard link between the paths */
	else
	{
		fread(&link_length, sizeof(int), 1, stdin);
		fread(l, sizeof(char), link_length, stdin);
		link(l, path);
	}

	/*chmod will change the mode of the file. This is the updating of the files. the st_atime represents
	  the last access time, st_mtime the last modification time. utime then CHANGES the modification times of
	  path's inode. */
	chmod(path, buf.st_mode);
	ubuf.actime = buf.st_atime;
	ubuf.modtime = buf.st_mtime;
	utime(path, &ubuf);
}
/*This funtion handles the x argument given from stdin. It will read the tar file and recreate all files in it. */
void x_option(JRB tree, JRB d)
{
	int path_length;
	struct stat buf;
	struct utimbuf ubuf;	
	char *path = malloc(sizeof(char)*300);
	JRB temp;
/* After reading in the length of the path, which is vital because we have to wrote how many bytes, 
  the char *path and buf are read into from stdin. */
	while( fread(&path_length, sizeof(int), 1, stdin) > 0 )
	{
		fread(path, sizeof(char), path_length, stdin);
		fread(&buf, sizeof(struct stat), 1, stdin);

		/*If the buffer is a directory AND the path is not "../" then make the directory with the path name and 0777 */
		if(S_ISDIR(buf.st_mode) != 0)
		{
			if (strcmp(path, "../") != 0)
			{
				mkdir(path, 0777);
				/* Destination, source, size --> memcpy is a void --> First memcpy field allocates the space for the buf.
				  The second field is the data to be copied, which in our case is the buf. The third is the number of bytes we want 
				  to copy, which is the number of bytes that the buf has, which is the size of a struct stat. */
				jrb_insert_str(d, strdup(path), new_jval_v(memcpy(malloc(sizeof(struct stat)), &buf, sizeof(struct stat))));
			}
		}
		/*Or else if the buffer is a file/link...call function explained above */
		else if(S_ISREG(buf.st_mode) != 0)
		{
			x_regfiles(buf, tree, path, ubuf);
		}
	}
	/*Traverse the JRB for directories (must be done after directories processed), and for the its key value holding the names 
	  call chmod for updating like with the files above. Store the access and modification times held by the keys into the ubuf. 
	  Then call utime which CHANGES the modification times of the directory.*/ 
	jrb_traverse(temp, d)
	{
		chmod(temp->key.s, ((struct stat *)(temp->val.v))->st_mode);
		ubuf.actime = ((struct stat *)(temp->val.v))->st_atime;
		ubuf.modtime = ((struct stat*)(temp->val.v))->st_mtime;
		utime(temp->key.s, &ubuf);
	}

}

int main(int argc, char *argv[])
{
	parse_commands(argc, argv);
	return 0;
}
