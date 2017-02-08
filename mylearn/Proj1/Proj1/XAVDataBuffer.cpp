#include "stdafx.h"
#include "XAVDataBuffer.h"
#include <stdlib.h>
#include<assert.h>

XAVDataBuffer::XAVDataBuffer(int size)
{
	assert(size > 0);
	m_data = malloc(size);
	assert(m_data);
	m_size = 0;
	m_buffer_size = size;
}


XAVDataBuffer::~XAVDataBuffer()
{
	if (m_data)
		free(m_data);
}

void XAVDataBuffer::Append(void * data, int len)
{
	int remainSize = m_buffer_size - m_size - len;
	if (remainSize < 0)
	{
		int newSize = m_buffer_size - remainSize;
		m_data = realloc(m_data, newSize);
		assert(m_data);
		m_buffer_size = newSize;
	}

	memcpy((char*)m_data + m_size, data, len);
	m_size = m_size+len;
}

void XAVDataBuffer::ReadFront(void* buff, int len)
{
	int oldSize = m_size;
	assert(len <= m_size);
	m_size -= len;
	memcpy(buff, m_data, len);
	if (m_size > 0)
	{
		memcpy( m_data, (char*)m_data + len , m_size);
	}
}
