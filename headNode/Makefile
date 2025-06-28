CC := gcc
CFLAGS := -Wall

SRC := src
INCLUDE := includes

GEN_NODES_SRCS := $(SRC)/nodes_list_main.c $(SRC)/nodes_list.c $(SRC)/cJSON.c $(SRC)/utils.c
GEN_NODES_HDRS := $(INCLUDE)/nodes_list.h $(INCLUDE)/types.h $(INCLUDE)/utils.h
DISPATCH_SRCS := $(SRC)/dispatch.c $(SRC)/current_jobs.c $(SRC)/nodes_list.c $(SRC)/cJSON.c $(SRC)/utils.c $(SRC)/node_status.c
DISPATCH_HDRS := $(INCLUDE)/current_jobs.h $(INCLUDE)/nodes_list.h $(INCLUDE)/types.h
MAIN_SRCS := $(SRC)/main.c $(SRC)/current_jobs.c $(SRC)/cJSON.c $(SRC)/utils.c $(SRC)/klaud_files.c
MAIN_HDRS := $(INCLUDE)/current_jobs.h $(INCLUDE)/types.h $(INCLUDE)/utils.h $(INCLUDE)/klaud_files.h
KTRACK_SRCS := $(SRC)/ktrack.c $(SRC)/utils.c $(SRC)/cJSON.c
KTRACK_HDRS := $(SRC)/cJSON.h $(INCLUDE)/utils.h

all: get-nodes dispatch klaudrun ktrack

get-nodes: $(GEN_NODES_SRCS) $(GEN_NODES_HDRS)
	$(CC) $(CFLAGS) -o get-nodes $(GEN_NODES_SRCS)
	@cp get-nodes ~/.local/bin/get-nodes

dispatch: $(DISPATCH_SRCS) $(DISPATCH_HDRS)
	$(CC) $(CFLAGS) -o dispatch $(DISPATCH_SRCS)
	@cp dispatch ~/.local/bin/dispatch

klaudrun: $(MAIN_SRCS) $(MAIN_HDRS)
	$(CC) $(CFLAGS) -o klaudrun $(MAIN_SRCS)
	@cp klaudrun ~/.local/bin/klaudrun
	
ktrack: $(KTRACK_SRCS) $(KTRACK_HDRS)
	$(CC) $(CFLAGS) -o ktrack $(KTRACK_SRCS)
	@cp ktrack ~/.local/bin/ktrack
global:
	@cp get-nodes ~/.local/bin/get-nodes
	@cp dispatch ~/.local/bin/dispatch
	@cp klaudrun ~/.local/bin/klaudrun
	@cp ktrack ~/.local/bin/ktrack
	
run_dispatch:
	./dispatch

run_nodes:
	./get-nodes

debug:
	$(CC) -g -o dispatch $(DISPATCH_SRCS)
	gdb ./dispatch

clean:
	rm -f get-nodes dispatch klaudrun
