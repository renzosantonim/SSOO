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
#include "SafeFD.h"
#include "SafeMap.h"

enum class parse_args_errors {
  missing_argument = -1, 
  unknown_option = -2
};

struct program_options {
  bool show_help = false;
  std::string input_filename;
  std::vector<std::string> additional_args;
};

void Usage() {
  std::cout << "Uso: docserver [-v | --verbose] [-h | --help] ARCHIVO\n\n";
  std::cout << "Opciones:\n";
  std::cout << "  -h, --help        Muestra este mensaje de ayuda\n";
  std::cout << "  -v, --verbose     Activa el modo detallado de ejecución\n";
}

std::expected<program_options, parse_args_errors> parse_args(int argc, char* argv[]) {
  std::vector<std::string_view> args(argv + 1, argv + argc);
  program_options options;
  bool has_file_argument = false;
  for (auto it = args.begin(), end = args.end(); it != end; ++it) {
    if (*it == "-h" || *it == "--help") {
      options.show_help = true;
      return options;
    } else if (*it == "-v" || *it == "--verbose") {
      if (++it != end) {
        options.input_filename = *it;  // El nombre del archivo sigue a -v
        has_file_argument = true;
      } else {
        return std::unexpected(parse_args_errors::missing_argument); 
      }
    } else if (!it->starts_with("-")) {
      // Si no empieza con "-", asumimos que es el nombre del archivo
      if (!has_file_argument) {
        options.input_filename = *it;
        has_file_argument = true;
      } else {
        options.additional_args.push_back(std::string(*it));
      }
    } else {
      return std::unexpected(parse_args_errors::unknown_option); 
    }
  }
  // Si no se ha indicado un archivo, generamos un error fatal
  if (!has_file_argument) {
    return std::unexpected(parse_args_errors::missing_argument);
  }
  return options; 
}

std::expected<SafeMap, int> read_all(const std::string& path) {
    // Abrimos el archivo con permisos de solo lectura
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        // Devolvemos el código de error si falla la apertura
        return std::unexpected(errno);
    }
    // Obtenemos el tamaño del archivo
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        close(fd); // Cerramos el descriptor antes de devolver el error
        return std::unexpected(errno);
    }
    size_t file_size = file_stat.st_size;
    if (file_size == 0) {
        // Caso especial: el archivo está vacío
        close(fd);
        return SafeMap{};
    }
    // Mapeamos el archivo en memoria
    void* address = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (address == MAP_FAILED) {
        close(fd);
        return std::unexpected(errno);
    }
    // Cerramos el descriptor de archivo porque ya no lo necesitamos
    close(fd);
    // Devolvemos un SafeMap con los datos mapeados
    return SafeMap{address, file_size};
}


void send_response(std::string_view header, std::string_view body = {}) {
  // Imprimimos el header
  std::cout << header;
  // Si el body no está vacío, imprimimos una línea en blanco y luego el body
  if (!body.empty()) {
    std::cout << "\n" << body;
  }
}

#endif
