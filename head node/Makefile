compile:
	@gcc -o gen_nodes_list nodes_list_main.c nodes_list.c cJSON.c nodes_list.h
	@gcc -o dispatch dispatch.c current_jobs.c nodes_list.c cJSON.c current_jobs.h nodes_list.h
	@gcc -o main main.c current_jobs.c current_jobs.h cJSON.c
compile_nodes_list:
	@gcc -o gen_nodes_list nodes_list.c cJSON.c
compile_dispatcher:
	@gcc -o dispatch dispatch.c current_jobs.c nodes_list.c current_jobs.h nodes_list.h cJSON.c
compile_main:
	@gcc -o main main.c current_jobs.c current_jobs.h
run_dispach:
	@./dispatch
debug:
	@gcc -g dispatch dispatch.c current_jobs.c nodes_list.c current_jobs.h nodes_list.h cJSON.c
	@gdb ./dispatch
run_nodes:
	@./gen_nodes_list
