#ifndef __WAVE_H__
#define __WAVE_H__

#include <glm/glm.hpp>

class Wave
{
private:
	int mSize;
	float* mData;

public:
	Wave();
	Wave(unsigned int size);
	Wave(const Wave& other);
	~Wave();

	Wave operator+(const Wave& other) const;
	Wave operator-(const Wave& other) const;
	Wave operator*(const Wave& other) const;
	Wave operator*(const float& f) const;
	Wave operator/(const float& f) const;

	Wave& operator+=(const Wave& other);
	Wave& operator-=(const Wave& other);

	Wave& operator=(const Wave& other);

	float& operator[](const unsigned int& i) const;

public:
	const unsigned int size() const;
	void Initialize(int size);
};

#endif
