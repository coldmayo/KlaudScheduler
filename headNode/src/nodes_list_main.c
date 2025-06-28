#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "cJSON.h"
#include "../includes/nodes_list.h"
#include "../includes/utils.h"
#include "../includes/types.h"


int main(int argc, char ** argv) {
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
    ConfigInfo * config;
    config = get_config_info();

    char file_name[200];
    sprintf(file_name, "%s/rankfile.txt", config->dir);
    FILE *rankfile = fopen(file_name, "w");
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

    for (int i = 0; nodes[i] != NULL; i++) {
	cJSON * node_info;
        node_info = check_nodes(nodes[i], tcp);
        if (node_info) {
            cJSON_AddItemToArray(jobs_array, node_info);
        }
    }

    // Save JSON to file
    //printf("Saving to JSON file\n");
    file_name[0] = '\0';
    sprintf(file_name, "%s/nodes.json", config->dir);
    FILE *json_file = fopen(file_name, "w");
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
