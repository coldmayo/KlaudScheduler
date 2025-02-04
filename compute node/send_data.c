#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5000
#define BUFFER_SIZE 2048

// Get list of available CPU cores
void get_cpu_info(char *buffer, size_t size) {
    FILE *fp = popen("lscpu --parse=CPU,CORE,SOCKET | grep -v '^#'", "r");
    if (!fp) {
        snprintf(buffer, size, "0:");
        return;
    }

    char count_cmd[256];
    snprintf(count_cmd, sizeof(count_cmd), "lscpu --parse=CPU,CORE,SOCKET | grep -v '^#' | wc -l");
    FILE *count_fp = popen(count_cmd, "r");
    if (count_fp) {
        char num[32];
        if (fgets(num, sizeof(num), count_fp)) {
            int cpu_count = atoi(num);
            snprintf(buffer, size, "%d:", cpu_count);
        }
        pclose(count_fp);
    }

    // Add core list
    size_t current_len = strlen(buffer);
    char temp[64];
    while (fgets(temp, sizeof(temp), fp) && current_len < size - 1) {
        char *newline = strchr(temp, '\n');
        if (newline) *newline = '\0';

        int cpu, core, socket;
        if (sscanf(temp, "%d,%d,%d", &cpu, &core, &socket) == 3) {
            int remaining = size - current_len;
            int written = snprintf(buffer + current_len, remaining, "%d,", cpu);
            if (written > 0) current_len += written;
        }
    }
    pclose(fp);

    // Remove trailing comma if exists
    if (current_len > 0 && buffer[current_len - 1] == ',') {
        buffer[current_len - 1] = '-';
    }
}

// Get list of available GPU IDs
void get_gpu_info(char *buffer, size_t size) {
    size_t current_len = strlen(buffer);
    if (current_len >= size - 1) return;

    FILE *fp = popen("nvidia-smi --query-gpu=index --format=csv,noheader", "r");
    if (!fp) {
        strncat(buffer, "0", size - current_len - 1);
        return;
    }

    char count_cmd[256];
    snprintf(count_cmd, sizeof(count_cmd), "nvidia-smi --query-gpu=index --format=csv,noheader | wc -l");
    FILE *count_fp = popen(count_cmd, "r");
    if (count_fp) {
        char num[32];
        if (fgets(num, sizeof(num), count_fp)) {
            int gpu_count = atoi(num);
            char count_str[32];
            snprintf(count_str, sizeof(count_str), "%d:", gpu_count);
            strncat(buffer, count_str, size - current_len - 1);
        }
        pclose(count_fp);
    }

    // Then add GPU indices
    current_len = strlen(buffer);
    char temp[64];
    while (fgets(temp, sizeof(temp), fp) && current_len < size - 1) {
        char *newline = strchr(temp, '\n');
        if (newline) *newline = '\0';

        int remaining = size - current_len;
        int written = snprintf(buffer + current_len, remaining, "%s,", temp);
        if (written > 0) current_len += written;
    }
    pclose(fp);

    // Remove trailing comma if exists
    if (current_len > 0 && buffer[current_len - 1] == ',') {
        buffer[current_len - 1] = '\0';
    }
}

void handle_client(int client_socket) {
    char response[BUFFER_SIZE] = {0};
    get_cpu_info(response, sizeof(response));
    get_gpu_info(response, sizeof(response));
    send(client_socket, response, strlen(response), 0);
    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(server_fd);
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }
	printf("Listening...\n");
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket >= 0) {
            handle_client(client_socket);
        }
    }

    close(server_fd);
    return 0;
}
