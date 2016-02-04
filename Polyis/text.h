#include <SDL.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <string>

enum alignment {
	LEFT,
	CENTER,
	RIGHT
};

class Text {
public:
	static SDL_Renderer *renderer;
	static std::string fontPath;
	static int defaultSize;

	std::string text;

	Text(std::string newText = "", int size = defaultSize, SDL_Color color = {255, 255, 255});
	~Text();
	void change(std::string newText, int size = defaultSize, SDL_Color color = {255, 255, 255});
	void paint(int x, int y, alignment h = CENTER, alignment v = LEFT);
	int getWidth();
	int getHeight();
	SDL_Texture *texture;

private:
	int width, height;
};