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
#include <string.h>
#include <syslog.h>
#include <time.h>
#include "memmgmt.h"
#include "logger.h"
#include "misc.h"
#include "load_settings.h"

void log_file(log_levels loglevel, char *message)
{
	static FILE *fp = NULL;
	time_t seconds;
	struct tm curtime;
	char date[16];
	static char *buf = NULL;
	char *tmp;

	if (!fp)
	{
		fp = fopen(LOG_FILE_NAME, "a");
		if (!fp)
		{
			fprintf(stderr, "Could not open log file '%s'...\n", LOG_FILE_NAME);
			return;
		}
	}

  /* write logs one line at a time */
  StrApp(&buf, message, (char*)NULL);
	
	if (strchr(buf, '\n'))
	{
		tmp = strtok(buf, "\n");
		while(tmp)
		{
			time(&seconds);
			localtime_r(&seconds, &curtime);
			strftime(date, sizeof(date), "%b %d %H:%M:%S", &curtime);

			fprintf(fp, "%s, qingy on tty%d, [%s] %s\n", date, current_tty, LOGLEVEL(loglevel), tmp);
			fflush(fp);

			tmp = strtok(NULL, "\n");
		}

		free(buf); buf = NULL;
	}

}

void log_syslog(log_levels loglevel, char *message)
{
	static char *buf  = NULL;
	static char msg[16];
	static int gotmsg = 0;
	char *tmp;
	int myloglevel = (loglevel == ERROR) ? LOG_ERR : LOG_DEBUG;

	if (!gotmsg)
	{
		snprintf(msg, sizeof(msg), "qingy(tty%d)", current_tty);
		gotmsg = 1;
	}
	openlog(msg, LOG_PID, LOG_USER);

  /* write logs one line at a time */
  StrApp(&buf, message, (char*)NULL);
	
	if (strchr(buf, '\n'))
	{
		tmp = strtok(buf, "\n");
		while(tmp)
		{
			syslog(myloglevel, "%s\n", tmp);
			tmp = strtok(NULL, "\n");
		}

		free(buf); buf = NULL;
	}

	closelog();
}

void writelog(log_levels loglevel, char *message)
{
	if (!message) return;
	if (loglevel > max_loglevel) return;

	if (log_facilities & LOG_TO_CONSOLE)
		fprintf(stderr, "%s", message);

	if (log_facilities & LOG_TO_FILE)
		log_file(loglevel, message);

	if (log_facilities & LOG_TO_SYSLOG)
		log_syslog(loglevel, message);
}
