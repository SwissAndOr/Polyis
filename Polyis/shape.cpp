#include "Shape.h"

const std::vector<gridArray> Shape::shapes = { // Temp hardcoded vector of shapes before making the import system
	{{Tile(000, 255, 255, false), Tile(000, 255, 255, false), Tile(000, 255, 255, false), Tile(000, 255, 255, false)},
	{Tile(000, 255, 255, true), Tile(000, 255, 255, true), Tile(000, 255, 255, true), Tile(000, 255, 255, true)},
	{Tile(000, 255, 255, false), Tile(000, 255, 255, false), Tile(000, 255, 255, false), Tile(000, 255, 255, false)},
	{Tile(000, 255, 255, false), Tile(000, 255, 255, false), Tile(000, 255, 255, false), Tile(000, 255, 255, false)}
	},
	{{Tile(000, 000, 255, true), Tile(000, 000, 255, false), Tile(000, 000, 255, false)},
	{Tile(000, 000, 255, true), Tile(000, 000, 255, true), Tile(000, 000, 255, true)},
	{Tile(000, 000, 255, false), Tile(000, 000, 255, false), Tile(000, 000, 255, false)}
	},
	{{Tile(255, 128, 000, false), Tile(255, 128, 000, false), Tile(255, 128, 000, true), },
	{Tile(255, 128, 000, true), Tile(255, 128, 000, true), Tile(255, 128, 000, true)},
	{Tile(255, 128, 000, false), Tile(255, 128, 000, false), Tile(255, 128, 000, false)}
	},
	{{Tile(255, 255, 000, true), Tile(255, 255, 000, true)},
	{Tile(255, 255, 000, true), Tile(255, 255, 000, true)}
	},
	{{Tile(128, 255, 000, false), Tile(128, 255, 000, true), Tile(128, 255, 000, true)},
	{Tile(128, 255, 000, true), Tile(128, 255, 000, true), Tile(128, 255, 000, false)},
	{Tile(128, 255, 000, false), Tile(128, 255, 000, false), Tile(128, 255, 000, false)}
	},
	{{Tile(128, 000, 128, false), Tile(128, 000, 128, true), Tile(128, 000, 128, false)},
	{Tile(128, 000, 128, true), Tile(128, 000, 128, true), Tile(128, 000, 128, true)},
	{Tile(128, 000, 128, false), Tile(128, 000, 128, false), Tile(128, 000, 128, false)}
	},
	{{Tile(255, 000, 000, true), Tile(255, 000, 000, true), Tile(255, 000, 000, false)},
	{Tile(255, 000, 000, false), Tile(255, 000, 000, true), Tile(255, 000, 000, true)},
	{Tile(255, 000, 000, false), Tile(255, 000, 000, false), Tile(255, 000, 000, false)}
	}
};

int Shape::tiles = 4;

bool Shape::rotate(gridArray grid, bool clockwise) {
	bool success = true;
	gridArray rotData;

	for (int y = 0; y < data.size(); y++) {
		rotData.push_back({});
		for (int x = 0; x < data[y].size(); x++) {
			rotData[y].push_back(Tile());
		}
	}

	/* x is x pos of data (grid)
	* y is y pos of data (grid)
	* xx is x pos of tile in raw data (data)
	* yy is y pos of tile in raw data (data)
	* ((n - *if CW) yy + x) is the x pos of tile (grid)
	* ((n - *if CCW) xx + y) is the y pos of tile (grid)
	*/
	for (int xx = 0; xx <= data.size() - 1; xx++) {
		for (int yy = 0; yy <= data.size() - 1; yy++) {
			int fY, fX;

			if (clockwise) {
				fY = xx;
				fX = data.size() - 1 - yy;
			} else {
				fY = data.size() - 1 - xx;
				fX = yy;
			}

			if (data[yy][xx].exists) {
				rotData[fY][fX] = data[yy][xx]; // TODO: Include left/right and up/down in formula (waiting for multi tile shapes first so I can debug)

				if (fX + x < 0 || fX + x >= grid[0].size() || fY + y < 0 || fY + y >= grid.size() || grid[fY + y][fX + x].exists) {
					success = false;
				}
			}
		}
	}

	if (!success) {
		if (!wallKick(grid, rotData)) return false;
	}

	data = rotData;
	return true;
}

bool Shape::wallKick(gridArray grid, gridArray rotData) {
	std::set<kickDist> sset = {};//int shift[12][2] = {{0, 1}, {-1, 0}, {1, 0}, {0, -1}, {-1, 1}, {1, 1}, {-1, 1}, {-1, -1}, {0, 2}, {-2, 0}, {2, 0}, {0, -2}};

	for (int a = 1; a < tiles; a++) {
		for (int b = 0; b <= std::min(tiles - 2, a); b++) {
			sset.insert(kickDist(b, a));
		}
	}

	std::vector<kickDist> shift(sset.begin(), sset.end());

	// for (int n = 1; n <= 2; n++) { // Can shift up to 2 tiles
	for (int s = 0; s < shift.size(); s++) {
		for (int n = 0; n < 8; n++) {
			int newX = 0;// x + shift[s][0];
			int newY = 0;// y + shift[s][1];

			switch (n) {
			case 0:
				newX = x - shift[s].x;
				newY = y + shift[s].y;
				break;
			case 1:
				newX = x + shift[s].x;
				newY = y + shift[s].y;
				break;
			case 2:
				newX = x - shift[s].y;
				newY = y + shift[s].x;
				break;
			case 3:
				newX = x + shift[s].y;
				newY = y + shift[s].x;
				break;
			case 4:
				newX = x - shift[s].y;
				newY = y - shift[s].x;
				break;
			case 5:
				newX = x + shift[s].y;
				newY = y - shift[s].x;
				break;
			case 6:
				newX = x - shift[s].x;
				newY = y - shift[s].y;
				break;
			case 7:
				newX = x + shift[s].x;
				newY = y - shift[s].y;
				break;
			}

			bool b = false;

			for (int yy = 0; yy < data.size(); yy++) {
				for (int xx = 0; xx < data.size(); xx++) {
					if (!rotData[yy][xx].exists) continue;
					if (xx + newX < 0 || xx + newX >= grid[0].size() || yy + newY < 0 || yy + newY >= grid.size()) {
						b = true;
						break;
					} else if (grid[yy + newY][xx + newX].exists) {
						b = true;
						break;
					}
				}
				if (b) break;
			}
			if (b) continue;

			x = newX;
			y = newY;
			// printf("%i, %i\n", shift[s][0], shift[s][1]);
			return true;
		}
	}
	// }

	return false;
}

bool Shape::move(gridArray grid, bool right) {
	int newX = x + (right ? 1 : -1);

	for (int yy = 0; yy < data.size(); yy++) {
		for (int xx = 0; xx < data.size(); xx++) {
			if (data[yy][xx].exists) {
				if (xx + newX < 0 || xx + newX >= grid[0].size()) {
					return false;
				}

				if (grid[yy + y][xx + newX].exists) {
					return false;
				}
			}
		}
	}

	x = newX;
	return true;
}

bool Shape::fall(gridArray grid, bool set) {
	int newY = y + 1;

	for (int xx = 0; xx < data.size(); xx++) {
		for (int yy = 0; yy < data.size(); yy++) {
			if (data[yy][xx].exists) {
				if (yy + newY >= grid.size()) {
					return false;
				}

				if (grid[yy + newY][xx + x].exists) {
					return false;
				}
			}
		}
	}

	if (set) y = newY;
	return true;
}