#include <stdio.h>
#include <stdlib.h>
#include <gcrypt.h>
#include <time.h>
#include <unistd.h>
#include "crypto.h"
#include "memmgmt.h"
#include "misc.h"

#define RETRIES 5
#define TEMP_FILE_NAME "/tmp/qingy-testencdata.txt"

void int_generate_keys(int try_to_restore, int fail_if_restore_fail);
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

	fp = fopen(TEMP_FILE_NAME, "w");

	fprintf(stdout, "Encrypting: ");
	for (;; i++)
	{
		if (!test[i]) break;

		fprintf(stdout, "'%s'",test[i]);
		
		if (test[i+1])
			fprintf(stdout, ", ");

		encrypt_item(fp, test[i]);
	}
	fclose(fp);
	fprintf(stdout, "\n");


	fprintf(stdout, "Decrypting: ");
	fp = fopen(TEMP_FILE_NAME, "r");
	for (i=0;; i++)
	{
		char *dec;

		if (!test[i]) break;

		dec = decrypt_item(fp);

		if (!dec)
		{
			fclose(fp);
			fprintf(stdout, "FAILURE!\n");
			return 0;
		}

		fprintf(stdout, "'%s'", dec);

		if (test[i+1])
			fprintf(stdout, ", ");

		if (strcmp(dec, test[i]))
		{
			fclose(fp);
			fprintf(stdout, "\n");
			return 0;
		}
	}
	fclose(fp);

	fprintf(stdout, "\n");

	return 1;
}

int main(void)
{
	char  logbuf[65535];
	int   retval = EXIT_FAILURE;
	int   count  = 0;
	char *datadir=strdup(SETTINGS_DIR "/");
	char *pubkey;
	char *prvkey;

	pubkey = StrApp((char**)NULL, datadir, "public_key",  (char*)NULL);
	prvkey = StrApp((char**)NULL, datadir, "private_key", (char*)NULL);

	fprintf(stdout, "\n\n\n");
	fprintf(stdout, "We are going to create a key pair for qingy and test it\n");
	fprintf(stdout, "against a variety of items in order to make sure it works.\n");

	srand((unsigned int)time(NULL));
	gcry_check_version(NULL);
	gcry_control( GCRYCTL_INIT_SECMEM, 16384, 0 );
	gcry_set_progress_handler ((&my_progress_handler), logbuf);

	set_key_files(datadir);

	for (; count < RETRIES; count++)
	{
		fprintf(stdout, "\n\n\n");
		fprintf(stdout, "Generating a new key pair... this can take a while!\n");
		fprintf(stdout, "Please keep your computer busy, make the hard disk work,\n");
		fprintf(stdout, "type something, move your mouse!\n");
		fprintf(stdout, "\n\n");

		/* generate key couple */
		int_generate_keys(0, 0);

		fprintf(stdout, "key generated!\n");

		/* throw it away */
		flush_keys();

		/* reload it from disk */
		int_generate_keys(1, 1);

		/* does this pair work? */
		if (test_keys()) break;

		/* nope, try again */
		remove(prvkey);
		remove(pubkey);

		fprintf(stdout, "Key pair does not work... ");
		if (count == RETRIES-1)
			fprintf(stdout, "giving up!\n");
		else
			fprintf(stdout, "trying again (will retry %d times)!\n", RETRIES-count-1);
	}
	
	free(datadir);

	if (!access(prvkey, F_OK) && !access(pubkey, F_OK))
	{
		retval = EXIT_SUCCESS;
		fprintf(stdout, "Key pair successfully generated!\n\n\n");
	}
	else
		fprintf(stdout, "Failed to generate key pair!\n\n\n");

	remove(TEMP_FILE_NAME);
	free(pubkey);
	free(prvkey);

	return retval;
}
