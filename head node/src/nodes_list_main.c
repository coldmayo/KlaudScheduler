#include <stdlib.h>
#include <stdio.h>
#include "cJSON.h"
#include "../includes/nodes_list.h"

int main() {
    const char *nodes[] = {"192.168.1.101", "192.168.1.102"};
    int num_nodes = sizeof(nodes) / sizeof(nodes[0]);

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

    for (int i = 0; i < num_nodes; i++) {
        cJSON *node_info = check_nodes(nodes[i]);
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
