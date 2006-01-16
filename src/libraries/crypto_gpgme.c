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
#include <stdlib.h>
#include <string.h>
#include <gpgme.h>
#include <unistd.h>

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

void progress_handler (void *HOOK, const char *WHAT, int TYPE, int CURRENT, int TOTAL)
{
	static char *what = NULL;

	if (!what || strcmp(WHAT, what))
	{
		fprintf(stderr, "%s: ", WHAT);
		if (what) fprintf(stderr, "\n");
		what = strdup(WHAT);
	}

	fprintf(stderr, "%c", TYPE);
}

gpgme_error_t passfunc (void *HOOK, const char *UID_HINT, const char *PASSPHRASE_INFO, int PREV_WAS_BAD, int FD)
{
	fprintf(stderr, "Enter passphrase (");

	if (UID_HINT)
		fprintf(stderr, " uid '%s' ", UID_HINT);

	if (PASSPHRASE_INFO)
		fprintf(stderr, " info '%s' ", PASSPHRASE_INFO);

	fprintf(stderr, "): abc\n");

	write(FD, "abc\n", 4);

	return 0;
}

void generate_keys()
{
/* 	if (rsa) RSA_free(rsa); */
/* 	srand((unsigned int)time(NULL)); */
/* 	rsa = RSA_generate_key(1024, 17, NULL, NULL);  */

	gpgme_error_t error;
	gpgme_ctx_t ctx = NULL;


/* 	gpgme_data_t public  = NULL; */
/* 	gpgme_data_t private = NULL; */

/* 	setlocale (LC_ALL, ""); */
	gpgme_check_version (NULL);
	
	error = gpgme_new (&ctx);
	if (error != GPG_ERR_NO_ERROR)
	{
		fprintf(stderr, "could not create context\n\n\n");
		exit(EXIT_FAILURE);
	}

	gpgme_protocol_t protocol = GPGME_PROTOCOL_OpenPGP;

	error = gpgme_set_protocol (ctx, protocol);

	if (error != GPG_ERR_NO_ERROR)
	{		
		fprintf(stderr, "could not set crypto protocol\n\n\n");
		exit(EXIT_FAILURE);
	}


	char parms[1024];

	strcpy(parms, "<GnupgKeyParms format=\"internal\">\n");
	strcat(parms, "Key-Type: DSA\n");
	strcat(parms, "Key-Length: 1024\n");
	strcat(parms, "Subkey-Type: ELG-E\n");
	strcat(parms, "Subkey-Length: 1024\n");
	strcat(parms, "Name-Real: Joe Tester\n");
	strcat(parms, "Name-Comment: with stupid passphrase\n");
	strcat(parms, "Name-Email: joe@foo.bar\n");
	strcat(parms, "Expire-Date: 0\n");
	strcat(parms, "Passphrase: abc\n");
	strcat(parms, "</GnupgKeyParms>\n");

	fprintf(stderr, "%s\n", parms);

/* 	error = gpgme_op_genkey_start (ctx, parms, NULL, NULL); */
/* 	if (error != GPG_ERR_NO_ERROR) */
/* 	{		 */
/* 		fprintf(stderr, "could not generate key1: %s\n\n\n", gpg_strerror (error)); */
/* 		exit(EXIT_FAILURE); */
/* 	} */

/* 	gpgme_wait (ctx, &error, 1); */
/* 	if (error != GPG_ERR_NO_ERROR) */
/* 	{		 */
/* 		fprintf(stderr, "could not generate key2: %s\n\n\n", gpg_strerror (error)); */
/* 		exit(EXIT_FAILURE); */
/* 	} */

	gpgme_data_t plain;
	gpgme_data_t enc;

	error = gpgme_data_new (&plain);
	if (error != GPG_ERR_NO_ERROR)
	{		
		fprintf(stderr, "could not generate key3: %s\n\n\n", gpg_strerror (error));
		exit(EXIT_FAILURE);
	}
	error = gpgme_data_new (&enc);
	if (error != GPG_ERR_NO_ERROR)
	{		
		fprintf(stderr, "could not generate key3: %s\n\n\n", gpg_strerror (error));
		exit(EXIT_FAILURE);
	}

	char *text="ciao mamma";
	/*ssize_t*/ gpgme_data_write (plain, text, sizeof(text));

	gpgme_set_progress_cb (ctx, (&progress_handler), NULL);

	error = gpgme_op_genkey(ctx, parms, NULL, NULL);
	if (error != GPG_ERR_NO_ERROR)
	{		
		fprintf(stderr, "could not generate key3: %s\n\n\n", gpg_strerror (error));
		exit(EXIT_FAILURE);
	}

	gpgme_genkey_result_t result;

	result = gpgme_op_genkey_result (ctx);

	fprintf(stderr, "prim: %d, sub: %d, fprint: %s\n", result->primary, result->sub, result->fpr);

	char *fpr = strdup(result->fpr);

	//error = gpgme_op_keylist_start (ctx, NULL, 0);
	error = gpgme_op_keylist_start (ctx, fpr, 0);
	if (error != GPG_ERR_NO_ERROR)
	{
		fprintf(stderr, "could not start key list\n\n\n");
		exit(EXIT_FAILURE);
	}

	gpgme_key_t *kset = NULL;

	kset = malloc(sizeof(gpgme_key_t)*(2));
	memset(kset, 0, sizeof(gpgme_key_t)*(2));

	int count = 0;
	while (1)
	{
		gpgme_key_t key;
		error =  gpgme_op_keylist_next (ctx, &key);
		if (error != GPG_ERR_NO_ERROR && gpg_err_code(error) != GPG_ERR_EOF)
		{
			fprintf(stderr, "could not list key\n\n\n");
			exit(EXIT_FAILURE);
		}
		if (gpg_err_code(error) == GPG_ERR_EOF) break;
		kset[0] = key;
		count++;
	}
	fprintf(stderr, "found %d keys in chain\n", count);

	error = gpgme_op_keylist_end (ctx);

	if (count != 1)
	{
		fprintf(stderr, "No key matching your criteria was found.\n");
		exit(EXIT_FAILURE);
	}

	gpgme_set_armor(ctx, 1);
	gpgme_data_seek (plain, 0, SEEK_SET);


	gpgme_set_passphrase_cb (ctx, &(passfunc), NULL);

	error = gpgme_op_encrypt (ctx, kset, GPGME_ENCRYPT_ALWAYS_TRUST, plain, enc);
	if (error != GPG_ERR_NO_ERROR)
	{		
		fprintf(stderr, "could not encrypt: %s\n\n\n", gpg_strerror (error));
		exit(EXIT_FAILURE);
	}

	gpgme_data_t dec;

	error = gpgme_data_new (&dec);
	if (error != GPG_ERR_NO_ERROR)
	{		
		fprintf(stderr, "could not generate data: %s\n\n\n", gpg_strerror (error));
		exit(EXIT_FAILURE);
	}

	gpgme_data_seek (enc, 0, SEEK_SET);

	error = gpgme_op_decrypt (ctx, enc, dec);
	if (error != GPG_ERR_NO_ERROR)
	{		
		fprintf(stderr, "could not decrypt: %s\n\n\n", gpg_strerror (error));
		exit(EXIT_FAILURE);
	}

	char res[65535];

	/*ssize_t*/ gpgme_data_read (dec, res, 65534);

	fprintf(stderr, "result: %s\n", res);

/* 	error = gpgme_op_delete (ctx, *key, 1); */

/* 	if (error != GPG_ERR_NO_ERROR) */
/* 	{ */
/* 		fprintf(stderr, "could not delete key from chain\n\n\n"); */
/* 		exit(EXIT_FAILURE); */
/* 	} */

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
