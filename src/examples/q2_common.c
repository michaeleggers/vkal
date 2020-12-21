
#include "q2_common.h"

int string_length(char * string)
{
	int len = 0;
	char * c = string;
	while (*c != '\0') { c++; len++; }
	return len;
}

void concat_str(uint8_t * str1, uint8_t * str2, uint8_t * out_result)
{
	strcpy(out_result, str1);
	strcat(out_result, str2);
}