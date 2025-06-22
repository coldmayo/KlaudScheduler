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

ConfigInfo * get_config_info() {
	FILE * config = fopen("~/.klaudrc", "r");

	char line[200];

	ConfigInfo * info = malloc(sizeof(ConfigInfo));

	// Setting defaults just in case
	info->priority_type = strdup("SJF");
	info->lottery = false;
	info->aging = true;
	info->ignore_hosts = malloc(100 * sizeof(char *));
	info->get_nodes_strat = strdup("SSH");

	while (fgets(line, sizeof(line), config)) {
		if (line[0] == '#' || line[0] == '\n') {
			continue;
		}

    	line[strcspn(line, "\n")] = '\0';

		// Check for these in the config file:
    	if (sscanf(line, "set-priority %ms", &info->priority_type) == 1) {
			continue;
    	}
    	if (sscanf(line, "set-nodes-strat %ms", &info->get_nodes_strat) == 1) {
			continue;
    	}

    	char * boolstr;
    		
    	if (sscanf(line, "set-priority-lottery %ms", &boolstr) == 1) {
			if (strcmp(boolstr, "true") == 0) {
				info->lottery = true;
    		} else if (strcmp(boolstr, "false") == 0) {
				info->lottery = false;
    		}
    		continue;
    	}

		if (sscanf(line, "set-priority-aging %ms", &boolstr) == 1) {
			if (strcmp(boolstr, "true") == 0) {
				info->aging = true;
    		} else if (strcmp(boolstr, "false") == 0) {
				info->aging = false;
    		}
    		continue;
		}
    	free(boolstr);

    	char * hosts;
    	if (sscanf(line, "ignore-host %ms", &hosts) == 1) {
			char * token = strtok(hosts, " ");
    		int i = 0;
    		while (token != NULL) {
				info->ignore_hosts[i] = strdup(token);
				token = strtok(NULL, " ");
				i++;
    		}
    		info->ignore_hosts[i] = NULL;
    		free(hosts);
    		continue;
    	}
	}

	fclose(config);
	return info;
}

bool allowed_ip(const char * ip) {
	ConfigInfo * config = get_config_info();
	int i = 0;
	while(config->ignore_hosts[i] != NULL) {
		if (strcmp(ip, config->ignore_hosts[i]) == 0) {
			return false;
		}
		i++;
	}
	return true;
}

char ** get_ip_hosts() {
    char ** nodes = calloc(101, sizeof(char *));
	FILE * hosts = fopen("/etc/hosts", "r");
	char line[200];

	int i = 0;
	while(fgets(line, sizeof(line), hosts)) {
		if (line[0] == '#' || line[0] == '\n') continue;

		char * line_cpy = strdup(line);
		char * line_ip = strtok(line_cpy, " \t\n");

		if (allowed_ip(line_ip)) {
			nodes[i] = strdup(line_ip);
		}
		

		free(line_cpy);
		i++;
	}

	fclose(hosts);
	nodes[i] = NULL;
	return nodes;
}

cJSON * read_json(char * filename) {
    FILE * fp = fopen(filename, "r");
    if (!fp) {
        printf("File does not exist");
        fflush(stdout);
        return cJSON_CreateArray();
    }
    cJSON * array = NULL;

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size > 0) {
        char *buffer = (char *)malloc(file_size + 1);
        fread(buffer, 1, file_size, fp);
        buffer[file_size] = '\0';

        array = cJSON_Parse(buffer);
        free(buffer);
    } else {
        //printf("Empty File\n");
        fflush(stdout);
        array = cJSON_CreateArray();
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
