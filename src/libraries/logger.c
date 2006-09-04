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
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "memmgmt.h"
#include "logger.h"
#include "misc.h"
#include "load_settings.h"
#include "vt.h"

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

			fprintf(fp, "%s, %s on tty%d, [%s] %s\n", date, program_name, current_tty, LOGLEVEL(loglevel), tmp);
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
		snprintf(msg, sizeof(msg), "%s(tty%d)", program_name, current_tty);
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

void file_logger_thread(char *filename)
{
	FILE   *fp    = fopen(filename, "r");
	char   *buf   = NULL;
	size_t  len   = 0;

	if (!fp)
	{
		writelog(ERROR, "Unable to hook to main thread's stderr!\n");
		abort();
	}

	/* we remove it so that noone will tamper */
	unlink(filename);

	while (1)
	{
		fflush(NULL);

		while (getline(&buf, &len, fp) != -1)
			writelog(DEBUG, buf);

		sleep(1);
	}
}

void log_stderr(void)
{
	pthread_t  log_thread;
	char      *filename   = StrApp((char**)NULL, tmp_files_dir, "/qingyXXXXXX", (char*)NULL);
	int        fd;

	fd = mkstemp(filename);
	if (fd == -1)
	{
		writelog(ERROR, "Could not create temporary file!\n");
		abort();
	}

	/* set file mode to 600 */
	if (chmod(filename, S_IRUSR|S_IWUSR))
	{
		writelog(ERROR, "Cannot chmod() file!\n");
		abort();		
	}

	if (!freopen(filename, "w", stderr))
	{
		writelog(ERROR, "Unable to redirect stderr!\n");
		abort();
	}

	/* close the file descriptor as we have no need for it */
	close(fd);
	
	/* spawn our stderr logger thread */
	if (pthread_create(&log_thread, NULL, (void*)(&file_logger_thread), filename))
	{
		writelog(ERROR, "Failed to create stderr log writer thread!\n");
		abort();
	}
}

void dontlog_stderr()
{
	stderr_disable();
	stderr_enable(&current_tty);
}
