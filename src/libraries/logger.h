/***************************************************************************
                          logger.h  -  description
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
 
#ifndef LOGGER_H
#define LOGGER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define LOG_FILE_NAME "/var/log/qingy.log"

#define LOG_NONE       000
#define LOG_TO_FILE    001
#define LOG_TO_SYSLOG  010
#define LOG_TO_CONSOLE 100
int log_facilities;
int log_facilities_tty;

typedef enum
{
	ERROR=0,
	DEBUG
} log_levels;
log_levels max_loglevel;

#define LOGLEVEL(level) (level == ERROR) ? "ERROR" : "DEBUG"

#define WRITELOG(loglevel, args...)	\
{                                   \
  char buf[512];                    \
  snprintf(buf, 512, args);         \
  writelog(loglevel, buf);          \
}

void writelog(log_levels loglevel, char *message);

void log_stderr();
void dontlog_stderr();

#endif
