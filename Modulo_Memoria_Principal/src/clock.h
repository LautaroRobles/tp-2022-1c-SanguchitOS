#ifndef SRC_CLOCK_H_
#define SRC_CLOCK_H_

#include "memoria.h"
#include "funciones_memoria.h"

void asignar_frame_a_pagina(t_memoria* memoria, t_tabla_entradas_primer_nivel* tabla_1er_nivel, t_pagina_segundo_nivel* pagina, int* marco_to_swap, int pcb_id);
t_marco* get_marco_puntero(t_tabla_entradas_primer_nivel* tabla_1er_nivel);
void aumentar_puntero(t_tabla_entradas_primer_nivel* tabla_1er_nivel, int marcos_por_proceso);
void clock_algoritmo(t_memoria* memoria, t_tabla_entradas_primer_nivel* tabla_1er_nivel, t_pagina_segundo_nivel* pagina_sin_frame, int* marco_to_swap, int pcb_id);

#endif /* SRC_CLOCK_H_ */
