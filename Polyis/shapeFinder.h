#include <algorithm>
#include <ctime>
#include <stdio.h>
#include <set>
#include <vector>

class ShapeFinder {
public:
	static void start(int newN);

private:
	static int n;
	static const int counts[10];

	static void createShapes(std::vector<std::vector<bool>> grid, std::vector<std::vector<std::vector<bool>>>* unique, std::vector<std::vector<std::vector<bool>>>* allRots);
	static std::vector<std::vector<bool>> rotate(std::vector<std::vector<bool>> grid, int r);

	static std::vector<std::vector<bool>> blank;
};