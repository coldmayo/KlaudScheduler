#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include "cJSON.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include "../includes/types.h"
#include "../includes/utils.h"

#define PORT 5000

// update availability of the cores for each node
void update_status(int core, char * host) {
    if (!host) return;  // Guard against NULL host

    FILE * fp = fopen("nodes.json", "r+");
    if (!fp) {
        printf("Failed to open nodes.json");
        fflush(stdout);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size <= 0) {
        fclose(fp);
        return;
    }

    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        fclose(fp);
        return;
    }

    if (fread(buffer, 1, file_size, fp) != file_size) {
        free(buffer);
        fclose(fp);
        return;
    }
    buffer[file_size] = '\0';

    cJSON * node_array = cJSON_Parse(buffer);
    free(buffer);
    fclose(fp);

    if (!node_array) {
        fprintf(stderr, "Failed to parse nodes.json\n");
        return;
    }

    char core_id[10];
    snprintf(core_id, sizeof(core_id), "%d", core);

    cJSON * node = NULL;
    cJSON_ArrayForEach(node, node_array) {
        cJSON * hostn = cJSON_GetObjectItem(node, "hostname");
        if (!hostn || !hostn->valuestring) continue;

        if (strcmp(hostn->valuestring, host) != 0) continue;

        cJSON * cpus = cJSON_GetObjectItem(node, "cpus");
        if (!cpus) continue;

        cJSON * cpu_info = NULL;
        cJSON_ArrayForEach(cpu_info, cpus) {
            cJSON * core_json = cJSON_GetObjectItem(cpu_info, "core #");
            cJSON * status = cJSON_GetObjectItem(cpu_info, "avail");

            if (!core_json || !status ||
                !core_json->valuestring || !status->valuestring) continue;

            if (strcmp(core_json->valuestring, core_id) == 0) {
                const char *new_status =
                    (strcmp(status->valuestring, "FREE") == 0) ?
                    "OCCUPIED" : "FREE";
                cJSON_ReplaceItemInObject(cpu_info, "avail",
                                        cJSON_CreateString(new_status));
                break;
            }
        }
    }

    fp = fopen("nodes.json", "w");
    if (fp) {
        char *json_string = cJSON_Print(node_array);
        if (json_string) {
            fprintf(fp, "%s", json_string);
            free(json_string);
        }
        fclose(fp);
    }

    cJSON_Delete(node_array);
}

// Parse the resource string into a ResourceInfo structure
ResourceInfo parse_resource_info(char *str) {
    ResourceInfo info = {0};
    char *count_str = strtok(str, ":");
    if (count_str) {
        info.count = atoi(count_str);
        char *cores_str = strtok(NULL, "-");
        if (cores_str) {
            char *core = strtok(cores_str, ",");
            int i = 0;
            while (core && i < MAX_CORES) {
                info.cores[i++] = atoi(core);
                core = strtok(NULL, ",");
            }
        }
    }
    return info;
}

char *from_node(const char *ip) {
    int sock;
    struct sockaddr_in server_addr;
    static char buffer[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        printf("Invalid address for %s\n", ip);
        close(sock);
        return NULL;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
        } else {
            printf("No data received from %s\n", ip);
            close(sock);
            return NULL;
        }
    } else {
        printf("Failed to connect to %s\n", ip);
        close(sock);
        return NULL;
    }

    close(sock);
    return buffer;
}

char * from_node_ssh(const char * ip) {

    // For CPUs
	char cmd[400];
	char ret[100];
	sprintf(cmd, "ssh master@%s \"cat /proc/stat\" > .stat.txt", ip);
	system(cmd);

	NODEINFO * info = malloc(sizeof(NODEINFO));
	memset(info, 0, sizeof(NODEINFO));
	char * conts = read_file(".stat.txt");

    char *ptr = conts;
    int cnt = 0;
    int ids[40];
    while ((ptr = strstr(ptr, "cpu")) != NULL) {
        int core, user, nice, system, idle, iowait, irq, softirq;
        if (sscanf(ptr, "cpu%d %d %d %d %d %d %d %d", &core, &user, &nice, &system, &idle, &iowait, &irq, &softirq) == 8) {
            ids[cnt] = core;
            cnt++;
        }
        ptr++;
    }

    sprintf(ret, "%d:", cnt);
    int i;
    for (i = 0; cnt > i; i++) {
        char str[20];
        sprintf(str, "%d,", ids[i]);
		strcat(ret, str);
    }

	int len = strlen(ret);
	if (len > 0 && ret[len - 1] == ',') {
		ret[len - 1] = '-';
	}

    info->num_cores = cnt;
    free(conts);

    // Do GPUs later

	remove(".stat.txt");
	return strdup(ret);
}

void save_to_rankfile(const ResourceInfo *cpu_info, const ResourceInfo *gpu_info, const char *hostname) {
    FILE *file = fopen("rankfile.txt", "a");
    if (!file) {
        perror("Failed to open rankfile.txt");
        return;
    }

    // Write CPU assignments
    for (int i = 0; i < cpu_info->count; i++) {
        fprintf(file, "rank %d=%s slot=%d\n", i, hostname, cpu_info->cores[i]);
    }

    // Write GPU assignments
    int rank = cpu_info->count;
    for (int i = 0; i < gpu_info->count; i++) {
        fprintf(file, "rank %d=%s gpu=%d:0\n", rank + i, hostname, gpu_info->cores[i]);
    }

    fclose(file);
}


cJSON * check_nodes(const char * ip, bool tcp) {
    char * info;
    if (tcp) {
		info = from_node(ip);
    } else {
		info = from_node_ssh(ip);
    }
    
    if (!info) {
        printf("Skipping %s due to connection failure.\n", ip);
        return NULL;
    }

    char *cpu_str = strtok(info, "-");
    char *gpu_str = strtok(NULL, "-");

    if (!cpu_str || !gpu_str) {
        printf("Invalid format received from %s\n", ip);
        return NULL;
    }

    ResourceInfo cpu_info = parse_resource_info(cpu_str);
    ResourceInfo gpu_info = parse_resource_info(gpu_str);

    // Save to rankfile
    save_to_rankfile(&cpu_info, &gpu_info, ip);

    // Create JSON object
    cJSON *node = cJSON_CreateObject();
    if (!node) return NULL;

    cJSON_AddStringToObject(node, "hostname", ip);

    // Add CPU info
    cJSON *cpus = cJSON_CreateObject();

    for (int i = 0; i < cpu_info.count; i++) {
		cJSON * cpu = cJSON_CreateObject();
        char key[16];
        char corenum[16];
        sprintf(key, "cpu%d", i);
        sprintf(corenum, "%d", cpu_info.cores[i]);
        cJSON_AddStringToObject(cpu, "avail", "FREE");
        cJSON_AddStringToObject(cpu, "core #", corenum);
		cJSON_AddItemToObject(cpus, key, cpu);
    }
    cJSON_AddItemToObject(node, "cpus", cpus);

    // Add GPU info
    cJSON *gpus = cJSON_CreateObject();
    for (int i = 0; i < gpu_info.count; i++) {
        cJSON * gpu = cJSON_CreateObject();
        char key[16];
        char corenum[16];
        sprintf(key, "gpu%d", i);
        sprintf(corenum, "%d", gpu_info.cores[i]);
        cJSON_AddStringToObject(gpu, "avail", "FREE");
        cJSON_AddStringToObject(gpu, "core #", corenum);
		cJSON_AddItemToObject(gpus, key, gpu);
    }
    cJSON_AddItemToObject(node, "gpus", gpus);

    return node;
}
