#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include <stdbool.h>
#include "../includes/utils.h"
#include "../includes/types.h"

void populate_groups() {
    ConfigInfo * config = get_config_info();
    char file_path[200];
    sprintf(file_path, "%s/group_cpu_time.txt", config->dir);
    
    FILE *fp = fopen(file_path, "w");
	
	if (config->groups[0] != NULL) {
		for (int i = 0; config->groups[i] != NULL; i++) {
			fprintf(fp, "%s 0\n", config->groups[i]->group_name);
		}
	}

	fclose(fp);
}

double get_group_cpu_time(char * group_name) {
	ConfigInfo * config = get_config_info();
    char file_path[200];
    sprintf(file_path, "%s/group_cpu_time.txt", config->dir);
    
    FILE *fp = fopen(file_path, "r");

    char line[200];
    while (fgets(line, sizeof(line), fp)) {
        char * name = NULL;
        double time;
        
        line[strcspn(line, "\n")] = '\0';
        
		if (sscanf(line, "%ms %lf", &name, &time) == 2) {
			if (strcmp(name, group_name) == 0) {
				return time;
			}
		}
    }

    return 0.0;
}

void update_group_times(char * group_name, double elapsed) {
    ConfigInfo * config = get_config_info();
    char file_path[200];
    sprintf(file_path, "%s/group_cpu_time.txt", config->dir);
    
    FILE *fp = fopen(file_path, "r");
    if (!fp) {
        perror("fopen");
        return;
    }

    char line[100];
    char **lines = NULL;
    int line_count = 0;
    int found = 0;

    while (fgets(line, sizeof(line), fp)) {
        char *name = NULL;
        double time;

        line[strcspn(line, "\n")] = '\0'; // strip newline

        if (sscanf(line, "%ms %lf", &name, &time) == 2) {
            if (strcmp(name, group_name) == 0) {
                time += elapsed;
                found = 1;
            }

            char new_line[100];
            snprintf(new_line, sizeof(new_line), "%s %.2f", name, time);
            lines = realloc(lines, sizeof(char *) * (line_count + 1));
            lines[line_count] = strdup(new_line);
            line_count++;

            free(name);
        }
    }
    fclose(fp);

    if (!found) {
        char new_line[100];
        snprintf(new_line, sizeof(new_line), "%s %.2f", group_name, elapsed);
        lines = realloc(lines, sizeof(char *) * (line_count + 1));
        lines[line_count] = strdup(new_line);
        line_count++;
    }

    // Write back to file
    fp = fopen("group_cpu_time.txt", "w");
    if (!fp) {
        perror("fopen");
        return;
    }

    for (int i = 0; i < line_count; i++) {
        fprintf(fp, "%s\n", lines[i]);
        free(lines[i]);
    }
    free(lines);
    fclose(fp);
}

bool is_user_saved(char * username) {
	ConfigInfo * config = get_config_info();
	char file_path[200];
	sprintf(file_path, "%s/users.json", config->dir);

	cJSON * user_array = read_json(file_path);
	cJSON * user = NULL;

	cJSON_ArrayForEach(user, user_array) {
		cJSON * usern = cJSON_GetObjectItem(user, "username");

		if (strcmp(usern->valuestring, username) == 0) {
			return true;
		}
	}

	return false;
}

void save_user(char * username, char * group_name) {
	ConfigInfo * config;
	config = get_config_info();

	char file_path[200];
	sprintf(file_path, "%s/users.json", config->dir);

	cJSON * user_array = read_json(file_path);
	if (!user_array || !cJSON_IsArray(user_array)) {
		if (user_array) cJSON_Delete(user_array);
		user_array = cJSON_CreateArray();
	}

	GROUP_INFO * g = get_group();

	cJSON * user = cJSON_CreateObject();
	cJSON_AddStringToObject(user, "username", username);
	cJSON_AddStringToObject(user, "groupname", g->group_name);
	cJSON_AddNumberToObject(user, "CPU time", 0);

	cJSON_AddItemToArray(user_array, user);

	FILE * fp = fopen(file_path, "w");
	if (fp) {
		char * json_string = cJSON_Print(user_array);
		fprintf(fp, "%s", json_string);
		free(json_string);
		fclose(fp);
	}

	cJSON_Delete(user_array);
}

void update_CPUt(char * username, double elapsed) {
	ConfigInfo * config = get_config_info();
	char file_path[200];
	sprintf(file_path, "%s/users.json", config->dir);

	cJSON * user_array = read_json(file_path);
	cJSON * user = NULL;

	cJSON_ArrayForEach(user, user_array) {
		cJSON * usern = cJSON_GetObjectItem(user, "username");
		cJSON * cputime = cJSON_GetObjectItem(user, "CPU time");

		if (strcmp(usern->valuestring, username) == 0) {
    		double new = cputime->valuedouble + elapsed;
			cJSON_ReplaceItemInObject(user, "CPU time", cJSON_CreateNumber(new));
			return;
		}
	}
	save_user(username, get_group()->group_name);
}

double get_CPU_time(char * username) {
	ConfigInfo * config = get_config_info();
	char file_path[200];
	sprintf(file_path, "%s/users.json", config->dir);

	cJSON * user_array = read_json(file_path);
	cJSON * user = NULL;

	cJSON_ArrayForEach(user, user_array) {
		cJSON * usern = cJSON_GetObjectItem(user, "username");
		cJSON * cputime = cJSON_GetObjectItem(user, "CPU time");

		if (strcmp(usern->valuestring, username) == 0) {
			return cputime->valuedouble;
		}
	}

	return 0.0;
}
