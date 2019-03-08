/*
	Author: Samuel Steinberg
	September 2018
*/
/*fcnt.h added here to allow the system calls */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "fields.h"
#include "jval.h"
#include "dllist.h"
#include "jrb.h"
/* struct IP will hold the address and the dllist of names. I made the char to hold 16 for: 15 possible IP address vals and a null terminator*/
typedef struct IP
{
	unsigned char address[16];
	Dllist names;
}IP;
/* Free the dllist and tree */
void free_mem(Dllist d, JRB j)
{
	free_dllist(d);
	jrb_free_tree(j);
}
/* Deallocates the ip object, MUST free the dllist of names before freeing ip in general */
void free_ip(IP *p)
{
	free(p->address);
	free_dllist(p->names);
	free(p);
}
/* Endian bit shift to convert the number of names to an unsigned integer from 4 bytes pulled from 2nd fread*/
unsigned int endian_conversion(unsigned int before_endian)
{
	unsigned int after_endian;
	unsigned int temp;

	temp = ((before_endian << 8) & 0xFF00FF00) | ((before_endian >> 8) & 0x00FF00FF);

	after_endian = (temp << 16) | (temp >> 16);

	return after_endian;
	
}
/*Error message for an invalid machine name */
void print_error_check(char * u_input)
{
	printf("no key %s \n", u_input);
	printf("\n");
}
/* ctrl-d in stdin will return a negative, once this is seen immediatley exit the program */
void print_ctrld()
{
	printf("\n");
	exit(0);
}
/*Bad file detector -- read returns int this int either a 0 for a good file and a -1 for a bad file*/
void file_err(int f)
{
	if (f < 0)
	{
		fprintf(stderr, "error opening converted \n");
		exit(0);
	}
}

void append_local(IP *ip, char* name)
{
	char *local;
	local = strtok(name,".");
	dll_append(ip->names, new_jval_s(local));
}
/*Inserts on tree...cleans up main a bit defining it up here */
void insert_to_tree(IP *ip, Dllist traverser, JRB j)
{
	dll_rtraverse(traverser, ip->names)
	{
		jrb_insert_str(j, jval_s(traverser->val), new_jval_v(ip));
	}		

}
/*Prints dllist whenever the name comes up in the JRB tree */
void printer(IP *ip, Dllist traverser, JRB temp)
{
	ip = ((IP*)temp->val.v);
	
	printf("%s : ", ip->address);
	dll_rtraverse(traverser, ip->names)
	{
		printf("%s ", jval_s(traverser->val));
	}
	printf("\n");
	printf("\n");

}
/*Carried out my read in main, comments below */
int main(int argc, char *argv[])
{
	IP *ip, *search_for_ip;
	JRB j, temp;
	Dllist traverser;
	
	j = make_jrb();

	int f;
	unsigned char buffer[4];
	
	unsigned int before_endian;
	unsigned int number_of_names;
	int i;
	int string_length;
	char * name;
	int end_name = 0;

	char user_input[30];
	char *input;
    int ctrl_d;
	
	//Open the file and only allow reading. Invalid file will return -1, valid file returns 0 */
	f = open("converted", O_RDONLY, O_RDONLY);
	file_err(f);
	
	/* read takes in a file stream, a char buffer, and a specified 4 elements. Like in part 1 I use this to get my IP address */
	while ( read(f, buffer, 4) > 0)
	{
		/*Allocate for IP and its Dllist of names */
		ip = (IP *)malloc(sizeof(ip));
		ip -> names = new_dllist(); 
		
		/*Store the IP address as the first four bytes that were stored into buffer */
		sprintf(ip->address, "%d.%d.%d.%d", buffer[0], buffer[1], buffer[2], buffer[3]);
		
		/*Read another 4 bytes, this time Im saying "read 4 bytes once". I am storing this in the space represented by &before_endian
		 and then I carry out an endian conversion (bit shift) taking into account an int is 32 bytes. I call my function for it here
		 and store it into the number of names*/
		read(f, &before_endian, 4); 
		number_of_names = endian_conversion(before_endian);

		/*From i to the number of names in this IP address, set/reset flags and length of string at the top.
		  Then, allocate space for a name (I assumed non would be more than 50 chars long). I then used
		  read to get the char at that place in the file, and if it was zero (reps null terminator) I broke out.
		  If I hit a '.' I flagged it so then I could extract the local name after the loop. I called this var end_name.
		  I then incremented string length. IMPORTANT: for Part 2 it is imperitive to initializa end_name to 0 BEFORE the for
		  loop because it will be recieving a value this time*/
		
		for (i = 0; i < number_of_names; i++)
		{
			int flag = 0;
			string_length = 0;
			name = (char*) malloc(sizeof(char)*50);
			while(1)
			{
				read(f, &end_name, 1);
				if(end_name == 0)
				{
					break;
				}
				if (end_name == '.')
				{
					flag = 1;
				}
				name[string_length] = end_name;
				string_length++;
			}
		/*Flag == 1, call function to append a string onto dllist for the absolute name */
		if (flag == 1)
		{
			dll_append(ip->names, new_jval_s(strdup(name)));
		}
		/*Call local name here*/
		append_local(ip, name);
		/*Reset name check to 0 */
		end_name = 0;
	}
		/*Outside of for loop when you have all the names for the address call traversal function
		 and insert onto tree */
		
	insert_to_tree(ip, traverser, j);		

	}
	free(name);
	/*Required command line specifications -- enter infinte while which only ends with crtl-d */
	printf("Hosts all read in \n");
	printf("\n");
	while(1)
	{
		/*Infinite input enabled, int ctrl_d is just there to check if nothing entered on stdin -- aka ctrl-d*/
		printf("Enter host name: ");
		ctrl_d = fscanf(stdin, "%s", user_input);
		input = strdup(user_input);
		/*Calls function to exit */
		if (ctrl_d < 0)
		{
			print_ctrld();
		}
		/*Search for input in tree, if not found call error print function */
		if (jrb_find_str(j,input) == NULL)
		{
			print_error_check(input);
		}
		else
		{
			jrb_rtraverse(temp, j)
			{
				if (strcmp((char*)temp->key.s, input) == 0)
				{
					printer(ip, traverser, temp);
				}
			}
		}
		
	}
	/*call free memory functions and close file */
	free_mem(traverser,j);
	free_ip(ip);
	close(f);
}
