#include "Text.h"

SDL_Renderer *Text::renderer = NULL;
std::string Text::fontPath = "resources/arial.ttf";
int Text::defaultSize = 1;

Text::Text(std::string newText, int size, SDL_Color color) {
	texture = NULL;
	text = newText;
	if (newText == "") return;
	change(newText, size, color);
}

Text::~Text() {
	if (texture != NULL) SDL_DestroyTexture(texture);
}

void Text::change(std::string newText, int size, SDL_Color color) {
	text = newText;
	TTF_Font *font = TTF_OpenFont(fontPath.c_str(), std::max(13, size));

	if (texture != NULL) SDL_DestroyTexture(texture);

	SDL_Surface *surface = TTF_RenderText_Blended(font, newText.c_str(), color);
	texture = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_FreeSurface(surface);
	TTF_CloseFont(font);
}

void Text::paint(int x, int y, alignment h, alignment v) {
	SDL_Rect area = {x, y, 0, 0};

	SDL_QueryTexture(texture, NULL, NULL, &area.w, &area.h);
	width = area.w;
	height = area.h;

	switch (h) {
	case CENTER:
		area.x = x - area.w / 2;
		break;
	case RIGHT:
		area.x = x - area.w;
		break;
	default:
		break;
	}

	switch (v) {
	case CENTER:
		area.y = y - area.h / 2;
		break;
	case RIGHT:
		area.y = y - area.h;
		break;
	default:
		break;
	}

	SDL_RenderCopy(renderer, texture, NULL, &area);
}

int Text::getWidth() {
	return width;
}

int Text::getHeight() {
	return height;
}