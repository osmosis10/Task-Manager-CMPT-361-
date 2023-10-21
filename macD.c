/************************************************************/
/*	Name: Moses Lemma
	Date: 2023/04/13
	Class: CMPT 360
	Instructor: Cam Macdonell
*/
/************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include <signal.h>
#include <time.h>


#define SOCK_PATH  "macd.socket"
//#define DATA "KILL"


typedef struct Data {
			int num_p;
			int *pids;
		} Data;


typedef struct Client {
		int num_p;
		int *pids;
		int client;
	} Client;

int SIGNAL = 5;
//Handler makes flag 1
void handler(int sig)
{
    SIGNAL = 1;
}

int sock_errno() {
    return 0;
}



void *socket_accept(Client *acceptance) {
	char buf[256];
	char *ret;
	int bytes_rec = 0;
	int rc;
	int client_sock = acceptance->client;
	int num_proc = acceptance->num_p;
	int *all_pids = acceptance->pids;
	int killed;
	
	bytes_rec = recv(client_sock, buf, 256, 0);
	
    if (bytes_rec == -1){
        printf("RECV ERROR: %d\n", sock_errno());
        close(client_sock);
        exit(1);
    }
    else {	
		char *message;
		
		//KILL
		if ((message = strstr(buf, "KILL"))) {		
			printf("In kill\n", buf);
			int int_vals[10];
			recv(client_sock, int_vals, 10, 0);
			killed = kill(acceptance->pids[int_vals[0]], SIGKILL);
			if (killed == -1) {
				printf("In kill\n");
				send(client_sock, "FAIL", 5, 0);
			}
			else {
				printf("in fai;\n");
				send(client_sock, "SUCC", 5, 0);
			}
		}
		//STAT
		else if ((message = strstr(buf, "STAT"))) {
			printf("Sending data...\n");
			printf("SENT %d\n", num_proc);
   			rc = send(client_sock, &num_proc, sizeof(int), 0);
  
		
		}
		//FAILURE
		else {
				memset(buf, 0, 256);
				strcpy(buf, "FAIL");
				printf("SENT %s\n", buf);
				rc = send(client_sock, "FAIL" , 4 * sizeof(char), 0);
				pthread_exit(ret);
				
		}	

        printf("DATA RECEIVED = %s (%d)\n", buf, bytes_rec);

	}
	pthread_exit(ret);
}

void *recieve(Data *info) {

    int server_sock, client_sock, rc;
    unsigned int len;
    int bytes_rec = 0;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;     
    char buf[256];
	char *ret;
    int backlog = 10;


	pthread_t *all_clients = (pthread_t *) malloc(10 * sizeof(pthread_t));
	int clt = 0;

    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(buf, 0, 256);                
	
	/**************************************/
    /* Create a UNIX domain stream socket */
    /**************************************/
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1){
        printf("SOCKET ERROR: %d\n", sock_errno());
        exit(1);
    }

    /***************************************/
    /* Set up the UNIX sockaddr structure  */
    /* by using AF_UNIX for the family and */
    /* giving it a filepath to bind to.    */
    /*                                     */
    /* Unlink the file so the bind will    */
    /* succeed, then bind to that file.    */
    /***************************************/
    server_sockaddr.sun_family = AF_UNIX;   
    strcpy(server_sockaddr.sun_path, SOCK_PATH); 
    len = sizeof(server_sockaddr);

    unlink(SOCK_PATH);
    rc = bind(server_sock, (struct sockaddr *) &server_sockaddr, len);
    if (rc == -1){
        printf("BIND ERROR: %d\n", sock_errno());
        close(server_sock);
        exit(1);
    }
	
    /*********************************/
    /* Listen for any client sockets */
    /*********************************/

    rc = listen(server_sock, backlog);
	
	while (1) {
		client_sock = accept(server_sock, (struct sockaddr *) &client_sockaddr, &len);
		Client *acc_data = (Client *) malloc(sizeof(Client));
		acc_data->num_p = info->num_p;
		acc_data->pids  = info->pids;
		acc_data->client = client_sock;

		pthread_create(&all_clients[clt], NULL, socket_accept, acc_data);
		clt++;

	}
	//pthread_join(all_clients, NULL);
	close(server_sock);
	close(client_sock);
	
	pthread_exit(ret);
}
	
	

//Calculates cpu usage
int calc(unsigned long utime, unsigned long stime, unsigned long long starttime)
{
        FILE *fp;
        char *buffer;
        char *token;
        size_t b_size = 50;
        int total;
        int sec;
        int cpu_use;
        int hertz;
        int uptime;

        fp = fopen("/proc/uptime", "r");
        buffer = (char *) malloc(b_size*sizeof(char));
        getline(&buffer, &b_size, fp);

        token = strtok(buffer, " ");
        uptime = atoi(token);

        hertz = sysconf(_SC_CLK_TCK);
        total = utime + stime;
        sec = uptime - (starttime/hertz);
        //printf("hertz: %d\ntotal: %d\nsec: %d\n", hertz, total, sec);
        cpu_use = (int)(100 * ((double)(total/hertz) / sec));
        free(buffer);
        return cpu_use;
}

//Parses and finds relevant cpu info
int proc_cpu(FILE *p)
{
        char *buffer;
        size_t b_size = 50;
        char *token;
        int counter = 1;
        unsigned long utime;
        unsigned long stime;
        unsigned long long starttime;
        int usage;

        buffer = (char *) malloc(b_size*sizeof(char));
        getline(&buffer, &b_size, p);

        token = strtok(buffer, " ");
        while (token != NULL) {

                if (counter == 14)
                        utime = atoi(token);

                if (counter == 15)
                        stime = atoi(token);

                if (counter == 22)
                        starttime = atoi(token);

                token = strtok(NULL, " ");
                counter++;
        }
        usage = calc(utime, stime, starttime);
        if (usage < 0)
                return 0;

        free(buffer);
        return usage;
}

int proc_mem(FILE *p)
{
        char *status_line = (char *) malloc(CHAR_MAX); //To store each line from the status file
        char *str_mem; //To store the string of the Vmem
        int mem = 0; //TO store the int of the Vem in KB
        int final_mem = 0; //To store the Vmem in MB
        //Loop goes through each line
        while (fgets(status_line, 100, p) != NULL) {
                        if (strstr(status_line, "VmSize:")) {
                                str_mem = strtok(status_line, "\t"); //Delimits the first tab
                                str_mem = strtok(NULL, " "); //Delimits by space to get the Vmem
                                mem = atoi(str_mem);
                                final_mem = mem / 1000;
                        }

        }
        free(status_line);
        return final_mem;
}

//Reads all lines and returns array of lines
char **line_reader(FILE *conf_file, char **lines, int *process, int *runtime)
{
        int i = 0;
        //char **lines = (char **) malloc(CHAR_MAX * sizeof(char *));
        char *buffer;
        char *limit;
        size_t b_len = 50;
        size_t chars;

        buffer = (char *) malloc(b_len * sizeof(char));
        chars = getline(&buffer, &b_len, conf_file);
        if (chars == -1) {
                lines[0] = (char *) malloc(10 * sizeof(char));
                strcpy(lines[0], "null");
                free(buffer);
                return lines;
        }
        limit = strstr(buffer, "limit");

        if (limit != NULL) {
                limit += 6;
                (*runtime) = atoi(limit);
        } else {
                lines[i] = (char *) malloc(200 * sizeof(char));
                strcpy(lines[i], buffer);
                i++;
                (*process)++;
        }


        while ((chars = getline(&buffer, &b_len, conf_file)) != -1) {
                buffer[strcspn(buffer, "\n")] = '\0';
                lines[i] = (char *) malloc(200 * sizeof(char));
                strcpy(lines[i], buffer);
                i++;
                (*process)++;
        }
        free(buffer);
        return lines;
}

//Returns an array of tokens
char **tokenize(char *lines)
{
        char *token;
  		char **proc_array = (char **) malloc(50 * sizeof(char *));
        int i = 0;

        token = strtok(lines, " ");

        while (token != NULL) {
                proc_array[i] = (char *) malloc(60*sizeof(char));
                strcpy(proc_array[i], token); //I believe this is where it crashes
				printf("token: %s\n", token);
                i++;
                token = strtok(NULL, " ");
        }
        proc_array[i] = NULL;
        //printf("INSIDE TOKENIZE\n");
        return proc_array;
}

int main(int argc, char *argv[])
{
	int op;
	char *conf_path = (char *) malloc(51 * sizeof(char));
	char *flag = (char *) malloc(5);
	FILE *fp;

	int quiet = 0;
	int status;
	int childs = 0;
	int wp;
	int p = 0;
	int *p_count = &p;
	int r = 0;
	int *run_time = &r;

	char *time = __TIME__;
	char *date = __DATE__;

	while ((op = getopt(argc, argv, "i:qo:")) != -1) {
		switch(op) {
		case 'i':
			strncpy(flag, argv[1], 5);
			strncpy(conf_path, optarg, 50);
			break;
		case 'q':
			quiet = 1;
			break;
		case 'o':
			freopen(optarg, "w", stdout);
			break;
		}
	}

	fp = fopen(conf_path, "r");
		
	if (fp == NULL) {
		printf("macD: %s not found\n", conf_path);
		free(conf_path);
		free(flag);
		return 1;
	}
	char **lines = (char **) malloc(CHAR_MAX * sizeof(char *));
	
	lines = line_reader(fp, lines, p_count, run_time);
	//If file is empty
	if (strstr(lines[0], "null")) {
		printf("Error empty file: %s\n", conf_path);
		free(conf_path);
		free(flag);
		free(lines[0]);
		free(lines);
		return 1;
	}
	char **all_args;
	char **str_cpids = (char **) malloc(20 * sizeof(char *)); //To store child PID
	int running;
		
	printf("Normal report, %s, %s\n", time, date);
	//LOOP FOR RUNNING PROCCESS'
   
	while (p != 0) {
		int fail = 0;
		int rc = fork();
		//If fork fails
			if (rc < 0) {
				fprintf(stderr, "fork failed\n");
				exit(1);
				//Child process
    			} else if (rc == 0) {
					if (quiet == 1) {
						close(STDOUT_FILENO);
					}

					all_args = tokenize(lines[childs]);
					running = execvp(all_args[0], all_args);

					if (running == -1)
						exit(0);
                }
				//Parent process
				else {
					usleep(100000);
					int child_pid = rc;

					if (fail == 0) {
						printf("[%d] was created succesfully\n", childs);
						str_cpids[childs] = (char *) malloc(10*sizeof(char));
						snprintf(str_cpids[childs], 6, "%d", child_pid);
					} else {
						printf("[%d] %s was not created successfully\n", childs, lines[childs]);
					}
				}
				p--;
				childs++;
	}
		
	int *int_cpids = (int *) malloc(4*sizeof(int)); // int PID array
	int store_pids = 0;
	//Stores PID's in int array
	while (store_pids < childs) {
		int_cpids[store_pids] = atoi(str_cpids[store_pids]);
		store_pids++;
	}
	
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = 0;
	sa.sa_handler = handler;

	int pid_index;
	int timelimit = 0;
	int exited = 0;
	int active = 0;
	//THREAD FOR SOCKET
	pthread_t socket_id;
	Data monitor_info = {active, int_cpids};
	Data *mon_ptr = &monitor_info;

	pthread_create(&socket_id, NULL, recieve, &monitor_info);

	while (1) {	
		for (pid_index = 0; pid_index < store_pids; pid_index++) {
			wp = waitpid(int_cpids[pid_index], &status, WNOHANG);
				//if all process' have exited
			if (exited > store_pids) {
				killer:
				pid_index = 0;
				printf("Signal Recieved - Terminating,  %s, %s\n", time, date);
				//kills all process'
				while (pid_index < store_pids) {
					printf("[%d] Terminated\n", pid_index);
					kill(atoi(str_cpids[pid_index]), SIGKILL);
					pid_index++;
				}
				goto outside;
			}
				
			if (SIGNAL != 1) {
				if (wp == 0) {
					char *status_path = (char *) malloc(CHAR_MAX);
					char *cpu_path = (char *) malloc(CHAR_MAX);
					FILE *status_file;
					FILE *cpu_file;

					sprintf(status_path, "/proc/%s/status", str_cpids[pid_index]); //Stores status path
					sprintf(cpu_path, "/proc/%s/stat", str_cpids[pid_index]);
					status_file = fopen(status_path, "r");

					cpu_file = fopen(cpu_path, "r");
					int cpu_usage;
					int v_mem; //To store the memory usage

					v_mem = proc_mem(status_file); //Stores mem usage
					cpu_usage = proc_cpu(cpu_file);
					printf("[%d] Running, cpu usage: %d%%, mem usage: %d MB\n", pid_index, cpu_usage, v_mem);

					mon_ptr->num_p++;
					fclose(status_file);
					fclose(cpu_file);
					free(status_path);
					free(cpu_path);
				}
			}
			if (sigaction(SIGINT, &sa, NULL) == -1)
				err(2, "sigaction");
			if (SIGNAL == 1)
				goto killer;
			if (wp == -1 && SIGNAL == 0) {
				printf("[%d] Exited\n", pid_index);
				exited += 1;
				printf("EXITED : %d\n", exited);
				if (exited > store_pids)
					goto outside;
				SIGNAL = 5;
			}
		}
		sleep(5);
		mon_ptr->num_p = 0;
		timelimit += 5;
		if (*run_time > 0) {
			if (timelimit == *run_time)
				goto killer;
		}
	}
	outside:
	pid_index = 0;
	//LOOP from line 294 got copied to line 349
	while (pid_index < store_pids) {
	kill(atoi(str_cpids[pid_index]), SIGKILL);
	pid_index++;
	}
	printf("Exiting (total time: %d seconds)\n", timelimit);
	for (int f = 0; f < childs; f++) {
		free(str_cpids[f]);
		free(lines[f]);
	}
	fclose(stdout);
	//MEMORY FREE UP
	free(str_cpids);
	free(lines);
	free(int_cpids);
	free(flag);
	free(conf_path);
	return 0;
}

