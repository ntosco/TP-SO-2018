#include "consola.h"
#include "../esi/esi.h"

//SIN PARAMETROS
/*
 * El Planificador no le dará nuevas órdenes de
 * ejecución a ningún ESI mientras se encuentre pausado
 * */
void comando_pausar();
/*
 * Reanuda la tarea del planificador
 * */
void comando_continuar();
/*
 * Analiza los deadlocks que existan en el
 * sistema y a que ESI están asociados
 * */
void comando_deadlock();

//CON PARAMETROS
/*
 * Se bloqueará el proceso ESI hasta ser desbloqueado,
 * especificado por dicho <ID> en la cola del recurso <clave>.
 * Cada línea del script a ejecutar es atómica, y no podrá ser interrumpida;
 * si no que se bloqueará en la próxima oportunidad posible. Solo se podrán bloquear
 * de esta manera ESIs que estén en el estado de listo o ejecutando.
 * */
void comando_bloquear_esi_por_id_y_recurso_de_clave(char* id_esi, char* clave);
/*
 * Se desbloqueara el primer proceso ESI bloquedo por la <clave> especificada.
 * Solo se bloqueará ESIs que fueron bloqueados con la consola.
 * */
void comando_desbloquear_primer_esi_por_clave(char* clave);
/*
 * Lista los procesos encolados esperando al <recurso>.
 * */
void comando_listar_procesos_por_recurso(char* recurso);
/*
 * Finaliza el proceso con <ID> brindado.
 * Al momento de eliminar el ESI, se debloquearan las claves que tenga tomadas.
 * */
void comando_kill_proceso_esi_por_id(char* id_esi);
/*
 * Con el objetivo de conocer el estado de una <clave>
 * y de probar la correcta distribución de las mismas,
 * el Coordinador permitirá consultar esta información:
 *
 * 1-Valor, en caso de no poseer valor un mensaje que lo indique.
 *
 * 2-Instancia actual en la cual se encuentra la clave.
 * (En caso de que la clave no exista, la Instancia actual debería)
 *
 * 3-Instancia en la cual se guardaría actualmente la clave.
 * (Calcula el valor mediante el algoritmo de distribución,
 *  sin afectar la distribución actual de las claves).
 *
 * 4-ESIs bloqueados a la espera de dicha clave.
 * */
void comando_status_instancias_por_clave(char* clave);

//EXTRAS
/*
 * Termina de ejecutar la consola finalizando el programa
 * */
void comando_exit();
/*
 * Muestra un listado de todos los esi
 * */
void comando_show_esis();
/*
 * Muestra un listado de todos los recursos
 * */
void comando_listar_recursos();

int retorno = CONTINUAR_EJECUTANDO_CONSOLA;
void _obtener_todos_los_esis();
void _finalizar_cadena(char *entrada);
char *_obtener_comando(char** split_comandos);
char *_obtener_primer_parametro(char** split_comandos);
char *_obtener_segundo_parametro(char** split_comandos);
void _liberar_comando_y_parametros(char** split_comandos);


int consolaLeerComando()
{
	size_t size = 20;
	char *entrada = malloc(20);

	char *comando = malloc(sizeof(char*));
	char *parametro1 = malloc(sizeof(char*));
	char *parametro2 = malloc(sizeof(char*));
	char** split_comandos = malloc(sizeof(char**));

	fgets(entrada, size, stdin);

	_finalizar_cadena(entrada);
	split_comandos = string_split(entrada, " ");
	comando = _obtener_comando(split_comandos);

	switch (getValorByClave(comando))
	{
	case CONSOLA_COMANDO_PAUSAR:
		comando_pausar();
		break;
	case CONSOLA_COMANDO_CONTINUAR:
		comando_continuar();
		break;
	case CONSOLA_COMANDO_DEADLOCK:
		comando_deadlock();
		break;
	case CONSOLA_COMANDO_EXIT:
		comando_exit();
		break;
	case CONSOLA_COMANDO_SHOW:
		comando_show_esis();
		break;
	case CONSOLA_COMANDO_VER_RECURSOS:
		comando_listar_recursos();
		break;

	case CONSOLA_COMANDO_DESBLOQUEAR:
		parametro1 = _obtener_primer_parametro(split_comandos);
		comando_desbloquear_primer_esi_por_clave(parametro1);
		break;
	case CONSOLA_COMANDO_LISTAR:
		parametro1 = _obtener_primer_parametro(split_comandos);
		comando_listar_procesos_por_recurso(parametro1);
		break;
	case CONSOLA_COMANDO_STATUS:

		parametro1 = _obtener_primer_parametro(split_comandos);
		comando_status_instancias_por_clave(parametro1);
		break;
	case CONSOLA_COMANDO_KILL:
		parametro1 = _obtener_primer_parametro(split_comandos);
		comando_kill_proceso_esi_por_id(parametro1);
		break;

	case CONSOLA_COMANDO_BLOQUEAR:
		parametro1 = _obtener_primer_parametro(split_comandos);
		parametro2 = _obtener_segundo_parametro(split_comandos);
		comando_bloquear_esi_por_id_y_recurso_de_clave(parametro1, parametro2);
		break;
	case CONSOLA_COMANDO_DESCONOCIDO:
		printf("Comando no encontrado\n");
		break;
	}

//	free(parametro2);
//	free(parametro1);
//	free(comando);
//	_liberar_comando_y_parametros(split_comandos);
	free(entrada);
	return retorno;
}

void _finalizar_cadena(char *entrada)
{
	if ((strlen(entrada) > 0) && (entrada[strlen (entrada) - 1] == '\n'))
			entrada[strlen (entrada) - 1] = '\0';
}

char* _obtener_comando(char** split_comandos)
{
	return split_comandos[0];
}

char* _obtener_primer_parametro(char** split_comandos)
{
	return split_comandos[1];
}

char* _obtener_segundo_parametro(char** split_comandos)
{
	return split_comandos[2];
}

void _liberar_comando_y_parametros(char** split_comandos)
{
	if(split_comandos[0] != NULL)
	{
		free(split_comandos[0]);
	}
	if(split_comandos[1] != NULL)
	{
		free(split_comandos[1]);
	}
	if(split_comandos[2] != NULL)
	{
		free(split_comandos[2]);
	}
	if(split_comandos != NULL)
	{
		free(split_comandos);
	}
}

void comando_pausar()
{
	log_info(console_log, "Consola: Pausar\n");
}

void comando_continuar()
{
	log_info(console_log, "Consola: Continuar\n");
}

void comando_deadlock()
{
	log_info(console_log, "Consola: Deadlock\n");
}

void comando_exit()
{
	printf("¡ADIOS!\n");
	log_info(console_log, "Consola: Exit\n");
	retorno = TERMINAR_CONSOLA;
}

void comando_show_esis()
{
	log_info(console_log, "Consola: Listar ESIs");
	printf("*******************************************\n");
	printf("ESI\t| ESTADO\n");
	printf("-------------------------------------------\n");

	_obtener_todos_los_esis();
	list_add_all(listaEsis, listaEsiTerminados);
	list_iterate(listaEsis, (void*) _list_esis);

	printf("-------------------------------------------\n");
	printf("\n");
}

void _list_esis(ESI_STRUCT *e)
{
	char * estado = malloc(sizeof(char*));
	switch (e->estado)
	{
	case ESI_LISTO:
		estado = "LISTO";
		break;
	case ESI_EJECUTANDO:
		estado = "EJECUTANDO";
		break;
	case ESI_BLOQUEADO:
		estado = "BLOQUEADO";
		break;
	case ESI_TERMINADO:
		estado = "TERMINADO";
		break;
	}
	printf("%d\t| %s\n", e->id, estado);
	free(estado);
}

void comando_bloquear_esi_por_id_y_recurso_de_clave(char* id_esi, char* clave)
{
	log_info(console_log, "Consola: Bloquear %s - %s\n", id_esi, clave);
}

void comando_desbloquear_primer_esi_por_clave(char* clave)
{
	log_info(console_log, "Consola: Desbloquear %s\n", clave);
}

void comando_listar_procesos_por_recurso(char* recurso)
{
	log_info(console_log, "Consola: Listar %s\n", recurso);
	_obtener_todos_los_esis();

	bool _espera_por_recurso(ESI_STRUCT* esi)
	{
		return string_equals_ignore_case(esi->informacionDeBloqueo.recursoNecesitado, recurso);
	}

	t_list* esis_filtrados = list_filter(listaEsis, (void*) _espera_por_recurso);
	list_iterate(esis_filtrados, (void*) _list_esis);
	list_destroy(esis_filtrados);
	list_clean(listaEsis);
}


void comando_kill_proceso_esi_por_id(char* id_esi) {
	log_info(console_log, "Consola: Kill %s\n", id_esi);

	_obtener_todos_los_esis();

	int _es_esi_unico(ESI_STRUCT *e)
	{
		return string_equals_ignore_case(string_itoa(e->id), id_esi);
	}

	ESI_STRUCT* esi = list_find(listaEsis, (void*) _es_esi_unico);
	/*TODO*/
	list_clean(listaEsis);
}


void comando_status_instancias_por_clave(char* clave)
{
	log_info(console_log, "Consola: Status %s\n", clave);

}

void comando_listar_recursos()
{
	log_info(console_log, "Consola: Listar Recursos\n");
	list_iterate(listaRecursos, (void*) _list_recursos);
}

void _list_recursos(RECURSO *r)
{
	printf("%s\n", r->nombre_recurso);
}

void _obtener_todos_los_esis()
{
	list_add_all(listaEsis, listaEsiListos);
	list_add_all(listaEsis, listaEsiBloqueados);
}

static t_command_struct tabla_referencia_comandos[] = {
		{ "show", CONSOLA_COMANDO_SHOW },
		{ "exit", CONSOLA_COMANDO_EXIT },
		{ "pausar", CONSOLA_COMANDO_PAUSAR },
		{ "continuar", CONSOLA_COMANDO_CONTINUAR },
		{ "bloquear", CONSOLA_COMANDO_BLOQUEAR },
		{ "desbloquear", CONSOLA_COMANDO_DESBLOQUEAR },
		{ "listar", CONSOLA_COMANDO_LISTAR },
		{ "kill", CONSOLA_COMANDO_KILL },
		{ "status", CONSOLA_COMANDO_STATUS },
		{ "deadlock", CONSOLA_COMANDO_DEADLOCK },
		{ "resources", CONSOLA_COMANDO_VER_RECURSOS } };

int getValorByClave(char *clave)
{
	int i;
	string_to_lower(clave);
	for (i = 0; i < NCLAVE; i++)
	{
		t_command_struct comando = tabla_referencia_comandos[i];
		if (strcmp(comando.clave, clave) == 0)
			return comando.valor;
	}
	return CONSOLA_COMANDO_DESCONOCIDO;
}
