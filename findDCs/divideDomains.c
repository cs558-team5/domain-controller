#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
	// make sure there are inputted values for the needed variables
	if (argc != 3)
	{
		printf("usage: ./divideDomains numOfDomains numOfFiles\n");
		return 0;
	}

	// validate the inputted values
	int numOfDomains = atoi(argv[1]);
	int numOfFiles = atoi(argv[2]);
	if (numOfFiles < 1 || numOfDomains < numOfFiles)
	{
		printf("numOfDomains and numOfFiles must be greater than 0; numOfDomains cannot be less than numOfFiles\n");
		return 0;
	}

	int i, j;
	char *domains[numOfDomains];
	// malloc space for the domains
	for (i = 0; i < numOfDomains; i++)
	{
		domains[i] = malloc(200);
	}

	FILE *fp = fopen("domains.txt", "r");

	// malloc space for the domains
	for (i = 0; i < numOfDomains; i++)
	{
		domains[i] = malloc(200);
	}

	// read in the domains
	i = 0;
	while (fgets(domains[i++], 199, fp) != NULL && i < numOfDomains);

	// close the file
	fclose(fp);

	for (i = 0; i < numOfFiles; i++)
	{
		int start = i * (numOfDomains / numOfFiles);
		int end = start + (numOfDomains / numOfFiles);
		if (i == (numOfFiles - 1))
		{
			end += (numOfDomains % numOfFiles);	
		}

		char filename[100];
		sprintf(filename, "domains/domains%d.txt", i);
		fp = fopen(filename, "ab+");
		fclose(fp);

		fp = fopen(filename, "w");
		for (j = start; j < end; j++)
		{
			fprintf(fp, "%s", domains[j]);
		}
		fclose(fp);

	}

	return 0;
}