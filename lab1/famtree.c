/*
	Author: Sam Steinberg
	September 2018

NOTE: I added headers for stdio to get rid of 'enabled by default' messages for outputting, same for 
respective libraries stdlib.h and string.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fields.h"
#include "jval.h"
#include "dllist.h"
#include "jrb.h"
/* n_person struct contains all info needed to 'build' a person.
  I set mother and father to be structs so that I can link them to their children
  with pointers, and their fields will be full. */
typedef struct n_person
{
	char *name;
	struct n_person *father;
	struct n_person *mother;
	char *sex;
	Dllist children;
	int visited;
	int print;
}n_person;
/* process_person will recieve a JRB tree and an extracted_name which, after a person object has
  been created and memory is allocated for it, will be set as the name field in the struct.
  Since I only call this function when somebody is not found in the tree, I 'intialize'
  their mother/father to null, sex to unknown, and their visited/print to 0.
  I will then create their children Dllist (will stay empty if no children) and insert them into the tree.
  Lastly, I return a n_person pointer so that I can copy that information into the current person I am working on in main*/
n_person*  process_person(JRB j, char extracted_name[200])
{
	n_person *np;
	np = malloc(sizeof(n_person));
	np->name = strdup(extracted_name);
	np->father = NULL;
	np->mother = NULL;
	np->sex = strdup("Unknown");
	np->visited = 0;
	np->print = 0;
	np->children = new_dllist();
	jrb_insert_str(j, np->name, new_jval_v(np));
	return np;
}
/*This function will see the tree is contaminated with bad data (somebody is their own ancestor
 Each persons Dllist is traversed, and if val.v is hit then that person is their own ancestor and in main
 an error message will be given. A n_person object is passed to this function and will use the previously created
 links to scour the tree.*/
int is_descendant(n_person *np)
{
	if (np->visited == 1)
		return 0;
	if (np->visited == 2)
		return 1;

	np->visited = 2;
	Dllist temp;
	if (!dll_empty(np->children))
	{
		dll_traverse(temp, np->children){
		if (is_descendant(temp->val.v)) 
			return 1;
		}
	}
	np->visited = 1;
	return 0;
}
/* char array is passed along with the IS struct, names concatenated and returned accordingly*/
char* set_name(IS is, char name[200])
{
	int i;
	char * temp = "";
	strcpy(name, is->fields[1]);
	if (is->NF > 2)
	{
		for (i=2; i < is->NF; i++)
		{
			strcat(name, " ");
			strcat(name, is->fields[i]);
		}
	}
	temp = strdup(name);
	return temp;
	
}
/* A temp_sex extracted from main represents the given sex from the file. If it is not "M" or "F"
   then the sex of the person will be set as unknown. Or else, the temp_sex will return. I made
   this function to make error checking and assignment easier. p->sex is then set in main.
 */
char* set_sex(char *temp_sex)
{
	if (strcmp(temp_sex, "F") == 0)
		temp_sex = strdup("F");
	else
		if (strcmp(temp_sex,"M") == 0)
			temp_sex = strdup("M");
	else
		temp_sex = strdup("Unknown");

	return temp_sex;
}
/*free_mem will take the struct for fields, red-black-tree, and dllist and free them accordingly. Called at end of main */
void free_mem(IS s, JRB j, Dllist d)
{
	jettison_inputstruct(s);
	jrb_free_tree(j);
	free_dllist(d);
}
/*Cleans up main a bit */
void sex_error(n_person *p, int ln)
{
	if(strcmp(p -> sex, "Unknown") != 0)
	{
		if(strcmp(p -> sex, "M") != 0)
			{
				fprintf(stderr, "Bad input - sex mismatch on line %d\n", ln);
				exit(1);
			}
	}
}
/*also cleans up main */
void sex_error_mother(n_person *p, int ln)
{
	if(strcmp(p -> sex, "Unknown") != 0)
	{
		if(strcmp(p -> sex, "F") != 0)
			{
				fprintf(stderr, "Bad input - sex mismatch on line %d\n", ln);
				exit(1);
			}
	}

}
/*Checks for father and error if so*/
void two_fathers_error(n_person *p, int ln)
{
	if(p -> father != NULL)
	{
		fprintf(stderr, "Bad input -- child with two fathers on line %d\n", ln);
		exit(1);
	}

}
/*Checks if p already has a mother and error if so */
void two_mothers(n_person *p, int ln)
{
	if(p -> mother != NULL)
	{
		fprintf(stderr, "Bad input -- child with two mothers on line %d\n", ln);
		exit(1);
	}
}
/*If p has no children, print none. Or else, traverse their children list and print them.*/
void print_children(n_person *p, n_person *temp, Dllist tp, Dllist cn)
{
	if(dll_empty(p -> children))
	{
		printf("  Children: None\n");
	}
	else
	{
		printf("  Children:\n");
		dll_traverse(cn, p -> children)
		{
			temp = (n_person *) cn -> val.v;
			printf("    %s\n", temp -> name);
			dll_append(tp, new_jval_v(temp));
		}
	}
	printf("\n");

}
/*If the node isnt found the father is created and sex is set to male. I then check to see
  if the person already has a father, which if triggered I exit the program. Or else,
  I will set the p's father struct to be the local father and I append to p's father dllist */ 
void process_father(n_person *f, n_person *p, JRB j, int ln, char*name)
{
	f = process_person(j, name);
	f -> sex = strdup("M");
	two_fathers_error(p,ln);
	p -> father = f;
	dll_append(f -> children, new_jval_v(p));
}
/* Same as above */
void process_mother(n_person *m, n_person *p, JRB j, int ln, char*name)
{
	m = process_person(j, name);
	m -> sex = strdup("F");
	two_mothers(p,ln);
	p -> mother = m;
	dll_append(m -> children, new_jval_v(p));
}
/* Similar to mother and father, but now its the child of the parent being processed.
 That means its the parent links of the child we are setting */
void process_fchild(n_person *c, n_person *p, JRB j, int ln, char *name)
{
	c = process_person(j, name);		
	sex_error(p, ln);
	p -> sex = "M";
	c -> father = p;
	dll_append(p -> children, new_jval_v(c));
}
/*Sam as process_fchild() but for mother*/
void process_mchild(n_person *c, n_person *p, JRB j, int ln, char* name)
{
	c = process_person(j, name);		
	sex_error_mother(p, ln);
	p -> sex = "F";
	c -> mother = p;
	dll_append(p -> children, new_jval_v(c));
}
/*If not null, father is casted to be the center of attention. Error checking for sex and a father already present ensure,
  and will exit with a message if triggered. If not, the sex is set to male and the links between father and 
  children are made. NOTE: For processing children, I reverted back to my revision 430 when I was correctly printing children.
  I was traversing the dllist is it existed, but the error checks render this useless as if a child is already on a dllist of the
  fathers sex it will recognize the child already has a father */
void add_father(n_person *f, JRB temp, n_person *p, int ln)
{					
	f = (n_person *) temp-> val.v;
	if(strcmp(f -> sex, "F") == 0)
	{
			fprintf(stderr, "Bad input - sex mismatch on line %d\n", ln);
			exit(1);
	}
	f -> sex = strdup("M");
	two_fathers_error(p,ln);
	p -> father = f;
	dll_append(f-> children, new_jval_v(p));
}
/*Same as for father but for mother list*/
void add_mother(n_person *m, JRB temp, n_person *p, int ln)
{
	m = (n_person *) temp -> val.v;
	if(strcmp(m -> sex, "M") == 0)
	{
			fprintf(stderr, "Bad input - sex mismatch on line %d\n", ln);
			exit(1);
	}
	m -> sex = "F";
	two_mothers(p,ln);
	p -> mother = m;
	dll_append(m -> children, new_jval_v(p));
}
/* Same as above but child is being linked to its father instead of p (who is this case is the father) */
void add_fchild(n_person *c, JRB temp, n_person *p, int ln)
{
	c = (n_person *) temp -> val.v;
	sex_error(p,ln);
	p -> sex = "M";
	c -> father = p;
	dll_append(p -> children, new_jval_v(c));
}
void add_mchild(n_person *c, JRB temp, n_person *p, int ln)
{
	c = (n_person *) temp -> val.v;
	sex_error_mother(p,ln);
	p -> sex = "F";
	c -> mother = p;
	dll_append(p -> children, new_jval_v(c));

}
/* In main, five person struct objects are created. I do this so I can link between the 'main person' which in
 this program is 'p' and its mother/father structures inside of n_person. I then created my field, jrbs, and dllists.
 the beginning of main also contains an error check to make sure the program is read from stdin*/
main(int argc, char **argv)
{
	n_person *p;
	n_person *father;
	n_person *mother; 
	n_person *child;
	n_person *temp_for_list;

	IS is;
	JRB jrb;
	JRB temp_node;
	Dllist toprint, current_node;

	int i;
	int line = 0;
	is = new_inputstruct(NULL);
	
	
	jrb = make_jrb();
	toprint = new_dllist();

	if(is == NULL)
	{ 
		perror(argv[1]); 
		exit(1); 
	}
	/*getline will read from the file. getline will return -1 when finished reading, so whenever it is greater
	 than or equal to zero (i.e. not positive, it will continue looping Line number counter and check for 
	 number of fields is included here as well*/
	while(get_line(is) >= 0)
	{
		line++;
		if(is -> NF >= 1)
		{
			/*If the field reads person, a char array is created than is safe to say large enough to hold all 
			 the chars in a name. I then copy is->fields[1] aka the name of the person, but I have to check for 
			 multiple names or last names. That is when the for loop is added and the string must be concatenated. 
			 To note, I made the char so large because strcat does not allocate memory. Spent a lot of time trying
			 to realize why I was segfaulting here. NOTE:I am describing my function here (elaborating on it)*/
			if(strcmp(is->fields[0], "PERSON") == 0)
			{
				char read_name[200];
				char *temp = set_name(is,read_name);

				/*temp_node will return null is the name is not found in the tree, i.e. the person has not been processed
				 yet. If this is the case process_person is called and they are added to the tree. If not, it is n_person*
				 casted and set to be the current person. In other words, I tell the program "I am talking about this person now" */
				temp_node = jrb_find_str(jrb, temp); 

				if(temp_node == NULL)
				{
					p = process_person(jrb, temp);

				}
				else
				{
					p = (n_person *) temp_node -> val.v;
				}
			}

			/*For processing a father, the same process occurs for processing the name and searching the jrb tree
			 to check to see if the father has been processed.*/
			else if(strcmp("FATHER", is->fields[0]) == 0)
			{ 
				char f_name[200];
				char *f_temp = set_name(is,f_name);

				temp_node = jrb_find_str(jrb, f_temp);
				if(temp_node == NULL)
				{
					process_father(father, p, jrb, line, f_temp);
				}
				else
				{
					add_father(father, temp_node, p, line);
				}

			}
			/*MOTHER works the same way as FATHER, but with the error checks and links changed accoringly*/
			else if(strcmp("MOTHER", is->fields[0]) == 0)
			{  
				char m_name[200];
				char *m_temp = "";
				m_temp = set_name(is,m_name);
				
				temp_node = jrb_find_str(jrb, m_temp);
				/* If node is null create person....*/
				if(temp_node == NULL)
				{
					process_mother(mother, p, jrb, line, m_temp);
				}
				/* or else set the person and error check and append...*/
				else
				{
					add_mother(mother, temp_node, p, line);
				}
			}
			/*Father of name processing same as above */
			else if(strcmp("FATHER_OF", is->fields[0]) == 0)
			{
				char fo_child[200];
				char * fo_temp = set_name(is,fo_child);

				temp_node = jrb_find_str(jrb, fo_temp);
				if(temp_node == NULL)
				{
					process_fchild(child, p, jrb, line, fo_temp);
				}
				else
				{
					add_fchild(child, temp_node, p, line);
				}
			}
			/*Same as father of but the center of attention is the mother */
			else if(strcmp("MOTHER_OF", is->fields[0]) == 0)
			{  
				
				char namem[200];
				char *mo_temp = set_name(is,namem);

				temp_node = jrb_find_str(jrb, mo_temp);
				/* Same processes and error checks */
				if(temp_node == NULL)
				{
					process_mchild(child,p,jrb,line,namem);
				}
				/*Check for sex errors and two mothers like above */
				else
				{
					add_mchild(child, temp_node, p, line);
				}
			}
			/*temp_sex holds the field given with sex, has an error check for a bad sex input,
			 then will call set_sex with temp_sex being passed. */
			else if(strcmp("SEX", is->fields[0]) == 0)
			{
				char * temp_sex = strdup(is->fields[1]);
				if (strcmp(p->sex,"Unknown") != 0)
				{
					if (strcmp(p->sex, strdup(is->fields[1])) != 0)
					{
						fprintf(stderr,"Bad input - sex mismatch on line %d\n",line);
						exit(1);
					}
				}
				p->sex = set_sex(temp_sex);
			}
		/*I dont know why but this fixed errors cutting off processing of people */
		if(is->NF == 0) { }

		}

	}
	/*For cycle detection, the jrb tree is traversed. p is typecasted as the void value
	  of a person, it is appended to toprint, and is descendent is called for the duration of traversal. */
	jrb_traverse(temp_node, jrb){
		p = (n_person *) temp_node -> val.v;
		dll_append(toprint, new_jval_v(p));
		if(is_descendant(p)){
			fprintf(stderr, "Bad input -- cycle in specification\n");
			exit(1);
		}
	}

	/*When the dllist is not empty, p is set as the person we are talking about. deleting the top we are replicating the
	  behavior of a queue, and when a person hasnt been printed....*/
	while(!dll_empty(toprint)){
		current_node = dll_first(toprint);
		p = (n_person *) current_node -> val.v;
		dll_delete_node(toprint -> flink);
		/*This takes into account all possible combinations. When p has no parents, print p. Then the mess of conditions ensue,
		  which making the mother and father structs made a lot easier to do. This is because I can access the parents of p and check to see
		  if they exist, or have already been printed. print == 1 notes that they have been printed. Followed the given 
		  pseudo-code pretty exact. */
		if(p -> print == 0)
		{
			if((p -> father == NULL && p -> mother == NULL) || (p -> father != NULL) && (p -> mother == NULL && p -> father -> print == 1) || (p -> mother != NULL) && (p -> father == NULL && p -> mother -> print == 1) || (p -> mother != NULL && p -> father != NULL) && (p -> father -> print == 1 && p -> mother -> print == 1))
		
			{
				printf("%s\n", p -> name);
				if(strcmp(p -> sex, "Unknown") == 0)
				{
					printf("  Sex: Unknown\n");
				}
				/*After printing the name and checking is the name is unknown, I use the condional ? to determine which element should be
				  printed. Learned this in 302 with Gregor. Didnt think it would be that useful but I was wrong. Had a TON of errors 
				  getting the sex to only be unknown before adding this. Many 1/100 gradescripts later I added them. */
				else
				{
					if (strcmp(p->sex, "M") == 0)
					{
						p->sex = strdup("Male");
					}
					else
						p->sex = strdup("Female");
					printf("  Sex: %s\n", p->sex);
				}
				if (p->father == NULL)	
					printf("  Father: Unknown\n");
				else
				printf("  Father: %s\n", p -> father -> name);
			
				if(p->mother == NULL)
					printf("  Mother: Unknown\n");
				else
					printf("  Mother: %s\n", p -> mother -> name);
				p->print = 1;
		
				print_children(p,temp_for_list,toprint,current_node);
			}
		}
		
	}
	/* Calls free memory function to deallocate */
	free_mem(is,jrb,toprint);
}
