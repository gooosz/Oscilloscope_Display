#include "ringbuf_t.hpp"

RingBuffer::RingBuffer(const size_t size)
{
	this->size = size;
	array = new T[size];
	read_ptr = 0;	// start read at pos 0
	write_ptr = 0;	// start write at pos 0
}

RingBuffer::~RingBuffer()
{
	delete array;
}

// write value to current write position
// go to next position afterwards
// TODO: this needs to be threadsafe
void RingBuffer::write(T value)
{
	// lock mutex for scope of this block (function call)
	pthread_mutex_lock(&rb_mutex);
	array[write_ptr] = value;
	write_ptr = (write_ptr + 1) % size;
	pthread_mutex_unlock(&rb_mutex);
}

// get value at current read position
// go to next position afterwards
// TODO: this needs to be threadsafe
T RingBuffer::get()
{
	// lock mutex for scope of this block (function call)
	pthread_mutex_lock(&rb_mutex);
	T readValue = array[read_ptr];
	read_ptr = (read_ptr + 1) % size;
	pthread_mutex_lock(&rb_mutex);
	return readValue;
}

