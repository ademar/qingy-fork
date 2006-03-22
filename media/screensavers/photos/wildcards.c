/***************************************************************************
                        wildcards.c  -  description
                            --------------------
    begin                : Mar 21 2006
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
#include <libgen.h>

#include <misc.h>



#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
# include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
# include <sys/dir.h>
# endif
# if HAVE_NDIR_H
# include <ndir.h>
# endif
#endif



typedef struct _pathTree
{
	char *path;
	struct _pathTree *child;
	struct _pathTree *sibling;

} pathTree;


static pathTree *expand_wildcard(char *path, char *wildcard)
{
	pathTree *paths = NULL;
	char     *left;
	char     *dn;
	char     *left_fn_part;
	char     *right_fn_part;
	DIR      *dir;

	static int step = -1;

	step++;

	if (!path)   return NULL;
	if (!wildcard) return NULL;

	/* we get string part before (and after) our wildcard */
	left          = strsep(&path, wildcard);
	right_fn_part = path;
	if (!left) return NULL;

	/* we get dir and file part of the string before our wildcard */
	if (!strcmp("/", strrchr(left, '/')))
	{
		size_t lenleft = strlen(left);

		if (lenleft == 1)
			dn         = strdup("/");
		else
			dn         = strndup(left, strlen(left)-sizeof(char));

		left_fn_part = NULL;
	}
	else
	{
		char *temp   = strdup(left);

		dn           = strdup(dirname(temp));
		free(temp);
		temp         = strdup(left);
		left_fn_part = strdup(basename(temp));
		free(temp);

		/* have we got a valid dir? */
		if (!strcmp(dn, "."))
		{
			free(dn);
			free(left_fn_part);
			return NULL;
		}
	}

	/* let's navigate our dir */
	dir = opendir(dn);
	if (dir)
	{
		struct dirent *entry;
		pathTree      *my_paths;
		size_t         lenleft  = 0;
		size_t         lenright = 0;

		if (left_fn_part)  lenleft  = strlen(left_fn_part);
		if (right_fn_part) lenright = strlen(right_fn_part);

		/* we test each dir antry against our pattern */
		while ((entry = readdir(dir)))
		{
			char   *retval      = NULL;
			char   *search      = entry->d_name;
			char   *other_wc    = NULL;
			int     free_retval = 0;
			char   *dest_value;
			size_t  lenentry;

			if (!strcmp(entry->d_name, "." )) continue;
			if (!strcmp(entry->d_name, "..")) continue;

			lenentry = strlen(entry->d_name);

			if (lenentry < (lenleft+lenright+1))
				continue;

			if (lenleft)
				if (strncmp(entry->d_name, left_fn_part, lenleft))
					continue;

			search += (lenleft+1);

			if (right_fn_part)
				other_wc = strpbrk(right_fn_part, "*?");

			/* is there another wildcard in our entry? */
			if (!other_wc)
			{
				/* if not, we test the whole file name... */
				if (!strcmp(wildcard, "*") && lenright)
				{
					if (strcmp(search+strlen(search)-lenright, right_fn_part))
						continue;
				}

				if (!strcmp(wildcard, "?"))
				{
					if (lenright)
					{
						if (strcmp(right_fn_part, search))
							continue;
					}
					else
					{
						if (lenentry != lenleft+1)
							continue;
					}
				}

				retval = entry->d_name;
			}
			else
			{
				/* ...otherwise we test it only until the next wildcard is reached */
				char   *my_right_fn_part = strndup(right_fn_part, other_wc-right_fn_part);
				size_t  my_lenright      = strlen(my_right_fn_part);

				if (!strcmp(wildcard, "*") && my_lenright)
				{
					search = strstr(search, my_right_fn_part);
					if (!search)
					{
						free(my_right_fn_part);
						continue;
					}
				}

				if (!strcmp(wildcard, "?"))
				{
					
					if (my_lenright)
					{
						if (strcmp(my_right_fn_part, search))
						{
							free(my_right_fn_part);
							continue;
						}
					}					
					else
					{
						if (lenentry < lenleft+1+lenright)
						{
							free(my_right_fn_part);
							continue;
						}
					}
				}

				free(my_right_fn_part);
				my_right_fn_part = strdup(right_fn_part+my_lenright); /* using this as a temp value */
				retval = StrApp((char**)NULL,strndup(entry->d_name, (search-entry->d_name)+my_lenright),my_right_fn_part,(char*)NULL);
				free(my_right_fn_part);
				free_retval = 1;
			}

			/* if we get this far, we got a file name that matches our pattern */
			dest_value = StrApp((char**)NULL, dn, ((strlen(dn) == 1) ? "" : "/"), retval, (char*)NULL);
			if (free_retval)
				free(retval);

			/* we add it to our paths list... */
			if (!paths)
			{
				paths = (pathTree *) calloc(1, sizeof(pathTree));
				paths->path    = dest_value;
				paths->sibling = NULL;
				paths->child   = NULL;
			}
			else
			{
				int add = 1;

				/* ...making sure to avoid duplicates */
				for (my_paths=paths; my_paths->sibling; my_paths=my_paths->sibling)
					if (!strcmp(my_paths->path, dest_value))
					{
						add = 0;
						break;
					}
				if (!add) continue;

				my_paths->sibling = (pathTree *) calloc(1, sizeof(pathTree));
				my_paths = my_paths->sibling;

				my_paths->path    = dest_value;
				my_paths->sibling = NULL;
				my_paths->child   = NULL;
			}
		}

		closedir(dir);
	}

	/* cleanup before returning */
	free(dn);
	if (left_fn_part)
		free(left_fn_part);

	return paths;
}


static void expand_tree(pathTree *tree)
{
	pathTree *my_tree = tree;

	if (!tree) return;

	while(my_tree)
	{
		if (my_tree->path)
		{
			char *wildcard = strpbrk(my_tree->path, "*?");

			if (wildcard)
			{
				wildcard = strndup(wildcard, sizeof(char));
				my_tree->child = expand_wildcard(my_tree->path, wildcard);
				free(wildcard);

				free(my_tree->path);
				my_tree->path = NULL;
				expand_tree(my_tree->child);
			}
		}

		my_tree = my_tree->sibling;
	}
}

static void collapse_tree(pathTree *tree, char ***collapsed, int *size)
{
	pathTree *my_tree = tree;

	if (!tree)      return;
	if (!collapsed) return;
	if (!size)      return;

	while (my_tree)
	{
		if (!(my_tree->child))
		{
			int add=1;
			int i=0;

			/* only add new value if it is not present */
			if (*collapsed)
				for (; (*collapsed)[i]; i++)
					if (!strcmp((*collapsed)[i], my_tree->path))
					{
						free(my_tree->path);
						add=0;
						break;
					}

			if (add)
			{
				*collapsed = (char **) realloc(*collapsed, ((*size)+2)*sizeof(char *));
				(*collapsed)[(*size)++] = my_tree->path;
				(*collapsed)[*size]     = NULL;
			}
		}
		else
			collapse_tree(my_tree->child, collapsed, size);

		my_tree = my_tree->sibling;
	}
}

static char **do_expansion(char *path)
{
	char     ***collapsed;
	char      **result;
	pathTree   *tree;
	int         size = 0;

	if (!path) return NULL;
	
	collapsed  = (char ***) calloc(1, sizeof(char **));
	*collapsed = NULL;

	tree          = (pathTree *) calloc(1, sizeof(pathTree));
	tree->path    = strdup(path);
	tree->child   = NULL;
	tree->sibling = NULL;

	expand_tree(tree);
	collapse_tree(tree, collapsed, &size);
	result = *collapsed;
	free(collapsed);

	return result;
}

/* wildcard expansion for paths */
char *expand_path(char *path)
{
	static char  *old_path = NULL;
	static char **expanded = NULL;

	if (!path) return NULL;

	if (path == old_path)
	{
		static int i=1;
		if (expanded[i])
			return expanded[i++];

		i=0;
		while (expanded[i])
			free(expanded[i++]);
		free(expanded);

		return NULL;
	}

	if (expanded)
	{
		int i=0;
		while (expanded[i])
			free(expanded[i++]);
		free(expanded);
	}

	old_path = path;

	expanded = do_expansion(path);

	if (!expanded)
		return NULL;

	return expanded[0];
}
