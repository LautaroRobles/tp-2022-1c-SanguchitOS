#include "planificadores.h"

void handler_planners(void* void_args){

	// semaforos para manejar estados
	pthread_mutex_t* grado_multiprogramacion = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(grado_multiprogramacion, NULL);
	pthread_mutex_t* hasPcb = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(hasPcb, NULL);
	pthread_mutex_t* hasPcbBlocked = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(hasPcbBlocked, NULL);
	pthread_mutex_t* hasPcbRunning = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(hasPcbRunning, NULL);
	pthread_mutex_t* hasGradoForNew = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(hasGradoForNew, NULL);
	pthread_mutex_t* hasPcbToAddReady = malloc(sizeof(pthread_mutex_t));;
	pthread_mutex_init(hasPcbToAddReady, NULL);
	pthread_mutex_t* hasNewPcbInReady = malloc(sizeof(pthread_mutex_t));;
	pthread_mutex_init(hasNewPcbInReady, NULL);
	sem_t* sem_suspended_ready = malloc(sizeof(sem_t));
	sem_init(sem_suspended_ready, 0, 1);

	pthread_mutex_lock(hasPcb);
	pthread_mutex_lock(hasGradoForNew);
	pthread_mutex_lock(hasPcbRunning);
	pthread_mutex_lock(hasPcbBlocked);

	bool* is_pcb_to_add_ready = malloc(sizeof(bool));
	*is_pcb_to_add_ready = false;
	t_monitor_pcb_to_add_ready* monitor_add_pcb_ready = malloc(sizeof(t_monitor_pcb_to_add_ready));
	monitor_add_pcb_ready->is_pcb_to_add_ready = is_pcb_to_add_ready;
	monitor_add_pcb_ready->mutex = hasPcbToAddReady;

	bool* is_new_pcb_in_ready = malloc(sizeof(bool));
	*is_new_pcb_in_ready = false;
	t_monitor_is_new_pcb_in_ready* monitor_is_new_pcb_in_ready = malloc(sizeof(t_monitor_is_new_pcb_in_ready));
	monitor_is_new_pcb_in_ready->is_new_pcb_in_ready = is_new_pcb_in_ready;
	monitor_is_new_pcb_in_ready->mutex = hasNewPcbInReady;

	// genero los estados dependiendo de si son una cola o una lista.
	t_state_queue_hanndler* new = malloc(sizeof(t_state_queue_hanndler));
	t_state_queue_hanndler* exit = malloc(sizeof(t_state_queue_hanndler));
	t_state_queue_hanndler* running = malloc(sizeof(t_state_queue_hanndler));
	t_state_list_hanndler* ready = malloc(sizeof(t_state_list_hanndler));
	t_state_list_hanndler* blocked = malloc(sizeof(t_state_list_hanndler));
	t_state_list_hanndler* suspended_blocked = malloc(sizeof(t_state_list_hanndler));
	t_state_list_hanndler* suspended_ready = malloc(sizeof(t_state_list_hanndler));

	generate_t_queue_state(new);
	generate_t_queue_state(exit);
	generate_t_queue_state(running);
	generate_t_list_state(ready);
	generate_t_list_state(blocked);
	generate_t_list_state(suspended_blocked);
	generate_t_list_state(suspended_ready);

	t_args_planificador* args = (t_args_planificador*) void_args;
	t_config_kernel* config_kernel = args->config_kernel;

	// conexiones a los demas modulos.
	t_sockets_cpu* sockets_cpu = malloc(sizeof(t_sockets_cpu));
	sockets_cpu->check_state_instructions = connect_to_check_state_instructions_cpu(config_kernel, "9000");
	sockets_cpu->interrupt = connect_to_interrupt_cpu(config_kernel);
	sockets_cpu->dispatch= connect_to_dispatch_cpu(config_kernel);
	int socket_kernel_memoria = connect_to_memoria(config_kernel);

	// variables que necesito como int.
	char* ALGORITMO_PLANIFICACION = config_kernel->ALGORITMO_PLANIFICACION;
	int TIEMPO_MAXIMO_BLOQUEADO = strtol(config_kernel->TIEMPO_MAXIMO_BLOQUEADO, &config_kernel->TIEMPO_MAXIMO_BLOQUEADO, 10);
	int ESTIMACION_INICIAL = strtol(config_kernel->ESTIMACION_INICIAL, &config_kernel->ESTIMACION_INICIAL, 10);
	int GRADO_MULTIPROGRAMACION = strtol(config_kernel->GRADO_MULTIPROGRAMACION, &config_kernel->GRADO_MULTIPROGRAMACION, 10);
	double ALFA = strtod(config_kernel->ALFA, &config_kernel->ALFA);
	preparar_grado(grado_multiprogramacion, GRADO_MULTIPROGRAMACION);
	sem_t* sem = malloc(sizeof(sem_t));
	sem_init(sem, 0, GRADO_MULTIPROGRAMACION);

	//create_grado_multi(GRADO_MULTIPROGRAMACION, gradoMulti);

	// creo estados
	t_states* states = malloc(sizeof(t_states));
	states->state_new = new;
	states->state_exit = exit;
	states->state_ready = ready;
	states->state_running = running;
	states->state_suspended_blocked = suspended_blocked;
	states->state_suspended_ready = suspended_ready;
	states->state_blocked = blocked;

	// Planificador de largo
	pthread_t hilo_long_term;
	t_args_long_term_planner* args_long_term_planner = malloc(sizeof(t_args_long_term_planner));
	args_long_term_planner->monitor_add_pcb_ready = monitor_add_pcb_ready;
	args_long_term_planner->GRADO_MULTIPROGRAMACION = GRADO_MULTIPROGRAMACION;
	args_long_term_planner->ESTIMACION_INICIAL = ESTIMACION_INICIAL;
	args_long_term_planner->ALGORITMO = ALGORITMO_PLANIFICACION;
	args_long_term_planner->pre_pcbs = args->pre_pcbs;
	args_long_term_planner->pre_pcbs_mutex = args->mutex;
	args_long_term_planner->hasNewConsole = args->hasNewConsole;
	args_long_term_planner->sem = sem;
	args_long_term_planner->hasPcb = hasPcb;
	args_long_term_planner->hasGradoForNew = hasGradoForNew;
	args_long_term_planner->grado_multiprogramacion = grado_multiprogramacion;
	args_long_term_planner->socket_memoria = socket_kernel_memoria;
	args_long_term_planner->socket_interrupt = sockets_cpu->interrupt;
	args_long_term_planner->states = states;
	args_long_term_planner->monitor_logger = args->monitor_logger;
	args_long_term_planner->monitor_is_new_pcb_in_ready = monitor_is_new_pcb_in_ready;
	args_long_term_planner->sem_suspended_ready = sem_suspended_ready;
	pthread_create(&hilo_long_term, NULL, long_term_planner, args_long_term_planner);
	pthread_detach(hilo_long_term);

	// Planificador de corto
	pthread_t hilo_short_term;
	t_args_short_term_planner* args_short_term_planner = malloc(sizeof(t_args_short_term_planner));
	args_short_term_planner->ALFA = ALFA;
	args_short_term_planner->ALGORITMO_PLANIFICACION = ALGORITMO_PLANIFICACION;
	args_short_term_planner->GRADO_MULTIPROGRAMACION = GRADO_MULTIPROGRAMACION;
	args_short_term_planner->TIEMPO_MAXIMO_BLOQUEADO = TIEMPO_MAXIMO_BLOQUEADO;
	args_short_term_planner->sockets_cpu = sockets_cpu;
	args_short_term_planner->socket_memoria = socket_kernel_memoria;
	args_short_term_planner->states = states;
	args_short_term_planner->config_kernel = config_kernel;
	args_short_term_planner->hasPcbBlocked = hasPcbBlocked;
	args_short_term_planner->hasPcb = hasPcb;
	args_short_term_planner->hasPcbRunning = hasPcbRunning;
	args_short_term_planner->sem = sem;
	args_short_term_planner->grado_multiprogramacion = grado_multiprogramacion;
	args_short_term_planner->monitor_add_pcb_ready = monitor_add_pcb_ready;
	args_short_term_planner->hasNewConsole = args->hasNewConsole;
	args_short_term_planner->hasGradoForNew = hasGradoForNew;
	args_short_term_planner->monitor_logger = args->monitor_logger;
	args_short_term_planner->monitor_is_new_pcb_in_ready = monitor_is_new_pcb_in_ready;
	args_short_term_planner->sem_suspended_ready = sem_suspended_ready;
	pthread_create(&hilo_short_term, NULL, short_term_planner, args_short_term_planner);
	pthread_detach(hilo_short_term);
}

void generate_t_list_state(t_state_list_hanndler* state_hanndler){
	state_hanndler->state = list_create();
	pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, NULL);
	state_hanndler->mutex = mutex;
}

void generate_t_queue_state(t_state_queue_hanndler* state_hanndler){
	state_hanndler->state = queue_create();
	pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, NULL);
	state_hanndler->mutex = mutex;
}

bool comparate_time_rafaga(void* previous, void* next){
	t_pcb* prebius_pcb = (t_pcb*)previous;
	t_pcb* next_pcb = (t_pcb*)next;
	return prebius_pcb->rafaga<next_pcb->rafaga || (prebius_pcb->rafaga == next_pcb->rafaga && (prebius_pcb->time_in_ready-next_pcb->time_in_ready) < 0);
}

void order_state(t_list* list_to_oreder){
	list_sort(list_to_oreder, comparate_time_rafaga);
}

bool hasCalculateRafaga(t_pcb* pcb){
	return pcb->rafaga > 0;
}

void calculate_rafaga(double alpha, int rafaga_anterior, int time_excecuted, t_pcb* pcb_to_calculate_rafaga){
	pcb_to_calculate_rafaga->rafaga = (alpha * time_excecuted) + ((1 - alpha) * rafaga_anterior);
}

void long_term_planner(void* args_long_term_planner){
	t_log* logger_long = log_create("largo.log", "Hilo planificador largo", 1, LOG_LEVEL_DEBUG);
	t_args_long_term_planner* args = (t_args_long_term_planner*) args_long_term_planner;
	log_info(logger_long, "PLANIFICADOR DE LARGO PLAZO CREADO");

	while(1){
		pthread_mutex_lock(args->hasNewConsole);
		pthread_mutex_lock(args->states->state_exit->mutex);
		log_info(logger_long, "PLANIFICADOR LARGO PLAZO: despertado");

		if (queue_size(args->states->state_exit->state) > 0) {
			log_info(logger_long, "TENEMOS PROCESOS FINALIZADOS");
			close_console_process(args->states->state_exit->state, args->socket_memoria, args->sem, logger_long);
		}else{
			log_info(logger_long, "NO TENEMOS PROCESOS FINALIZADOS");
		}
		pthread_mutex_unlock(args->states->state_exit->mutex);

		pthread_mutex_lock(args->pre_pcbs_mutex);
		if (!queue_is_empty(args->pre_pcbs)) {
			t_pre_pcb* pre_pcb = queue_pop(args->pre_pcbs);
			pthread_mutex_unlock(args->pre_pcbs_mutex);
			log_info(logger_long, "FUE UN EVENTO PARA CREAR UN PCB =====> CHECKEAMOS SI PODEMOS CREAR PCB");

			t_args_add_pcbs* args_add_pcbs = malloc(sizeof(t_args_add_pcbs));
			args_add_pcbs->ALGORITMO = args->ALGORITMO;
			args_add_pcbs->ESTIMACION_INICIAL = args->ESTIMACION_INICIAL;
			args_add_pcbs->grado_multiprogramacion = args->grado_multiprogramacion;
			args_add_pcbs->hasPcb = args->hasPcb;
			args_add_pcbs->sem = args->sem;
			args_add_pcbs->logger_long = logger_long;
			args_add_pcbs->monitor_is_new_pcb_in_ready = args->monitor_is_new_pcb_in_ready;
			args_add_pcbs->pre_pcb = pre_pcb;
			args_add_pcbs->pre_pcbs_mutex = args->pre_pcbs_mutex;
			args_add_pcbs->socket_memoria = args->socket_memoria;
			args_add_pcbs->states = args->states;
			args_add_pcbs->socket_interrupt = args->socket_interrupt;
			args_add_pcbs->sem_suspended_ready = args->sem_suspended_ready;

			pthread_t hilo_add_pcbs;
			pthread_create(&hilo_add_pcbs, NULL, add_pcbs_to_new, args_add_pcbs);
			pthread_detach(hilo_add_pcbs);
		}
		else {
			pthread_mutex_unlock(args->pre_pcbs_mutex);
		}
	}
}

void short_term_planner(void* args_short_planner){
	t_log* logger_short = log_create("corto.log", "Hilo planificador corto", 1, LOG_LEVEL_DEBUG);
	bool* isExitInstruction = malloc(sizeof(bool));
	*isExitInstruction = false;
	bool* isBlockedInstruction = malloc(sizeof(bool));
	*isBlockedInstruction = false;

	sem_t* has_pcb_blocked = malloc(sizeof(sem_t));
	sem_init(has_pcb_blocked, 0, 1);
	sem_t* has_pcb_suspended_blocked = malloc(sizeof(sem_t));
	sem_init(has_pcb_suspended_blocked, 0, 1);
	sem_t* has_pcb_suspended_ready = malloc(sizeof(sem_t));
	sem_init(has_pcb_suspended_ready, 0, 1);
	sem_t* has_pcb_in_io = malloc(sizeof(sem_t));
	sem_init(has_pcb_in_io, 0, 1);

	pthread_mutex_t* has_wait_pcb = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(has_wait_pcb, NULL);

	sem_wait(has_pcb_blocked);
	pthread_mutex_lock(has_wait_pcb);
	sem_wait(has_pcb_suspended_ready);
	sem_wait(has_pcb_suspended_blocked);

	t_args_short_term_planner* args = (t_args_short_term_planner*) args_short_planner;

	t_state_list_hanndler* ready = args->states->state_ready;
	t_state_list_hanndler* state_suspended_ready = args->states->state_suspended_ready;
	t_state_list_hanndler* wait_queue = malloc(sizeof(t_state_list_hanndler));
	generate_t_list_state(wait_queue);
	t_state_queue_hanndler* running = args->states->state_running;
	t_state_queue_hanndler* new = args->states->state_new;
	t_state_queue_hanndler* exit = args->states->state_exit;

	pthread_mutex_t* mutex_check_instruct = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_check_instruct, NULL);
	// Hilos de apoyo a los planificadores
	t_args_check_instructions* args_instruction_thread = malloc(sizeof(t_args_check_instructions));
	args_instruction_thread->mutex_check_instruct = mutex_check_instruct;
	args_instruction_thread->hasPcb = args->hasPcb;
	args_instruction_thread->hasUpdateState = false;
	args_instruction_thread->hasPcbRunning = args->hasPcbRunning;
	args_instruction_thread->socket = args->sockets_cpu->check_state_instructions;
	create_check_instructions_thread(args_instruction_thread);

	// Hilos de apoyo a los planificadores
	t_args_blocked* args_blocked = malloc(sizeof(t_args_blocked));
	args_blocked->has_pcb_blocked = has_pcb_blocked;
	args_blocked->state_ready = ready;
	args_blocked->has_pcb_suspended_blocked = has_pcb_suspended_blocked;
	args_blocked->state_blocked = args->states->state_blocked;
	args_blocked->monitor_is_new_pcb_in_ready = args->monitor_is_new_pcb_in_ready;
	args_blocked->TIEMPO_MAX_BLOQUEADO = args->TIEMPO_MAXIMO_BLOQUEADO;
	args_blocked->ALFA = args->ALFA;
	args_blocked->ALGORITMO = args->ALGORITMO_PLANIFICACION;
	args_blocked->hasPcb = args->hasPcb;
	args_blocked->state_running = running;
	args_blocked->socket_interrupt = args->sockets_cpu->interrupt;
	args_blocked->state_suspended_blocked = args->states->state_suspended_blocked;
	args_blocked->has_pcb_in_io = has_pcb_in_io;
	create_blocked_thread(args_blocked);

	t_args_wait_queue* args_wait_queue = malloc(sizeof(t_args_wait_queue));
	args_wait_queue->has_pcb_blocked = has_pcb_blocked;
	args_wait_queue->blocked = args->states->state_blocked;
	args_wait_queue->has_pcb_suspended_blocked = has_pcb_suspended_blocked;
	args_wait_queue->TIEMPO_MAX_BLOQUEADO = args->TIEMPO_MAXIMO_BLOQUEADO;
	args_wait_queue->suspended_blocked = args->states->state_suspended_blocked;
	args_wait_queue->wait_queue = wait_queue;
	args_wait_queue->has_wait_pcb = has_wait_pcb;
	create_wait_blocked_thread(args_wait_queue);

	// PLANIFICADOR MEDIO PLAZO
	t_args_suspended_blocked* args_suspended_blocked_thread = malloc(sizeof(t_args_suspended_blocked));
	args_suspended_blocked_thread->has_pcb_suspended_blocked = has_pcb_suspended_blocked;
	args_suspended_blocked_thread->TIEMPO_MAX_BLOQUEADO = args->TIEMPO_MAXIMO_BLOQUEADO;
	args_suspended_blocked_thread->socket_memoria = args->socket_memoria;
	args_suspended_blocked_thread->blocked = args->states->state_blocked;
	args_suspended_blocked_thread->state_suspended_blocked = args->states->state_suspended_blocked;
	args_suspended_blocked_thread->has_pcb_suspended_ready = has_pcb_suspended_ready;
	args_suspended_blocked_thread->sem = args->sem;
	args_suspended_blocked_thread->state_suspended_ready = state_suspended_ready;
	args_suspended_blocked_thread->has_pcb_in_io = has_pcb_in_io;
	create_suspended_blocked_thread(args_suspended_blocked_thread);

	// Hilos de apoyo a los planificadores
	t_args_suspended_ready* args_suspended_ready_thread = malloc(sizeof(t_args_suspended_ready));
	args_suspended_ready_thread->ALGORITMO = args->ALGORITMO_PLANIFICACION;
	args_suspended_ready_thread->hasPcb = args->hasPcb;
	args_suspended_ready_thread->has_pcb_suspended_ready = has_pcb_suspended_ready;
	args_suspended_ready_thread->socket_memoria = args->socket_memoria;
	args_suspended_ready_thread->state_suspended_blocked = args->states->state_suspended_blocked;
	args_suspended_ready_thread->state_suspended_ready = state_suspended_ready;
	args_suspended_ready_thread->state_ready = ready;
	args_suspended_ready_thread->grado_multiprogramacion = args->grado_multiprogramacion;
	args_suspended_ready_thread->ALFA = args->ALFA;
	args_suspended_ready_thread->monitor_is_new_pcb_in_ready = args->monitor_is_new_pcb_in_ready;
	args_suspended_ready_thread->sem = args->sem;
	args_suspended_ready_thread->socket_interrupt = args->sockets_cpu->interrupt;
	args_suspended_ready_thread->state_running = running;
	args_suspended_ready_thread->sem_suspended_ready = args->sem_suspended_ready;
	create_suspended_ready_thread(args_suspended_ready_thread);
	use_logger(args->monitor_logger, "PLANIFICADOR MEDIO PLAZO: creado");
	use_logger(args->monitor_logger, "PLANIFICADOR CORTO PLAZO: creado");

	while(1){
		bool is_next_pcb = false;

		pthread_mutex_lock(args->hasPcb);
		log_info(logger_short, "PLANIFICADOR CORTO PLAZO: despertado");
		bool hasRunning = hasRunningPcb(running->state);

		pthread_mutex_lock(args->monitor_is_new_pcb_in_ready->mutex);
		if(*args->monitor_is_new_pcb_in_ready->is_new_pcb_in_ready && strcmp(args->ALGORITMO_PLANIFICACION, "SRT") == 0){
			// desalojamos al proceso en CPU.
			if(hasRunning){
				log_info(logger_short, "EXISTE un PROCESO EN RUNNING, interrumpiendo...");
				interrupt_cpu(args->sockets_cpu->interrupt, INTERRUPT);
				*args->monitor_is_new_pcb_in_ready->is_new_pcb_in_ready = false;
			}
		}
		pthread_mutex_unlock(args->monitor_is_new_pcb_in_ready->mutex);

		pthread_mutex_lock(ready->mutex);
		pthread_mutex_lock(args_instruction_thread->mutex_check_instruct);
		pthread_mutex_lock(args->monitor_is_new_pcb_in_ready->mutex);
		if(args_instruction_thread->hasUpdateState){
			log_info(logger_short, "CPU: llega una actualizacion");
			t_pcb* pcb_excecuted = queue_pop(running->state);
			update_pcb_with_cpu_data(args->sockets_cpu->dispatch, pcb_excecuted, isExitInstruction, isBlockedInstruction);
			if(*isExitInstruction){
				log_info(logger_short, "Es una SALIDA, PCB %d FINALIZADO!", pcb_excecuted->id);
				pthread_mutex_lock(exit->mutex);
				queue_push(exit->state, pcb_excecuted);
				pthread_mutex_unlock(exit->mutex);
				pthread_mutex_unlock(args->hasNewConsole);
				if (!list_is_empty(ready->state)) {
					is_next_pcb = true;
					if (strcmp(args->ALGORITMO_PLANIFICACION,"SRT") == 0) {
						//log_info(logger_short, "Ordenando READY");
						//order_state(ready->state);
					}
				}
				*isExitInstruction = false;
			}
			else if (*isBlockedInstruction) {
				log_info(logger_short, "EL PROCESO %d SE VA A BLOQUEAR", pcb_excecuted->id);
				pthread_mutex_lock(args->states->state_blocked->mutex);
				pthread_mutex_lock(wait_queue->mutex);
				if (list_is_empty(args->states->state_blocked->state) && list_is_empty(wait_queue->state)) {
					list_add(args->states->state_blocked->state, pcb_excecuted);
					pcb_excecuted->time_blocked = time(NULL);
					sem_post(has_pcb_blocked);
					//log_info(logger_short, "EL PROCESO %d TIENE SUSPENDED: %d", pcb_excecuted->id, pcb_excecuted->is_suspended);
				}
				else if (!list_is_empty(wait_queue->state)) {
					list_add(wait_queue->state, pcb_excecuted);
					pcb_excecuted->time_blocked = time(NULL);
				}
				else {
					list_add(wait_queue->state, pcb_excecuted);
					pcb_excecuted->time_blocked = time(NULL);
					pthread_mutex_unlock(has_wait_pcb);
				}
				pthread_mutex_unlock(wait_queue->mutex);
				pthread_mutex_unlock(args->states->state_blocked->mutex);
				if (!list_is_empty(ready->state)) {
					is_next_pcb = true;
					if (strcmp(args->ALGORITMO_PLANIFICACION,"SRT") == 0) {
						//log_info(logger_short, "SE ORDENA READY");
						//order_state(ready->state);
					}
				}
				*isBlockedInstruction = false;
			}
			else{
				log_info(logger_short, "Fue una INTERRUPCION, ENVIA a CPU");
				list_add(ready->state, pcb_excecuted);
				if (strcmp(args->ALGORITMO_PLANIFICACION,"SRT") == 0) {
					log_info(logger_short, "SE ORDENA READY");
					order_state(ready->state);
					for (int index = 0; index < list_size(ready->state); index++) {
						t_pcb* pcb = list_get(ready->state, index);
						log_info(logger_short, "PROCESO %d en pos %d con RAFAGA %d", pcb->id, index, pcb->rafaga);
					}
				}
				*args->monitor_is_new_pcb_in_ready->is_new_pcb_in_ready = true;
			}
			args_instruction_thread->hasUpdateState = false;
			hasRunning = false;
		}
		pthread_mutex_unlock(args->monitor_is_new_pcb_in_ready->mutex);
		pthread_mutex_unlock(args_instruction_thread->mutex_check_instruct);
		pthread_mutex_unlock(ready->mutex);

		pthread_mutex_lock(ready->mutex);
		pthread_mutex_lock(args->monitor_is_new_pcb_in_ready->mutex);
		if(!list_is_empty(ready->state) && (*args->monitor_is_new_pcb_in_ready->is_new_pcb_in_ready || is_next_pcb)){
			*args->monitor_is_new_pcb_in_ready->is_new_pcb_in_ready = false;
			t_pcb* pcb_ready_to_run = list_remove(ready->state, 0);
			pthread_mutex_lock(running->mutex);
			queue_push(running->state, pcb_ready_to_run);
			pthread_mutex_unlock(running->mutex);
			send_pcb_to_cpu(pcb_ready_to_run, args->sockets_cpu->dispatch);
			log_info(logger_short, "PCB %d: ENVIADO A RUNNING", pcb_ready_to_run->id);
			pthread_mutex_unlock(args->hasPcbRunning);
		}
		pthread_mutex_unlock(args->monitor_is_new_pcb_in_ready->mutex);
		pthread_mutex_unlock(ready->mutex);
	}
}

bool isNewPcbIntoReady(int pre_evaluate_add_pcb_to_ready_size, t_list* state_ready){
	return pre_evaluate_add_pcb_to_ready_size < list_size(state_ready);
}

void check_and_update_blocked_to_ready(t_list* state, t_states* states, t_monitor_is_new_pcb_in_ready* monitor_is_new_pcb_in_ready){
	if(list_size(state) > 0){
		t_pcb* pcb_to_add = list_get(states->state_blocked->state, 0);
		int time_blocked = abs(pcb_to_add->time_blocked - clock());
		if(pcb_to_add != NULL && pcb_to_add->time_io <= time_blocked){
			list_add(states->state_ready->state, list_remove(states->state_blocked->state, 0));
			*monitor_is_new_pcb_in_ready->is_new_pcb_in_ready = true;
		}
	}
}

void use_logger(t_monitor_log* monitor, char* log_message){
	pthread_mutex_lock(monitor->mutex);
	log_info(monitor->logger, log_message);
	pthread_mutex_unlock(monitor->mutex);
}

bool hasRunningPcb(t_queue* state_running){
	return !queue_is_empty(state_running);
}

void check_process_finished_io_and_pass_to_suspended_ready(t_state_list_hanndler* state_suspended_blocked, t_state_list_hanndler* state_suspended_ready){
	pthread_mutex_lock(state_suspended_blocked->mutex);
	if(list_size(state_suspended_blocked->state) > 0){
		t_pcb* pcb_to_add = list_get(state_suspended_blocked->state, 0);
		int time_blocked = abs(pcb_to_add->time_blocked - clock());
		if(pcb_to_add != NULL && time_blocked >= pcb_to_add->time_io){
			add_pcb_to_suspended_blocked(list_remove(state_suspended_blocked->state, 0), state_suspended_ready);
		}
	}
	pthread_mutex_unlock(state_suspended_blocked->mutex);
}

void check_time_in_blocked_and_pass_to_suspended_blocked(t_state_list_hanndler* state_suspended_blocked, t_state_list_hanndler* state_blocked, t_monitor_grado_multiprogramacion* monitorGradoMulti, int socket_memoria, int TIEMPO_MAXIMO_BLOQUEADO){
	t_list* pos_pcbs_with_time_out = list_create();

	for(int index = 0; index < list_size(state_blocked->state); index++){
		t_pcb* pcb_blocked = list_get(state_blocked->state, index);
		int time_blocked = abs(pcb_blocked->time_blocked - clock());

		// se checkea en short que se haya terminado su tiempo de bloqueado.

		if(time_blocked >= TIEMPO_MAXIMO_BLOQUEADO && time_blocked < pcb_blocked->time_io){
			add_pcb_to_suspended_blocked(pcb_blocked, state_suspended_blocked);
			list_add(pos_pcbs_with_time_out, index);
			pthread_mutex_lock(monitorGradoMulti->mutex);
			monitorGradoMulti->gradoMultiprogramacionActual--;
			pthread_mutex_unlock(monitorGradoMulti->mutex);
		}
	}

	for(int index = 0; index < list_size(pos_pcbs_with_time_out); index++){
		t_pcb* pcb_to_send = list_remove(state_blocked->state, list_get(pos_pcbs_with_time_out, index));
		send_action_to_memoria(pcb_to_send, socket_memoria, SWAP);
	}
}

void recive_information_from_memoria(t_pcb* pcb, int socket_memoria){
	int op_code;
	// recibo mensaje y pcb de memoria.
	t_pcb* pcb_received = deserializate_pcb(socket_memoria, &op_code);

	if (op_code == ERROR || op_code < 0) {
		error_show("OCURRIO UN PROBLEMA INTENTANDO CONECTARSE CON MEMORIA, CODIGO ERROR: %d", op_code);
		//exit(1);
	} else if (op_code == NEW || op_code == RE_SWAP) {
		pcb->tabla_paginas = pcb_received->tabla_paginas;
	} else if (op_code == SWAP) {
		// TODO loggeamos que se realizo la liberacion de espacio memoria.
	}
	 else if (op_code == RE_SWAP) {
	}
}

void add_pcb_to_suspended_blocked(t_pcb* pcb_blocked, t_state_list_hanndler* state_suspended_blocked){
	list_add(state_suspended_blocked, pcb_blocked);
}

void close_console_process(t_queue* state_exit, int socket_memoria, sem_t* grado_multiprogramacion, t_log* logger_long){
	int size_exit_state = queue_size(state_exit);
	for(int elem_destroy = 0; elem_destroy < size_exit_state; elem_destroy++){
		t_pcb* pcb_to_deleat = queue_pop(state_exit);
		// avisar a memoria que se eleimina el pcb.
		send_pcb_to_memoria(pcb_to_deleat, socket_memoria, DELETE);
		close(pcb_to_deleat->id);
		log_info(logger_long, "CERRAMOS PROCESO: %d", pcb_to_deleat->id);
		list_destroy(pcb_to_deleat->instrucciones);
		free(pcb_to_deleat);
		//pthread_mutex_unlock(grado_multiprogramacion);
		sem_post(grado_multiprogramacion);
	}
}

int total_pcbs_short_mid_term(t_states* states){
	return list_size(states->state_ready->state) + list_size(states->state_blocked->state) + queue_size(states->state_running->state);
}

void* add_pcbs_to_new(t_args_add_pcbs* args){
	t_pcb* pcb = create_pcb(args->ESTIMACION_INICIAL, args->pre_pcb);
	log_info(args->logger_long, "PCB CREADO: ID %d", pcb->id);

	op_memoria_message op_code = NEW;
	if(pcb != NULL){
		log_info(args->logger_long, "SE INTENTA PASAR A MEMORIA SI ES QUE HAY GRADO DISPONIBLE.....");
		//pthread_mutex_lock(args->grado_multiprogramacion);
		sem_wait(args->sem_suspended_ready);
		sem_wait(args->sem);

		pthread_mutex_lock(args->states->state_ready->mutex);
		t_pcb* pcb_tabla = send_action_to_memoria(pcb, args->socket_memoria, op_code);
		log_info(args->logger_long, "EXISTE GRADO PARA EL PCB %d SE CREA SU TABLA, COMENZAMOS EL PASAJE A READY.....", pcb_tabla->id);
		pthread_mutex_lock(args->states->state_new->mutex);
		queue_push(args->states->state_new->state, pcb_tabla);
		pthread_mutex_unlock(args->states->state_new->mutex);
		add_pcb_to_state(pcb_tabla, args->states->state_ready->state);
		pcb->time_in_ready = time(NULL);
		log_info(args->logger_long, "SE AGREGO EL PCB %d DE NEW A READY", pcb_tabla->id);
		pthread_mutex_lock(args->states->state_running->mutex);
		if (strcmp(args->ALGORITMO, "SRT") == 0 && !queue_is_empty(args->states->state_running->state)) {
			log_info(args->logger_long, "AVISAMOS A LA CPU QUE SE DEBE INTERRUMPIR PARA REPLANIFICAR");
			interrupt_cpu(args->socket_interrupt, INTERRUPT);
		}
		else {
			order_state(args->states->state_ready->state);
			pthread_mutex_lock(args->monitor_is_new_pcb_in_ready->mutex);
			*args->monitor_is_new_pcb_in_ready->is_new_pcb_in_ready = true;
			pthread_mutex_unlock(args->monitor_is_new_pcb_in_ready->mutex);
			pthread_mutex_unlock(args->hasPcb);
		}
		pthread_mutex_unlock(args->states->state_running->mutex);
		pthread_mutex_unlock(args->states->state_ready->mutex);
	}
	sem_post(args->sem_suspended_ready);
	pthread_exit(NULL);
}

void add_pcb_to_state(t_pcb* pcb, t_list* state){
	list_add(state, pcb);
}

t_pcb* create_pcb(int ESTIMACION_INICIAL, t_pre_pcb* pre_pcb){
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->id = pre_pcb->pcb_id;
	pcb->processSize = pre_pcb->processSize;
	pcb->program_counter = 0;
	pcb->instrucciones = pre_pcb->instructions;
	pcb->is_suspended = 0;
	pcb->rafaga = ESTIMACION_INICIAL;
	pcb->time_blocked = 0;
	pcb->time_in_ready = 0;
	pcb->time_excecuted_rafaga = 0;
	pcb->time_io = 0;
	pcb->tabla_paginas = 0;
	return pcb;
}

void send_pcb_to_memoria(t_pcb* pcb , int socket_memoria, op_memoria_message MENSSAGE){
	if(pcb != NULL){
		t_cpu_paquete* paquete = malloc(sizeof(t_cpu_paquete));
		void* pcb_serializate = serializate_pcb(pcb, paquete, (int)MENSSAGE);
		int code_operation = send_data_to_server(socket_memoria, pcb_serializate, (paquete->buffer->size + sizeof(int) + sizeof(int)));

		if(code_operation < 0){
			error_show("CONEXION MEMORIA: IMPOSIBLE CONECTAR");
			exit(1);
		}
		free(pcb_serializate);
	}
}

void send_pcb_to_cpu(t_pcb* pcb , int socket_cpu_dispatch){
	if(pcb != NULL){
		t_cpu_paquete* paquete = malloc(sizeof(t_cpu_paquete));
		void* pcb_serializate = serializate_pcb(pcb, paquete, DISPATCH);
		int code_operation = send_data_to_server(socket_cpu_dispatch, pcb_serializate, (paquete->buffer->size + sizeof(int) + sizeof(int)));

		if(code_operation < 0){
			error_show("CONEXION CPU: IMPOSIBLE CONECTAR");
			exit(1);
		}
		free(pcb_serializate);
	}
}

void update_pcb_with_cpu_data(int socket_kernel_interrupt_cpu, t_pcb* pcb, bool* isExitInstruction, bool* isBlockedInstruction){
	op_memoria_message op_code;
	t_pcb* pcb_excecuted = deserializate_pcb(socket_kernel_interrupt_cpu, &op_code);

	if(op_code < 0){
		error_show("NO SE PUDO EXTRAER INFORMACION ENVIADA POR LA CPU");
		exit(1);
	} else if (op_code == EXIT){
		*isExitInstruction = true;
	} else {
		pcb->time_excecuted_rafaga += pcb_excecuted->time_excecuted_rafaga;

		if(op_code == I_O){
			*isBlockedInstruction = true;
			pcb->time_io = pcb_excecuted->time_io;
		}

		pcb->is_suspended = pcb_excecuted->is_suspended;
		pcb->program_counter = pcb_excecuted->program_counter;
	}
}

int interrupt_cpu(int socket_kernel_interrupt_cpu, op_code INTERRUPT){
	int op_code = INTERRUPT;
	bool* isExitInstruction = malloc(sizeof(bool));
	*isExitInstruction = false;
	send_data_to_server(socket_kernel_interrupt_cpu, &op_code, sizeof(int), 0);
}

int connect_to_interrupt_cpu(t_config_kernel* config_kernel){
	return realize_connection(config_kernel->IP_CPU, config_kernel->PUERTO_CPU_INTERRUPT);
}

int connect_to_check_state_instructions_cpu(t_config_kernel* config_kernel, char* port){
	return realize_connection(config_kernel->IP_CPU, port);
}

int connect_to_dispatch_cpu(t_config_kernel* config_kernel){
	return realize_connection(config_kernel->IP_CPU, config_kernel->PUERTO_CPU_DISPATCH);
}

int realize_connection(char* ip, char* puerto) {
	int connection = 0;
	do{
		connection = create_client_connection(ip,puerto);
	}while(connection <= 0);
	return connection;
}

int connect_to_memoria(t_config_kernel* config_kernel){
	return realize_connection(config_kernel->IP_MEMORIA, config_kernel->PUERTO_MEMORIA);
}

int has_tabla_paginas(t_pcb* pcb_to_add){
	return pcb_to_add->tabla_paginas != NULL;
}

void check_state_of_pcb(void* void_args){
	t_args_check_instructions* args = (t_args_check_instructions*) void_args;
	op_code code;
	while(1){
		recv(args->socket, &code, sizeof(int), 0);
		pthread_mutex_lock(args->mutex_check_instruct);
		if(code == BLOCKED_FINISHED){
			args->hasUpdateState = true;
			pthread_mutex_unlock(args->hasPcb);
		}
		pthread_mutex_unlock(args->mutex_check_instruct);
	}
}

t_pcb* send_action_to_memoria(t_pcb* pcb, int socket_memoria, op_memoria_message ACTION){
	send_pcb_to_memoria(pcb , socket_memoria, ACTION);
	recive_information_from_memoria(pcb , socket_memoria);
	return pcb;
}

void state_blocked (t_args_blocked* args) {
	t_log* logger_blocked = log_create("blocked.log", "Hilo blocked", 1, LOG_LEVEL_DEBUG);
	while (1) {
		sem_wait(args->has_pcb_blocked);
		sem_wait(args->has_pcb_in_io);

		pthread_mutex_lock(args->state_blocked->mutex);
		t_pcb* pcb_blocked = list_get(args->state_blocked->state, 0);
		pthread_mutex_unlock(args->state_blocked->mutex);
		log_info(logger_blocked, "PROCESO %d ESTA EN BLOCKED", pcb_blocked->id);

		if (args->TIEMPO_MAX_BLOQUEADO >= pcb_blocked->time_io) {
			pcb_blocked->time_blocked = 0;
			int spleep_time = pcb_blocked->time_io/1000;
			log_info(logger_blocked, "Realizando SLEEP de %ds",  spleep_time);
			sleep(spleep_time);

			if (pcb_blocked->is_suspended) {
				pthread_mutex_lock(args->state_suspended_blocked->mutex);
				list_add_in_index(args->state_suspended_blocked->state, 0, pcb_blocked);
				pthread_mutex_unlock(args->state_suspended_blocked->mutex);

				sem_post(args->has_pcb_suspended_blocked);
			}
			else if (strcmp(args->ALGORITMO, "SRT") == 0) {
				pthread_mutex_lock(args->state_ready->mutex);
				calculate_rafaga(args->ALFA, pcb_blocked->rafaga, pcb_blocked->time_excecuted_rafaga, pcb_blocked);
				list_add(args->state_ready->state, pcb_blocked);
				pthread_mutex_unlock(args->state_ready->mutex);
				pcb_blocked->time_in_ready = clock();
				order_state(args->state_ready->state);
				pthread_mutex_lock(args->state_running->mutex);
				if (!queue_is_empty(args->state_running->state)) {
					interrupt_cpu(args->socket_interrupt, INTERRUPT);
				}
				else {
					pthread_mutex_lock(args->monitor_is_new_pcb_in_ready->mutex);
					*args->monitor_is_new_pcb_in_ready->is_new_pcb_in_ready = true;
					pthread_mutex_unlock(args->monitor_is_new_pcb_in_ready->mutex);
					pthread_mutex_unlock(args->hasPcb);
				}
				pthread_mutex_unlock(args->state_running->mutex);
				pthread_mutex_lock(args->state_blocked->mutex);
				list_remove(args->state_blocked->state, 0);
				pthread_mutex_unlock(args->state_blocked->mutex);
				sem_post(args->has_pcb_in_io);
				log_info(logger_blocked, "PROCESO %d DESBLOQUEADO, pasa a READY", pcb_blocked->id);
			} else {
				pthread_mutex_lock(args->state_ready->mutex);
				list_add(args->state_ready->state, pcb_blocked);
				pthread_mutex_unlock(args->state_ready->mutex);
				pthread_mutex_lock(args->monitor_is_new_pcb_in_ready->mutex);
				*args->monitor_is_new_pcb_in_ready->is_new_pcb_in_ready = true;
				pthread_mutex_unlock(args->monitor_is_new_pcb_in_ready->mutex);
				pthread_mutex_lock(args->state_blocked->mutex);
				list_remove(args->state_blocked->state, 0);
				pthread_mutex_unlock(args->state_blocked->mutex);
				pthread_mutex_unlock(args->hasPcb);
				sem_post(args->has_pcb_in_io);
				log_info(logger_blocked, "PROCESO %d SE DESBLOQUEO, pasa a READY", pcb_blocked->id);
			}
		}
		else {
			pcb_blocked->time_blocked = 0;
			sleep(args->TIEMPO_MAX_BLOQUEADO/1000);
			pcb_blocked->time_io = abs(pcb_blocked->time_io - args->TIEMPO_MAX_BLOQUEADO);
			log_info(logger_blocked, "Proceso %d SUPERO TIEMPO MAX BLOQUEADO, pasa a SUSPEND BLOCKED", pcb_blocked->id);

			pthread_mutex_lock(args->state_suspended_blocked->mutex);
			list_add_in_index(args->state_suspended_blocked->state, 0, pcb_blocked);
			pthread_mutex_unlock(args->state_suspended_blocked->mutex);

			sem_post(args->has_pcb_suspended_blocked);
		}
	}
}


void wait_queue_for_blocked(t_args_wait_queue* args){
	while (1) {
		pthread_mutex_lock(args->has_wait_pcb);

		while (1) {
			sleep(1);
			pthread_mutex_lock(args->wait_queue->mutex);
			t_pcb* pcb = list_get(args->wait_queue->state, 0);
			pthread_mutex_unlock(args->wait_queue->mutex);

			int time_now = time(NULL) - pcb->time_blocked;
			int timpo_max = (args->TIEMPO_MAX_BLOQUEADO/1000);
			if (time_now > timpo_max) {
				pcb->is_suspended = 1;
				pthread_mutex_lock(args->suspended_blocked->mutex);
				list_add(args->suspended_blocked->state, pcb);
				pthread_mutex_unlock(args->suspended_blocked->mutex);
				pthread_mutex_lock(args->blocked->mutex);
				pthread_mutex_lock(args->wait_queue->mutex);
				list_add(args->blocked->state, list_remove(args->wait_queue->state, 0));
				pthread_mutex_unlock(args->wait_queue->mutex);
				pthread_mutex_unlock(args->blocked->mutex);
				sem_post(args->has_pcb_suspended_blocked);
				sem_post(args->has_pcb_blocked);
			} else {
				pthread_mutex_lock(args->blocked->mutex);
				if (list_is_empty(args->blocked->state)) {
					pthread_mutex_lock(args->wait_queue->mutex);
					list_add(args->blocked->state, list_remove(args->wait_queue->state, 0));
					pthread_mutex_unlock(args->wait_queue->mutex);
					sem_post(args->has_pcb_blocked);
				}
				pthread_mutex_unlock(args->blocked->mutex);
			}

			pthread_mutex_lock(args->wait_queue->mutex);
			if (list_is_empty(args->wait_queue->state)) {
				pthread_mutex_unlock(args->wait_queue->mutex);
				break;
			}
			pthread_mutex_unlock(args->wait_queue->mutex);
		}
	}
}

void state_suspended_blocked (t_args_suspended_blocked* args) {
	t_log* logger_suspend_blocked = log_create("suspend_blocked.log", "Hilo suspendido blocked", 1, LOG_LEVEL_DEBUG);
	while (1) {
		sem_wait(args->has_pcb_suspended_blocked);

		pthread_mutex_lock(args->state_suspended_blocked->mutex);
		t_pcb* pcb_blocked = list_remove(args->state_suspended_blocked->state, 0);
		pthread_mutex_unlock(args->state_suspended_blocked->mutex);

		if (pcb_blocked->time_blocked == 0 && pcb_blocked->is_suspended == 0) {
			pcb_blocked->is_suspended = 1;
			log_info(logger_suspend_blocked, "PROCESO %d esta en SUSPENDED BLOCKED", pcb_blocked->id);
			send_pcb_to_memoria(pcb_blocked, args->socket_memoria, SWAP);
			sem_post(args->sem);
			sleep(pcb_blocked->time_io/1000);
			log_info(logger_suspend_blocked, "PROCESO %d pasa de SUSPENDED BLOCKED A SUSPENDED READY", pcb_blocked->id);

			pthread_mutex_lock(args->state_suspended_ready->mutex);
			list_add(args->state_suspended_ready->state, pcb_blocked);
			pthread_mutex_unlock(args->state_suspended_ready->mutex);

			pthread_mutex_lock(args->blocked->mutex);
			list_remove(args->blocked->state, 0);
			pthread_mutex_unlock(args->blocked->mutex);

			sem_post(args->has_pcb_suspended_ready);
			sem_post(args->has_pcb_in_io);
		}
		else if (pcb_blocked->time_blocked == 0 && pcb_blocked->is_suspended == 1) {
			log_info(logger_suspend_blocked, "EL PROCESO %d YA HABIA SIDO SUSPENDIDO POR SUPERAR EL TIEMPO MAX.", pcb_blocked->id);
			if (pcb_blocked->time_io > args->TIEMPO_MAX_BLOQUEADO) {
				log_info(logger_suspend_blocked, "PERO LE QUEDA TIEMPO DE I/O PENDIENTE......", pcb_blocked->id);
				sleep((args->TIEMPO_MAX_BLOQUEADO - pcb_blocked->time_io)/1000);
				log_info(logger_suspend_blocked, "I/O FINALIZADO", pcb_blocked->id);
			}

			pthread_mutex_lock(args->state_suspended_ready->mutex);
			log_info(logger_suspend_blocked, "PASA A SUSPEND READY", pcb_blocked->id);
			list_add(args->state_suspended_ready->state, pcb_blocked);
			pthread_mutex_unlock(args->state_suspended_ready->mutex);
			pthread_mutex_lock(args->blocked->mutex);
			list_remove(args->blocked->state, 0);
			pthread_mutex_unlock(args->blocked->mutex);
			sem_post(args->has_pcb_in_io);
			sem_post(args->has_pcb_suspended_ready);
		}
		else {
			send_pcb_to_memoria(pcb_blocked, args->socket_memoria, SWAP);
			sem_post(args->sem);
			log_info(logger_suspend_blocked, "PROCESO %d pasa DE BLOCKED A SUSPENDED BLOCKED por SUPERAR TIEMPO MAX.", pcb_blocked->id);
		}
	}
}

void state_suspended_ready (t_args_suspended_ready* args) {
	t_log* logger_suspend_ready = log_create("suspend_ready.log", "Hilo suspendido ready", 1, LOG_LEVEL_DEBUG);

	while (1) {
		sem_wait(args->has_pcb_suspended_ready);
		sem_wait(args->sem_suspended_ready);

		pthread_mutex_lock(args->state_suspended_ready->mutex);
		t_pcb* pcb_to_add = list_remove(args->state_suspended_ready->state, 0);
		pthread_mutex_unlock(args->state_suspended_ready->mutex);
		log_info(logger_suspend_ready, "EL PROCESO %d esta en SUSPENDED READY", pcb_to_add->id);
		if (pcb_to_add != NULL) {
			sem_wait(args->sem);
			send_pcb_to_memoria(pcb_to_add, args->socket_memoria, RE_SWAP);
			log_info(logger_suspend_ready, "EL PROCESO %d es pedido a MEMORIA", pcb_to_add->id);
			pthread_mutex_lock(args->state_ready->mutex);
			calculate_rafaga(args->ALFA, pcb_to_add->rafaga, pcb_to_add->time_excecuted_rafaga, pcb_to_add);
			pcb_to_add->time_in_ready = time(NULL);
			add_pcb_to_state(pcb_to_add, args->state_ready->state);
			log_info(logger_suspend_ready, "PCB %d pasa de SUSPENDED READY a READY", pcb_to_add->id);

			pthread_mutex_lock(args->state_running->mutex);
			if (strcmp(args->ALGORITMO, "SRT") == 0) {
				/*log_info(logger_blocked, "REPLANIFICAMOS");*/
				order_state(args->state_ready->state);
				if (!queue_is_empty(args->state_running->state)) {
					interrupt_cpu(args->socket_interrupt, INTERRUPT);
				}
				else {
					pthread_mutex_lock(args->monitor_is_new_pcb_in_ready->mutex);
					*args->monitor_is_new_pcb_in_ready->is_new_pcb_in_ready = true;
					pthread_mutex_unlock(args->monitor_is_new_pcb_in_ready->mutex);
					pthread_mutex_unlock(args->hasPcb);
				}
			} else {
				pthread_mutex_lock(args->monitor_is_new_pcb_in_ready->mutex);
				*args->monitor_is_new_pcb_in_ready->is_new_pcb_in_ready = true;
				pthread_mutex_unlock(args->monitor_is_new_pcb_in_ready->mutex);
				pthread_mutex_unlock(args->hasPcb);
			}
			pthread_mutex_unlock(args->state_running->mutex);
		}
		sem_post(args->sem_suspended_ready);
		pthread_mutex_unlock(args->state_ready->mutex);
	}
}

void create_check_instructions_thread(t_args_check_instructions* args){
	pthread_t hilo_check_instructions;
	pthread_create(&hilo_check_instructions, NULL, check_state_of_pcb, args);
	pthread_detach(hilo_check_instructions);
}

void create_suspended_blocked_thread(t_args_suspended_blocked* args){
	pthread_t hilo_suspended_blocked;
	pthread_create(&hilo_suspended_blocked, NULL, state_suspended_blocked, args);
	pthread_detach(hilo_suspended_blocked);
}

void create_suspended_ready_thread(t_args_suspended_ready* args){
	pthread_t hilo_suspended_ready;
	pthread_create(&hilo_suspended_ready, NULL, state_suspended_ready, args);
	pthread_detach(hilo_suspended_ready);
}

void create_blocked_thread(t_args_blocked* args){
	pthread_t hilo_blocked;
	pthread_create(&hilo_blocked, NULL, state_blocked, args);
	pthread_detach(hilo_blocked);
}

void create_wait_blocked_thread(t_args_wait_queue* args){
	pthread_t hilo_wait_blocked;
	pthread_create(&hilo_wait_blocked, NULL, wait_queue_for_blocked, args);
	pthread_detach(hilo_wait_blocked);
}

void preparar_grado(pthread_mutex_t* grado_multiprogramacion, int GRADO) {
	for (int grado = 1; grado <= GRADO; grado++) {
		pthread_mutex_unlock(grado_multiprogramacion);
	}
}
