/*
 * Imprimer: Command-line interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "imprimer.h"
#include "conversions.h"
#include "sf_readline.h"
#include "helper.h"

/*****************PRINTER FUNCTIONS*********************/

int get_next_index_printers(){
	int i = 0;
	for(i = 0; i < MAX_PRINTERS; i++){
		if(printers[i] == NULL)
			return i;
	}
	return -1;
}

int find_printer(char * name, char * file_name){
	int i = 0;
	for(i = 0; i < MAX_PRINTERS; i++){
		if(printers[i] == NULL)
			return -1;
		else if(!strcmp(printers[i]->name, name) && !strcmp(printers[i]->type, file_name))
			return i;
	}
	return -1;
}

PRINTER * get_printer(char * name){
	int i = 0;
	for(i = 0; i < MAX_PRINTERS; i++){
		if(printers[i] == NULL)
			return NULL;
		else if(!strcmp(printers[i]->name, name))
			return printers[i];
	}

	return NULL;
}

int get_printer_index(char * name){
	int i = 0;
	for(i = 0; i < MAX_PRINTERS; i++){
		if(printers[i] == NULL)
			return -1;
		else if(!strcmp(printers[i]->name, name))
			return i;
	}

	return -1;
}

int create_printer(char * name, char * file_name){

	if(find_printer(name, file_name) == -1){
		FILE_TYPE * file = find_type(file_name);
		if(file == NULL){
			fprintf(stderr, "%s\n", "File type doesn't exist.");
			return -1;
		}
		int index = get_next_index_printers();
		PRINTER * new_printer = malloc(sizeof(PRINTER));
		PRINTER_STATUS status = PRINTER_DISABLED;
		new_printer->name = malloc(strlen(name)+1);
		new_printer->type = malloc(strlen(file_name)+1);
		strcpy(new_printer->name, name);
		strcpy(new_printer->type, file_name);
		new_printer->status = status;
		printers[index] = new_printer;
		sf_printer_defined(name, file_name);
		return index;
	} else {
		return -1;
	}
}

void free_printers(){
	int i;
	for(i = 0; i < MAX_PRINTERS; i++){
		if(printers[i] != NULL){
			free(printers[i]->type);
			free(printers[i]->name);
			free(printers[i]);
		}
	}
}

void print_printers(FILE * out){
	int i;
	for(i = 0; i < MAX_PRINTERS; i++){
		if(printers[i] != NULL){
			char * status;
			if(printers[i]->status == PRINTER_DISABLED){
				status = "disabled";
			} else if(printers[i]->status == PRINTER_IDLE){
				status = "idle";
			} else {
				status = "busy";
			}
			fprintf(out, "PRINTER: id=%d, name=%s, type=%s, status=%s\n", i,
				printers[i]->name, printers[i]->type, status);
		}
	}
}

PRINTER * enable_printer(char * name){
	PRINTER * printer = get_printer(name);
	if(printer == NULL)
		return NULL;
	else {
		printer->status = PRINTER_IDLE;
		return printer;
	}

}

PRINTER * disable_printer(char * name){
	PRINTER * printer = get_printer(name);
	if(printer == NULL)
		return NULL;
	else {
		printer->status = PRINTER_DISABLED;
		return printer;
	}
}

/************************ JOB FUNCTIONS ***************************/

int get_next_index_jobs(){
	int i = 0;
	for(i = 0; i < MAX_JOBS; i++){
		if(jobs[i] == NULL)
			return i;
	}
	return -1;
}
int create_job(char * file_name, char ** eligible_printers){

	    //do forking here

		FILE_TYPE * file = infer_file_type(file_name);
		if(file == NULL){
			fprintf(stderr, "%s\n", "File type doesn't exist.");
			return -1;
		}
		char * name = file->name;
		int index = get_next_index_jobs();
		if(index == -1){
			return -1;
		}
		JOB * job = malloc(sizeof(JOB));
		JOB_STATUS status = JOB_CREATED;
		job->type = malloc(strlen(name)+1);
		job->file_name = malloc(strlen(file_name)+1);
		strcpy(job->file_name, file_name);
		strcpy(job->type, name);
		job->index = index;
		job->status = status;
		int eligible = 0;
		//char ** printer_set = malloc(sizeof(char *) * MAX_PRINTERS);
		char * ptr = *eligible_printers;
		int i = 0;
		while(ptr != NULL){
			if(get_printer(ptr) != NULL){
				int index = get_printer_index(ptr);
				int number = 1;
				number = number << index;
				eligible = eligible | number;
			}
			i++;
			ptr = eligible_printers[i];
		}
		if(eligible == 0){
			eligible = 0xffffffff;
		}
		job->eligible = eligible;
		time_t rawtime;
   		struct tm *info;
   		time( &rawtime );
    	info = localtime( &rawtime );
    	job->creation = malloc(sizeof(struct tm));
    	memcpy(job->creation, info, sizeof(struct tm));
    	//job->creation = info;
		jobs[index] = job;
		sf_job_created(index, file_name, file->name);
		return index;
}

void free_jobs(){
	int i;
	for(i = 0; i < MAX_PRINTERS; i++){
		if(jobs[i] != NULL){
			free(jobs[i]->type);
			free(jobs[i]->file_name);
			free(jobs[i]->creation);
			free(jobs[i]);
		}
	}
}

int find_job(int job_number){
	if(jobs[job_number] != NULL)
		return 0;
	return -1;
}
int update_job_status(int job_number, char * status){

	if(!strcmp(status, "running"))
		jobs[job_number]->status = JOB_RUNNING;
	else if(!strcmp(status, "paused"))
		jobs[job_number]->status = JOB_PAUSED;
	else if(!strcmp(status, "finished"))
		jobs[job_number]->status = JOB_FINISHED;
	else if(!strcmp(status, "aborted"))
		jobs[job_number]->status = JOB_ABORTED;
	else if(!strcmp(status, "deleted"))
		jobs[job_number]->status = JOB_DELETED;
	else
		return -1;

	return 0;
}

void print_jobs(FILE * out){
	int i;
	for(i = 0; i < MAX_JOBS; i++){
		if(jobs[i] != NULL){
			char * status;
			if(jobs[i]->status == JOB_CREATED){
				status = "created";
			} else if(jobs[i]->status == JOB_RUNNING){
				status = "running";
			} else if(jobs[i]->status == JOB_PAUSED){
				status = "paused";
			} else if(jobs[i]->status == JOB_FINISHED){
				status = "finished";
			} else if(jobs[i]->status == JOB_ABORTED){
				status = "aborted";
			} else {
				status = "deleted";
			}

			time_t t = time(0);
			struct tm *ptr = localtime(&t);
			char * time = asctime(ptr);
			if (time[strlen(time)-1] == '\n') time[strlen(time)-1] = '\0';
			char * creation = asctime(jobs[i]->creation);
			if (creation[strlen(creation)-1] == '\n') creation[strlen(creation)-1] = '\0';

			fprintf(out, "JOB[%d]: type=%s, creation(%s), status(%s)=%s, eligible=%08x, file=%s\n", i,
				jobs[i]->type, creation, time, status, jobs[i]->eligible, jobs[i]->file_name);
		}
	}
}

PRINTER * get_eligible_printer(int eligible, char * file_name){
	FILE_TYPE * file = infer_file_type(file_name);
	char * from_type = file->name;
	int counter = 0;
	int ptr = 1;
	int i = 0;
	for(i = 0; i < MAX_PRINTERS; i++){
		if(printers[counter] != NULL){
			if((eligible & (ptr << counter)) == (ptr << counter)){
				if((printers[counter]->status == PRINTER_IDLE) && (find_conversion_path(from_type, printers[counter]->type) != NULL))
					return printers[counter];
				else
					counter++;
			} else {
				counter++;
			}
		}
	}

	return NULL;
}

int available_jobs(){
	int i = 0;
	int flag = -1;
	for(i = 0; i < MAX_JOBS; i++){
		if(jobs[i] == NULL){
			continue;
		} else{
			if(jobs[i]->status != JOB_FINISHED || jobs[i]->status != JOB_ABORTED){
				//if we find a job that canrun, we need to check if we can find an eligible printer
				PRINTER * printer = get_eligible_printer(jobs[i]->eligible, jobs[i]->file_name);
				if(printer != NULL){
					CONVERSION ** conversions = find_conversion_path(jobs[i]->type, printer->type);
					jobs[i]->printer = printer;
					jobs[i]->conversions = conversions;
					//start_job(jobs[i]);
					flag = 0;
					return flag;
				}
			}
		}
	}
	return flag;
}

int start_job(JOB * job){
	int status;
	//printf("var: %d\n", var);
	//sf_job_started(job->index, job->printer->name, )
	//sf_printer_status(job->printer->name, PRINTER_BUSY);
	//sigset_t mask_all, mask_one, prev_one;
	//sigfillset(&mask_all);
	//sigemptyset(&mask_one);
	//sigaddset(&mask_one, SIGINT);
	//sigprocmask(SIG_BLOCK, &mask_one, &prev_one);
	//printf("cptr: %s\n", *(*(job->conversions))->cmd_and_args);

	//blocking sig int

	int pid = fork();

	if(pid == 0){
		//sigprocmask(SIG_SETMASK, &prev_one, NULL);
		//set sigchld to sig deafault and unhooks the handler
		sf_printer_status(job->printer->name, PRINTER_BUSY);
		sf_job_status(job->index, JOB_RUNNING);
		job->pgid = pid;
		setpgid(pid, pid); //sets master pid to process group
		CONVERSION * cptr =*(job->conversions);
		//check status of job --> finished or aborted

		//let's see how many pipes we need
		int length = 0;
		CONVERSION * ptr = *(job->conversions);
		while(ptr != NULL){
			length += 1;
			ptr = (job->conversions)[length];
		}

		int num = 0;
		char *** arr = malloc(sizeof(char *)*(length+1));
		while(num < length){
			arr[num] = (cptr->cmd_and_args);
			num++;
			cptr = (job->conversions)[num];
		}
		arr[num] = NULL;

		cptr =*(job->conversions);
		int pipe_num = num-1;

		if(cptr == NULL){ // no pipe
			int pchild = fork();
			if(pchild == 0){
				//need to ask about this
				char * args[3] = {"/bin/cat", job->file_name, NULL};
				int var = imp_connect_to_printer(job->printer->name, job->printer->type, PRINTER_NORMAL);
				if(var == -1){
					return -1;//unsuccessful so return -1
				}
				//printf("var: %d\n", var);
				dup2(var, STDOUT_FILENO);
				dup2(var, STDERR_FILENO);
				close(var);
				int result = execvp(args[0], args);
				if(result < 0){
					fprintf(stderr, "errno: %d\n", errno);
					exit(-1);
				}
				/*Child doesn't reach here because execvp shouldn't return*/
			}

			sf_job_started(job->index, job->printer->name, job->pgid, cptr->cmd_and_args);

			while(waitpid(pchild, &status, WNOHANG) != pchild);
			free(arr);

		} else if (num == 1){ //no pipe

			int new_pgid1 = fork();
			if(new_pgid1 == 0){ //redirect output
				int var1 = open(job->file_name, O_RDONLY);
				dup2(var1, STDIN_FILENO);
				close(var1);
				int var = imp_connect_to_printer(job->printer->name, job->printer->type, PRINTER_NORMAL);
				if(var == -1){
					return -1;//unsuccessful so return -1
				}
				dup2(var, STDOUT_FILENO);
				close(var);
				int result = execvp((cptr->cmd_and_args)[0], cptr->cmd_and_args);
				if(result < 0){
					fprintf(stderr, "errno: %d\n", errno);
					exit(-1);
				}
			}

			while(waitpid(job->pgid, &status, WNOHANG) != job->pgid);
			free(arr);

		} else {
			int i = 0;
			int pipefds[pipe_num][2];
			sf_job_started(job->index, job->printer->name, job->pgid, arr[i]);

			for(i = 0; i < num; i++){
				//sf_job_started(job->index, job->printer->name, job->pgid, arr[i]);
				cptr = (job->conversions)[i];
				//int pipefds[pipe_num][2];
				if(i < num-1){
					if(pipe(pipefds[i]) < 0) {
						perror("Couldn't pipe.");
						exit(-1); // want to kill the children
					}
				}

				int new_pgid = fork();
				if(new_pgid == 0){
					if(i > 0) close(pipefds[i-1][1]);
					if(i != num - 1) close(pipefds[i][0]);
					int read = -1;
					int write = -1;

					/* first stage */
					if(i == 0){
						read = open(job->file_name, O_RDONLY);
					} else {
						read = pipefds[i - 1][0];
					}

						/*last stage */
						if(i == num - 1){
							int var = imp_connect_to_printer(job->printer->name, job->printer->type, PRINTER_NORMAL);
							if(var == -1){
								return -1;//unsuccessful so return -1
							}
							write = var;
						} else {
							write = pipefds[i][1];
						}

						dup2(read, STDIN_FILENO);
						close(read);
						dup2(write, STDOUT_FILENO);
						close(write);
						int result = execvp(cptr->cmd_and_args[0], cptr->cmd_and_args);
						if(result < 0){
							fprintf(stderr, "errno: %d\n", errno);
							exit(-1);
						}
				}
				if(i > 0){
					//close(pipefds[i-1][0]);
					close(pipefds[i-1][1]);
				}
				//close(pipefds[i][0]);
				//close(pipefds[i][1]);

			}
			while(i--){
				waitpid(job->pgid, &status, WNOHANG);
			}

			// int j = 0;
			// for(j = 0; j < num; j++){
			// 	close(pipefds[j][0]);
			// 	close(pipefds[j][1]);
			// }
			//close(var);
			//need an if statement before we job start function and free it
			//sf_job_started(job->index, job->printer->name, job->pgid, arr[0]);
			free(arr);
			//sf_job_started(job->index, job->printer->name, job->pgid, arr[0]);

		}
		exit(0);
	} else if(pid < 0){
    	perror("Forking could not be done. Error\n");
    }
	//sf_job_started(job->id, job->printer, )
    while(waitpid(-1, &status, WNOHANG) != pid);

    if(pid > 0){ // if back in main process and everything is cool:
    	//sf_job_started(job->index, job->printer->name, job->pgid, (*(job->conversions))->cmd_and_args);
    	sf_job_status(job->index, JOB_FINISHED);
    	update_job_status(job->index, "finished");
		sf_job_finished(job->index, JOB_FINISHED);
		sf_job_deleted(job->index);
		update_job_status(job->index, "deleted");
		sf_printer_status(job->printer->name, PRINTER_IDLE);
		//close(var);
		//free(arr);

    }
    //free()

	return 0;
}

int parse_arguments(FILE * in, char * line, FILE * out){

	if(in != stdin){
		if(line == NULL)
			return 0;
	}

	char * params = strtok(line, " ");

		if(*line=='\0'){
    		//free(input);
    		//input = sf_readline(PROMPT);
    		printf("um\n");
    		return 2;
    	}

		if((!strcmp(params, "help")) || (!strcmp(params, "help\n"))){
		   fprintf(out, HELP);
		   fflush(out);
		   sf_cmd_ok();
		   return 0;
    	}

    	if((!strcmp(params, "quit")) || (!strcmp(params, "quit\n"))){
    		//printf("hello\n");
    		sf_cmd_ok();
    		return 1;
    	}

    	if(!strcmp(params, "type")){
    		char * word = strtok(NULL, " ");
    		//printf("word: %s\n", word);
    		//printf("word[len - 1]: %c\n", word[strlen(word)-1]);
    		if(word == NULL){
    			sf_cmd_error("Too little arguments for type command.");
    			return 0;
    		}
    		int len = strlen(word);
    		if(word[len - 1] == '\n'){
    			//printf("here");
    			word[len - 1] = 0;
    		}
    		char * extra = strtok(NULL, " ");
    		if(extra != NULL){
    			sf_cmd_error("Too many arguments for type command.");
    			return 0;
    		} else {
    			FILE_TYPE * file = find_type(word);
    			if(file == NULL){
    				file = define_type(word);
    				if(file == NULL){
    					sf_cmd_error("File type could not be made.");
    					return 0;
    				}
    			}
    		}
    		sf_cmd_ok();
    		return 0;
    	}

    	if(!strcmp(params, "printer")){
    		char * name = strtok(NULL, " ");
    		if(name == NULL){
    			sf_cmd_error("Too little arguments for printer command.");
    			return 0;
    		}
    		int len = strlen(name);
    		if(name[len - 1] == '\n')
    			name[len - 1] = 0;
    		char * file = strtok(NULL, " ");
    		if(file == NULL){
    			sf_cmd_error("Too little arguments for printer command.");
    			return 0;
    		}
    		len = strlen(file);
    		if(file[len - 1] == '\n')
    			file[len - 1] = 0;
    		char * extra = strtok(NULL, " ");
    		if(extra != NULL){
    			sf_cmd_error("Too many arguments for printer command.");
    			return 0;
    		} else {
    			int code = create_printer(name, file);
    			if(code == -1){
    				sf_cmd_error("Printer could not be made.");
    				return 0;
    			}
    		}
    		sf_cmd_ok();
    		return 0;
    	}

    	if(!strcmp(params, "conversion")){
    		char * file1 = strtok(NULL, " ");
    		if(file1 == NULL){
    			sf_cmd_error("Too little arguments for conversion command.");
    			return 0;
    		}
    		int len = strlen(file1);
    		if(file1[len - 1] == '\n')
    			file1[len - 1] = 0;
    		char * file2 = strtok(NULL, " ");
    		if(file2 == NULL){
    			sf_cmd_error("Too little arguments for conversion command.");
    			return 0;
    		}
    		len = strlen(file2);
    		if(file2[len - 1] == '\n')
    			file2[len - 1] = 0;
    		char * program = strtok(NULL, " ");
    		if(program == NULL){
    			sf_cmd_error("Too little arguments for conversion command.");
    			return 0;
    		}
    		len = strlen(program);
    		if(program[len - 1] == '\n')
    			program[len - 1] = 0;
    		int counter = 1;
    		char ** arguments = malloc(counter * sizeof(char *));
    		arguments[counter-1] = program;
    		char * optarg = strtok(NULL, " ");
    		while(optarg != NULL){
    			counter += 1;
    			arguments = realloc(arguments, sizeof(char *) * counter);
    			arguments[counter-1]=optarg;
    			optarg = strtok(NULL, " ");
    		}
    		arguments = realloc(arguments, sizeof(char *) * (counter+1));
    		arguments[counter] = '\0';
    		CONVERSION * new_conversion = define_conversion(file1, file2, arguments);
    		if(new_conversion == NULL){
    			sf_cmd_error("Could not create new conversion.");
    			free(arguments);
    			return 0;
    		}
    		free(arguments);
    		sf_cmd_ok();
    		return 0;
    	}

    	if(!strcmp(params, "printers")){
    		char * extra = strtok(NULL, " ");
    		if(extra != NULL){
    			sf_cmd_error("Too many arguments for printers command.");
    		}
    		print_printers(out);
    		sf_cmd_ok();
    		return 0;
    	}

    	if(!strcmp(params, "jobs")){
    		char * extra = strtok(NULL, " ");
    		if(extra != NULL){
    			sf_cmd_error("Too many arguments for jobs command.");
    			return 0;
    		}
    		print_jobs(out);
    		sf_cmd_ok();
    		return 0;
    	}

    	/* Spooling commands */
    	if(!strcmp(params, "print")){

    		char * file_name = strtok(NULL, " ");
    		if(file_name == NULL){
    			sf_cmd_error("Too little arguments for print command.");
    			return 0;
    		}
    		int len = strlen(file_name);
    		if(file_name[len - 1] == '\n')
    			file_name[len - 1] = 0;
    		char * printer = strtok(NULL, " ");
    		int counter = 1;
    		char ** eligible_printers = malloc(counter * sizeof(char *));
    		if(printer == NULL){
    			// all printers are eligible
    			eligible_printers[counter-1] = NULL;
    		} else {
    			len = strlen(printer);
    			if(printer[len - 1] == '\n')
    			printer[len - 1] = 0;
    			eligible_printers[counter-1] = printer;
    		}
    		char * optarg = strtok(NULL, " ");
    		while(optarg != NULL){
    			counter += 1;
    			eligible_printers = realloc(eligible_printers, sizeof(char *) * counter);
    			eligible_printers[counter-1]=optarg;
    			optarg = strtok(NULL, " ");
    		}
    		eligible_printers = realloc(eligible_printers, sizeof(char *) * (counter+1));
    		eligible_printers[counter] = NULL;
    		int index = create_job(file_name, eligible_printers);
    		free(eligible_printers);
    		sf_cmd_ok();
    		JOB * job = jobs[index];
    		int status = available_jobs();
    		//printf("status: %d\n", status);
    		if(status != -1){
    			start_job(job);
    			//printf("x: %d\n", x);
    		}

    		return 0;
    	}

    	if(!strcmp(params, "cancel")){
    		char * job_number = strtok(NULL, " ");
    		if(job_number == NULL){
    			sf_cmd_error("Too little arguments for cancel command.");
    			return 0;
    		}
    		char * extra = strtok(NULL, " ");
    		if(extra != NULL){
    			sf_cmd_error("Too many arguments for cancel command.");
    			return 0;
    		}
    		//signal(SIGTERM, SIGCHLDHandler);
    		update_job_status(atoi(job_number), "aborted"); //not exactly this
    		sf_cmd_ok();
    		return 0;
    	}

    	if(!strcmp(params, "pause")){
    		char * job_number = strtok(NULL, " ");
    		if(job_number == NULL){
    			sf_cmd_error("Too little arguments for pause command.");
    			return 0;
    		}
    		char * extra = strtok(NULL, " ");
    		if(extra != NULL){
    			sf_cmd_error("Too many arguments for pause command.");
    		}
    		update_job_status(atoi(job_number), "paused"); //not exactly this
    		sf_cmd_ok();
    		return 0;
    	}

    	if(!strcmp(params, "resume")){
    		char * job_number = strtok(NULL, " ");
    		if(job_number == NULL){
    			sf_cmd_error("Too little arguments for resume command.");
    			return 0;
    		}
    		char * extra = strtok(NULL, " ");
    		if(extra != NULL){
    			sf_cmd_error("Too many arguments for resume command.");
    			return 0;
    		}
    		sf_cmd_ok();
    		return 0;
    	}

    	if(!strcmp(params, "disable")){
    		char * printer_name = strtok(NULL, " ");
    		if(printer_name == NULL){
    			sf_cmd_error("Too little arguments for disable command.");
    			return 0;
    		} else {
    			int len = strlen(printer_name);
    			if(printer_name[len - 1] == '\n')
    				printer_name[len - 1] = 0;
    		}
    		char * extra = strtok(NULL, " ");
    		if(extra != NULL){
    			sf_cmd_error("Too many arguments for disable command.");
    			return 0;
    		}
    		PRINTER * printer = disable_printer(printer_name);
    		if(printer == NULL){
    			sf_cmd_error("Printer was not able to be disabled.");
    			return 0;
    		}
    		sf_printer_status(printer->name, printer->status);
    		sf_cmd_ok();
    		return 0;
    	}

    	if(!strcmp(params, "enable")){
    		char * printer_name = strtok(NULL, " ");
    		if(printer_name == NULL){
    			sf_cmd_error("Too little arguments for enable command.");
    			return 0;
    		} else {
    			int len = strlen(printer_name);
    			if(printer_name[len - 1] == '\n')
    				printer_name[len - 1] = 0;
    		}
    		char * extra = strtok(NULL, " ");
    		if(extra != NULL){
    			sf_cmd_error("Too many arguments for enable command.");
    			return 0;
    		}
    		PRINTER * printer = enable_printer(printer_name);
    		if(printer == NULL){
    			sf_cmd_error("Printer was not able to be enabled.");
    			return 0;
    		}
    		sf_printer_status(printer->name, printer->status);
    		sf_cmd_ok();
    		int status = available_jobs();
    		if(status != -1){
    			int job = start_job(jobs[status]);
    			if(job == 0)
    				return 0;
    		}
    		return 0;
    	}

    return -1;
}