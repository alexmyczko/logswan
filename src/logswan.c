/*****************************************************************************/
/*                                                                           */
/* Logswan (c) by Frederic Cambus 2015                                       */
/* https://github.com/fcambus/logswan                                        */
/*                                                                           */
/* Created:      2015/05/31                                                  */
/* Last Updated: 2015/06/07                                                  */
/*                                                                           */
/* Logswan is released under the BSD 3-Clause license.                       */
/* See LICENSE file for details.                                             */
/*                                                                           */
/*****************************************************************************/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LINE_MAX_LENGTH 4096

clock_t begin, end;
double runtime;

char lineBuffer[LINE_MAX_LENGTH];

uint64_t invalidLines = 0;
uint64_t objectSize = 0;
uint64_t bandwidth = 0;
uint64_t hits = 0;

struct sockaddr_in ipv4;
struct sockaddr_in6 ipv6;

int isIPv4, isIPv6;

struct stat logFileSize;
FILE *logFile;

char *endptr;

int main (int argc, char *argv[]) {
	printf("-------------------------------------------------------------------------------\n" \
	       "                      Logswan (c) by Frederic Cambus 2015                      \n" \
	       "-------------------------------------------------------------------------------\n\n");

	if (argc != 2) {
		printf("ERROR : No input file specified.\n");
		return EXIT_FAILURE;
	}

	/* Starting timer */
	begin = clock();

	/* Get log file size */
	stat(argv[1], &logFileSize);

	printf("Processing file : %s\n\n", argv[1]);

	logFile = fopen(argv[1], "r");
	if(!logFile) {
		perror("Can't open log file");
		return 1;
	}

	while (fgets(lineBuffer, LINE_MAX_LENGTH, logFile) != NULL) {
		/* Tokenize line */

		/* Remote host */
		char* token = strtok(lineBuffer, " ");

		if (token) { /* Do not feed NULL tokens to inet_pton */
			isIPv4 = inet_pton(AF_INET, token, &(ipv4.sin_addr));
			isIPv6 = inet_pton(AF_INET6, token, &(ipv6.sin6_addr));
		}

		if (isIPv4 || isIPv6) {
			/* User-identifier */
			token = strtok(NULL, " ");

			/* User ID */
			token = strtok(NULL, " ");

			/* Date */
			token = strtok(NULL, " [");

			/* UTC time offset */
			token = strtok(NULL, " ]");

			/* Method */
			token = strtok(NULL, " \"");

			/* Requested resource */
			token = strtok(NULL, " ");

			/* Protocol */
			token = strtok(NULL, " \"");

			/* HTTP status codes */
			token = strtok(NULL, " ");

			/* Returned object size */
			token = strtok(NULL, " ");

			/* Increment bandwidth usage */
			if (token) { /* Do not feed NULL tokens to strtol */
				objectSize = strtol(token, &endptr, 10);
				bandwidth += objectSize;
			}

			/* Increment hits counter */
			hits++;
		} else {
			/* Invalid line */

			invalidLines++;
		}
	}

	/* Stopping timer */
	end = clock();
	runtime = (double)(end - begin) / CLOCKS_PER_SEC;

	/* Printing results */
	printf("Hits : %llu\n", hits);
	printf("Invalid lines : %llu\n", invalidLines);
	printf("Bandwidth : %llu\n", bandwidth);
	printf("Log file size : %llu\n", logFileSize.st_size);
	printf("Runtime : %f\n", runtime);

	fclose(logFile);

	return EXIT_SUCCESS;
}