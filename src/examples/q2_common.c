
#include "q2_common.h"

int string_length(char * string)
{
	int len = 0;
	char * c = string;
	while (*c != '\0') { c++; len++; }
	return len;
}