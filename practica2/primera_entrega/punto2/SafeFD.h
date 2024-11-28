#ifndef SAFEFD_H
#define SAFEFD_H

#include <iostream>
#include <unistd.h>

class SafeFD {
 public:
  // Constructor explícito que inicializa el descriptor de archivo.
  explicit SafeFD(int fd) noexcept : fd_{fd} {}

  // Constructor explícito predeterminado que inicializa el descriptor a -1.
  explicit SafeFD() noexcept : fd_{-1} {}

  // Prohibir la copia de objetos de SafeFD.
  SafeFD(const SafeFD&) = delete;
  SafeFD& operator=(const SafeFD&) = delete;

  // Constructor de movimiento.
  SafeFD(SafeFD&& other) noexcept : fd_{other.fd_} {
    other.fd_ = -1;
  }

  // Operador de asignación por movimiento.
  SafeFD& operator=(SafeFD&& other) noexcept {
    if (this != &other && fd_ != other.fd_) {
      // Cerrar el descriptor de archivo actual.
      close(fd_);

      // Mover el descriptor de archivo de 'other' a este objeto.
      fd_ = other.fd_;
      other.fd_ = -1;
    }
    return *this;
  }

  // Destructor que cierra el descriptor de archivo si es válido.
  ~SafeFD() noexcept {
    if (fd_ >= 0) {
      close(fd_);
    }
  }

  // Devuelve true si el descriptor de archivo es válido.
  [[nodiscard]] bool is_valid() const noexcept { 
    return fd_ >= 0; 
  }

  // Devuelve el descriptor de archivo.
  [[nodiscard]] int get() const noexcept { 
    return fd_; 
  }

 private:
  int fd_;  // Descriptor de archivo.
};

#endif
