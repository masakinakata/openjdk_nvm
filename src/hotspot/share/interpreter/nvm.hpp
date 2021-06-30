#ifndef SHARE_INTERPRETER_NVM_HPP
#define SHARE_INTERPRETER_NVM_HPP

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <mutex>
#include <thread>
#include <iostream>
#include "oops/oop.hpp"

#define NVM_SIZE 1 << 30

class ManagementNVM {
public:
  char *mem, *start, *end;
  char *mem_static, *start_static, *end_static;
  char *log, *start_log, *end_log;
  
  ManagementNVM(int size) {
    int fd, fd_static, fd_log;
    fd = open("/dev/zero", O_RDONLY);
    mem = (char*)mmap(NULL, size, PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);
    fd_static = open("/dev/zero", O_RDONLY);
    mem_static = (char*)mmap(NULL, size, PROT_WRITE, MAP_PRIVATE, fd_static, 0);
    close(fd_static);

    fd_log = open("/dev/zero", O_RDONLY);
    log = (char*)mmap(NULL, size, PROT_WRITE, MAP_PRIVATE, fd_log, 0);
    close(fd_log);

    start = mem;
    end = mem + size;
    start_static = mem_static;
    end_static = mem_static + size;
    start_log = log;
    end_log = log + size;
  }

  ManagementNVM(int size, int size_static) {
    int fd, fd_static;
    fd = open("/dev/zero", O_RDONLY);
    mem = (char*)mmap(NULL, size, PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);
    fd_static = open("/dev/zero", O_RDONLY);
    mem_static = (char*)mmap(NULL, size_static, PROT_WRITE, MAP_PRIVATE, fd_static, 0);
    close(fd_static);

    start = mem;
    end = mem + size;
    start_static = mem_static;
    end_static = mem_static + size_static;
  }

  ~ManagementNVM() {
  }
  
  // Allocate memory space in new space of NVM
  void call_new(oopDesc* obj);
  // Allocate memory space in static space of NVM
  void call_static(oopDesc* obj);
  // Allocate memory space
  void call_new_nvm(long int klass_size, long int *ad, bool is_static);
  void debug_call_new_nvm(long int klass_size, long int *ad, bool is_static);
};

extern ManagementNVM nvm;

class StaticNVM {
private:
  static void make_nvm_space(oopDesc* obj, bool is_static);
public:
  static void call_new(oopDesc* obj);
  static void call_static(oopDesc* obj);
  static void compare_obj_detail(oopDesc* obj);
  static void compare_obj(oopDesc* obj);
  static void compare_only_new(oopDesc* obj);
  static void compare_field(oopDesc* obj, int offset, int byte);
  static void deep_copy_nvm(oopDesc* obj);
  static void debug_make_nvm_space(long int klass_size, long int *ad);
};
#endif
