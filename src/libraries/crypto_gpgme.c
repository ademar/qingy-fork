/***************************************************************************
                      crypto_gpgme.c  -  description
                            --------------------
    begin                : Dec 20 2005
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
#include <string.h>
#include <gpgme.h>

#include "qingy_constants.h"


/* static RSA *rsa = NULL; */


void encrypt_item(FILE *fp, char *item)
{
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
}

char *decrypt_item(FILE *fp)
{
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

	return NULL;
}

void generate_keys()
{
/* 	if (rsa) RSA_free(rsa); */
/* 	srand((unsigned int)time(NULL)); */
/* 	rsa = RSA_generate_key(1024, 17, NULL, NULL);  */

/* 	setlocale (LC_ALL, ""); */
	gpgme_check_version (NULL);
/* 	gpgme_set_locale (NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL)); */
/* 	gpgme_set_locale (NULL, LC_MESSAGES, setlocale (LC_MESSAGES, NULL)); */
}

void save_public_key(FILE *fp)
{
/* 	char *temp; */

/* 	if (!fp)  return; */
/* 	if (!rsa) return; */

/* 	/\* we write only the public modulus... *\/ */
/* 	temp = BN_bn2hex(rsa->n); */
/* 	if (!temp) */
/* 	{ */
/* 		fprintf(stderr, "qingy: fatal error: unable to write public key to file!\n"); */
/* 		abort(); */
/* 	} */
/* 	fprintf(fp, "%s\n", temp); */

/* 	/\* ...and exponent *\/ */
/* 	free(temp); */
/* 	temp = BN_bn2hex(rsa->e); */
/* 	if (!temp) */
/* 	{ */
/* 		fprintf(stderr, "qingy: fatal error: unable to write public key to file!\n"); */
/* 		abort(); */
/* 	} */
/* 	fprintf(fp, "%s\n", temp); */
/* 	free(temp); */
}

void restore_public_key(FILE *fp)
{
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
}
