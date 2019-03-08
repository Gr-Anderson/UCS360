/*
	Author: Samuel Steinberg
	September 2018
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fields.h"
#include "jval.h"
#include "dllist.h"
#include "jrb.h"
/*struct IP will hold the address and the dllist of names. I made the char to hold 16 for: 15 possible IP address vals and a null terminator*/
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

void file_err(FILE *f)
{
	if (!f)
	{

		fprintf(stderr, "error opening converted\n");
		exit(0);
	}
}
/*Use strtok to get "substring" up to the first dot --> local name */
void append_local(IP *ip, char* name)
{
	char *local;
	local = strtok(name, ".");
	dll_append(ip->names, new_jval_s(strdup(local)));
}
/*Traverses dllist and inserts string and jvals */
void insert_to_tree(IP *ip, Dllist traverser, JRB j)
{
	dll_rtraverse(traverser, ip->names)
	{
		jrb_insert_str(j, jval_s(traverser->val), new_jval_v(ip));
	}
}
/*This will be inside of main portion of printing and simply traverse and print the addresses and machines */
void printer(IP *ip, Dllist traverser, JRB temp)
{
	ip = ((IP*)temp->val.v);
	
	printf("%s : ", ip->address);
	dll_traverse(traverser, ip->names)
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

	FILE *f;
	unsigned char buffer[4];
	
	unsigned int before_endian;
	unsigned int number_of_names;
	int i;
	int string_length;
	char * name;
	int end_name;

	char user_input[30];
	char *input;
    int ctrl_d;
	
	//Open the file, if the file is invalid or there is no file the program will close */
	f = fopen("converted", "rb");
	
	file_err(f);

	/*The while loop will first grab the IP address from the buffer at the beginning of each iteration.
	 fread takes the following args: pointer to mem block (my buffer), the byte size of each element it grabs (a char in this case --1 byte,
	 the number of elements to grab -- 4 (we want the first four for the IP address), and the input stream. 
	 I made the condition greater than zero because fread returns an int, and when EOF is reached will return 0 */
	while ( fread(buffer,sizeof(char), 4, f) > 0)
	{
		/*Allocate for IP and its Dllist of names */
		ip = (IP *)malloc(sizeof(ip));
		ip -> names = new_dllist(); 
		
		/*Store the IP address as the first four bytes that were stored into buffer */
		sprintf(ip->address, "%d.%d.%d.%d", buffer[0], buffer[1], buffer[2], buffer[3]);
		
		/*Read another 4 bytes, this time Im saying "read 4 bytes once". I am storing this in the space represented by &before_endian
		 and then I carry out an endian conversion (bit shift) taking into account an int is 32 bytes. I call my function for it here
		 and store it into the number of names*/
		fread(&before_endian, sizeof(before_endian), 1, f); //grabs int worth 4 bytes
		number_of_names = endian_conversion(before_endian);

		/*From i to the number of names in this IP address, set/reset flags and length of string at the top.
		  Then, allocate space for a name (I assumed non would be more than 50 chars long). I then used
		  fgetc to get the char at that place in the file, and if it was zero (reps null terminator) I broke out.
		  If I hit a '.' I flagged it so then I could extract the local name after the loop. I called this var end_name.
		  I then incremented string length.*/
		for (i = 0; i < number_of_names; i++)
		{
			int flag = 0;
			string_length = 0;
			name = (char*) malloc(sizeof(char)*50);
			while(1)
			{
				end_name = fgetc(f);
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
		/*Call append_local here to get local name */
		append_local(ip,name);
		free(name);
		/*Reset name check to 0 */
		end_name = 0;
	}
		/*Outside of for loop when you have all the names for the address call traversal function
		 and insert onto tree */
		
	/*Insert into tree */
	insert_to_tree(ip, traverser, j);
	}
	/*free name */
	//free(name);
	
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

		/*If found, call print function to traverse list */
		else
		{
			jrb_rtraverse(temp,j)
			{
				if (strcmp((char*)temp->key.s, input) == 0)
				{
					printer(ip,traverser,temp);
				}
			}
		}
	}
	/*call free memory functions and close file */
	free_mem(traverser,j);
	free_ip(ip);
	fclose(f);
}
