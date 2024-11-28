// Universidad de La Laguna
// Escuela Superior de Ingenieria y Tecnologia
// Grado en Ingenieria Informatica
// Asignatura: Sistemas Operativos
// Curso: 2º
// Practica 2: Programación de aplicaciones — Servidor de documentación 
// Autor: Renzo Santoni Moyano
// Correo: alu0101501703@ull.edu.es
// Fecha: 20/11/2024
// Archivo: tools.h

#ifndef TOOLS_H
#define TOOLS_H

#include <iostream>
#include <string.h>
#include <vector>
#include <sstream>
#include <expected>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <netinet/in.h>
#include <arpa/inet.h>     
#include <optional>
#include "safefd.h"
#include "safemap.h"

#define ESUCCESS 0  // Define ESUCCESS si no está definido en el entorno

enum class parse_args_errors {
  missing_argument = -1, 
  unknown_option = -2
};

struct program_options {
  bool show_help = false;
  bool verbose = false;
  std::string input_filename;
  std::vector<std::string> additional_args;
  std::optional<uint16_t> port = {};  // Se añade el campo port con valor predeterminado 8080
};

void Usage();

std::expected<program_options, parse_args_errors> parse_args(int argc, char* argv[]);

std::expected<SafeMap, int> read_all(const std::string& path, const program_options& options);

int send_response(const SafeFD& socket, std::string_view header, std::string_view body = {});

std::string getenv(const std::string& name);

std::expected<SafeFD, int> make_socket(uint16_t port, const program_options& options);

int listen_connection(const SafeFD& socket, const program_options& options);

std::expected<SafeFD, int> accept_connection(const SafeFD& socket, sockaddr_in& client_addr);

#endif