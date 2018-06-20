#include "Instancia.h"
#include "redis.h"
#include <unistd.h> // Para close
#include <commons/string.h>

void print_header() {
	printf("\n\t\e[31;1m=========================================\e[0m\n");
	printf("\t.:: Bienvenido a ReDistinto ::.");
	printf("\t.:: Instancia -  ::.");
	printf("\n\t\e[31;1m=========================================\e[0m\n\n");
}

void print_goodbye() {
	printf("\n\t\e[31;1m=========================================\e[0m\n");
	printf("\t.:: Gracias por utilizar ReDistinto ::.");
	printf("\n\t\e[31;1m=========================================\e[0m\n\n");
}

void exit_program(int retVal) {
	instance_setup_destroy(&instance_setup);

	if (console_log != NULL)
		log_destroy(console_log);
	if (coordinator_socket != 0)
		close(coordinator_socket);

	redis_destroy(redis);

	print_goodbye();
	exit(retVal);
}

void create_log() {
	console_log = log_create("instancia.log", "ReDistinto-Instancia", true,
			LOG_LEVEL_TRACE);

	if (console_log == NULL) {
		printf(" FALLO - Creacion de Log");
		exit_program(EXIT_FAILURE);
	}
}

void loadConfig() {

	log_info(console_log, " Cargan datos del archivo de configuracion");

	t_config *config = config_create(INSTANCE_CFG_FILE);

	if (config != NULL) {

		instance_setup.IP_COORDINADOR = string_duplicate(config_get_string_value(config,"IP_COORDINADOR"));
		instance_setup.PUERTO_COORDINADOR = config_get_int_value(config,"PUERTO_COORDINADOR");
		instance_setup.ALGORITMO_REEMPLAZO = config_get_int_value(config,"ALGORITMO_REEMPLAZO");
		instance_setup.PUNTO_MONTAJE = config_get_string_value(config,"PUNTO_MONTAJE");
		instance_setup.NOMBRE_INSTANCIA = string_duplicate(config_get_string_value(config,"NOMBRE_INSTANCIA"));
		instance_setup.INTERVALO_DUMP_SEGs = config_get_int_value(config, "INTERVALO_DUMP_SEGs");

		log_info(console_log, "COORDINADOR: IP: %s, PUERTO: %d",
					instance_setup.IP_COORDINADOR, instance_setup.PUERTO_COORDINADOR);

		log_info(console_log, "Punto de montaje: %s",
				instance_setup.PUNTO_MONTAJE);
		log_info(console_log, "Nombre de la instancia: %s",
				instance_setup.NOMBRE_INSTANCIA);
		log_info(console_log, "Intervalo de dump en segundos: %d",
				instance_setup.INTERVALO_DUMP_SEGs);


		log_info(console_log, " Carga exitosa de archivo de configuracion");
		config_destroy(config);
	} else {
		log_error(console_log, "No se encontro la configuracion de la instancia");
		exit_program(EXIT_FAILURE);
	}
}

void load_dump_files (){
	redis_load_dump_files(redis);
}

bool wait_for_init_data(){
	void* buffer = malloc(INSTANCE_INIT_VALUES_SIZE);

	if (recv(coordinator_socket, buffer, INSTANCE_INIT_VALUES_SIZE, MSG_WAITALL) < INSTANCE_INIT_VALUES_SIZE) {
		log_error(console_log, "Error receiving handshake response. Aborting execution.");
		free(buffer);
		return false;
	}

	t_instance_init_values *init_values = deserialize_instance_init_values(buffer);

	entry_size = init_values->entry_size;
	number_of_entries = init_values->number_of_entries;
	storage_size = entry_size * number_of_entries;

	log_info(console_log, "Init values received from Coordinator. Entry size: %i, Number of entries: %i. Storage size: %i.",
			entry_size, number_of_entries, storage_size);

	free(buffer);
	free(init_values);

	return true;
}

bool setup_with_coordinator(){

	if(!send_connection_header(coordinator_socket, instance_setup.NOMBRE_INSTANCIA, REDIS_INSTANCE, console_log)){
		return false;
	}

	log_trace(console_log, "Handshake message sent. Waiting for response...");

	if(wait_for_init_data()){
		log_info(console_log, "Init data received from coordinator.");
		return true;
	} else {
		log_error(console_log, "Could not retrieve init data from coordinator!");
		return false;
	}

}

void connect_with_coordinator() {
	log_info(console_log, "Connecting to Coordinador.");
	coordinator_socket = connect_to_server(instance_setup.IP_COORDINADOR, instance_setup.PUERTO_COORDINADOR, console_log);
	if(coordinator_socket <= 0){
		exit_program(EXIT_FAILURE);
	}

	if(!setup_with_coordinator()){
		exit_program(EXIT_FAILURE);
	}
	log_info(console_log, "Successfully connected to Coordinador.");

}

void instance_setup_destroy(t_instance_setup* instance_setup){
	if(instance_setup->NOMBRE_INSTANCIA != NULL)
		free(instance_setup->NOMBRE_INSTANCIA);

	if(instance_setup->IP_COORDINADOR != NULL)
		free(instance_setup->IP_COORDINADOR);

	if(instance_setup->PUNTO_MONTAJE != NULL)
		free(instance_setup->PUNTO_MONTAJE);
}

coordinator_operation_type_e wait_for_signal_from_coordinator(){
	void* buffer = malloc(COORDINATOR_OPERATION_HEADER_SIZE);

	if (recv(coordinator_socket, buffer, COORDINATOR_OPERATION_HEADER_SIZE, MSG_WAITALL) < COORDINATOR_OPERATION_HEADER_SIZE) {
		log_error(console_log, "Error receiving handshake response. Aborting execution.");
		free(buffer);
		exit_program(EXIT_FAILURE);
	}

	t_coordinator_operation_header* header = deserialize_coordinator_operation_header(buffer);

	coordinator_operation_type_e op_type = header->coordinator_operation_type;

	free(header);
	return op_type;
}

t_operation* receive_operation_from_coordinator(){
	void* buffer = malloc(OPERATION_REQUEST_SIZE);

	if (recv(coordinator_socket, buffer, OPERATION_REQUEST_SIZE, MSG_WAITALL) < OPERATION_REQUEST_SIZE) {
		log_error(console_log, "Error receiving operation from Coordinator. Aborting execution.");
		free(buffer);
		exit_program(EXIT_FAILURE);
	}

	t_operation_request* op_request = deserialize_operation_request(buffer);

	t_operation* operation = malloc(sizeof(t_operation));
	strcpy(operation->key, op_request->key);
	operation->operation_type = op_request->operation_type;
	operation->value_size = op_request->payload_size;
	operation->value = NULL;

	free(buffer);
	free(op_request);

	if(operation->value_size> 0){
		char* value_buffer = malloc(op_request->payload_size);

		if (recv(coordinator_socket, value_buffer, operation->value_size, MSG_WAITALL) < operation->value_size) {
			log_error(console_log, "Error receiving payload from Coordinator. Aborting execution.");
			free(value_buffer);
			free(operation);
			exit_program(EXIT_FAILURE);
		}
		operation->value = value_buffer;
	}

	return operation;
}

void send_response_to_coordinator(instance_status_e status, int payload_size){
	t_instance_response response;
	response.status = status;
	response.payload_size = payload_size;

	log_debug(console_log, "Sending response to cordinator: %i. Payload size: %i", status, payload_size);

	void* buffer = serialize_instance_response(&response);

	int result = send(coordinator_socket, buffer, INSTANCE_RESPONSE_SIZE, 0);

	free(buffer);

	if (result < INSTANCE_RESPONSE_SIZE) {
		log_error(console_log, "Could not send response to Coordinator. Aborting execution.");
		exit_program(EXIT_FAILURE);
	}
}

void send_stored_value_to_coordinator(char* stored_value, int value_length){
	log_debug(console_log, "Sending payload to coordinator.");

	int result = send(coordinator_socket, stored_value, value_length, 0);

	if (result < value_length) {
		log_error(console_log, "Could not send value to Coordinator. Aborting execution.");
		exit_program(EXIT_FAILURE);
	}
}

void handle_get(t_operation* operation){
	char* stored_value = redis_get(redis, operation->key);
	if(stored_value == NULL){
		log_error(console_log, "Attempted GET on inexistent Key: %s.", operation->key);

		send_response_to_coordinator(INSTANCE_ERROR, 0);
		return;
	}

	log_info(console_log, "Retrieved value for key: %s. Value: %s", operation->key, stored_value);

	int value_size = strlen(stored_value);
	send_response_to_coordinator(INSTANCE_SUCCESS, value_size);
	send_stored_value_to_coordinator(stored_value, value_size);

	free(stored_value);
}

void handle_set(t_operation* operation){
	bool success = redis_set(redis, operation->key, operation->value, operation->value_size);

	if(success){
		send_response_to_coordinator(INSTANCE_SUCCESS, 0);
	} else {
		log_info(console_log, "Need to compact!");
		send_response_to_coordinator(INSTANCE_COMPACT, 0);
	}
}

void handle_store(t_operation* operation){
	int success = redis_store(redis, operation->key);

	if(success == 0){
		send_response_to_coordinator(INSTANCE_SUCCESS, 0);
	} else if (success == 1){
		log_error(console_log, "Attempted STORE on inexistent Key: %s.", operation->key);

		send_response_to_coordinator(INSTANCE_ERROR, 0);
		return;
	} else {
		log_error(console_log, "Could not store key: %s. Aborting execution.", operation->key);
		exit_program(EXIT_FAILURE);
	}
}

void operation_destroy(t_operation* operation){
	if(operation != NULL){
		if(operation->value != NULL) free(operation->value);
		free(operation);
	}
}

void handle_operation(){
	log_debug(console_log, "Received key operation from Coordinator.");
	t_operation* operation = receive_operation_from_coordinator();

	switch(operation->operation_type){
		case GET:
			log_info(console_log, "Received GET for Key: %s", operation->key);
			handle_get(operation);
			break;
		case SET:
			log_info(console_log, "Received SET for Key: %s Value: %s", operation->key, operation->value);
			handle_set(operation);
			break;
		case STORE:
			log_info(console_log, "Received STORE for Key: %s", operation->key);
			handle_store(operation);
			break;
	}

	operation_destroy(operation);
}

void compact() {
	redis_compact(redis);
}

void initialize_instance(){
	void (*replacement_fn)(struct Redis*, unsigned int);
	switch (instance_setup.ALGORITMO_REEMPLAZO) {
	case CIRC:
		log_info(console_log, "Algoritmo de reemplazo: CIRC");
		replacement_fn = redis_replace_circular;
		break;
	case LRU:
		log_info(console_log, "Algoritmo de reemplazo: LRU");
		replacement_fn = redis_replace_lru;
		break;
	case BSU:
		log_info(console_log, "Algoritmo de planificacion: BSU");
		replacement_fn = redis_replace_bsu;
		break;
	}

	redis = redis_init(entry_size, number_of_entries, console_log,
			instance_setup.PUNTO_MONTAJE, replacement_fn);
}

int main(int argc, char **argv) {
	if (argc > 1 && strcmp(argv[1], "-runTests") == 0){
		run_tests();
		return 0;
	}

	print_header();
	create_log();
	loadConfig();

	connect_with_coordinator();

	initialize_instance();

	load_dump_files();


	// 1. AL conectarse definir tamanio de entradas

	//Recibe sentencia
	//Extrae la clave
	//Identifica donde guardarlo. Si no hay espacio, le avisa al coordinador que tiene que compactar. Compacta
	//Guardar la clave
	//Envía el resultado al Coordinador

	coordinator_operation_type_e coordinator_operation_type;

	while(true){
		coordinator_operation_type = wait_for_signal_from_coordinator();

		switch(coordinator_operation_type){
		case  KEY_OPERATION:
			handle_operation();
			break;
		case COMPACT:
			compact();
			break;
		}
	}

	return 0;
}

