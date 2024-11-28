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
  std::cerr << "Uso: docserver [-v | --verbose] [-h | --help] [-p <puerto> | --port <puerto>] ARCHIVO" << std::endl;
  std::cerr << "  -h, --help        Mostrar esta ayuda" << std::endl;
  std::cerr << "  -v, --verbose     Activar el modo detallado" << std::endl;
  std::cerr << "  -p, --port        Puerto en el que escuchar (por defecto 8080)" << std::endl;
  std::cerr << "  ARCHIVO           Archivo que se servirá" << std::endl;
}

std::expected<program_options, parse_args_errors> parse_args(int argc, char* argv[]) {
  std::vector<std::string_view> args(argv + 1, argv + argc);
  program_options options;
  bool has_file_argument = false;

  // Procesa los argumentos de la línea de comandos
  for (auto it = args.begin(), end = args.end(); it != end; ++it) {
    if (*it == "-h" || *it == "--help") {
      options.show_help = true;
      return options;
    } else if (*it == "-v" || *it == "--verbose") {
      options.verbose = true;
    } else if (*it == "-p" || *it == "--port") {
      // El siguiente argumento debe ser el puerto
      if (++it != end) {
        try {
          options.port = std::stoi(std::string(*it));  // Convertir el puerto a entero
        } catch (const std::invalid_argument& e) {
          return std::unexpected(parse_args_errors::missing_argument);  // Si no es un número válido
        }
      } else {
        return std::unexpected(parse_args_errors::missing_argument);  // Si falta el puerto
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

  // Si no se especificó un puerto, buscarlo en la variable de entorno
  if (!options.port.has_value()) {  // Verificamos si el puerto no está configurado
    const char* port_env = std::getenv("DOCSERVER_PORT");
    if (port_env) {
      options.port = static_cast<uint16_t>(std::stoi(port_env));  // Usar el valor de la variable de entorno
    } else {
      options.port = 8080;  // Puerto por defecto
    }
  }

  return options;
}



std::expected<SafeMap, int> read_all(const std::string& path, const program_options& options) {
  // Si el modo verbose está activado, mostramos el mensaje de apertura del archivo
  if (options.verbose) {
    std::cerr << "open: se abre el archivo \"" << path << "\"" << std::endl;
  }

  // Abre el archivo en modo solo lectura
  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1) {
    return std::unexpected(errno); // Error al abrir el archivo
  }

  // Obtener información sobre el archivo (tamaño)
  struct stat file_stat;
  if (fstat(fd, &file_stat) == -1) {
    close(fd);  // Cierra el descriptor de archivo en caso de error
    return std::unexpected(errno);
  }
  
  size_t file_size = file_stat.st_size;

  // Si el archivo tiene contenido, lo mapeamos en memoria
  if (file_size > 0) {
    if (options.verbose) {
        std::cerr << "read: se leen " << file_size << " bytes del archivo \"" << path << "\"" << std::endl;
    }
    void* address = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (address == MAP_FAILED) {
      close(fd);  // Cierra el archivo en caso de error al mapear
      return std::unexpected(errno);
    }
    close(fd);  // Cierra el archivo después de mapearlo
    return SafeMap{address, file_size};  // Devolvemos el contenido mapeado
  } else {
    close(fd);  // El archivo está vacío, cerramos el archivo
    return SafeMap{};  // Devolvemos un objeto SafeMap vacío
  }
}

int send_response(const SafeFD& socket, std::string_view header, std::string_view body) {
  // Enviar el encabezado
  ssize_t sent = send(socket.get(), header.data(), header.size(), 0);
  if (sent == -1) {
    int err = errno;
    if (err == ECONNRESET) {
      std::cerr << "Error leve: la conexión fue restablecida por el cliente (ECONNRESET)" << std::endl;
      return err;  // Error leve, no terminamos el programa
    }
    std::cerr << "Error fatal al enviar encabezado: " << strerror(err) << std::endl;
    return err;  // Error fatal, terminamos la función
  }
  // Si hay cuerpo, enviarlo después de una línea en blanco
  if (!body.empty()) {
    sent = send(socket.get(), "\n", 1, 0);  // Enviar la línea en blanco
    if (sent == -1) {
      int err = errno;
      std::cerr << "Error fatal al enviar línea en blanco: " << strerror(err) << std::endl;
      return err;  // Error fatal, terminamos la función
    }
    sent = send(socket.get(), body.data(), body.size(), 0);
    if (sent == -1) {
      int err = errno;
      std::cerr << "Error fatal al enviar cuerpo: " << strerror(err) << std::endl;
      return err;  // Error fatal, terminamos la función
    }
  }
  return ESUCCESS;  // Todo salió bien
}

std::string getenv(const std::string& name) {
  // Obtiene el valor de la variable de entorno
  char* value = std::getenv(name.c_str());
  if (value) {
    return std::string(value);  // Si existe la variable de entorno, devuelve su valor
  } else {
    return std::string();  // Si no existe, devuelve una cadena vacía
  }
}

std::expected<SafeFD, int> make_socket(uint16_t port, const program_options& options) {
  // Si el modo verbose está activado, mostramos un mensaje sobre la creación del socket
  if (options.verbose) {
    std::cerr << "socket: creando un socket en el puerto " << port << std::endl;
  }
  // Crear el socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    return std::unexpected(errno);  // Error al crear el socket
  }
  // Configurar la dirección del socket
  sockaddr_in address {};
  address.sin_family = AF_INET;
  address.sin_port = htons(port);  // Convertir el puerto a formato de red
  address.sin_addr.s_addr = INADDR_ANY;  // Escuchar en todas las interfaces
  // Asignar la dirección al socket
  if (bind(sockfd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) == -1) {
    close(sockfd);  // Cerramos el socket en caso de error
    return std::unexpected(errno);  // Error al asociar la dirección
  }
  return SafeFD(sockfd);  // Devolvemos el descriptor de archivo envuelto en SafeFD
}

int listen_connection(const SafeFD& socket, const program_options& options) {
  // Si el modo verbose está activado, mostramos un mensaje sobre la escucha del socket
  if (options.verbose) {
    std::cerr << "listen: poniendo el socket a la escucha" << std::endl;
  }
  // Poner el socket a la escucha
  int result = listen(socket.get(), SOMAXCONN);
  if (result == -1) {
    return errno;  // Error al poner el socket a la escucha
  }
  return ESUCCESS;  // Éxito
}

std::expected<SafeFD, int> accept_connection(const SafeFD& socket, sockaddr_in& client_addr) {
  socklen_t client_addr_len = sizeof(client_addr);
  // Aceptar una conexión entrante
  int client_fd = accept(socket.get(), reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);
  if (client_fd == -1) {
      return std::unexpected(errno);  // Error al aceptar la conexión
  }
  return SafeFD(client_fd);  // Devolvemos el descriptor de archivo del cliente envuelto en SafeFD
}
