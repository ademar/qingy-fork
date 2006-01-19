#include <stdio.h>
#include <stdlib.h>
#include <gcrypt.h>
#include <time.h>
#include <unistd.h>
#include "crypto.h"
#include "memmgmt.h"
#include "misc.h"

#define RETRIES 50

void int_generate_keys(int try_to_restore);
void set_key_files(char *path);
void flush_keys();

void my_progress_handler (void *cb_data, const char *what, int printchar, int current, int total)
{
	if (cb_data || printchar || current || total) {}

	/* Do something.  */
	fprintf(stderr, "%s\n", what);
}

int test_keys(void)
{
	FILE *fp;
	char *test[] = {"moc", "mamma", "papa", "pippo", "s4t4n", "123 prova", "sdpoifj", "spdofk", "psodfkj", "psokf", "psodkf", "mic", "sdf", "moc", "Text: Console", "Gnome", NULL};
	int   i      = 0;

	fp = fopen("encdata.txt", "w");
	for (;; i++)
	{
		if (!test[i]) break;

fprintf(stderr, "'%s', ",test[i]);

		encrypt_item(fp, test[i]);
	}
	fclose(fp);
fprintf(stderr, "\n");


	fp = fopen("encdata.txt", "r");
	for (i=0;; i++)
	{
		char *dec;

		if (!test[i]) break;

		dec = decrypt_item(fp);

fprintf(stderr, "'%s', ", dec);

		if (!dec)
		{
			fclose(fp);fprintf(stderr, "\n");
			return 0;
		}

		if (strcmp(dec, test[i]))
		{
			fclose(fp);fprintf(stderr, "\n");
			return 0;
		}
	}
	fclose(fp);

fprintf(stderr, "\n");

	return 1;
}

int main(void)
{
	char  logbuf[65535];
	int   retval = EXIT_FAILURE;
	int   count  = 0;
	char *path;
	char *pubkey;
	char *prvkey;

	if (!getcwd(logbuf, 65535*sizeof(char)))
	{
		fprintf(stderr, "Failed getting current working directory!\n");
		exit(EXIT_FAILURE);
	}
	strcat(logbuf, "/");
	path = strdup(logbuf);

	pubkey = StrApp((char**)NULL, path, "public_key",  (char*)NULL);
	prvkey = StrApp((char**)NULL, path, "private_key", (char*)NULL);

	fprintf(stdout, "\n\n\n");
	fprintf(stdout, "Please note that libgcrypt support is still somewhat experimental...\n");
	fprintf(stdout, "For some reason, some key pairs are not able to decrypt all items,\n");
	fprintf(stdout, "so we are going to create one during install and test it against\n");
	fprintf(stdout, "a variety of items in order to minimize this effect.\n");

	srand((unsigned int)time(NULL));
	gcry_check_version(NULL);
	gcry_control( GCRYCTL_INIT_SECMEM, 16384, 0 );
	gcry_set_progress_handler ((&my_progress_handler), logbuf);

	set_key_files(path);

	for (; count < RETRIES; count++)
	{
		fprintf(stdout, "\n\n\n");
		fprintf(stdout, "Generating a new key pair... this can take a while!\n");
		fprintf(stdout, "Please keep your computer busy, make the hard disk work,\n");
		fprintf(stdout, "type something, move your mouse!\n");
		fprintf(stdout, "\n\n");

		/* generate key couple */
		int_generate_keys(0);

fprintf(stderr, "key generated!\n");

		/* throw it away */
		flush_keys();

fprintf(stderr, "key deleted!\n");

		/* reload it from disk */
		int_generate_keys(1);

fprintf(stderr, "key restored!\n");

		/* does this pair work? */
		if (test_keys()) break;

		/* nope, try again */
		remove(prvkey);
		remove(pubkey);

		fprintf(stdout, "Key pair does not work... ");
		if (count == RETRIES-1)
			fprintf(stdout, "giving up!\n");
		else
			fprintf(stdout, "trying again!\n");
	}
	
	free(path);

	if (!access(prvkey, F_OK) && !access(pubkey, F_OK))
	{
		retval = EXIT_SUCCESS;
		fprintf(stdout, "Key pair successfully generated!\n\n\n");
	}
	else
		fprintf(stdout, "Failed to generate key pair!\n\n\n");

	remove("encdata.txt");
	free(pubkey);
	free(prvkey);

	return retval;
}
