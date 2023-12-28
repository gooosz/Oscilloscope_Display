#pragma once

#include <pthread.h>

/*
 * Thread safe ringbuffer of fixed size
 * holds elements of class T
*/

template <typename T>
class RingBuffer {
public:
	RingBuffer() = 0;	// default constructor
	RingBuffer(const size_t size);
	
	~RingBuffer();
	
	void write(T value);	// write value to current write position
	T get();				// get value at current read position
	
private:
	T *array;
	size_t size;
	size_t read_ptr;	// read ptr to position in array
	size_t write_ptr;	// write ptr to position in array
	pthread_mutex_t rb_mutex;	// for thread safety
};


