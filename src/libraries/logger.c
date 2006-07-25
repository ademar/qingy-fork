/***************************************************************************
                          logger.c  -  description
                            --------------------
    begin                : Jun 09 2006
    copyright            : (C) 2006 by Noberasco Michele
    e-mail               : michele.noberasco@tiscali.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.              *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include "logger.h"

void log_file(log_levels loglevel, char *message)
{
	static FILE *fp = NULL;

	if (!fp)
	{
		fp = fopen(LOG_FILE_NAME, "a");
		if (!fp)
		{
			fprintf(stderr, "Could not open log file '%s'...\n", LOG_FILE_NAME);
			return;
		}
	}

	fprintf(fp, "%s\n", message);
}

void log_syslog(log_levels loglevel, char *message)
{
}

void writelog(log_levels loglevel, char *message)
{
	if (!message) return;
	if (loglevel > max_loglevel) return;

	if (log_facilities & LOG_CONSOLE)
		fprintf(stderr, "%s\n", message);

	if (log_facilities & LOG_FILE)
		log_file(loglevel, message);

	if (log_facilities & LOG_SYSLOG)
		log_syslog(loglevel, message);
}
