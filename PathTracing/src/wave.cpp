#include "wave.h"

Wave::Wave()
{
	mSize = 0;
	mData = nullptr;
}

Wave::Wave(unsigned int size)
{
	mSize = size;
	mData = new float[size]();
}

Wave::Wave(const Wave& other)
{
	mSize = other.mSize;
	mData = new float[mSize]();
	for (int i = 0; i < mSize; i++)
		mData[i] = other.mData[i];
}

Wave::~Wave()
{
	if (mData)
		delete[] mData;
}

Wave Wave::operator+(const Wave& other) const
{
	Wave res(mSize);
	int n = mSize;
	if (other.mSize < mSize)
		n = other.mSize;
	for (int i = 0; i < mSize; i++)
	{
		if (i >= n)
			res[i] = mData[i];
		else
			res[i] = mData[i] + other.mData[i];
	}
	return res;
}

Wave Wave::operator-(const Wave& other) const
{
	Wave res(mSize);
	int n = mSize;
	if (other.mSize < mSize)
		n = other.mSize;
	for (int i = 0; i < mSize; i++)
	{
		if (i >= n)
			res[i] = mData[i];
		else
			res[i] = mData[i] - other.mData[i];
	}
	return res;
}

Wave Wave::operator*(const Wave& other) const
{
	Wave res(mSize);
	int n = mSize;
	if (other.mSize < mSize)
		n = other.mSize;
	for (int i = 0; i < mSize; i++)
	{
		if (i >= n)
			res[i] = mData[i];
		else
			res[i] = mData[i] * other.mData[i];
	}
	return res;
}

Wave Wave::operator*(const float& f) const
{
	Wave res(mSize);
	for (int i = 0; i < mSize; i++)
		res[i] = mData[i] * f;
	return res;
}

Wave Wave::operator/(const float& f) const
{
	Wave res(mSize);
	for (int i = 0; i < mSize; i++)
		res[i] = mData[i] / f;
	return res;
}

Wave& Wave::operator+=(const Wave& other)
{
	int n = mSize;
	if (other.mSize < mSize)
		n = other.mSize;
	for (int i = 0; i < n; i++)
		mData[i] += other.mData[i];
	return *this;
}

Wave& Wave::operator-=(const Wave& other)
{
	int n = mSize;
	if (other.mSize < mSize)
		n = other.mSize;
	for (int i = 0; i < n; i++)
		mData[i] -= other.mData[i];
	return *this;
}

Wave& Wave::operator=(const Wave& other)
{
	if (mData)
		delete[] mData;

	mSize = other.mSize;
	mData = new float[mSize]();
	for (int i = 0; i < mSize; i++)
		mData[i] = other.mData[i];
	return *this;
}

float& Wave::operator[](const unsigned int& i) const
{
	return mData[i];
}

const unsigned int Wave::size() const
{
	return mSize;
}

void Wave::Initialize(int size)
{
	if (mData)
		delete[] mData;

	mSize = size;
	mData = new float[size]();
}
