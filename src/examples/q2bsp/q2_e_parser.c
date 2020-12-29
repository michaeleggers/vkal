#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "q2_e_parser.h"
#include "q2_common.h"

/*
{
"sky" "unit_"
"_tb_textures" "textures/e1u1;textures/makkon1_textures"
"classname" "worldspawn"
}
{
	"origin" "0 0 -72"
		"classname" "info_player_start"
}
{
	"origin" "216 152 40"
		"classname" "light"
}
{
	"origin" "72 136 40"
		"classname" "light"
}
*/

void q2_e_skip_ws(char ** entity_string)
{
	while ( **entity_string == ' ' ) { (*entity_string)++; }
}

void q2_e_skip_linebreaks(char ** entity_string)
{
	while ( (**entity_string == '\n') || (**entity_string == '\r') ) {
		(*entity_string)++;
	}
}

void q2_e_get_string(char ** entity_string, char * out_str)
{
	while ( (**entity_string != '\n') && (**entity_string != '\r') && (**entity_string != '"') ) {
		*out_str = **entity_string; 
		(*entity_string)++;
		out_str++;
	}
	*out_str = '\0';
}

Q2EntityToken q2_e_next_token(char ** entity_string)
{
	Q2EntityToken token = { 0 };

	q2_e_skip_ws(entity_string);
	q2_e_skip_linebreaks(entity_string);

	if ( string_length(*entity_string) ) {
		switch( **entity_string ) 
		{
		case '{' :  { 
			(*entity_string)++;
			token.type = TOKEN_OPEN;
		} break;

		case '}' : {
			(*entity_string)++;
			token.type = TOKEN_CLOSE;
		} break;

		case '"' : {
			(*entity_string)++;		
			token.type = TOKEN_STRING;			
			q2_e_get_string( entity_string, token.name );
			(*entity_string)++;
		} break;
			
		default : token.type = TOKEN_UNKNOWN;
		}
	}
	else {
		token.type = TOKEN_EOF;
	}

	return token;
}

Q2EntityToken q2_e_match(char ** entity_string, Q2EntityTokenType token_type)
{
	Q2EntityToken token = q2_e_next_token(entity_string);
	assert( token.type == token_type );
	return token;
}

void q2_e_extract_numeric(char ** str, char * out_number)
{
	char * c = out_number;
	q2_e_skip_ws( str );
	while ( (**str >= '0') && (**str <= '9') || (**str == '.') || (**str == '-') ) {
		*c++ = **str;
		(*str)++;
	}
	*c = '\0';
}

void q2_e_string_to_vec3(char * str, vec3 * out_vec3)
{
	char number[32];
	vec3 v;
	q2_e_extract_numeric(&str,  number);
	v.x = (float)atof(number);
	q2_e_extract_numeric(&str,  number);
	v.y = (float)atof(number);
	q2_e_extract_numeric(&str,  number);
	v.z = (float)atof(number);
	*out_vec3 = v;
}

void add_keyval(Q2Entity * entity, Q2EntityKeyValue keyval)
{
	if ( !strcmp(keyval.key, "classname") ) {
		if ( !strcmp(keyval.value, "light") ) {
			entity->type = ENTITY_TYPE_LIGHT;
		}
		else if ( !strcmp(keyval.value, "info_player_start") ) {
			entity->type = ENTITY_TYPE_INFO_PLAYER_START;
		}
		else if ( !strcmp(keyval.value, "worldspawn") ) {
			entity->type = ENTITY_TYPE_WORLDSPAWN;
		}
	}
	else if ( !strcmp(keyval.key, "origin") ) {
		q2_e_string_to_vec3(keyval.value, &entity->pos);
	}
	else if ( !strcmp(keyval.key, "sky") ) {
		strcpy(entity->sky, keyval.value);
	}
}

Q2Entities q2_e_parse(char * entity_string)
{
	Q2Entities entities = { 0 };

	char * start = entity_string;
	Q2EntityToken token = { .type = TOKEN_UNKNOWN };
	Q2Entity entity = { .type = ENTITY_TYPE_UNKNOWN };
	
	token = q2_e_next_token( &start );	
	while ( token.type != TOKEN_EOF ) {

		if (token.type == TOKEN_OPEN) { // new entity: match at least one key value pair.
			token = q2_e_match( &start, TOKEN_STRING );
			while (token.type == TOKEN_STRING) {
				Q2EntityKeyValue key_value = { 0 };
				strcpy(key_value.key, token.name);
				token = q2_e_match( &start, TOKEN_STRING );
				strcpy(key_value.value, token.name);
				add_keyval( &entity, key_value );

				token = q2_e_next_token( &start );
			}			
		}
		else if (token.type == TOKEN_CLOSE) {
			if ( entity.type == ENTITY_TYPE_LIGHT ) {
				entities.lights[ entities.light_count++ ] = entity;
			}
			else if ( entity.type == ENTITY_TYPE_INFO_PLAYER_START ) {
				entities.info_player_start = entity;
			}
			else if ( entity.type == ENTITY_TYPE_WORLDSPAWN ) {
				entities.worldspawn = entity;
			}

			entity = (Q2Entity){ 0 };
			token = q2_e_next_token( &start );
		}
	}

	return entities;
}
