/* 
	Author: Samuel Steinberg
	October 2018
*/
#include <stdio.h>
#include <string.h>
#include "malloc.h"

int main(int argc, char *argv[])
{
	/*LARGE SIZE TEST */
	printf("LARGE BUFFER TESTING: \n");
	char *big_buf = malloc(8193);
	printf("Address of malloc: 0x%x\n\n", big_buf);
	
	/*FREE LIST TESTING */
	printf("\nLIST TESTING: RE-USE OF ADDRESSES THAT WERE FREED\n\n");
	char *lt = malloc(12);
	printf("Address of malloc and free call: 0x%x\n", lt);
	free(lt);
	char *lt1 = malloc(12);
	printf("Address of malloc and free call: 0x%x\n", lt1);
	free(lt1);
	printf("\n");

	/*MALLOC TESTING*/
	printf("\nMALLOC TESTING WHEN CALLED EXPLICITLY:\n\n");
	char *test = strdup("This is a test");
	printf("Address of malloc: 0x%x\n", test);
	char *test2 = malloc(20);
	strcpy(test2, "This is a test");
	printf("Address of malloc: 0x%x\n", test2);

	/*CALLOC TESTING*/
	printf("\n\nCALLOC TESTING: \n\n");
	char *post_calloc = malloc(20);
	printf("Address before calloc: 0x%x\n", post_calloc);
	strcpy(post_calloc, "Calloc-Tester");
	printf("String before calloc is called: %s\n\n", post_calloc);
	post_calloc = calloc(0, 2*sizeof(char));
	printf("Address after calloc: 0x%x\n", post_calloc);
	printf("String after calloc is: %s\n\n", post_calloc);

	
	/* REALLOC TESTING*/
	printf("\nREALLOC TESTING: \n\n");
	char *rtest = malloc(40);
	strcpy(rtest, "Tiger");
	printf("Address before realloc is: 0x%x\n", rtest);
	rtest = realloc(rtest, 40);
	strcat(rtest, "+realloc");
	printf("Address after realloc is: 0x%x\n\n", rtest);
	printf("\n\n");


	/*FREE TESTING -- Please note that these are down here for readability: So to better see the flow of addresses for malloc-realloc-calloc*/
	printf("FREE TESTING (THESE ARE THE VARIABLES WHEN MALLOC IS EXPLICITLY CALLED ABOVE): \n\n");
	printf("Address of ptr: 0x%x\n",test);
	free(test);
	printf("Address of ptr: 0x%x\n",test2);
	free(test2);

	return 0;
}
