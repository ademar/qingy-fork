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

#include "memmgmt.h"
#include "misc.h"
#include "qingy_constants.h"

GCRY_THREAD_OPTION_PTHREAD_IMPL;


#define KEYS_FILE "/etc/qingy/keys"

#define CHECK(x)                                                                          \
  error = x;                                                                              \
	if (error)                                                                              \
	{                                                                                       \
		fprintf (stderr, "Failure: %s/%s\n", gcry_strsource (error), gcry_strerror (error));  \
		sleep(2);                                                                             \
		exit(EXIT_FAILURE);													                                          \
	}



static gcry_sexp_t *keypair = NULL;


static void dump_data(FILE *fp, char *tagname, char *buf, size_t len)
{
	size_t written;

	if (!tagname) return;
	/* fp as assumed to be a FILE pointer, pointing to an already opened file */

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

void encrypt_item(FILE *fp, char *item)
{
	gcry_error_t error;
	gcry_sexp_t *temp;
	gcry_sexp_t *encrypted;
	char        *buf;
	size_t       len;

	if (!fp)      return;
	if (!item)    return;
	if (!keypair) return;

	FILE *oldstderr = stderr;
	freopen("/qingylog.txt", "a", stderr);

fprintf(stderr, "encrypting: \"%s\"\n", item);

	buf       = StrApp((char**)NULL,"(data(flags raw)(value \"", item, "\"))", (char*)NULL);
	len       = strlen(buf);
	temp      = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	encrypted = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));

	CHECK(gcry_sexp_new(temp, buf, len, 1));
	CHECK(gcry_pk_encrypt(encrypted, *temp, *keypair));

gcry_sexp_dump ( *temp );
gcry_sexp_dump ( *encrypted );

	gcry_sexp_release( *temp );
	free(buf);

	/* save encrypted data to file */

	/* first get a pointer to the raw encrypted data... */
	*temp = gcry_sexp_find_token (*encrypted, "a", sizeof(char));
	buf   = (char *)gcry_sexp_nth_data (*temp, 1, &len);

	/* ...then dump it to file */
	dump_data(fp, "item", buf, len);

dump_data(stderr, "item", buf, len);

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
	static char    buf[32767];
	static size_t  buflen;
	static size_t  erroffset;
	static FILE   *my_fp = NULL;
	static char   *pos1  = NULL;
	static char   *pos2  = NULL;
	char          *item  = NULL;
	char          *temp;
	size_t         len_item;
	size_t         len_decitem;

	FILE *oldstderr = stderr;
	freopen("/qingylog.txt", "a", stderr);

	if (!keypair) return NULL;
	if (!fp)      return NULL;

	if (!my_fp)
	{
		my_fp = fp;
		strcpy(buf, "\0");
	}

	if (fp != my_fp) return NULL;

	/* now we are sure that all readings will come from the same source */

	/* get the raw encrypted item */
	if (!strlen(buf))
	{
		buflen = fread(buf, sizeof(char), 32767, fp);
		pos1   = buf;
	}

	if (!buflen) return NULL;

	pos1 = find_token(pos1, "<item>",  buflen);
	pos2 = find_token(pos1, "</item>", buflen);

	if (!pos1 || !pos2) return NULL;

	len_item = pos2 - pos1 - 6;
	item     = (char*)calloc(len_item, sizeof(char));
	memcpy(item, pos1+6, len_item);

	pos1   += len_item + 14;
	buflen -= len_item + 14;

	encrypted = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	decrypted = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));

	/* build an expression based on the raw encrypted item */
	CHECK(gcry_sexp_build (encrypted, &erroffset, "(enc-val(rsa(a %b)))", len_item, item));

gcry_sexp_dump ( *encrypted );

	free(item);

	/* decrypt the item */
	CHECK(gcry_pk_decrypt (decrypted, *encrypted, *keypair));

	/* get a pointer to the raw decrypted data */
	temp = (char*)gcry_sexp_nth_data (*decrypted, 0, &len_decitem);

	/* almost there ;-) */
	item = (char*)calloc(len_decitem+1, sizeof(char));
	memcpy(item, temp, len_decitem);

	gcry_sexp_release( *encrypted );
	gcry_sexp_release( *decrypted );
	free(encrypted);
	free(decrypted);

fprintf(stderr, "decrypted: \"%s\"\n", item);

	return item;
}

void generate_keys()
{
	gcry_error_t  error;
	gcry_sexp_t  *parms       = calloc(1, sizeof(gcry_sexp_t));
	char         *buf         = "(genkey(rsa(nbits 4:1024)))";
	size_t        len         = strlen(buf);

	FILE *oldstderr = stderr;
	freopen("/qingylog.txt", "a", stderr);

	/* initialize stuff */
	if (keypair) free(keypair);
	keypair = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	srand((unsigned int)time(NULL));

	gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
	gcry_check_version(NULL);
	gcry_control( GCRYCTL_INIT_SECMEM, 16384, 0 );

	/* try to load key pair from disc */
	//TO BE IMPLEMENTED

	/* generate new key pair */
	CHECK(gcry_sexp_new(parms, buf, len, 1));
	CHECK(gcry_pk_genkey(keypair, *parms));
	gcry_sexp_release( *parms );
	free(parms);

gcry_sexp_dump ( *keypair );

	/* save key pair to file */
	//TO BE IMPLEMENTED
}

void save_public_key(FILE *fp)
{
	gcry_sexp_t *public_key;
	gcry_sexp_t *n;
	gcry_sexp_t *e;
	char        *temp = "public-key";
	size_t       len;

	if (!fp)      return;
	if (!keypair) return;

	FILE *oldstderr = stderr;
	freopen("/qingylog.txt", "a", stderr);

	public_key  = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));	
	n           = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	e           = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));

	/* get public key from keypair */
	*public_key = gcry_sexp_find_token (*keypair, temp, strlen(temp));

	/* get public key modulus */
	*n = gcry_sexp_find_token (*public_key, "n", sizeof(char));

	/* get public key exponent */
	*e = gcry_sexp_find_token (*public_key, "e", sizeof(char));

	/* save public key modulus to file */
	temp = (char *)gcry_sexp_nth_data (*n, 1, &len);
	if (!temp)
	{
		fprintf(stderr, "qingy: failure: something is wrong with your libgcrypt!\n");
		sleep(2);
		exit(EXIT_FAILURE);
	}
	dump_data(fp, "modulus", temp, len);

	/* save public key exponent to file */
	temp = (char *)gcry_sexp_nth_data (*e, 1, &len);
	if (!temp)
	{
		fprintf(stderr, "qingy: failure: something is wrong with your libgcrypt!\n");
		sleep(2);
		exit(EXIT_FAILURE);
	}
	dump_data(fp, "exponent", temp, len);

	/* clean up */
	gcry_sexp_release( *public_key );
	gcry_sexp_release( *n );
	gcry_sexp_release( *e );
	free(public_key);
	free(n);
	free(e);
}

void restore_public_key(FILE *fp)
{
	gcry_error_t  error;
	char         *temp;
	char         *temp2;
	char         *modulus;
	char         *exponent;
	unsigned int  len_modulus;
	unsigned int  len_exponent;
	gcry_mpi_t   *modulus_mpi;
	gcry_mpi_t   *exponent_mpi;
	char          filebuf[255];
	size_t        buflen;
	size_t        nscanned;
	size_t        erroff;

	if (!fp) return;

	FILE *oldstderr = stderr;
	freopen("/qingylog.txt", "a", stderr);

	if (keypair) free(keypair);
	keypair = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));

	gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
	gcry_check_version(NULL);
	gcry_control( GCRYCTL_INIT_SECMEM, 16384, 0 );

	buflen=fread(filebuf, sizeof(char), 255, fp);
	if (!buflen)
	{
		fprintf(stderr, "qingy: failure: could not retrieve public key1\n");
		sleep(3);
		exit(EXIT_FAILURE);
	}

	if (strncmp(filebuf, "<modulus>", 9))
	{
		fprintf(stderr, "qingy: failure: could not retrieve public key2\n");
		sleep(3);
		exit(EXIT_FAILURE);
	}

	temp = find_token(filebuf, "</modulus>", buflen);
	if (!temp)
	{
		fprintf(stderr, "qingy: failure: could not retrieve public key3\n");
		sleep(3);
		exit(EXIT_FAILURE);
	}

	buflen      = temp - filebuf - 9;
	modulus     = (char*)calloc(buflen, sizeof(char));
	len_modulus = buflen;
	memcpy(modulus, filebuf+9, buflen);

	buflen = temp - filebuf;
	temp   = find_token(temp, "<exponent>",  buflen);
	temp2  = find_token(temp, "</exponent>", buflen);

	if (!temp || !temp2)
	{
		fprintf(stderr, "qingy: failure: could not retrieve public key4\n");
		sleep(2);
		exit(EXIT_FAILURE);
	}

	len_exponent = temp2 - temp - 10;
	exponent     = (char*)calloc(len_exponent, sizeof(char));
	modulus_mpi  = (gcry_mpi_t *)calloc(1, sizeof(gcry_mpi_t));
	exponent_mpi = (gcry_mpi_t *)calloc(1, sizeof(gcry_mpi_t));
	memcpy(exponent, temp+10, len_exponent);
	
	CHECK(gcry_mpi_scan   (modulus_mpi,  GCRYMPI_FMT_STD, modulus,  len_modulus,  &nscanned));
	CHECK(gcry_mpi_scan   (exponent_mpi, GCRYMPI_FMT_STD, exponent, len_exponent, &nscanned));
	CHECK(gcry_sexp_build (keypair, &erroff, "(public-key(rsa(n %m)(e %m)))", *modulus_mpi, *exponent_mpi));

gcry_sexp_dump ( *keypair );
}
