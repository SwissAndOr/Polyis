#include <SDL.h>

#include <algorithm>
#include <set>
#include <vector>

struct Tile {
	Tile(Uint8 newR = 0, Uint8 newG = 0, Uint8 newB = 0, bool doesExist = false) : r(newR), g(newG), b(newB), exists(doesExist) {}

	bool exists;
	Uint8 r, g, b;
};

/// For easy sorting
struct kickDist {
public:
	kickDist(unsigned x, unsigned y) : x(x), y(y) {}
	unsigned x : 4, y : 4;

	bool operator<(const struct kickDist &kd2) const {
		return sqrt(this->x * this->x + this->y * this->y) < sqrt(kd2.x * kd2.x + kd2.y * kd2.y);
	}
};

typedef std::vector<std::vector<Tile>> gridArray;

class Shape {
public:
	static const std::vector<gridArray> shapes;

	static int tiles;

	gridArray data;
	int y = 0, x = 0;

	bool rotate(gridArray grid, bool clockwise);
	bool wallKick(gridArray grid, gridArray rotData);
	bool move(gridArray grid, bool right);
	bool fall(gridArray grid, bool set);
};