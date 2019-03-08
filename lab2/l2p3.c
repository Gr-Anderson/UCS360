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
/* add local names but getting "substring" up to first dot --> local name */
void append_local(IP *ip, char *name)
{
	char *local;
	local = strtok(name,".");
	dll_append(ip->names, new_jval_s(local));
}
/*Insert into jrb tree traversing dllist*/
void insert_to_tree(IP *ip, Dllist traverser, JRB j)
{
	dll_rtraverse(traverser, ip->names)
	{
		jrb_insert_str(j, jval_s(traverser->val), new_jval_v(ip));
	}		
}

/*Traverser dllist when string found in tree in main */
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
	unsigned char store_ip[4];
	unsigned char buffer[350000];	
	unsigned int number_of_names;	
	int num_bytes = 0;
	int i;
	int string_length;
	char * name;
	int end_name;
	char user_input[30];
	char *input;
	char *local;
    int ctrl_d;
	int x;
	int end_check = 0;
	//Open the file and only allow reading. Invalid file will return -1, valid file returns 0 */
	f = open("converted", O_RDONLY, O_RDONLY);
	file_err(f);
	
	/*Read in directly from buffer, while loops to the size of buffer -- num_bytes is just a counter*/
	read(f, buffer, sizeof(buffer));
	while (num_bytes < 350000)
	{
		/*Reset number of names read in, store first four vals in IP address */
		number_of_names = 0;
		for (x = 0; x < 4; x++)
		{
			store_ip[x] = buffer[num_bytes++];
		}
		/*Allocate for IP and its Dllist of names */
		ip = (IP *)malloc(sizeof(ip));
		ip -> names = new_dllist(); 
		
		/*Store the IP address, stopper value will be when the ip address is "0.0.0.0"  */
		sprintf(ip->address, "%d.%d.%d.%d", store_ip[0], store_ip[1], store_ip[2], store_ip[3]);
		
		if (strcmp(ip->address, "0.0.0.0") == 0)
		{
			break;
		}

		/*Shift each bit left 8 to get int val, allows fast multiplication, increment to get next byte in buffer 
		  NOTE: Took out my hex shift because I couldnt figure out the incrementing*/
		number_of_names = (buffer[num_bytes++] << 8);
		number_of_names |= (buffer[num_bytes++] << 8);
		number_of_names |= (buffer[num_bytes++] << 8);
		number_of_names |= buffer[num_bytes++];

		/*From i to the number of names in this IP address, set/reset flags and length of string at the top.
		  Then, allocate space for a name (I assumed non would be more than 50 chars long). I then used
		  read to get the char at that place in the file, and if it was zero (reps null terminator) I broke out.
		  If I hit a '.' I flagged it so then I could extract the local name after the loop. I called this var end_name.
		  I then incremented string length. PART 3: end_name char is at the buffer this time since we are reading directly*/
		
		for (i = 0; i < number_of_names; i++)
		{
			int flag = 0;
			string_length = 0;
			name = (char*) malloc(sizeof(char)*50);
			while(1)
			{
				end_name = buffer[num_bytes++];
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
		append_local(ip,name);
		
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
		/*If string not found in jrb give error message key not found, if not NULL traverse tree and whenever
		 the string is found in the tree append */
		if ( jrb_find_str(j,input) == NULL)
		{
			print_error_check(input);
		}
		else
		{		
			jrb_rtraverse(temp, j)
			{
				if(strcmp((char*)temp->key.s, input) == 0)
				{
					printer(ip,traverser,temp);
				}
			}
		}
	
	}
	/*call free memory functions and close file */
	free_mem(traverser,j);
	free_ip(ip);
	close(f);
}
