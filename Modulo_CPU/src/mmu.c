#include "./mmu.h"

int dir_logica_a_fisica(t_cpu* cpu, t_pcb* pcb, int dir_logica, op_memoria_message instructions_code) {

	// Se piden a memoria en handshake inicial de CPU.
	t_mem_config* mem_config = cpu->mem_config;


	int tabla_1er_nivel = pcb->tabla_paginas;
	int numero_pagina = floor(dir_logica / mem_config->size_pagina);


	int entrada_tabla_1er_nivel = floor(numero_pagina / mem_config->cant_entradas_por_tabla);
	int entrada_tabla_2do_nivel = numero_pagina % mem_config->cant_entradas_por_tabla;
	int desplazamiento = dir_logica - numero_pagina * mem_config->size_pagina;

	int resultado_tlb = consultar_tlb(cpu, numero_pagina);
	if (resultado_tlb != -1) return resultado_tlb * mem_config->size_pagina + desplazamiento;

	int tabla_2do_nivel = get_tabla_segundo_nivel(cpu, tabla_1er_nivel, entrada_tabla_1er_nivel, instructions_code, pcb->id); // Preguntar a memoria, pasando tabla_1er_nivel y entrada_tabla_1er_nivel como datos.
	int marco = get_marco(cpu, tabla_2do_nivel, entrada_tabla_2do_nivel, instructions_code, pcb->id); // Preguntar a memoria, pasando tabla_2do_nivel y entrada_tabla_2do_nivel como datos.
	agregar_entry_tlb(cpu, numero_pagina, marco);

	return marco * mem_config->size_pagina + desplazamiento;
}

int get_tabla_segundo_nivel(t_cpu* cpu, int tabla_nivel, int entrada_tabla_nivel, op_memoria_message instructions_code, int pcb_id){
	return get_tabla_marco_from_memoria(cpu, tabla_nivel, entrada_tabla_nivel, TABLA_SEGUNDO_NIVEL, instructions_code, pcb_id);
}

int get_marco(t_cpu* cpu, int tabla_nivel, int entrada_tabla_nivel, op_memoria_message instructions_code, int pcb_id){
	return get_tabla_marco_from_memoria(cpu, tabla_nivel, entrada_tabla_nivel, MARCO, instructions_code, pcb_id);
}

int get_tabla_marco_from_memoria(t_cpu* cpu, int tabla_nivel, int entrada_tabla_nivel, int OPERACION, op_memoria_message ES_MODIFICADO, int pcb_id){
	int tabla_segundo_nivel, op_code, marco_to_swap;
	t_cpu_paquete* paquete = malloc(sizeof(t_cpu_paquete));
	void* to_send = serialize_mmu_memoria(paquete, tabla_nivel, entrada_tabla_nivel, OPERACION, ES_MODIFICADO, pcb_id);
	send_data_to_server(cpu->mem_config->socket, to_send, paquete->buffer->size + sizeof(int) + sizeof(int));
	recv(cpu->mem_config->socket, &tabla_segundo_nivel, sizeof(int), MSG_WAITALL);
	recv(cpu->mem_config->socket, &marco_to_swap, sizeof(int), MSG_WAITALL);

	if (marco_to_swap >= 0) {
		eliminar_marco_tlb(cpu, marco_to_swap);
	}

	return tabla_segundo_nivel;
}
