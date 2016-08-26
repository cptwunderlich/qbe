#include <stdio.h>

#include "all.h"

static FILE *urandom = NULL;

/* Returns -1 on error */
int
openrnd()
{
	if (urandom != NULL) {
		fprintf(stderr, "Warning: /dev/urandom already opened! Closing...\n");
		if (closernd() != 0) {
			return -1;
		}
	}

	urandom = fopen("/dev/urandom", "r");
	if (urandom == NULL) {
		fprintf(stderr, "Error opening /dev/urandom:\n");
	        perror(__func__);
	        urandom = NULL;
        	return -1;
	}

	return 0;
}

int
closernd()
{
	if (urandom == NULL)
		return 0;

	int res;
	if (fclose(urandom) != 0) {
		// FAIL
	        fprintf(stderr, "Error closing /dev/urandom:\n");
	        perror(__func__);
		res = -1;
	} else {
		res = 0;
	}

	urandom = NULL;
	return res;
}

int
get_random_0to3()
{
	if (urandom == NULL) {
		return -1;
	}

	int rnd = 0;
	size_t size = fread(&rnd, sizeof(int), 1, urandom);
	if (size < 1) {
		return -1;
	}

	return rnd % 4;
}
