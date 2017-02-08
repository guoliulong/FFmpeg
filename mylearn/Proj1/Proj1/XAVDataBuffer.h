#pragma once
class XAVDataBuffer
{
private:
	void*	m_data;
	int		m_size;
	int		m_buffer_size;

public:
	XAVDataBuffer(int size);
	~XAVDataBuffer();

	void Append(void*data, int len);

	void ReadFront(void* buff,int len);

	inline int GetSize()
	{
		return m_size;
	}
};

