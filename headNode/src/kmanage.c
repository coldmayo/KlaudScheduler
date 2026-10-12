#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ncurses.h>
#include "cJSON.h"
#include "../includes/utils.h"
#include "../includes/node_status.h"

void show_health(HEALTH * health, int index) {
    erase();

    mvprintw(0, 0, "Cluster Health");
    mvprintw(1, 0, "Use LEFT/RIGHT to navigate, q to quit");
    mvprintw(2, 0, "Nodes: %d", health->num_nodes);

    if (index >= 0 && index < health->num_nodes) {
        mvprintw(4, 0, "Node: %d / %d", index + 1, health->num_nodes);
        mvprintw(5, 0, "CPU Usage: %.2f%%", health->tot_core[index] * 100.0);

        mvprintw(6, 0, "RAM Usage: %.2f%%", health->RAM[index] * 100.0);

        for (int j = 0; j < MAX_CORES; j++) {
            mvprintw(8 + j, 0, "Core %d: %.2f%%", j, health->cores[index][j] * 100.0);
        }
    } else {
        mvprintw(4, 0, "No node health data available.");
    }

    refresh();
}

int main(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    timeout(2000);

    int curr_node = 0;
    bool running = true;

    while (running) {
        CLUSTERINFO *info = cluster_info();
        HEALTH *health = updateNodeHealth_TUI();

        if (info == NULL || health == NULL) {
            erase();
            mvprintw(0, 0, "Cluster Health");
            mvprintw(2, 0, "Unable to retrieve cluster health.");
            mvprintw(4, 0, "Press q to quit or wait to retry.");
            refresh();
        } else {
            int total_nodes = health->num_nodes;

            if (total_nodes > 0) {
                if (curr_node >= total_nodes) {
                    curr_node = 0;
                }

                show_health(health, curr_node);
            } else {
                erase();
                mvprintw(0, 0, "Cluster Health");
                mvprintw(2, 0, "No nodes available.");
                refresh();
            }
        }

        int ch = getch();

        switch (ch) {
            case KEY_RIGHT:
                if (health != NULL && health->num_nodes > 0) {
                    curr_node = (curr_node + 1) % health->num_nodes;
                }
                break;

            case KEY_LEFT:
                if (health != NULL && health->num_nodes > 0) {
                    curr_node =
                    (curr_node - 1 + health->num_nodes)
                    % health->num_nodes;
                }
                break;

            case 'q':
            case 'Q':
                running = false;
                break;

            default:
                break;
        }

        free(health);
        free(info);
    }

    endwin();
    return 0;
}
