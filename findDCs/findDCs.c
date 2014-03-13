#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/stat.h>

void nslookup(char *subdomain);

int main(int argc, char *argv[])
{
	// make sure there are inputted values for the needed variables
	if (argc != 4)
	{
		printf("usage: ./findDCs numOfSubs numOfDomains numOfProcesses\n");
		return 0;
	}

	// validate the inputted values
	int numOfSubs = atoi(argv[1]);
	int numOfDomains = atoi(argv[2]);
	int numOfProcesses = atoi(argv[3]);
	if (numOfSubs < 1 || numOfDomains < 1 || numOfProcesses > numOfDomains)
	{
		printf("numOfSubs, numOfDomains, and numOfProcesses must be greater than 0; numOfProcesses cannot be greater than numOfDomains\n");
		return 0;
	}

	char *subs[numOfSubs];

	printf("\n");

	// open the subs file
	FILE *fp = fopen("subs.txt", "r");

	// malloc space for the subs
	int i, j, k;
	for (i = 0; i < numOfSubs; i++)
	{
		subs[i] = malloc(100);
	}

	// read in the subs
	i = 0;
	while (fgets(subs[i++], 99, fp) != NULL && i < numOfSubs);

	// close the file
	fclose(fp);

	// remove all new lines
	for (i = 0; i < numOfSubs; i++)
	{
		char *p = strstr(subs[i], "\n");
		*p = '\0';
	}

	// divide the domains
	if (mkdir("domains", 0700) == 0)
	{
		char command[100];
		sprintf(command, "./divideDomains %d %d", numOfDomains, numOfProcesses);
		system(command);
	}

	// create the processes and start making DNS queries
	pid_t pids[numOfProcesses];
	for (i = 0; i < numOfProcesses; i++)
	{
		pid_t pid = fork();
		if (pid == 0)
		{
			int start = i * (numOfDomains / numOfProcesses);
			int end = start + (numOfDomains / numOfProcesses);
			if (i == (numOfProcesses - 1))
			{
				end += (numOfDomains % numOfProcesses);	
			}
			char filename[100];
			sprintf(filename, "domains/domains%d.txt", i);
			fp = fopen(filename, "r");

			// malloc space for the domains
			char *domains[end - start];
			for (j = 0; j < end - start; j++)
			{
				domains[j] = malloc(200);
			}

			// read in the domains
			j = 0;
			while (fgets(domains[j++], 199, fp) != NULL);

			// close the file
			fclose(fp);
			
			// loop through all domains
			for (j = 0; j < end - start; j++)
			{
				char *p = strstr(domains[j], "\n");
				*p = '\0';
				nslookup(domains[j]);
			}

			// loop through all subdomains
			for (j = 0; j < numOfSubs; j++)
			{
				for (k = 0; k < end - start; k++)
				{
					char subdomain[300];
					sprintf(subdomain, "%s.%s", subs[j], domains[k]);

					//printf("%s\n", subdomain);
					if (gethostbyname(subdomain))
					{
						nslookup(subdomain);
					}
					
				}
			}

			// free the subs
			for (i = 0; i < numOfSubs; i++)
			{
				free(subs[i]);
			}

			// free the domains
			for (i = 0; i < end - start; i++)
			{
				free(domains[i]);
			}
			
			return 0;
		}
		else
		{
			pids[i] = pid;
		}
	}

	// wait for the processes to finish
	for (i = 0; i < numOfProcesses; i++)
	{
		waitpid(pids[i], NULL, 0);
	}

	// free the subs
	for (i = 0; i < numOfSubs; i++)
	{
		free(subs[i]);
	}

	// delete the divided domains folder
	system("rm -rf domains");

	return 0;
}

void nslookup(char *subdomain)
{
	char hostname[300];
	char command[400];
	sprintf(hostname, "_ldap._tcp.dc._msdcs.%s", subdomain);
	sprintf(command, "nslookup -q=srv %s", hostname);

	char *lines[50];
	int i, j;
	for (i = 0; i < 50; i++)
	{
		lines[i] = malloc(300);
	}

	FILE *fp = popen(command, "r");

	i = 0;
	while (fgets(lines[i++], 299, fp) != NULL);

	int dc = 0;
	i = 3;
	while (strstr(lines[++i], hostname) != NULL)
	{
		char *p1 = lines[i];
		p1 = strstr(p1, "\t");
		if (p1 == NULL || strstr(p1, "service") == NULL)
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

		if (strlen(p1)>0)
			printf("%s\n", p1);
	}
	if (dc)
	{
		//printf("\n");
	}

	for (i = 0; i < 50; i++)
	{
		free(lines[i]);
	}

	pclose(fp);
}
