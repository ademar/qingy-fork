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

#include "memmgmt.h"
#include "misc.h"
#include "qingy_constants.h"

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


/* void encrypt_item(FILE *fp, char *item) */
/* { */
/* 	char *encrypted; */
/* 	int   status; */

/* 	if (!fp)   return; */
/* 	if (!item) return; */
/* 	if (!rsa)  return; */

/* 	encrypted = (char*) calloc(1, RSA_size(rsa)); */
/* 	status = RSA_public_encrypt(strlen(item), item, encrypted, rsa, RSA_PKCS1_OAEP_PADDING); */
/* 	if (status == -1) */
/* 	{ */
/* 		fprintf(stderr, "qingy: fatal error: RSA_public_encrypt() failed!\n"); */
/* 		exit(QINGY_FAILURE); */
/* 	} */
/* 	fwrite(encrypted, sizeof(char), RSA_size(rsa), fp); */
/* 	free(encrypted); */
/* } */

/* char *decrypt_item(FILE *fp) */
/* { */
/* 	char *retval; */
/* 	char *buf; */

/* 	if (!rsa) return NULL; */
/* 	if (!fp)  return NULL; */

/* 	buf = (char*) calloc(1, RSA_size(rsa)); */

/* 	if (fread(buf, sizeof(char), RSA_size(rsa), fp) != (unsigned int)RSA_size(rsa)) */
/* 		return NULL; */

/* 	retval = (char*) calloc(1, RSA_size(rsa)); */

/* 	if (RSA_private_decrypt(RSA_size(rsa), buf, retval, rsa, RSA_PKCS1_OAEP_PADDING) == -1) */
/* 	{ */
/* 		free(retval); */
/* 		retval = NULL; */
/* 	} */

/* 	return retval; */
/* } */

void generate_keys()
{
	gcry_error_t  error;
	gcry_sexp_t  *parms       = calloc(1, sizeof(gcry_sexp_t));
	char         *buf         = "(genkey(rsa(nbits 4:1024)))";
	size_t        len         = strlen(buf);

	/* initialize stuff */
	if (keypair) free(keypair);
	keypair = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	srand((unsigned int)time(NULL));
	gcry_check_version(NULL);
	gcry_control( GCRYCTL_INIT_SECMEM, 16384, 0 );

	/* try to load key pair from disc */
	//TO BE IMPLEMENTED

	/* generate new key pair */
	CHECK(gcry_sexp_new(parms, buf, len, 1));
	CHECK(gcry_pk_genkey(keypair, *parms));
	gcry_sexp_release( *parms );
	free(parms);

	/* save key pair to file */
	//TO BE IMPLEMENTED
}

static void dump(FILE *fp, char *tagname, char *buf, size_t len)
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

void save_public_key(FILE *fp)
{
	gcry_sexp_t *public_key;
	gcry_sexp_t *n;
	gcry_sexp_t *e;
	char        *temp = "public-key";
	size_t       len;

	if (!fp)      return;
	if (!keypair) return;

	public_key  = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));	
	n           = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));
	e           = (gcry_sexp_t *)calloc(1, sizeof(gcry_sexp_t));

	/* get public key from keypair */
	*public_key = gcry_sexp_find_token (*keypair, temp, strlen(temp));

	/* get public key modulus */
	temp = (char *)calloc(2, sizeof(char));
	strcpy(temp, "n");
	*n = gcry_sexp_find_token (*public_key, temp, strlen(temp));

	/* get public key exponent */
	strcpy(temp, "e");
	*e = gcry_sexp_find_token (*public_key, temp, strlen(temp));
	free(temp);

	/* save public key modulus to file */
	temp = (char *)gcry_sexp_nth_data (*n, 1, &len);
	if (!temp)
	{
		fprintf(stderr, "qingy: failure: something is wrong with your libgcrypt!\n");
		sleep(2);
		exit(EXIT_FAILURE);
	}
	dump(fp, "modulus", temp, len);

	/* save public key exponent to file */
	temp = (char *)gcry_sexp_nth_data (*e, 1, &len);
	if (!temp)
	{
		fprintf(stderr, "qingy: failure: something is wrong with your libgcrypt!\n");
		sleep(2);
		exit(EXIT_FAILURE);
	}
	dump(fp, "exponent", temp, len);

	/* clean up */
	gcry_sexp_release( *public_key );
	gcry_sexp_release( *n );
	gcry_sexp_release( *e );
	free(public_key);
	free(n);
	free(e);
}

/* void restore_public_key(FILE *fp) */
/* { */
/* 	char   *temp = NULL; */
/* 	size_t  len  = 0; */

/* 	if (!fp) return; */
/* 	if (rsa) RSA_free(rsa); */

/* 	rsa = RSA_new(); */
/* 	if (!rsa) */
/* 	{ */
/* 		fprintf(stderr, "qingy: fatal error: unable to restore public key from file!\n"); */
/* 		exit(QINGY_FAILURE); */
/* 	} */

/* 	/\* we load the public key which we will use to encrypt out data: public modulus... *\/ */
/* 	if (getline(&temp, &len, fp) == -1) */
/* 	{ */
/* 		fprintf(stderr, "qingy: fatal error: unable to restore public key from file!\n"); */
/* 		exit(QINGY_FAILURE); */
/* 	} */
/* 	temp[strlen(temp)-1] = '\0'; */
/* 	if (!BN_hex2bn(&(rsa->n), temp)) */
/* 	{ */
/* 		fprintf(stderr, "qingy: fatal error: unable to restore public key from file!\n"); */
/* 		exit(QINGY_FAILURE); */
/* 	} */

/* 	/\* ...and exponent *\/ */
/* 	if (getline(&temp, &len, fp) == -1) */
/* 	{ */
/* 		fprintf(stderr, "qingy: fatal error: unable to restore public key from file!\n"); */
/* 		exit(QINGY_FAILURE); */
/* 	} */
/* 	temp[strlen(temp)-1] = '\0'; */
/* 	if (!BN_hex2bn(&(rsa->e), temp)) */
/* 	{ */
/* 		fprintf(stderr, "qingy: fatal error: unable to restore public key from file!\n"); */
/* 		exit(QINGY_FAILURE); */
/* 	} */
/* 	free(temp); */
/* } */
