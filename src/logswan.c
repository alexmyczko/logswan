/*****************************************************************************/
/*                                                                           */
/* Logswan 2.0.3                                                             */
/* Copyright (c) 2015-2018, Frederic Cambus                                  */
/* https://www.logswan.org                                                   */
/*                                                                           */
/* Created:      2015-05-31                                                  */
/* Last Updated: 2018-10-15                                                  */
/*                                                                           */
/* Logswan is released under the BSD 2-Clause license.                       */
/* See LICENSE file for details.                                             */
/*                                                                           */
/*****************************************************************************/

#define _XOPEN_SOURCE 600
#define _POSIX_C_SOURCE 199309L
#define _POSIX_SOURCE

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <err.h>
#include <getopt.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <maxminddb.h>

#include "compat.h"
#include "config.h"
#include "continents.h"
#include "countries.h"
#include "hll.h"
#include "output.h"
#include "parse.h"

bool geoip;
MMDB_s geoip2;

struct timespec begin, end, elapsed;

char lineBuffer[LINE_LENGTH_MAX];

struct results results;
struct date parsedDate;
struct logLine parsedLine;
struct request parsedRequest;

struct sockaddr_in ipv4;
struct sockaddr_in6 ipv6;
bool isIPv4, isIPv6;

uint64_t bandwidth;
uint32_t statusCode;
uint32_t hour;
uint32_t countryId;

FILE *logFile;
struct stat logFileStat;

const char *errstr;

int8_t getoptFlag;

struct HLL uniqueIPv4, uniqueIPv6;
char *intputFile;

void
displayUsage() {
	printf("USAGE: logswan [options] inputfile\n\n" \
	    "Options are:\n\n" \
	    "	-g Enable GeoIP lookups\n" \
	    "	-h Display usage\n" \
	    "	-v Display version\n");
}

int
main(int argc, char *argv[]) {
	int gai_error, mmdb_error;
	MMDB_lookup_result_s lookup;

	if (pledge("stdio rpath", NULL) == -1) {
		err(1, "pledge");
	}

	hll_init(&uniqueIPv4, HLL_BITS);
	hll_init(&uniqueIPv6, HLL_BITS);

	while ((getoptFlag = getopt(argc, argv, "ghv")) != -1) {
		switch (getoptFlag) {
		case 'g':
			geoip = true;
			break;

		case 'h':
			displayUsage();
			return 0;

		case 'v':
			printf("%s\n", VERSION);
			return 0;
		}
	}

	if (optind < argc) {
		intputFile = argv[optind];
	} else {
		displayUsage();
		return 0;
	}

	argc -= optind;
	argv += optind;

	/* Starting timer */
	clock_gettime(CLOCK_MONOTONIC, &begin);

	/* Initializing GeoIP */
	if (geoip) {
		if (MMDB_open(GEOIP2DIR "GeoLite2-Country.mmdb",
		    MMDB_MODE_MMAP, &geoip2) != MMDB_SUCCESS) {
			perror("Can't open database");
			return 1;
		}

	}

	/* Open log file */
	if (!strcmp(intputFile, "-")) {
		/* Read from standard input */
		logFile = stdin;
	} else {
		/* Attempt to read from file */
		if (!(logFile = fopen(intputFile, "r"))) {
			perror("Can't open log file");
			return 1;
		}
	}

	/* Get log file size */
	if (fstat(fileno(logFile), &logFileStat)) {
		perror("Can't stat log file");
		return 1;
	}

	results.fileName = intputFile;
	results.fileSize = logFileStat.st_size;

	while (fgets(lineBuffer, LINE_LENGTH_MAX, logFile)) {
		/* Parse and tokenize line */
		parseLine(&parsedLine, lineBuffer);

		/* Detect if remote host is IPv4 or IPv6 */
		if (parsedLine.remoteHost) { /* Do not feed NULL tokens to inet_pton */
			if ((isIPv4 = inet_pton(AF_INET, parsedLine.remoteHost, &(ipv4.sin_addr)))) {
				isIPv6 = false;
			} else {
				isIPv6 = inet_pton(AF_INET6, parsedLine.remoteHost, &(ipv6.sin6_addr));

				if (!isIPv6) {
					results.invalidLines++;
					continue;
				}
			}
		} else {
			/* Invalid line */
			results.invalidLines++;
			continue;
		}

		if (isIPv4) {
			/* Increment hits counter */
			results.hitsIPv4++;

			/* Unique visitors */
			hll_add(&uniqueIPv4, parsedLine.remoteHost, strlen(parsedLine.remoteHost));
		}

		if (isIPv6) {
			/* Increment hits counter */
			results.hitsIPv6++;

			/* Unique visitors */
			hll_add(&uniqueIPv6, parsedLine.remoteHost, strlen(parsedLine.remoteHost));
		}

		if (geoip) {
			MMDB_entry_data_s entry_data;

			lookup = MMDB_lookup_string(&geoip2, parsedLine.remoteHost, &gai_error, &mmdb_error);

			MMDB_get_value(&lookup.entry, &entry_data, "country", "iso_code", NULL);

			if (entry_data.has_data) {
				/* Increment countries array */
				for (size_t loop = 0; loop < COUNTRIES; loop++) {
					if (!strncmp(countriesId[loop], entry_data.utf8_string, 2)) {
						results.countries[loop]++;
						break;
					}
				}
			}

			MMDB_get_value(&lookup.entry, &entry_data, "continent", "code", NULL);

			if (entry_data.has_data) {
				/* Increment continents array */
				for (size_t loop = 0; loop < CONTINENTS; loop++) {
					if (!strncmp(continentsId[loop], entry_data.utf8_string, 2)) {
						results.continents[loop]++;
						break;
					}
				}
			}
		}

		/* Hourly distribution */
		if (parsedLine.date) {
			parseDate(&parsedDate, parsedLine.date);

			if (parsedDate.hour) {
				hour = strtonum(parsedDate.hour, 0, 23, &errstr);

				if (!errstr) {
					results.hours[hour]++;
				}
			}
		}

		/* Parse request */
		if (parsedLine.request) {
			parseRequest(&parsedRequest, parsedLine.request);

			if (parsedRequest.method) {
				for (size_t loop = 0; loop < METHODS; loop++) {
					if (!strcmp(methodsNames[loop], parsedRequest.method)) {
						results.methods[loop]++;
						break;
					}
				}
			}

			if (parsedRequest.protocol) {
				for (size_t loop = 0; loop < PROTOCOLS; loop++) {
					if (!strcmp(protocolsNames[loop], parsedRequest.protocol)) {
						results.protocols[loop]++;
						break;
					}
				}
			}
		}

		/* Count HTTP status codes occurences */
		if (parsedLine.statusCode) {
			statusCode = strtonum(parsedLine.statusCode, 0, STATUS_CODE_MAX-1, &errstr);

			if (!errstr) {
				results.status[statusCode]++;
			}
		}

		/* Increment bandwidth usage */
		if (parsedLine.objectSize) {
			bandwidth = strtonum(parsedLine.objectSize, 0, INT64_MAX, &errstr);

			if (!errstr) {
				results.bandwidth += bandwidth;
			}
		}
	}

	/* Counting hits and processed lines */
	results.hits = results.hitsIPv4 + results.hitsIPv6;
	results.processedLines = results.hits + results.invalidLines;

	/* Counting unique visitors */
	results.visitsIPv4 = hll_count(&uniqueIPv4);
	results.visitsIPv6 = hll_count(&uniqueIPv6);
	results.visits = results.visitsIPv4 + results.visitsIPv6;

	/* Stopping timer */
	clock_gettime(CLOCK_MONOTONIC, &end);

	timespecsub(&end, &begin, &elapsed);
	results.runtime = elapsed.tv_sec + elapsed.tv_nsec / 1E9;

	/* Generate timestamp */
	time_t now = time(NULL);
	strftime(results.timeStamp, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

	/* Printing results */
	fprintf(stderr, "Processed %" PRIu64 " lines in %f seconds\n", results.processedLines, results.runtime);
	fputs(output(&results), stdout);

	/* Clean up */
	fclose(logFile);

	MMDB_close(&geoip2);

	hll_destroy(&uniqueIPv4);
	hll_destroy(&uniqueIPv6);

	return 0;
}
