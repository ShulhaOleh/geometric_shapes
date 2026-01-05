#include <cmath>

bool circle(float x, float y, float r)
{
	return x * x + y * y < r;
}

bool oval(float x, float y, float rx, float ry) {
	return (x * x) / (rx * rx) + (y * y) / (ry * ry) < 1.0f;
}

bool rectangle(float x, float y, float w, float h)
{
	return abs(x) < w && abs(y) < h;
}

bool square(float x, float y, float l)
{
	return rectangle(x, y, l, l);
}
