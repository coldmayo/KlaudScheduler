compile:
	@gcc -o gen_nodes_list nodes_list_main.c nodes_list.c nodes_list.h -lcjson
	@gcc -o dispatch dispatch.c current_jobs.c nodes_list.c current_jobs.h nodes_list.h -lcjson
	@gcc -o main main.c current_jobs.c current_jobs.h -lcjson
compile_nodes_list:
	@gcc -o gen_nodes_list nodes_list.c -lcjson
compile_dispatcher:
	@gcc -o dispatch dispatch.c current_jobs.c nodes_list.c current_jobs.h nodes_list.h
compile_main:
	@gcc -o main main.c current_jobs.c current_jobs.h
run_dispach:
	@./dispatch
run_nodes:
	@./gen_nodes_list
