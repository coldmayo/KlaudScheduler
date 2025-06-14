#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../includes/types.h"

// Literally a file of random smaller functions that I think can be helpful in multiple other files

char * read_file(char * file_name) {
	FILE * f;
	f = fopen(file_name, "rb");
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	rewind(f);
	char * str = malloc(size+1);
	fread(str, 1, size, f);
	str[size] = '\0';
	fclose(f);
	return str;
}

cJSON * read_json(char * filename) {
    FILE * fp = fopen(filename, "r");
	cJSON * array;

	fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size > 0) {
        char *buffer = (char *)malloc(file_size + 1);
        fread(buffer, 1, file_size, fp);
        buffer[file_size] = '\0';

        array = cJSON_Parse(buffer);
        free(buffer);
    }
    fclose(fp);
    return array;
}

char * ip_alias(char * ip) {
    FILE * hosts = fopen("/etc/hosts", "r");
    if (!hosts) return NULL;

    char line[200];
    char *result = NULL;

    while (fgets(line, sizeof(line), hosts)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        char *line_cpy = strdup(line);
        char *line_ip = strtok(line_cpy, " \t\n");

        if (line_ip && strcmp(line_ip, ip) == 0) {
            char *alias = strtok(NULL, " \t\n");
            if (alias) {
                result = strdup(alias);
                free(line_cpy);
                break;
            }
        }
        free(line_cpy);
    }

    fclose(hosts);
    return result;
}

void gen_rankfile(int id, CPUout * c) {
        char file_name[40];
        sprintf(file_name, "%d_rankfile.txt", id);
        FILE *file = fopen(file_name, "w+");
        
        for (int i = 0; c->hosts[i] != NULL; i++) {
	    fprintf(file, "rank %d=%s slot=%d\n", i, ip_alias(c->hosts[i]), c->cpu_cores[i]);
	}
	
	fclose(file);
}

int cpu_ranks(char * hostname, int id) {
    char file_name[40];
    sprintf(file_name, "%d_rankfile.txt", id);
    FILE *file = fopen(file_name, "r");
    if (!file) return 0;

    char line[100];
    int count = 0;

    while (fgets(line, sizeof(line), file)) {
        char *rank_ptr = strstr(line, "rank ");
        if (!rank_ptr) continue;

        char *eq = strchr(rank_ptr, '=');
        if (!eq) continue;

        char *host = strtok(eq+1, " \t\n");
        if (host && strcmp(host, hostname) == 0) {
            count++;
        }
    }

    fclose(file);
    return count;
}
