// Universidad de La Laguna
// Escuela Superior de Ingenieria y Tecnologia
// Grado en Ingenieria Informatica
// Asignatura: Sistemas Operativos
// Curso: 2º
// Practica 2: Programación de aplicaciones — Servidor de documentación 
// Autor: Renzo Santoni Moyano
// Correo: alu0101501703@ull.edu.es
// Fecha: 20/11/2024
// Archivo: docserver.cc

#include "tools.h"

int main(int argc, char* argv[]) {
    // Procesamos los argumentos
    auto opts = parse_args(argc, argv);

    // Si hubo un error al procesar los argumentos, mostramos el error y usamos Usage()
    if (!opts) {
        if (opts.error() == parse_args_errors::missing_argument) {
            std::cerr << "Error: Argumento faltante." << std::endl;
        } else if (opts.error() == parse_args_errors::unknown_option) {
            std::cerr << "Error: Opción desconocida." << std::endl;
        }
        return 1;  // Terminar el programa con error
    }

    // Si el usuario pidió la ayuda (-h o --help)
    if (opts->show_help) {
        Usage();  // Mostrar el mensaje de ayuda
        return 0;  // Terminar el programa con éxito
    }

    // Si no hay errores y no se pidió ayuda, el servidor se ejecuta normalmente

    // Modo detallado (verbose)
    if (opts->verbose) {
        std::cerr << "Modo detallado activado." << std::endl;
    }

    // Intentamos crear el socket con el puerto especificado
    auto socket = make_socket(opts->port.value_or(8080), *opts);
    if (!socket) {
        std::cerr << "Error al crear el socket: " << strerror(errno) << std::endl;
        return 1;
    }

    // Ponemos el socket a escuchar
    if (listen_connection(*socket, *opts) != ESUCCESS) {
        std::cerr << "Error al poner el socket a la escucha: " << strerror(errno) << std::endl;
        return 1;
    }

    // Bucle para aceptar y manejar conexiones entrantes
    while (true) {
        sockaddr_in client_addr;
        auto client_socket = accept_connection(*socket, client_addr);

        if (!client_socket) {
            std::cerr << "Error al aceptar la conexión: " << strerror(errno) << std::endl;
            continue;  // Continuamos esperando nuevas conexiones
        }

        // Leemos el archivo para enviarlo
        auto file_content = read_all(opts->input_filename, *opts);
        if (!file_content) {
            // En caso de error al leer el archivo
            send_response(*client_socket, "HTTP/1.1 404 Not Found", "Archivo no encontrado", *opts);
        } else {
            // Enviamos la respuesta con el contenido del archivo
            send_response(*client_socket, "HTTP/1.1 200 OK", file_content->get(), *opts);
        }
    }
    return 0;
}




