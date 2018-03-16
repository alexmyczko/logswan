/*****************************************************************************/
/*                                                                           */
/* Logswan 1.07                                                              */
/* Copyright (c) 2015-2018, Frederic Cambus                                  */
/* https://www.logswan.org                                                   */
/*                                                                           */
/* Created:      2015-05-31                                                  */
/* Last Updated: 2018-03-16                                                  */
/*                                                                           */
/* Logswan is released under the BSD 2-Clause license.                       */
/* See LICENSE file for details.                                             */
/*                                                                           */
/*****************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "Logswan 1.07"

enum {
	HLL_BITS = 20,
	LINE_LENGTH_MAX = 65536,
	STATUS_CODE_MAX = 512,

	COUNTRIES = 512,
	CONTINENTS = 7,
	METHODS = 9,
	PROTOCOLS = 2
};

extern char *continentsId[];
extern char *continentsNames[];
extern char *methodsNames[];
extern char *protocolsNames[];

#endif /* CONFIG_H */
