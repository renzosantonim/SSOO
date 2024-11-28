// Universidad de La Laguna
// Escuela Superior de Ingenieria y Tecnologia
// Grado en Ingenieria Informatica
// Asignatura: Sistemas Operativos
// Curso: 2º
// Practica 2: Programación de aplicaciones — Servidor de documentación 
// Autor: Renzo Santoni Moyano
// Correo: alu0101501703@ull.edu.es
// Fecha: 20/11/2024
// Archivo: safemap.h

#ifndef SAFEMAP_H
#define SAFEMAP_H

#include <iostream>
#include <string>
#include <sys/mman.h>


class SafeMap {
 public:
  SafeMap() : sv_{}, size_{0} {}
  SafeMap(void* addr, size_t size) : sv_{static_cast<char*>(addr), size}, size_{size} {}

  ~SafeMap() {
    if (!sv_.empty()) {
      munmap(const_cast<char*>(sv_.data()), size_);
    }
  }

  std::string_view get() const { return sv_; }
  size_t size() const { return size_; }

  SafeMap(SafeMap&& other) noexcept : sv_{other.sv_}, size_{other.size_} {
    other.sv_ = {};
    other.size_ = 0;
  }

  SafeMap& operator=(SafeMap&& other) noexcept {
    if (this != &other) {
      this->~SafeMap();
      sv_ = other.sv_;
      size_ = other.size_;
      other.sv_ = {};
      other.size_ = 0;
    }
    return *this;
  }
  SafeMap(const SafeMap&) = delete;
  SafeMap& operator=(const SafeMap&) = delete;

 private:
    std::string_view sv_;
    size_t size_;
};

#endif