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

#ifndef PARSE_H
#define PARSE_H

struct date {
	char *day;
	char *month;
	char *year;
	char *hour;
	char *minute;
	char *second;
};

struct logLine {
	char *remoteHost;
	char *date;
	char *request;
	char *statusCode;
	char *objectSize;
};

struct request {
	char *method;
	char *resource;
	char *protocol;
};

void parseDate(struct date *, char *);
void parseLine(struct logLine *, char *);
void parseRequest(struct request *, char *);

#endif /* PARSE_H */
