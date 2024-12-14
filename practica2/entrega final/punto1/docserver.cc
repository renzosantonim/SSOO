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
  auto options = parse_args(argc, argv);
  if (!options) {
    send_response("404 Not Found\n");
    return 1;
  }

  if (options->show_help) {
    Usage();
    return 0;
  }

  auto file_map = read_all(options->input_filename, *options);
  if (!file_map) {
    // Comprobamos si el error es el específico para un archivo demasiado pequeño
    if (file_map.error() == EINVAL) {
      std::cerr << "Error fatal: El archivo es demasiado pequeño para procesarse (menos de 1024 bytes)" << std::endl;
      return 1;  // Terminamos el programa con un error
    }

    send_response("403 Forbidden\n");
    return 1;
  }

  std::ostringstream oss;
  oss << "Content-Length: " << file_map->size() << '\n';
  std::string header = oss.str();
  send_response(header, file_map->get());

  return 0;
}