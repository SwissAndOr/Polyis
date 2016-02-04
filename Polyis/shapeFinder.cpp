#include "shapeFinder.h"

int ShapeFinder::n = 0;
const int ShapeFinder::counts[10] = {1, 1, 2, 7, 18, 60, 196, 704, 2500, 9189};
std::vector<std::vector<bool>> ShapeFinder::blank = {};


void ShapeFinder::start(int newN) {
	int startTime = std::clock();

	std::vector<std::vector<std::vector<bool>>> unique = {};
	std::vector<std::vector<std::vector<bool>>> allRots = {};
	n = std::min(10, std::max(1, newN));

	printf("Shape Finder Started with %i tiles\n", n);

	for (int x = 0; x < n; x++) {
		blank.push_back({});
		for (int y = 0; y < n; y++) {
			blank[x].push_back(false);
		}
	}

	std::vector<std::vector<bool>> grid = blank;
	grid[floor((float)n / 2 - 0.5)][floor((float)n / 2 - 0.5)] = true;

	createShapes(grid, &unique, &allRots);

	printf("n = %i\n", unique.size());

	for (int i = 0; i < unique.size(); i++) {

		for (int y = 0; y < n; y++) {
			for (int x = 0; x < n; x++) {
				printf("%c", unique[i][y][x] ? 219 : ' ');
			}
			printf("\n");
		}
		printf("\n");

	}

	printf("n = %i\n", unique.size());
	printf("%ims passed\n", std::clock() - startTime);
}

void ShapeFinder::createShapes(std::vector<std::vector<bool>> grid, std::vector<std::vector<std::vector<bool>>>* unique, std::vector<std::vector<std::vector<bool>>>* allRots) {
	if (unique->size() >= counts[n - 1])
		return;

	int relTiles[4][2] = { { -1, 0 }, { 0, 1 }, { 1, 0 }, { 0, -1 } };
	std::vector<std::pair<int, int>> tiles;

	for (int x = 0; x < n; x++) {
		for (int y = 0; y < n; y++) {
			if (grid[y][x] == true) {
				for (int i = 0; i < 4; i++) {

					if (x + relTiles[i][0] >= 0 && x + relTiles[i][0] < n) {
						if (y + relTiles[i][1] >= 0 && y + relTiles[i][1] < n) {
							if (!grid[y + relTiles[i][1]][x + relTiles[i][0]]) {

								bool stop = false;
								for (int v = 0; v < tiles.size(); v++) {
									if (tiles[v].second == x + relTiles[i][0] && tiles[v].first == y + relTiles[i][1]) {
										stop = true;
										break;
									}
								}

								if (!stop) {
									tiles.push_back({ y + relTiles[i][1], x + relTiles[i][0] });
								}
							}
						}
					}
				}
			}
		}
	}
	
	for (int i = 0; i < tiles.size(); i++) {
		std::vector<std::vector<bool>> newGrid = grid;
		newGrid[tiles[i].first][tiles[i].second] = true;

		int tileNumber = 0;
		for (int x = 0; x < n; x++) {
			for (int y = 0; y < n; y++) {
				if (newGrid[y][x])
					tileNumber++;
			}
		}

		if (tileNumber == n) {
			int topmostTile = n, leftmostTile = n;

			for (int y = 0; y < n; y++) {
				for (int x = 0; x < n; x++) {
					if (newGrid[y][x]) {
						if (x < leftmostTile)
							leftmostTile = x;
						if (y < topmostTile)
							topmostTile = y;
					}
				}
			}

			for (int y = topmostTile; y < n; y++) {
				for (int x = leftmostTile; x < n; x++) { 
					if (newGrid[y][x]) {
						newGrid[y][x] = false;
						newGrid[y - topmostTile][x - leftmostTile] = true;
					}
				}
			}

			bool stop = false;
			for (int ii = 0; ii < allRots->size(); ii++) {
				if (newGrid == (*allRots)[ii]) {
					stop = true;
					break;
				}
			}

			if (stop) continue;

			unique->push_back(newGrid);
			allRots->push_back(newGrid);

			printf("Unique Shapes Found: %i\n", unique->size());

			for (int ii = 0; ii < 3; ii++)
				allRots->push_back(rotate(newGrid, ii));
		} else {
			createShapes(newGrid, unique, allRots);
		}
	}
}

std::vector<std::vector<bool>> ShapeFinder::rotate(std::vector<std::vector<bool>> grid, int r) {
	std::vector<std::vector<bool>> rotGrid = blank;
	int topmostTile = n, leftmostTile = n;

	for (int y = 0; y < n; y++) {
		for (int x = 0; x < n; x++) {
			if (!grid[y][x]) continue;
			std::pair<int, int> rots[3] = {
				{ x, n - 1 - y },
				{ n - 1 - y, n - 1 - x },
				{ n - 1 - x, y }
			};

			rotGrid[rots[r].first][rots[r].second] = true;

			if (rots[r].first < topmostTile)
				topmostTile = rots[r].first;
			if (rots[r].second < leftmostTile)
				leftmostTile = rots[r].second;
		}
	}

	for (int y = 0; y < n; y++) {
		for (int x = 0; x < n; x++) {
			if (rotGrid[y][x]) {
				rotGrid[y - topmostTile][x - leftmostTile] = true;
				rotGrid[y][x] = false;
			}
		}
	}

	return rotGrid;
}