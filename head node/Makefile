CC := gcc
CFLAGS := -Wall

SRC := src
INCLUDE := includes

GEN_NODES_SRCS := $(SRC)/nodes_list_main.c $(SRC)/nodes_list.c $(SRC)/cJSON.c
GEN_NODES_HDRS := $(INCLUDE)/nodes_list.h $(INCLUDE)/types.h
DISPATCH_SRCS := $(SRC)/dispatch.c $(SRC)/current_jobs.c $(SRC)/nodes_list.c $(SRC)/cJSON.c $(SRC)/utils.c $(SRC)/node_status.c
DISPATCH_HDRS := $(INCLUDE)/current_jobs.h $(INCLUDE)/nodes_list.h $(INCLUDE)/types.h
MAIN_SRCS := $(SRC)/main.c $(SRC)/current_jobs.c $(SRC)/cJSON.c $(SRC)/utils.c $(SRC)/klaud_files.c
MAIN_HDRS := $(INCLUDE)/current_jobs.h $(INCLUDE)/types.h $(INCLUDE)/utils.h $(INCLUDE)/klaud_files.h

all: gen_nodes_list dispatch main

gen_nodes_list: $(GEN_NODES_SRCS) $(GEN_NODES_HDRS)
	$(CC) $(CFLAGS) -o gen_nodes_list $(GEN_NODES_SRCS)

dispatch: $(DISPATCH_SRCS) $(DISPATCH_HDRS)
	$(CC) $(CFLAGS) -o dispatch $(DISPATCH_SRCS)

main: $(MAIN_SRCS) $(MAIN_HDRS)
	$(CC) $(CFLAGS) -o main $(MAIN_SRCS)

run_dispatch:
	./dispatch

run_nodes:
	./gen_nodes_list

debug:
	$(CC) -g -o dispatch $(DISPATCH_SRCS)
	gdb ./dispatch

clean:
	rm -f gen_nodes_list dispatch main
