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

// Función principal del servidor de documentación.
int main(int argc, char* argv[]) {
  // Parsear los argumentos de la línea de comandos.
  auto options = parse_args(argc, argv);
  // Si ocurre un error en los argumentos, mostrar el uso y salir.
  if (!options.has_value()) {
    Usage();
    return 1;
  }

  // Crear el socket para la comunicación.
  auto socket_result = make_socket(options.value().port.value_or(8080), options.value());
  // Si hay un error al crear el socket, mostrar el error y salir.
  if (!socket_result.has_value()) {  // Usar has_value() en lugar de has_error()
    std::cerr << "Error al crear el socket: " << strerror(socket_result.error()) << std::endl;
    return 1;
  }

  // Mover el socket creado al objeto server_socket.
  SafeFD server_socket = std::move(socket_result.value());  // Usar std::move

  // Poner el socket a escuchar conexiones entrantes.
  if (listen_connection(server_socket, options.value()) != ESUCCESS) {
    std::cerr << "Error al poner el socket a escuchar" << std::endl;
    return 1;
  }

  sockaddr_in client_addr;
  while (true) {
    // Aceptar una conexión entrante.
    auto client_socket_result = accept_connection(server_socket, client_addr);
    // Si ocurre un error al aceptar la conexión, mostrar el error y continuar con la siguiente conexión.
    if (!client_socket_result.has_value()) {  // Usar has_value() en lugar de has_error()
      std::cerr << "Error al aceptar conexión: " << strerror(client_socket_result.error()) << std::endl;
      continue;
    }

    // Mover el socket del cliente al objeto client_socket.
    SafeFD client_socket = std::move(client_socket_result.value());  // Usar std::move

    // Recibir la solicitud del cliente.
    char buffer[1024];
    ssize_t received = recv(client_socket.get(), buffer, sizeof(buffer) - 1, 0);
    // Si no se recibe nada o hay un error, ignoramos la conexión.
    if (received <= 0) {
      continue;
    }
    buffer[received] = '\0';  // Aseguramos que el buffer esté bien terminado.

    // Procesar la solicitud (extraer el archivo solicitado).
    std::string request(buffer);
    size_t start_pos = request.find("GET /");
    size_t end_pos = request.find(" HTTP/1.1");

    // Si la solicitud no es válida, devolver un error 400 (Bad Request).
    if (start_pos == std::string::npos || end_pos == std::string::npos) {
      send_response(client_socket, "HTTP/1.1 400 Bad Request\r\n\r\n");
      close(client_socket.get());
      continue;
    }

    // Extraer el archivo solicitado de la petición.
    std::string file_requested = request.substr(start_pos + 5, end_pos - (start_pos + 5));
    // Obtener la ruta completa del archivo solicitado.
    std::string full_path = get_full_path(options.value(), file_requested);

    // Intentar leer el contenido del archivo solicitado.
    auto file_content = read_all(full_path, options.value());
    // Si el archivo existe, devolverlo con un código 200 (OK).
    if (file_content.has_value()) {
      send_response(client_socket, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n", 
                    std::string_view(static_cast<const char*>(file_content.value().get().data()), file_content.value().size()));
    } else {
      // Si el archivo no existe, devolver un error 404 (Not Found).
      send_response(client_socket, "HTTP/1.1 404 Not Found\r\n\r\n");
    }

    // Cerramos la conexión con el cliente.
    close(client_socket.get());
  }
  return 0;
}