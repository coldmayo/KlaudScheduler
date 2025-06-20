#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "cJSON.h"
#include "../includes/nodes_list.h"

char ** get_ip_hosts() {
    char ** nodes = calloc(101, sizeof(char *));
	FILE * hosts = fopen("/etc/hosts", "r");
	char line[200];

	int i = 0;
	while(fgets(line, sizeof(line), hosts)) {
		if (line[0] == '#' || line[0] == '\n') continue;

		char * line_cpy = strdup(line);
		char * line_ip = strtok(line_cpy, " \t\n");

		nodes[i] = strdup(line_ip);

		free(line_cpy);
		i++;
	}

	fclose(hosts);
	nodes[i] = NULL;
	return nodes;
}

int main(int argc, char ** argv) {
    //const char *nodes[] = {"192.168.1.101", "192.168.1.102"};
	bool tcp = false;
    if (argc > 1) {
		if (strcmp(argv[1], "--tcp") == 0) {
			tcp = true;
		} else if (strcmp(argv[1], "--ssh") == 0) {
			tcp = false;
		} else {
			printf("Usage: ./gen_nodes_list <type>\nWhere <type> can be --tcp or ssh");
			return EXIT_FAILURE;
		}
    }
    
    char ** nodes = get_ip_hosts();

    // Clear rankfile
    FILE *rankfile = fopen("rankfile.txt", "w");
    if (rankfile) {
        fclose(rankfile);
    } else {
        perror("Failed to create rankfile.txt");
        return 1;
    }

    cJSON *jobs_array = cJSON_CreateArray();
    if (!jobs_array) {
        printf("Failed to create JSON array\n");
        return 1;
    }

    for (int i = 0; strcmp(nodes[i], "\0") != 0; i++) {
		cJSON * node_info;
        node_info = check_nodes(nodes[i], tcp);
        if (node_info) {
            cJSON_AddItemToArray(jobs_array, node_info);
        }
    }

    // Save JSON to file
    FILE *json_file = fopen("nodes.json", "w");
    if (json_file) {
        char *json_string = cJSON_Print(jobs_array);
        if (json_string) {
            fprintf(json_file, "%s\n", json_string);
            free(json_string);
        }
        fclose(json_file);
    }

    cJSON_Delete(jobs_array);
    return 0;
}
