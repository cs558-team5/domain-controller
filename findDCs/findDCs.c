#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <limits.h>
#include <pthread.h>

#define NUM_OF_THREADS 200
#define NUM_OF_SUBS 31291

void *workThread(void *data);
void nslookup(char *subdomain);

char *domain;
char *lines[32000];
int linenumber = 0;
pthread_mutex_t line_mutex;

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: ./findDCs [domain]");
		return 0;
	}
	printf("\n");

	domain = argv[1];
	pthread_mutex_init(&line_mutex, NULL);

	// Open the subs file
	FILE *fp = fopen("subs.txt", "r");

	// Malloc space for the subs
	int i;
	for (i = 0; i < NUM_OF_SUBS; i++)
	{
		lines[i] = malloc(100);
	}

	// Read in the subs
	i = 0;
	while (fgets(lines[i++], 99, fp) != NULL);

	// Close the file
	fclose(fp);

	pthread_t threads[NUM_OF_THREADS]; // make an array of the threads to be run
	
	pthread_attr_t attr; // make an attr for the threads
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN); // allows for many more threads to be made
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	int ret;
	for (i = 0; i < NUM_OF_THREADS; i++) {
		ret = pthread_create(&threads[i], &attr, workThread, NULL);
		if (ret) { // end the progam if there was an error creating a thread
			printf("Error creating thread #%d, return cod is %d. Exiting program.\n", i + 1, ret);
			return 0;
		}
	}
	
	// destroy the attr and wait for all of the threads to finish
	void *status;
	pthread_attr_destroy(&attr);
	for (i = 0; i < NUM_OF_THREADS; i++) {
		ret = pthread_join(threads[i], &status);
	}

	return 0;
}

void *workThread(void *data)
{
	while (1)
	{
		pthread_mutex_lock(&line_mutex);

		if (linenumber >= NUM_OF_SUBS - 1)
		{
			pthread_mutex_unlock(&line_mutex);
			break;
		}

		char *line = lines[linenumber++];
		pthread_mutex_unlock(&line_mutex);

		char *p = strstr(line, "\n");
		*p = '\0';

		char subdomain[100];
		sprintf(subdomain, "%s.%s", line, domain);

		if (gethostbyname(subdomain))
		{
			nslookup(subdomain);
		}

		free(line);
	}

	pthread_exit(NULL);
}

void nslookup(char *subdomain)
{
	char hostname[200];
	char command[300];
	sprintf(hostname, "_ldap._tcp.dc._msdcs.%s", subdomain);
	sprintf(command, "nslookup -q=srv %s", hostname);

	char *lines[50];
	int i, j;
	for (i = 0; i < 50; i++)
	{
		lines[i] = malloc(200);
	}

	FILE *fp = popen(command, "r");

	i = 0;
	while (fgets(lines[i++], 199, fp) != NULL);

	int dc = 0;
	i = 3;
	while (strstr(lines[++i], hostname) != NULL)
	{
		char *p1 = lines[i];
		p1 = strstr(p1, "\t");
		if (strstr(p1, "service") == NULL)
		{
			break;
		}

		for (j = 0; j < 5; j++)
		{
			p1 = strstr(p1, " ");
			p1 += 1;
		}

		char *p2 = p1;
		for (j = 0; j < 4; j++)
		{
			p2 = strstr(p2, ".");
			p2 += 1;
		}
		p2 -= 1;
		p2[0] = '\0';

		if (!dc)
		{
			dc = 1;
			printf("--%s--\n", subdomain);
		}

		printf("%s\n", p1);
	}
	if (dc)
	{
		printf("\n");
	}

	for (i = 0; i < 50; i++)
	{
		free(lines[i]);
	}

	pclose(fp);
}
