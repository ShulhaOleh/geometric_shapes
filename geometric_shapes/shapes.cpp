#include <cmath>

bool circle(float x, float y, float r)
{
	return x * x + y * y < r;
}

bool rectangle(float x, float y, float w, float h)
{
	return abs(x) < w && abs(y) < h;
}

bool square(float x, float y, float l)
{
	return rectangle(x, y, l, l);
}
