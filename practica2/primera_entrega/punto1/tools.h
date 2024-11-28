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
#include <string>
#include <fstream>
#include <vector>
#include <expected>
#include <sstream>
#include <fcntl.h>    
#include <sys/stat.h> 
#include <sys/mman.h> 
#include <unistd.h>   
#include <cerrno>     
#include "safefd.h"
#include "safemap.h"

enum class parse_args_errors {
  missing_argument = -1, 
  unknown_option = -2
};

struct program_options {
  bool show_help = false;
  std::string input_filename;
  std::vector<std::string> additional_args;
};

void Usage();

std::expected<program_options, parse_args_errors> parse_args(int argc, char* argv[]);

std::expected<SafeMap, int> read_all(const std::string& path);

void send_response(std::string_view header, std::string_view body = {});

#endif