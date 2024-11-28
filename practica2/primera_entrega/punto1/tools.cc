// Universidad de La Laguna
// Escuela Superior de Ingenieria y Tecnologia
// Grado en Ingenieria Informatica
// Asignatura: Sistemas Operativos
// Curso: 2º
// Practica 2: Programación de aplicaciones — Servidor de documentación 
// Autor: Renzo Santoni Moyano
// Correo: alu0101501703@ull.edu.es
// Fecha: 20/11/2024
// Archivo: tools.cc

#include "tools.h"

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
      options.verbose = true;
      if (++it != end) {
        options.input_filename = *it;   // El nombre del archivo sigue a -v
        has_file_argument = true;
      } else {
        return std::unexpected(parse_args_errors::missing_argument); 
      }
    } else if (*it == "-w") {           // Nueva opción -w de modificación
      options.check_file_size = true;
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

std::expected<SafeMap, int> read_all(const std::string& path, const program_options& options) {
  if (options.verbose) {
    std::cerr << "open: se abre el archivo \"" << path << "\"" << std::endl;
  }

  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1) {
    return std::unexpected(errno);  // Error de apertura
  }

  struct stat file_stat;
  if (fstat(fd, &file_stat) == -1) {
    close(fd);
    return std::unexpected(errno);  // Error en fstat
  }

  size_t file_size = file_stat.st_size;

  // Verificar si el archivo es demasiado pequeño (menos de 1024 bytes)
  if (file_size < 1024) {
    close(fd);
    return std::unexpected(EINVAL);  // Error personalizado para archivos demasiado pequeños
  }

  void* address = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (address == MAP_FAILED) {
    close(fd);
    return std::unexpected(errno);  // Error en mmap
  }

  close(fd);
  return SafeMap{address, file_size};
}


void send_response(std::string_view header, std::string_view body) {
  // Imprimimos el header
  std::cout << header;
  // Si el body no está vacío, imprimimos una línea en blanco y luego el body
  if (!body.empty()) {
    std::cout << "\n" << body << std::endl;
  }
}