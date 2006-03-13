/***************************************************************************
                      crypto-libgcrypt.c  -  description
                            --------------------
    begin                : Nov 23 2005
    copyright            : (C) 2005 by Noberasco Michele
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
#include <time.h>
#include <unistd.h>
#include <gcrypt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "memmgmt.h"
#include "misc.h"
#include "qingy_constants.h"
#include "load_settings.h"


#define CHECK1(x)                                                                         \
  error = x;                                                                              \
	if (error)                                                                              \
	{                                                                                       \
		fprintf (stderr, "Failure: %s/%s\n", gcry_strsource (error), gcry_strerror (error));  \
		sleep(2);                                                                             \
		exit(EXIT_FAILURE);													                                          \
	}

#define CHECK2(x)                                                                 \
	x;                                                                              \
	if (!temp)                                                                      \
	{                                                                               \
		fprintf(stderr, "qingy: failure: something is wrong with your libgcrypt!\n"); \
		sleep(2);                                                                     \
		exit(EXIT_FAILURE);                                                           \
	}

#define CHECK3(x)                                                                         \
  error = x;                                                                              \
	if (error)                                                                              \
	{                                                                                       \
		if (!exit_on_failure) return 0;                                                       \
		fprintf (stderr, "Failure: %s/%s\n", gcry_strsource (error), gcry_strerror (error));  \
		sleep(2);                                                                             \
		exit(EXIT_FAILURE);													                                          \
	}

#define CHECK4(x)         \
	temp = x;               \
	if (!temp) return 0;    \
	buflen = temp-filebuf;	



static gcry_sexp_t *public_key       = NULL;
static gcry_sexp_t *private_key      = NULL;
static char        *public_key_file  = NULL;
static char        *private_key_file = NULL;


void set_key_files(char *path)
{
	if (!path) path = datadir;

	public_key_file  = StrApp((char**)NULL, path, "public_key",  (char*)NULL);
	private_key_file = StrApp((char**)NULL, path, "private_key", (char*)NULL);
}

static void dump_data(FILE *fp, char *tagname, char *buf, size_t len)
{
	size_t written;

	if (!tagname) return;

	/* fp is assumed to be a FILE pointer, pointing to an open (for writing) file */

	fprintf(fp, "<%s>", tagname);
	written = fwrite(buf, sizeof(char), len, fp);
	if (written != len)
	{
		fprintf(stderr, "qingy: error writing public key to file\n");
		sleep(2);
		exit(EXIT_FAILURE);
	}
	fprintf(fp, "</%s>\n", tagname);
}

static char *find_token(char *haystack, char *needle, size_t haystack_size)
{
	int     i=0;
	size_t  needle_length;
	int     my_limit;
	char   *my_haystack;

	if (!haystack)      return NULL;
	if (!needle)        return NULL;
	if (!haystack_size) return NULL;

	needle_length = strlen(needle);
	my_limit      = haystack_size-needle_length;
	my_haystack   = haystack;

	for (; i<my_limit; i++)
	{
		if (!strncmp(my_haystack, needle, needle_length))
			return my_haystack;

		my_haystack++;
	}

	return NULL;
}

static char *find_item(FILE *fp, char *begin_tag, char *end_tag, size_t *token_length)
{
	char    buf[255];
	size_t  len1;
	size_t  len2;
	size_t  len  = 0;
	char   *pos1 = NULL;
	char   *pos2 = NULL;
	char   *retval;
	int     c;

	if (!fp)        return NULL;
	if (!begin_tag) return NULL;
	if (!end_tag)   return NULL;

	len1 = strlen(begin_tag);
	len2 = strlen(end_tag);

	while ( (c=fgetc(fp)) != EOF )
	{
		buf[len++] = (char)c;
		
		if (!pos2 && len >= len2)
		{
			char *temp = buf + len - len2;
			if (!strncmp(temp, end_tag, len2))
			{
				pos2 = temp;
				break;
			}
		}

		if (!pos1 && len >= len1)
		{
			char *temp = buf + len - len1;
			if (!strncmp(temp, begin_tag, len1))
			{
				pos1 = temp;
			}
		}		
		
	}

	if (!pos1 || !pos2)
		return NULL;

	pos1 += len1;
	len   = pos2-pos1;

	*token_length = len;
	retval = (char*)calloc(len,sizeof(char));

	memcpy(retval, pos1, len);

	return retval;
}

void encrypt_item(FILE *fp, char *item)
{
	gcry_error_t error;
	gcry_sexp_t *temp;
	gcry_sexp_t *encrypted;
	char        *buf;
	size_t       len;

	if (!fp)         return;
	if (!item)       return;
	if (!public_key) return;

	buf       = StrApp((char**)NULL,"(data(flags raw)(value \"", item, "\"))", (char*)NULL);
	len       = strlen(buf);
	temp      = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	encrypted = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));

	CHECK1(gcry_sexp_new(temp, buf, len, 1));
	CHECK1(gcry_pk_encrypt(encrypted, *temp, *public_key));

	gcry_sexp_release( *temp );
	free(buf);

	/* save encrypted data to file */

	/* first get a pointer to the raw encrypted data... */
	*temp = gcry_sexp_find_token (*encrypted, "a", sizeof(char));
	buf   = (char *)gcry_sexp_nth_data (*temp, 1, &len);

	/* ...then dump it to file */
	dump_data(fp, "item", buf, len);

	/* clean up */
	gcry_sexp_release( *encrypted );
	gcry_sexp_release( *temp      );
	free(encrypted);
	free(temp);
}

char *decrypt_item(FILE *fp)
{
	gcry_error_t   error;
	gcry_sexp_t   *encrypted;
	gcry_sexp_t   *decrypted;
	size_t         erroffset;
	char          *item = NULL;
	char          *temp;
	size_t         len_item;
	size_t         len_decitem;

	if (!private_key) return NULL;
	if (!fp)          return NULL;

	item = find_item(fp, "<item>", "</item>", &len_item);

	if (!item)
		return NULL;

	encrypted = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	decrypted = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));

	/* build an expression based on the raw encrypted item */
	CHECK1(gcry_sexp_build (encrypted, &erroffset, "(enc-val(rsa(a %b)))", len_item, item));
	free(item);

	/* decrypt the item */
	CHECK1(gcry_pk_decrypt (decrypted, *encrypted, *private_key));

	/* get a pointer to the raw decrypted data */
	temp = (char*)gcry_sexp_nth_data (*decrypted, 0, &len_decitem);

	/* almost there ;-) */
	item = (char*)calloc(len_decitem+1, sizeof(char));
	memcpy(item, temp, len_decitem);

	gcry_sexp_release( *encrypted );
	gcry_sexp_release( *decrypted );
	free(encrypted);
	free(decrypted);

	return item;
}

void save_public_key(FILE *fp)
{
	gcry_sexp_t *n;
	gcry_sexp_t *e;
	char        *temp;
	size_t       len;

	if (!fp)         return;
	if (!public_key) return;

	n = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	e = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));

	/* get public key modulus */
	*n = gcry_sexp_find_token (*public_key, "n", sizeof(char));

	/* get public key exponent */
	*e = gcry_sexp_find_token (*public_key, "e", sizeof(char));

	/* save public key modulus to file */
	CHECK2(temp = (char *)gcry_sexp_nth_data (*n, 1, &len));
	dump_data(fp, "modulus", temp, len);

	/* save public key exponent to file */
	CHECK2(temp = (char *)gcry_sexp_nth_data (*e, 1, &len));
	dump_data(fp, "exponent", temp, len);

	/* clean up */
	gcry_sexp_release( *n );
	gcry_sexp_release( *e );
	free(n);
	free(e);
}

static void save_private_key(FILE *fp)
{
	gcry_sexp_t *n;
	gcry_sexp_t *e;
	gcry_sexp_t *d;
	gcry_sexp_t *p;
	gcry_sexp_t *q;
	gcry_sexp_t *u;
	char        *temp;
	size_t       len;

	if (!fp)      return;
	if (!private_key) return;

	n = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	e = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	d = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	p = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	q = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	u = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));

	/* get the various private key components... */
	*n = gcry_sexp_find_token (*private_key, "n", sizeof(char));
	*e = gcry_sexp_find_token (*private_key, "e", sizeof(char));
	*d = gcry_sexp_find_token (*private_key, "d", sizeof(char));
	*p = gcry_sexp_find_token (*private_key, "p", sizeof(char));
	*q = gcry_sexp_find_token (*private_key, "q", sizeof(char));
	*u = gcry_sexp_find_token (*private_key, "u", sizeof(char));

	/* ...and save them to file */
	CHECK2(temp = (char *)gcry_sexp_nth_data (*n, 1, &len));
	dump_data(fp, "nn", temp, len);
	CHECK2(temp = (char *)gcry_sexp_nth_data (*e, 1, &len));
	dump_data(fp, "ee", temp, len);
	CHECK2(temp = (char *)gcry_sexp_nth_data (*d, 1, &len));
	dump_data(fp, "dd", temp, len);
	CHECK2(temp = (char *)gcry_sexp_nth_data (*p, 1, &len));
	dump_data(fp, "pp", temp, len);
	CHECK2(temp = (char *)gcry_sexp_nth_data (*q, 1, &len));
	dump_data(fp, "qq", temp, len);
	CHECK2(temp = (char *)gcry_sexp_nth_data (*u, 1, &len));
	dump_data(fp, "uu", temp, len);

	/* clean up */
	gcry_sexp_release( *n );
	gcry_sexp_release( *e );
	gcry_sexp_release( *d );
	gcry_sexp_release( *p );
	gcry_sexp_release( *q );
	gcry_sexp_release( *u );
	free(n);
	free(e);
	free(d);
	free(p);
	free(q);
	free(u);
}

static char *get_mpi_from_buf(gcry_mpi_t *mpi, char *tag, char *buf, size_t buflen, int exit_on_failure)
{
	gcry_error_t  error;
	char         *begin_tag = StrApp((char**)NULL, "<",  tag, ">", (char*)NULL);
	char         *end_tag   = StrApp((char**)NULL, "</", tag, ">", (char*)NULL);
	size_t        taglen    = strlen(begin_tag);
	char         *temp1     = find_token(buf,   begin_tag, buflen);
	char         *temp2     = find_token(temp1, end_tag,   buflen);
	size_t        nscanned;
	char         *temp;
	size_t        len;

	free(begin_tag);
	free(end_tag);

	if (!temp1 || !temp2) return NULL;

	len  = temp2-temp1-taglen;
	temp = (char *)calloc(len, sizeof(char));
	memcpy(temp, temp1+taglen, len);
	CHECK3(gcry_mpi_scan(mpi, GCRYMPI_FMT_STD, temp, len, &nscanned));
	free(temp);

	return temp2;
}

static int int_restore_public_key(FILE *fp, int exit_on_failure)
{
	gcry_error_t  error;
	char         *temp;
	gcry_mpi_t   *modulus_mpi  = (gcry_mpi_t *)calloc(1, sizeof(gcry_mpi_t));
	gcry_mpi_t   *exponent_mpi = (gcry_mpi_t *)calloc(1, sizeof(gcry_mpi_t));
	char          filebuf[255];
	size_t        buflen;
	size_t        erroff;

	if (!fp) return 0;

	if (public_key) free(public_key);
	public_key = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));

	buflen=fread(filebuf, sizeof(char), 255, fp);
	if (!buflen)
	{
		if (!exit_on_failure) return 0;
		fprintf(stderr, "qingy: failure: could not retrieve public key\n");
		sleep(3);
		exit(EXIT_FAILURE);
	}

	CHECK4(get_mpi_from_buf(modulus_mpi,  "modulus",  filebuf, buflen, exit_on_failure));
	CHECK4(get_mpi_from_buf(exponent_mpi, "exponent", temp,    buflen, exit_on_failure));

	CHECK3(gcry_sexp_build (public_key, &erroff, "(public-key(rsa(n %m)(e %m)))", *modulus_mpi, *exponent_mpi));

	gcry_mpi_release (*modulus_mpi);
	gcry_mpi_release (*exponent_mpi);

	free(modulus_mpi);
	free(exponent_mpi);

	return 1;
}

static int restore_private_key(FILE *fp)
{
	gcry_error_t  error;
	size_t        erroff;
	size_t        buflen;
	char          filebuf[1023];
	char         *temp;
	gcry_mpi_t   *n_mpi = (gcry_mpi_t *)calloc(1, sizeof(gcry_mpi_t));
	gcry_mpi_t   *e_mpi = (gcry_mpi_t *)calloc(1, sizeof(gcry_mpi_t));
	gcry_mpi_t   *d_mpi = (gcry_mpi_t *)calloc(1, sizeof(gcry_mpi_t));
	gcry_mpi_t   *p_mpi = (gcry_mpi_t *)calloc(1, sizeof(gcry_mpi_t));
	gcry_mpi_t   *q_mpi = (gcry_mpi_t *)calloc(1, sizeof(gcry_mpi_t));
	gcry_mpi_t   *u_mpi = (gcry_mpi_t *)calloc(1, sizeof(gcry_mpi_t));
	int           exit_on_failure = 0;

	if (!fp) return 0;

	if (private_key) free(private_key);
	private_key = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));

	buflen=fread(filebuf, sizeof(char), 1023, fp);
	if (!buflen) return 0;

	CHECK4(get_mpi_from_buf(n_mpi, "nn", filebuf, buflen, exit_on_failure));
	CHECK4(get_mpi_from_buf(e_mpi, "ee", temp,    buflen, exit_on_failure));
	CHECK4(get_mpi_from_buf(d_mpi, "dd", temp,    buflen, exit_on_failure));
	CHECK4(get_mpi_from_buf(p_mpi, "pp", temp,    buflen, exit_on_failure));
	CHECK4(get_mpi_from_buf(q_mpi, "qq", temp,    buflen, exit_on_failure));
	CHECK4(get_mpi_from_buf(u_mpi, "uu", temp,    buflen, exit_on_failure));

	CHECK3(gcry_sexp_build (private_key, &erroff, "(private-key(rsa(n %m)(e %m)(d %m)(p %m)(q %m)(u %m)))", *n_mpi, *e_mpi, *d_mpi, *p_mpi, *q_mpi, *u_mpi));

	gcry_mpi_release (*n_mpi);
	gcry_mpi_release (*e_mpi);
	gcry_mpi_release (*d_mpi);
	gcry_mpi_release (*p_mpi);
	gcry_mpi_release (*q_mpi);
	gcry_mpi_release (*u_mpi);

	free(n_mpi);
	free(e_mpi);
	free(d_mpi);
	free(p_mpi);
	free(q_mpi);
	free(u_mpi);

	return 1;
}

void restore_public_key(FILE *fp)
{
	srand((unsigned int)time(NULL));
	gcry_check_version(NULL);
	gcry_control( GCRYCTL_INIT_SECMEM, 16384, 0 );

	int_restore_public_key(fp, 1);
}

static int restore_keys()
{
	FILE *fp = fopen(public_key_file, "r");

	/* restore public key... */
	if (!fp) return 0;
	if (!int_restore_public_key(fp, 0))
	{
		fclose(fp);
		return 0;
	}
	fclose(fp);

	/* restore private key... */
	fp = fopen(private_key_file, "r");
	if (!fp)
	{
		gcry_sexp_release( *public_key );
		free(public_key);
		return 0;
	}
	if (!restore_private_key(fp))
	{
		gcry_sexp_release( *public_key );
		free(public_key);
		fclose(fp);
		return 0;
	}
	fclose(fp);

	return 1;
}

int int_generate_keys(int try_to_restore, int fail_if_restore_fail)
{
	FILE         *fp;
	gcry_error_t  error;
	gcry_sexp_t  *parms    = calloc(1, sizeof(gcry_sexp_t));
	char         *buf      = "(genkey(rsa(nbits 4:1024)))";
	size_t        len      = strlen(buf);
	gcry_sexp_t  *keypair;

	/* try to load key pair from disc... */
	if (try_to_restore)
		if (restore_keys())
			return 1;

	if (fail_if_restore_fail)
		return 0;

	/* ...otherwise we generate a new key pair and save it */
	keypair = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));

	CHECK1(gcry_sexp_new(parms, buf, len, 1));
	error = 1;
	while (error)
	{
		error = gcry_pk_genkey(keypair, *parms);
		if (!error)
			error = gcry_pk_testkey (*keypair);
	}
	gcry_sexp_release( *parms );
	free(parms);

	/* divide keypair into public and private keys */
	public_key   = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));	
	*public_key  = gcry_sexp_find_token (*keypair, "public-key", strlen("public-key"));
	private_key  = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));	
	*private_key = gcry_sexp_find_token (*keypair, "private-key", strlen("private-key"));
	gcry_sexp_release( *keypair );
	free(keypair);

	/* save public key to file */
	fp = fopen(public_key_file, "w");
	if (!fp)
	{
		fprintf(stderr, "qingy: failure: could not open file %s to save public key!\n", public_key_file);
		sleep(2);
		exit(EXIT_FAILURE);
	}
	chmod(public_key_file, S_IRUSR|S_IWUSR); /* Fix file permissions */
	save_public_key(fp);
	fclose(fp);

	/* save private key to file */
	fp = fopen(private_key_file, "w");
	if (!fp)
	{
		fprintf(stderr, "qingy: failure: could not open file %s to save private key!\n", private_key_file);
		sleep(2);
		exit(EXIT_FAILURE);
	}
	chmod(private_key_file, S_IRUSR|S_IWUSR); /* Fix file permissions */
	save_private_key(fp);
	fclose(fp);

	return 1;
}

int generate_keys()
{
	/* initialize stuff */
	srand((unsigned int)time(NULL));
	gcry_check_version(NULL);
	gcry_control( GCRYCTL_INIT_SECMEM, 16384, 0 );

	set_key_files(NULL);

	return int_generate_keys(1, 1);
}

void flush_keys()
{
	if (public_key)
	{
		gcry_sexp_release( *public_key );
		free (public_key);
		public_key = NULL;
	}
	if (private_key)
	{
		gcry_sexp_release( *private_key );
		free (private_key);
		private_key = NULL;
	}
}
