#ifndef READ_IM_CONFIG_H_
#define READ_IM_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <jansson.h>
#include <string.h>

typedef struct _IM_HOST_URL_REG{
		char 	*host;
		char	*url;
		char	*reg;
		struct _IM_HOST_URL_REG *next;
}_IM_HOST_URL_REG;
int IM_memcpy(char **p1, const char *p2);
_IM_HOST_URL_REG  *init_IM_list();
int add_IM_list(_IM_HOST_URL_REG *nodelist, const char *host, const char * url, const char *reg);
int read_IM_config();

#endif