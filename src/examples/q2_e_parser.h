#ifndef Q2_E_PARSER_H
#define Q2_E_PARSER_H

#include <stdint.h>

#include "utils/tr_math.h"

#define MAX_LIGHTS			1024

typedef enum Q2EntityTokenType
{
	TOKEN_OPEN,
	TOKEN_CLOSE,
	TOKEN_ORIGIN, TOKEN_VEC3,
	TOKEN_STRING,
	TOKEN_CLASSNAME, TOKEN_ENTITY_NAME,
	TOKEN_QUOTE,
	TOKEN_UNKNOWN,
	TOKEN_EOF,
	TOKEN_MAX
} Q2EntityTokenType;

typedef struct Q2EntityToken
{
	Q2EntityTokenType type;

	char name[64];
} Q2EntityToken;

typedef struct Q2EntityKeyValue
{
	char key[64];
	char value[64];
} Q2EntityKeyValue;

typedef enum Q2EntityType
{
	ENTITY_TYPE_UNKNOWN,
	ENTITY_TYPE_WORLDSPAWN,
	ENTITY_TYPE_INFO_PLAYER_START,
	ENTITY_TYPE_LIGHT,
	MAX_ENTITY_TYPES
} Q2EntityType;

typedef struct Q2LightEntity
{
	vec3  pos;
	float brightness;
} Q2LightEntity;

typedef struct Q2Entity
{
	Q2EntityType type;

	vec3         pos;
	float	     brightness;

	char         sky[64];
} Q2Entity;

typedef struct Q2Entities 
{
	Q2Entity      worldspawn;
	Q2Entity      info_player_start;
	Q2Entity      lights[MAX_LIGHTS];	
	uint32_t      light_count;
} Q2Entities;

Q2Entities q2_e_parse(char * entity_string);

#endif