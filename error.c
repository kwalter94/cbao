#include "error.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

void warn(char *msg)
{
	fprintf(stderr, "%s: %s: %s\n", program_invocation_short_name, msg,
			strerror(errno));
}


void choke(char *msg)
{
	warn(msg);
	exit(EXIT_FAILURE);
}

