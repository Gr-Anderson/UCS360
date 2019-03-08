/*
	Author: Samuel Steinberg
	November - December 2018
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "fields.h"
#include "jrb.h"
#include "dllist.h"
#include "jval.h"
#include "socketfun.h"

/*Client structure needs a client name, their fd, whether they are logged in or not, their join/quit/last message times, and
  their input and output streams */
typedef struct Client 
{
	char *name;
	int fd;
	int logged_in;
	struct tm join;
	struct tm quit;
	struct tm last_msg;
	FILE *in;
	FILE *out;
}Client;
/*Server struct has a list of clients (current), messages, all client ever to be on the server, a mutex lock, a conditional,
  and the host/port the server is serving */
typedef struct Server 
{
	Dllist clients;
	Dllist messages;
	Dllist all;
	pthread_mutex_t *user_lock;
	pthread_cond_t *cvar;
	int port;
	char *host;
}Server;

/*Very important -- global server struct makes things a whole lot easier */
struct Server* server;

/*This function adds the name of the client. I call strtok to get the string up to the ":", since that is how jtalk
  sends that line */
/*void add_client(Client *client, char *temp_name, int name_size)
{
	pthread_mutex_lock(server->user_lock);

	dll_append(server->messages, new_jval_s(strdup(temp_name)));
	client->name = malloc(name_size);
	client->name = strtok(temp_name, ":");
	pthread_cond_signal(server->cvar);
	pthread_mutex_unlock(server->user_lock);
	
	dll_append(server->all, new_jval_v(client));
	dll_append(server->clients, new_jval_v(client));
}*/
/*Bad output stream -- client */
void err_out(Client *client)
{
	if (client->out == NULL)
	{
		perror("fd in output");
		exit(1);
	}
}
/*Bad input stream -- client */
void err_in(Client *client)
{
	if (client->in == NULL)
	{
		perror("fd in input");
		exit(1);
	}
}
/*This function handles the client thread */
void *handle_client(void* t_client)
{
	Dllist temp;
	Client* client;
	client = (Client *)t_client;
	/*Open file descriptors, throw errors if bad */
	client->in = fdopen(client->fd, "r");
	err_in(client);
	client->out = fdopen(client->fd, "w");
	err_out(client);
	
	int *name_size = malloc(sizeof(int));
	char *temp_name;
	/*Read in the size of the name */
	if ( fread(name_size, sizeof(int), 1, client->in) < 1)
	{
		perror( "Error reading name size\n");
		exit(1);
	}

	client->name = malloc((*name_size));
	/*Read in the name byte by byte up to the size read from above */
	if ( (fread(client->name, *name_size, 1, client->in) < 1) )
	{
		perror("Error reading in name\n");
		exit(1);
	}
	free(name_size);
	pthread_mutex_lock(server->user_lock);

	dll_append(server->messages, new_jval_s(strdup(client->name)));
	pthread_cond_signal(server->cvar);
	pthread_mutex_unlock(server->user_lock);
	client->name = strtok(client->name, ":");
	
	dll_append(server->all, new_jval_v(client));
	dll_append(server->clients, new_jval_v(client));
//	add_client(client, temp_name, name_size);
	while(1)
	{
		
	int content_length = 0;
		/*Handle the client leaving. Set their log flag to zero,  append the quit message, delete them from 
		 the message stream, and append the 'has quit'. Update server */
	//	if (fread((char *)&content_length, sizeof(int), 1, client->in) < 1)
		if (read(client->fd, (char*)&content_length, 4) < 1)
		{
			fclose(client->in);
			time_t times = time(NULL);
			client->logged_in = 0;
			client->quit = *localtime(&times);
			int len = strlen(client->name) + 50;
			char *t_name = malloc(sizeof(char)*len);
			char *t = strdup(client->name);
			strcpy(t_name, t);
			strcat(t_name, " has quit\n");
			pthread_mutex_lock(server->user_lock);
			dll_append(server->messages, new_jval_s(strdup(t_name)));
			free(t_name);
			fclose(client->out);
			dll_traverse(temp, server->clients)
			{
				if (strcmp(((Client *)temp->val.v)->name, client->name) == 0)
				{
					dll_delete_node(temp);
					break;
				}
			}
			pthread_cond_signal(server->cvar);
			pthread_mutex_unlock(server->user_lock);
			pthread_exit(NULL);
		}
		/*Handle the client leaving. Set their log flag to zero,  append the quit message, delete them from 
		 the message stream, and append the 'has quit'. Update server */
		char *content = malloc(content_length + 1);
		if (fread(content, content_length, 1, client->in) < 1)
		{	
			fclose(client->in);
			time_t times2 = time(NULL);
			client->logged_in = 0;
			client->quit = *localtime(&times2);
			int len2 = strlen(client->name) + 50;
			char *t_name2 = malloc(sizeof(char)*len2);
			char *t2 = strdup(client->name);
			strcpy(t_name2, t2);
			strcat(t_name2, " has quit\n");
			pthread_mutex_lock(server->user_lock);
			dll_append(server->messages, new_jval_s(strdup(t_name2)));
			free(t_name2);
			fclose(client->out);
			dll_traverse(temp, server->clients)
			{
				if (strcmp(((Client *)temp->val.v)->name, client->name) == 0)
				{
					dll_delete_node(temp);
					break;
				}
			}
			pthread_cond_signal(server->cvar);
			pthread_mutex_unlock(server->user_lock);
			pthread_exit(NULL);
		}
		/*Need to add in the NULL character at the end of the message, then update times and append the message
		  to the server. (content is the message in my case).*/
		else
		{
			content[content_length] = '\0';
			time_t ti = time(NULL);
			client->last_msg = *localtime(&ti);
			pthread_mutex_lock(server->user_lock);
			dll_append(server->messages, new_jval_s(strdup(content)));
			pthread_cond_signal(server->cvar);
			pthread_mutex_unlock(server->user_lock);
		}
		free(content);
	}
	return NULL;
}
/*This function is a modified version from jtalk. The +4 accounts for the size of the name */
void send_stbytes(char *p, char *ptr, Client *client, int i)
{
	while (ptr < (p + 4) )
	{
		i = write(client->fd, ptr, (p-ptr)+4);
		ptr+=i;
	}

}
/*This function handles the server thread */
void *handle_server(void *s)
{
	server = (Server *)s;
	Client *client;
	Dllist message;
	Dllist clients;
	char* temp_message;
	int message_size;
	char *ptr;
	int i;
	char *p;
	pthread_mutex_lock(server->user_lock);
	Dllist temp;
	while(1)
	{
		pthread_cond_wait(server->cvar, server->user_lock);

		/*Traverse messages*/
		dll_traverse(clients, server->clients)
		{
			client = (Client *)clients->val.v;
			/*Handle messages on server and write the messages*/
			dll_traverse(message, server->messages)
			{
				message_size = strlen(message->val.s);
				temp_message = message->val.s;
				p = (char *)&message_size; 
				ptr = (char *)&message_size;
				send_stbytes(p,ptr,client,i);
				/*Handle the client quitting. Set their log flag to zero,  append the quit message, delete them from 
				  the message stream, and append the 'has quit' */
				if (fputs(message->val.s, client->out) == EOF)
				{
					fclose(client->in);
					time_t qtime = time(NULL);
					client->logged_in = 0;
					client->quit = *localtime(&qtime);
					int len = strlen(client->name) + 50;
					char *t_name = malloc(sizeof(char)*len);
					char *t = strdup(client->name);
					strcpy(t_name, t);
					strcat(t_name, " has quit\n");
					pthread_mutex_lock(server->user_lock);
					dll_append(server->messages, new_jval_s(strdup(t_name)));
					free(t_name);
					fclose(client->out);
					dll_traverse(temp, server->clients)
					{
						if (strcmp(((Client *)temp->val.v)->name, client->name) == 0)
						{
							dll_delete_node(temp);
							break;
						}
					}
					pthread_cond_signal(server->cvar);
					pthread_mutex_unlock(server->user_lock);
					pthread_exit(NULL);
				}
				/*Handle the client quitting. Set their log flag to zero,  append the quit message, delete them from 
				  the message stream, and append the 'has quit' */
				if (fflush(client->out) == EOF)
				{
					fclose(client->in);
					time_t qtime2 = time(NULL);
					client->logged_in = 0;
					client->quit = *localtime(&qtime2);
					int len2 = strlen(client->name) + 50;
					char *t_name2 = malloc(sizeof(char)*len2);
					char *t2 = strdup(client->name);
					strcpy(t_name2, t2);
					strcat(t_name2, " has quit\n");
					pthread_mutex_lock(server->user_lock);
					dll_append(server->messages, new_jval_s(strdup(t_name2)));
					free(t_name2);
					fclose(client->out);
					dll_traverse(temp, server->clients)
					{
						if (strcmp(((Client *)temp->val.v)->name, client->name) == 0)
						{
							dll_delete_node(temp);
							break;
						}
					}
					pthread_cond_signal(server->cvar);
					pthread_mutex_unlock(server->user_lock);
					pthread_exit(NULL);
				}
			}
		}
		free_dllist(server->messages);
		server->messages = new_dllist();
	}
	//return NULL;
}
/* This function handles printing all clients who have connected to the server. */
void print_all(int i, Dllist temp, Client *client)
{
	i = 1;
	printf("Jtalk server on %s port %d\n", server->host, server->port);
	dll_traverse(temp, server->all)
	{
		client = temp->val.v;
		printf("%d      %-8s", i, ((Client*)temp->val.v)->name);
		/*If the client is still logged in, print LIVE. If they are not print DEAD */
		if ( ((Client*)temp->val.v)->logged_in == 1)
		{
			printf("  LIVE\n");
		}
		else if ( ((Client*)temp->val.v)->logged_in == 0)
		{
			printf("  DEAD\n");
		}
		/*Times and if client quit then print the time they quit */
		printf("   %-11s at %s", "Joined",  asctime(&(client->join)));
		printf("   %-11s at %s", "Last talked", asctime(&(client->last_msg)));
		if ( !(((Client *)temp->val.v)->logged_in) )
		{
			printf("   %-11s at %s", "Quit", asctime(&(client->quit)));
		}
		i++;
	}
}
/*This function prints all the users currently on the server. Start the index at 1 and give the name of the
  machine, and the name of the client, and the time they last spoke */
void print_talkers(int j, Dllist temp, Client *client)
{
	j = 1;
	printf("Jtalk server on %s port %d\n", server->host, server->port);
	dll_traverse(temp, server->clients)
	{
		client = temp->val.v;
		printf("%d    %-11s last talked at %s", j, ((Client*)temp->val.v)->name, asctime(&(client->last_msg)));
		j++;
	}
}
/*Terminal thread */
void *terminal_window()
{
	IS is;
	is = new_inputstruct(NULL);
	Dllist temp;
	int i;
	int j;
	Client *client;
	printf("Jtalk_server_console> ");
	while(get_line(is) >= 0)
	{
		/* If the command is not ALL or TALKERS then throw an error -- text1 gives the whole line that was types */
		if (strcmp(is->fields[0], "ALL") != 0 && strcmp(is->fields[0], "TALKERS") != 0)
		{
			printf("Unknown console command: %s", is->text1);
		}
		/*If command is ALL print all clients who have connected to server*/
		else if (strcmp(is->fields[0], "ALL") == 0)
		{
			print_all(i, temp, client);
		}
		/*If command is TALKERS print all clients logged into server */
		else if (strcmp(is->fields[0], "TALKERS") == 0)
		{
			print_talkers(j,temp,client);
		}

		printf("Jtalk_server_console> ");
	}
	dll_traverse(temp, server->all)
	{
		client = (Client *)temp->val.v;
		free(client->name);
		free(client);
	}
}
/*Initialize client struct -- name, file descriptor, input stream, output stream, flag that the user has logged in*/
void initialize_client(Client* c, int fd)
{
	c->name = NULL;
	c->fd = fd;
	c->in = NULL;
	c->out = NULL;
	c->logged_in = 1;
}
int main(int argc, char *argv[])
{
	int sock, fd, port;
	
	pthread_t *thread;
	Client* client;
	/* Need a host number and a port */
	if (argc != 3)
	{
		fprintf(stderr, "usage: jtalk_server host port\n");
		exit(1);
	}
	port = atoi(argv[2]);
	/*port must be less than 5000 */
	if (port < 5000)
	{
		fprintf(stderr, "Must use a port >= 5000");
		exit(1);
	}
	/* Setting up Server -- malloc server and initialize it. Then create a thread for it*/
	server = (Server *)malloc(sizeof(Server));		
	server->clients = new_dllist();
	server->messages = new_dllist();
	server->all = new_dllist();
	server->port = port;
	server->host = strdup(argv[1]);
	server->user_lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(server->user_lock, NULL);
	server->cvar = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
	pthread_cond_init(server->cvar, NULL);
	thread = (pthread_t *)malloc(sizeof(pthread_t));
	if (pthread_create(thread, NULL, handle_server, server) != 0)
	{
		perror("server thread");
		exit(1);
	}
	pthread_detach(*thread);

	/*Setting up terminal  -- Simply creates a thread for it*/
	thread = (pthread_t *)malloc(sizeof(pthread_t));
	if (pthread_create(thread, NULL, terminal_window, NULL) != 0)
	{
		perror("Terminal thread");
		exit(1);
	}
	pthread_detach(*thread);

	/*Setting up clients -- sock is a fd that represents the socket. Infinite loop to wait for and accept connections, when fd passed to 
	 accept_connection on success will give a new fd, client is created and initialized. I then initialize the times and 
	 create a thread for the client connections.*/
	sock = serve_socket(argv[1], port);
	while(1)
	{
		fd = accept_connection(sock);
		client = (Client*)malloc(sizeof(Client));
		initialize_client(client, fd);
		time_t t = time(NULL);
		client->last_msg = *localtime(&t);
		client->join = *localtime(&t);
		thread = (pthread_t *)malloc(sizeof(pthread_t));
		if (pthread_create(thread, NULL, handle_client, client) != 0)
		{
			perror("client thread");
			exit(1);
		}
		pthread_detach(*thread);
	}
	return 0;
}
