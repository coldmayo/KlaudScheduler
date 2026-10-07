#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include "../includes/types.h"
#include "../includes/utils.h"
#include "../includes/current_jobs.h"

static void usage(FILE *out) {
    fprintf(out, "Example Usage:\n");
    fprintf(out, "1. kcancel --id=<job id>\n");
    fprintf(out, "2. kcancel --id=<job id> --force\n");
    fprintf(out, "3. kcancel --help\n");
}

static void help(void) {
    printf("The kcancel command is used to cancel jobs already running or in the queue\n");
    printf("Synopsis\n\tkcancel --id=<job id> [--force] [--help]\n");
    printf("Description\n");
    printf("\t--id=job id\n\t\tThe job to cancel. Example: --id=1001\n");
    printf("\t--force\n\t\tSend SIGKILL instead of SIGTERM. Remote ranks may be left running.\n");
}

int main(int argc, char *argv[]) {
    long id = -1;
    bool force = false;

    if (argc < 2) { usage(stderr); return EXIT_FAILURE; }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            help();
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "--force") == 0) {
            force = true;
        } else if (strncmp(argv[i], "--id=", 5) == 0) {
            char *end;
            errno = 0;
            id = strtol(argv[i] + 5, &end, 10);
            if (errno || end == argv[i] + 5 || *end != '\0' || id <= 0 || id > INT_MAX) {
                fprintf(stderr, "Invalid job id: %s\n", argv[i] + 5);
                return EXIT_FAILURE;
            }
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            usage(stderr);
            return EXIT_FAILURE;
        }
    }

    if (id < 0) { usage(stderr); return EXIT_FAILURE; }

    if (cancel_job((int)id, force) == 0) {
        printf("Job %ld cancelled\n", id);
        return EXIT_SUCCESS;
    }
    fprintf(stderr, "Failed to cancel job %ld\n", id);
    return EXIT_FAILURE;
}
