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

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <expected>
#include "tools.h"

int main(int argc, char* argv[]) {
  auto options_result = parse_args(argc, argv);
  if (options_result.has_value()) {
    auto options = options_result.value();

    // Usar el puerto configurado (ya sea de los argumentos o de la variable de entorno)
    uint16_t port = options.port.value();  // Se asegura de que el puerto está configurado

    // Crear socket
    auto socket = make_socket(port, options);
    if (!socket.has_value()) {
      std::cerr << "Error al crear socket: " << strerror(socket.error()) << std::endl;
      return socket.error();  // Termina si hay error
    }

    // Poner el socket a la escucha
    int result = listen_connection(socket.value(), options);
    if (result != ESUCCESS) {
      std::cerr << "Error al poner el socket a la escucha: " << strerror(result) << std::endl;
      return result;  // Termina si no se puede escuchar
    }

    // Aceptar una conexión
    sockaddr_in client_addr;
    auto client_socket = accept_connection(socket.value(), client_addr);
    if (!client_socket.has_value()) {
      std::cerr << "Error al aceptar conexión: " << strerror(client_socket.error()) << std::endl;
      return client_socket.error();  // Termina si no se puede aceptar la conexión
    }

    // Preparar la respuesta (ejemplo, como 404 Not Found)
    std::string header = "HTTP/1.1 404 Not Found";
    std::string body = "<html><body><h1>404 Not Found</h1></body></html>";

    // Enviar la respuesta
    int send_result = send_response(client_socket.value(), header, body);
    if (send_result != ESUCCESS) {
      std::cerr << "Error al enviar la respuesta: " << strerror(send_result) << std::endl;
      return send_result;  // Termina si hay error al enviar la respuesta
    }

    // El objeto client_socket se destruye aquí, y su destructor cerrará automáticamente la conexión
    std::cout << "Conexión cerrada automáticamente después de enviar la respuesta." << std::endl;

    return ESUCCESS;  // Todo salió bien
  } else {
    std::cerr << "Error al procesar los argumentos: " << strerror(static_cast<int>(options_result.error())) << std::endl;
    return static_cast<int>(options_result.error());  // Termina si hay error en los argumentos
  }
}


