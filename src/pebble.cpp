#include "pebble.hpp"

void* operator new(size_t size) {
	return malloc(size);
}

void* operator new[](size_t size) {
	return malloc(size);
}

void operator delete(void *ptr) {
	free(ptr);
}

void operator delete[](void *ptr) {
	free(ptr);
}

namespace std {
	void __throw_bad_alloc(void) {}
	void __throw_bad_function_call(void) {}
	void __throw_length_error(const char*) {} 
	void __throw_out_of_range(const char*) {}
	void __throw_runtime_error(const char*) {}
	void __throw_logic_error(const char*) {}
}

#define _FUNCTEXCEPT_H 1
#include <bits/exception_defines.h>

extern "C" void _exit (int) {
  while(1) {};
}

extern "C" int _getpid(void) {
  return 1;
}

extern "C" void _kill(int pid) { while(1); }

int __exidx_start = 0;
int __exidx_end = 0;
