#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <direct.h>
#include <iterator>
#include <random>
#include <stdio.h>
#include <string>
#include <unordered_set>
#include <vector>

#include "Shape.h"
#include "Text.h"

typedef std::vector<int> intArray;
typedef std::vector<float> floatArray;
typedef std::vector<std::vector<Tile>> gridArray;
typedef std::vector<Text> textArray;

enum fallState {
	FELL,
	PLACED,
};

enum gameState {
	PLAYING,
	ENDED,
	PAUSED,
	MAIN_MENU
};

bool handleEvents();
bool update(float deltaTime);
void paintGame();
void paintMenu(float deltaTime);

fallState fall();
void addShape();
void newShape();

void refreshText();

void beginGame(int lvl = 1, bool customLevel = false);
void pauseGame(bool pause = true);
void returnToMenu();

void saveSettings();
void loadSettings();

void saveScores();
void loadScores();

bool init();
void initSettings(bool load);

bool godDammitEthanWhyDidYouNameTheSoundGameOver();
void close();

SDL_Window *window;
SDL_Renderer *renderer;

Mix_Music *korobeinki;
Mix_Chunk *move, *rotate, *clear, *difficult, *placed, *gameOver;

Text scoreT, scoreNumT, linesT, linesNumT, levelT, levelNumT, nextT, menuT, holdT;

int tileLength = 34, tiles = 4, width = 10, height = 22, gridLineWidth = 2, wdx = 0, wdy = 0;
int screenWidth = tileLength * (width + 12), screenHeight = tileLength * (height - 2);

int selectedMenuIndex = 0, selectedSubmenuIndex = 0, selectedEndMenuIndex = 0;

Shape *currentShape;
intArray shapeIndexes;
intArray nextShapeIndexes;
int currentShapeIndex = 0, heldIndex = -1;
gridArray grid(height, std::vector<Tile>(width));

unsigned int score = 0, lines = 0, level = 1, lineClearCombos = 0, startingLevel = 1;
floatArray lineClearPoints = {100, 300, 500, 800, 1.5, 50};
bool lastClearDifficult = false;

float fastSpeed = 30.0F, normalSpeed = (float) level, fastFallTime = 0.0F, normalFallTime = 0.0F, lockTime = 0.0F, lockDelay = .25F;
int nextShapes = 3; // No more than 7
bool isFast = false, canHold = true, isLocking = false, menuFocus = true, debugShowDataArea = false, isCustom = false;

gameState state = MAIN_MENU;

Text title;

struct menuOption {
	Text text;
	float y, size = (float) Text::defaultSize;
} mainSubs[6];

int playX = 0, playY = 0;
textArray playOptions(10), endOptions(2);

enum displayType {
	BOOLEAN,
	INTEGER,
	FLOAT,
	STRING
};

struct customOption {
	Text text, valueText;
	displayType type;
	int currentOption;

	int Imin, Imax;
	float Fmin, Fmax;
	std::vector<std::string> strings;
} custom[11], options[4];

std::vector<textArray> highscores;

Text customPlay, resetControls;

struct controlOption {
	Text text, keyText;
	SDL_Keycode key;
} controls[9];
int currentEditingIndex = -1;

struct scoreEntry {
	std::string name;
	int score;
	Text nameT, scoreT;
};
std::vector<scoreEntry*> scoreEntries;

int main(int argc, char *argv[]) {
	init();

	Uint32 lastTime, currentTime = SDL_GetTicks();
	float deltaTime;

	while (true) {
		lastTime = currentTime;
		currentTime = SDL_GetTicks();
		deltaTime = (currentTime - lastTime) / 1000.0f;

		if (!handleEvents()) break;
		if (!update(deltaTime)) break;
	}

	close();
	return 0;
}

bool handleEvents() {
	SDL_Event e;

	while (SDL_PollEvent(&e) != 0) {
		switch (e.type) {
		case SDL_QUIT:
			return false;
		case SDL_WINDOWEVENT:
			if ((e.window.event == SDL_WINDOWEVENT_MOVED || e.window.event == SDL_WINDOWEVENT_FOCUS_LOST || e.window.event == SDL_WINDOWEVENT_RESIZED) && state == PLAYING) {
				pauseGame(true);
			}

			if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
				SDL_GetWindowSize(window, &screenWidth, &screenHeight);

				float width = (float) screenWidth, height = (float) screenHeight;
				
				tileLength = (int) std::min(width / 22.0F, height / 20.0F);
				Text::defaultSize = tileLength;
				gridLineWidth = std::max(1, tileLength / 12);

				wdx = (screenWidth - (tileLength * 22)) / 2;
				wdy = (screenHeight - (tileLength * 20)) / 2;

				printf("%i, %i, %i, %i, %f\n", screenWidth, screenHeight, wdx, wdy, (float) tileLength * 0.6F);

				refreshText();
			}
			break;
		case SDL_KEYDOWN:
			switch (state) {
			case PLAYING:
				if (e.key.keysym.sym == controls[0].key || e.key.keysym.sym == controls[1].key) {
					if (currentShape->move(grid, e.key.keysym.sym == controls[1].key)) {
						lockTime = 0;
						Mix_PlayChannel(-1, move, 0);
					}
				}

				if (e.key.keysym.sym == controls[4].key || e.key.keysym.sym == controls[5].key) {
					if (currentShape->rotate(grid, e.key.keysym.sym == controls[4].key)) {
						lockTime = 0;
						Mix_PlayChannel(-1, rotate, 0);
					}
				}

				if (e.key.keysym.sym == controls[2].key && e.key.repeat == 0) {
					isFast = true;
				}

				if (e.key.keysym.sym == controls[3].key && e.key.repeat == 0) {
					while (fall() == FELL) {
						score += 2;
						scoreNumT.change(std::to_string(score));
					}
					lockTime = lockDelay;
				}

				if (e.key.keysym.sym == controls[6].key && canHold) {
					int p = currentShapeIndex;
					bool first = heldIndex == -1;
					if (first) newShape(); else currentShapeIndex = heldIndex;
					heldIndex = p;

					if (!first) {
						if (currentShape != NULL) delete currentShape;
						currentShape = new Shape;

						currentShape->data = Shape::shapes[currentShapeIndex];
						currentShape->x = (grid[0].size() - currentShape->data.size()) / 2;

						godDammitEthanWhyDidYouNameTheSoundGameOver();
						isFast = false;
					}

					canHold = false;
				}

				if (e.key.keysym.sym == controls[7].key || e.key.keysym.sym == SDLK_ESCAPE) {
					pauseGame(true);
				}

				if (e.key.keysym.sym == controls[8].key) {
					debugShowDataArea = !debugShowDataArea;
				}
				break;
			case PAUSED:
				if (e.key.keysym.sym == controls[7].key || e.key.keysym.sym == SDLK_ESCAPE) {
					pauseGame(false);
				}

				if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
					returnToMenu();
				}
				break;
			case MAIN_MENU:
				if (selectedMenuIndex == 3 && currentEditingIndex >= 0) {
					controls[currentEditingIndex].key = e.key.keysym.sym;
					currentEditingIndex = -1;
				} else {
					if (e.key.keysym.sym == SDLK_UP && (e.key.repeat == 0 || !menuFocus)) {
						if (menuFocus) {
							selectedMenuIndex = std::max(0, std::min(5, selectedMenuIndex - 1));
							playX = playY = selectedSubmenuIndex = 0;
						} else {
							switch (selectedMenuIndex) {
							case 0:
								playY = std::max(0, std::min(1, playY - 1));
								break;
							case 1:
								if (currentEditingIndex < 0)
									selectedSubmenuIndex = std::max(0, std::min(11, selectedSubmenuIndex - 1));
								else {
									switch (custom[currentEditingIndex].type) {
									case BOOLEAN:
										custom[currentEditingIndex].currentOption = 1 - custom[currentEditingIndex].currentOption;
										break;
									case INTEGER:
										custom[currentEditingIndex].currentOption = std::min(custom[currentEditingIndex].Imax, std::max(custom[currentEditingIndex].Imin, custom[currentEditingIndex].currentOption + 1));
										break;
									case FLOAT:
										custom[currentEditingIndex].currentOption = std::min((int) log2(custom[currentEditingIndex].Fmax), std::max((int) log2(custom[currentEditingIndex].Fmin), custom[currentEditingIndex].currentOption + 1));
										break;
									case STRING:
										custom[currentEditingIndex].currentOption++;
										if (custom[currentEditingIndex].currentOption >= custom[currentEditingIndex].strings.size()) custom[currentEditingIndex].currentOption = 0;
									}
								}
								break;
							case 2:
								if (currentEditingIndex < 0)
									selectedSubmenuIndex = std::max(0, std::min(3, selectedSubmenuIndex - 1));
								else {
									switch (options[currentEditingIndex].type) {
									case BOOLEAN:
										options[currentEditingIndex].currentOption = 1 - options[currentEditingIndex].currentOption;
										break;
									case INTEGER:
										options[currentEditingIndex].currentOption = std::min(options[currentEditingIndex].Imax, std::max(options[currentEditingIndex].Imin, options[currentEditingIndex].currentOption + 1));
										break;
									}
								}
								break;
							case 3:
								selectedSubmenuIndex = std::max(0, std::min(9, selectedSubmenuIndex - 1));
								break;
							}
						}
					}

					if (e.key.keysym.sym == SDLK_DOWN && (e.key.repeat == 0 || !menuFocus)) {
						if (menuFocus) {
							selectedMenuIndex = std::max(0, std::min(5, selectedMenuIndex + 1));
							playX = playY = selectedSubmenuIndex = 0;
						} else {
							switch (selectedMenuIndex) {
							case 0:
								playY = std::max(0, std::min(1, playY + 1));
								break;
							case 1:
								if (currentEditingIndex < 0)
									selectedSubmenuIndex = std::max(0, std::min(11, selectedSubmenuIndex + 1));
								else {
									switch (custom[currentEditingIndex].type) {
									case BOOLEAN:
										custom[currentEditingIndex].currentOption = 1 - custom[currentEditingIndex].currentOption;
										break;
									case INTEGER:
										custom[currentEditingIndex].currentOption = std::min(custom[currentEditingIndex].Imax, std::max(custom[currentEditingIndex].Imin, custom[currentEditingIndex].currentOption - 1));
										break;
									case FLOAT:
										custom[currentEditingIndex].currentOption = std::min((int) log2(custom[currentEditingIndex].Fmax), std::max((int) log2(custom[currentEditingIndex].Fmin), custom[currentEditingIndex].currentOption - 1));
										break;
									case STRING:
										custom[currentEditingIndex].currentOption--;
										if (custom[currentEditingIndex].currentOption < 0) custom[currentEditingIndex].currentOption = custom[currentEditingIndex].strings.size() - 1;
									}
								}
								break;
							case 2:
								if (currentEditingIndex < 0)
									selectedSubmenuIndex = std::max(0, std::min(3, selectedSubmenuIndex + 1));
								else {
									switch (options[currentEditingIndex].type) {
									case BOOLEAN:
										options[currentEditingIndex].currentOption = 1 - options[currentEditingIndex].currentOption;
										break;
									case INTEGER:
										options[currentEditingIndex].currentOption = std::min(options[currentEditingIndex].Imax, std::max(options[currentEditingIndex].Imin, options[currentEditingIndex].currentOption + 1));
										break;
									}
								}
								break;
							case 3:
								selectedSubmenuIndex = std::max(0, std::min(9, selectedSubmenuIndex + 1));
								break;
							}
						}
					}

					if (e.key.keysym.sym == SDLK_LEFT && !menuFocus) {
						switch (selectedMenuIndex) {
						case 0:
							if (playX - 1 < 0) {
								menuFocus = true;
							} else {
								playX = std::max(0, std::min(4, playX - 1));
							}
							break;
						case 1:
							if (currentEditingIndex < 0)
								menuFocus = true;
							else {
								switch (custom[currentEditingIndex].type) {
								case BOOLEAN:
									custom[currentEditingIndex].currentOption = 1 - custom[currentEditingIndex].currentOption;
									break;
								case INTEGER:
									custom[currentEditingIndex].currentOption = std::min(custom[currentEditingIndex].Imax, std::max(custom[currentEditingIndex].Imin, custom[currentEditingIndex].currentOption - 1));
									break;
								case FLOAT:
									custom[currentEditingIndex].currentOption = std::min((int) log2(custom[currentEditingIndex].Fmax), std::max((int) log2(custom[currentEditingIndex].Fmin), custom[currentEditingIndex].currentOption - 1));
									break;
								case STRING:
									custom[currentEditingIndex].currentOption--;
									if (custom[currentEditingIndex].currentOption < 0) custom[currentEditingIndex].currentOption = custom[currentEditingIndex].strings.size() - 1;
								}
							}
							break;
						case 2:
							if (currentEditingIndex < 0)
								menuFocus = true;
							else {
								switch (options[currentEditingIndex].type) {
								case BOOLEAN:
									options[currentEditingIndex].currentOption = 1 - options[currentEditingIndex].currentOption;
									break;
								case INTEGER:
									options[currentEditingIndex].currentOption = std::min(options[currentEditingIndex].Imax, std::max(options[currentEditingIndex].Imin, options[currentEditingIndex].currentOption - 1));
									break;
								}
							}
							break;
						case 3:
							menuFocus = true;
							break;
						}
					}

					if (e.key.keysym.sym == SDLK_RIGHT && !menuFocus) {
						switch (selectedMenuIndex) {
						case 0:
							playX = std::max(0, std::min(4, playX + 1));
							break;
						case 1:
							switch (custom[currentEditingIndex].type) {
							case BOOLEAN:
								custom[currentEditingIndex].currentOption = 1 - custom[currentEditingIndex].currentOption;
								break;
							case INTEGER:
								custom[currentEditingIndex].currentOption = std::min(custom[currentEditingIndex].Imax, std::max(custom[currentEditingIndex].Imin, custom[currentEditingIndex].currentOption + 1));
								break;
							case FLOAT:
								custom[currentEditingIndex].currentOption = std::min((int) log2(custom[currentEditingIndex].Fmax), std::max((int) log2(custom[currentEditingIndex].Fmin), custom[currentEditingIndex].currentOption + 1));
								break;
							case STRING:
								custom[currentEditingIndex].currentOption++;
								if (custom[currentEditingIndex].currentOption >= custom[currentEditingIndex].strings.size()) custom[currentEditingIndex].currentOption = 0;
							}
							break;
						case 2:
							switch (options[currentEditingIndex].type) {
							case BOOLEAN:
								options[currentEditingIndex].currentOption = 1 - options[currentEditingIndex].currentOption;
								break;
							case INTEGER:
								options[currentEditingIndex].currentOption = std::min(options[currentEditingIndex].Imax, std::max(options[currentEditingIndex].Imin, options[currentEditingIndex].currentOption + 1));
								break;
							}
							break;
						}
					}

					if ((e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) && !menuFocus) {
						switch (selectedMenuIndex) {
						case 0:
							beginGame(1 + playX + playY * 5, false);
							break;
						case 1:
							switch (currentEditingIndex) { // Apply Custom settings?
							default:
								break;
							}
							if (currentEditingIndex >= 0) {
								currentEditingIndex = -1;
							} else if (selectedSubmenuIndex == 11) {
								beginGame(1, true);
							} else {
								currentEditingIndex = selectedSubmenuIndex;
							}
							break;
						case 2:
							switch (currentEditingIndex) { // Apply Options
							case 0:
								Mix_VolumeMusic((128 * options[0].currentOption * options[1].currentOption) / 10000);
							case 2:
								Mix_VolumeChunk(move, (128 * options[0].currentOption * options[2].currentOption) / 10000);
								Mix_VolumeChunk(rotate, (128 * options[0].currentOption * options[2].currentOption) / 10000);
								Mix_VolumeChunk(clear, (128 * options[0].currentOption * options[2].currentOption) / 10000);
								Mix_VolumeChunk(difficult, (128 * options[0].currentOption * options[2].currentOption) / 10000);
								Mix_VolumeChunk(placed, (128 * options[0].currentOption * options[2].currentOption) / 10000);
								Mix_VolumeChunk(gameOver, (128 * options[0].currentOption * options[2].currentOption) / 10000);
								break;
							case 1:
								Mix_VolumeMusic((128 * options[0].currentOption * options[1].currentOption) / 10000);
								break;
							//case 3:
							//	tileLength = options[3].currentOption;
							//	// TODO: Reload windows
							//	break;
							default:
								break;
							}

							if (currentEditingIndex >= 0)
								currentEditingIndex = -1;
							else
								currentEditingIndex = selectedSubmenuIndex;
							break;
						case 3:
							if (currentEditingIndex >= 0) {
								currentEditingIndex = -1;
							} else if (selectedSubmenuIndex == 9) {
								initSettings(false);
							} else {
								currentEditingIndex = selectedSubmenuIndex;
							}
							break;
						}
					}

					if ((e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER || e.key.keysym.sym == SDLK_RIGHT) && menuFocus && selectedMenuIndex != 4) {
						if (selectedMenuIndex == 5) {
							if (e.key.keysym.sym != SDLK_RIGHT)
								return false;
						} else {
							menuFocus = false;
						}
					}
				}
				break;
			case ENDED:
				if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
					switch (selectedEndMenuIndex) {
					case 0:
						returnToMenu();
						break;
					case 1:
						beginGame(startingLevel, isCustom);
						break;
					default:
						break;
					}
				}

				if (e.key.keysym.sym == SDLK_DOWN) {
					selectedEndMenuIndex = std::max(0, std::min(1, selectedEndMenuIndex + 1));
				}

				if (e.key.keysym.sym == SDLK_UP) {
					selectedEndMenuIndex = std::max(0, std::min(1, selectedEndMenuIndex - 1));
				}
			default:
				break;
			}
			break;
		case SDL_KEYUP:
			if (e.key.keysym.sym == controls[2].key) {
				isFast = false;
			}
		default:
			break;
		}
	}

	return true;
}

bool update(float deltaTime) {
	if (state == PLAYING) {
		if (isLocking) {
			lockTime += deltaTime;
			if (lockTime >= lockDelay) {
				addShape();
				isLocking = false;
				lockTime = 0;
			} else if (currentShape->fall(grid, false)) {
				isLocking = false;
				lockTime = 0;
			}
		} else {
			(isFast ? fastFallTime : normalFallTime) += deltaTime;

			if ((isFast ? fastFallTime : normalFallTime) >= 1 / (isFast ? fastSpeed : normalSpeed)) {
				(isFast ? fastFallTime : normalFallTime) -= 1 / (isFast ? fastSpeed : normalSpeed);
				if (isFast) {
					score++;
					scoreNumT.change(std::to_string(score));
				}

				fall();
			}
		}
	}

	if (state == MAIN_MENU) paintMenu(deltaTime);
	else paintGame();

	SDL_RenderPresent(renderer);
	return true;
}

void paintGame() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	if (state != PAUSED) {
		int one = state == ENDED ? 208 : 64;
		SDL_SetRenderDrawColor(renderer, one, one, one, 255);
		SDL_Rect game = {wdx + tileLength * 6, wdy, tileLength * 10, tileLength * 20};
		SDL_RenderFillRect(renderer, &game);

		if (options[3].currentOption == 1) { // Only if "Ghost Piece" option is enabled
			int ghostY = currentShape->y;
			bool stop = false;
			do {
				ghostY++;
				for (int y = 0; y < currentShape->data.size() && !stop; y++) {
					for (int x = 0; x < currentShape->data.size() && !stop; x++) {
						if (currentShape->data[y][x].exists) {
							stop = (ghostY + y > 21 || grid[ghostY + y][currentShape->x + x].exists);
						}
					}
				}
			} while (!stop);

			for (int y = 0; y < currentShape->data.size(); y++) { // Paint ghost
				for (int x = 0; x < currentShape->data.size(); x++) {
					if (currentShape->data[y][x].exists) {
						SDL_Rect tile = {wdx + tileLength * (x + currentShape->x + 6), wdy + tileLength * (y - 3 + ghostY), tileLength, tileLength};
						SDL_SetRenderDrawColor(renderer, 96, 96, 96, 255);
						SDL_RenderFillRect(renderer, &tile);
					}
				}
			}
		}

		for (int y = 2; y < height; y++) { // Paint grid
			for (int x = 0; x < width; x++) {
				if (grid[y][x].exists) {
					SDL_Rect tile = {wdx + tileLength * (x + 6), wdy + tileLength * (y - 2), tileLength, tileLength};
					if (state == PLAYING) {
						SDL_SetRenderDrawColor(renderer, grid[y][x].r, grid[y][x].g, grid[y][x].b, 255);
					} else {
						SDL_SetRenderDrawColor(renderer, (grid[y][x].r + 765) / 4, (grid[y][x].g + 765) / 4, (grid[y][x].b + 765) / 4, 255);
					}
					SDL_RenderFillRect(renderer, &tile);
				}
			}
		}

		for (int y = 0; y < currentShape->data.size(); y++) { // Paint shape
			if (currentShape->y + y <= 1) continue;
			for (int x = 0; x < currentShape->data.size(); x++) {
				if (!currentShape->data[y][x].exists && !debugShowDataArea) continue;

				SDL_Rect tile = {wdx + tileLength * (x + currentShape->x + 6), wdy + tileLength * (y - 2 + currentShape->y), tileLength, tileLength};

				if (debugShowDataArea)
					SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

				if (currentShape->data[y][x].exists || !debugShowDataArea) {
					if (state == PLAYING) {
						SDL_SetRenderDrawColor(renderer, (Uint8) ((1 - (lockTime / lockDelay)) * currentShape->data[y][x].r + (72 * (lockTime / lockDelay))), (Uint8) ((1 - (lockTime / lockDelay)) * currentShape->data[y][x].g + (72 * (lockTime / lockDelay))), (Uint8) ((1 - (lockTime / lockDelay)) * currentShape->data[y][x].b + (72 * (lockTime / lockDelay))), 255);
					} else {
						SDL_SetRenderDrawColor(renderer, (currentShape->data[y][x].r + 765) / 4, (currentShape->data[y][x].g + 765) / 4, (currentShape->data[y][x].b + 765) / 4, 255);
					}
				}

				SDL_RenderFillRect(renderer, &tile);
			}
		}

		int two = state == ENDED ? 224 : 128;

		SDL_SetRenderDrawColor(renderer, two, two, two, 255);

		for (int x = 0; x <= width; x++) {
			SDL_Rect line = {wdx + tileLength * (6 + x) - gridLineWidth / 2, wdy, gridLineWidth, tileLength * 20};
			SDL_RenderFillRect(renderer, &line);
		}

		for (int y = 0; y < height - 1; y++) {
			SDL_Rect line = {wdx + tileLength * 6, wdy + tileLength * y - gridLineWidth / 2, tileLength * 10, gridLineWidth};
			SDL_RenderFillRect(renderer, &line);
		}
	}

	if (state != PLAYING) {
		if (state == ENDED) {
			SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
			SDL_Rect pauseBox = {wdx + (int) (tileLength * 7.5), wdy + tileLength * 7, tileLength * 7, tileLength * 6};
			SDL_RenderFillRect(renderer, &pauseBox);

			for (int n = 0; n < 2; n++) {
				if (selectedEndMenuIndex == n) {
					SDL_Rect selection = {wdx + tileLength * 8, wdy + tileLength * (9.375 + 1.5 * n), tileLength * 6, tileLength * 1.5};
					SDL_SetRenderDrawColor(renderer, 128, 0, 255, 255);
					SDL_RenderFillRect(renderer, &selection);
				}
				endOptions[n].paint(wdx + tileLength * 11, wdy + tileLength * (9.75 + 1.5 * n));
			}
		} else {
			SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
			SDL_Rect bg = {wdx + tileLength * 6, wdy, tileLength * 10, tileLength * 20};
			SDL_RenderFillRect(renderer, &bg);
		}

		SDL_Color red = {255, 128, 128};
		SDL_Color white = {255, 255, 255};
		menuT.change(state == ENDED ? "Game Over" : "Paused", tileLength, state == ENDED ? red : white);
		menuT.paint(wdx + tileLength * 11, wdy + tileLength * 7.75);
	}

	//SDL_Rect bar = {0, 0, tileLength * 6, tileLength * 20};
	//SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	//SDL_RenderFillRect(renderer, &bar);

	//bar = {tileLength * 16, 0, tileLength * 6, tileLength * 20};
	//SDL_RenderFillRect(renderer, &bar);

	scoreT.paint(wdx + tileLength * 3, wdy + tileLength / 2);
	scoreNumT.paint(wdx + tileLength * 3, wdy + tileLength * 2);

	linesT.paint(wdx + tileLength * 3, wdy + (int) (tileLength * 3.5));
	linesNumT.paint(wdx + tileLength * 3, wdy + tileLength * 5);

	levelT.paint(wdx + tileLength * 3, wdy + (int) (tileLength * 6.5));
	levelNumT.paint(wdx + tileLength * 3, wdy + tileLength * 8);

	nextT.paint(wdx + tileLength * 19, wdy + tileLength / 2);

	/// tileLength for the polyominoes on the sidebar (shrink to fit) - WIP
	int sideTile = tileLength * (4.0F / tiles);

	SDL_Rect next = {wdx + (int) (tileLength * 16.5), wdy + (int) (tileLength * 2.5), tileLength * 5, tileLength * ((ceil(tiles / 2) + 1) * nextShapes)};
	SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
	SDL_RenderFillRect(renderer, &next);

	holdT.paint(wdx + tileLength * 3, wdy + tileLength * 11);

	SDL_Rect hold = {wdx + tileLength / 2, wdy + (int) (tileLength * 12.5), tileLength * 5, tileLength * 5};
	SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);

	SDL_RenderFillRect(renderer, &hold);

	if (state != PAUSED) {

		int nextShapeIndex = shapeIndexes.size();

		for (unsigned x = 0; x < shapeIndexes.size(); x++) {
			if (shapeIndexes[x] != -1) {
				nextShapeIndex = x;
				break;
			}
		}

		for (int n = 0; n < nextShapes; n++) { // Print next shapes
			int nextShape;
			gridArray shape;
			if (nextShapeIndex + n >= (int) shapeIndexes.size()) {
				nextShape = nextShapeIndexes[nextShapeIndex + n - shapeIndexes.size()];
			} else {
				nextShape = shapeIndexes[nextShapeIndex + n];
			}

			shape = Shape::shapes[nextShape];

			float dx = 17.0F + (tiles - shape.size()) / 2.0F;//(nextShape == 0 || nextShape == 3 ? 17 : 17.5F);
			float dy = 0;// (nextShape == 3 ? 1 : (nextShape == 0 ? 0.5F : 0));

			for (int y = 0; y < shape.size(); y++) {
				for (int x = 0; x < shape.size(); x++) {
					if (!shape[y][x].exists) continue;
					SDL_Rect tile = {wdx + (int) (tileLength * dx + sideTile * x), wdy + (int) (tileLength * (3 - dy + (n * 3)) + sideTile * y), sideTile, sideTile};

					SDL_SetRenderDrawColor(renderer, shape[y][x].r, shape[y][x].g, shape[y][x].b, 255);
					SDL_RenderFillRect(renderer, &tile);
				}
			}
		}

		if (heldIndex >= 0) { // Paint held shape
			gridArray shape(tiles, std::vector<Tile>(tiles));
			shape = Shape::shapes[heldIndex];

			float dx = 1.0F + (tiles - shape.size()) / 2.0F;//(heldIndex == 0 || heldIndex == 3 ? 1 : 1.5F);
			float dy = 0;// (heldIndex == 3 ? 1 : (heldIndex == 0 ? 0.5F : 0));
			for (int y = 0; y < shape.size(); y++) {
				for (int x = 0; x < shape.size(); x++) {
					if (!shape[y][x].exists) continue;
					SDL_Rect tile = {wdx + (int) (tileLength * dx + sideTile * x), wdy + (int) (tileLength * (14 - dy) + sideTile * y), sideTile, sideTile};

					SDL_SetRenderDrawColor(renderer, shape[y][x].r, shape[y][x].g, shape[y][x].b, 255);
					SDL_RenderFillRect(renderer, &tile);
				}
			}
		}
	}
}

void paintMenu(float deltaTime) {
	SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
	SDL_RenderClear(renderer);

	title.paint(wdx + (tileLength * 12.5), wdy + tileLength);

	for (int y = 0; y < 6; y++) {
		if (y == selectedMenuIndex) {
			float oldSize = mainSubs[y].size;
			mainSubs[y].size = std::min(1.75F, mainSubs[y].size + deltaTime * 6);

			if (menuFocus) {
				SDL_Color selected = {255, selectedMenuIndex == 5 ? 128 : 255, 128};
				mainSubs[y].text.change(mainSubs[y].text.text, (int) (tileLength * mainSubs[y].size), selected);
			} else {
				mainSubs[y].text.change(mainSubs[y].text.text, (int) (tileLength * mainSubs[y].size));
			}

			if (mainSubs[y].y > 0) {
				mainSubs[y].y = std::max(0.0F, mainSubs[y].y - deltaTime * 18);
			} else if (mainSubs[y].y < 0) {
				mainSubs[y].y = std::min(0.0F, mainSubs[y].y + deltaTime * 18);
			}
		} else {
			float oldSize = mainSubs[y].size;
			mainSubs[y].size = std::max(1.0F, mainSubs[y].size - deltaTime * 6);

			if (oldSize != mainSubs[y].size)
				mainSubs[y].text.change(mainSubs[y].text.text, (int) (tileLength * mainSubs[y].size));

			float targetY = 2.0F * (y - selectedMenuIndex);
			// if (y > selectedMenuIndex) targetY++;

			if (mainSubs[y].y > targetY) {
				mainSubs[y].y = std::max(targetY, mainSubs[y].y - deltaTime * 18);
			} else if (mainSubs[y].y < targetY) {
				mainSubs[y].y = std::min(targetY, mainSubs[y].y + deltaTime * 18);
			}
		}

		mainSubs[y].text.paint(wdx + tileLength * 4, wdy + (int) (tileLength * (10.5 + mainSubs[y].y)), CENTER, CENTER);
	}

	if (selectedMenuIndex > 0 && selectedMenuIndex < 4) {
		SDL_Rect box = {wdx + (int) (tileLength * 7.5), wdy + (int) (tileLength * (4.75 + 1.25 * (float) selectedSubmenuIndex)), tileLength * 13, (int) (tileLength * 1.25)};
		if (menuFocus) {
			SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
		} else if ((selectedMenuIndex == 1 && selectedSubmenuIndex == 11) || (selectedMenuIndex == 3 && selectedSubmenuIndex == 9)) {
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		} else {
			SDL_SetRenderDrawColor(renderer, 128, 0, 255, 255);
		}
		SDL_RenderFillRect(renderer, &box);
	}

	switch (selectedMenuIndex) {
	case 0: // PLAY
		for (int n = 0; n < 10; n++) {
			SDL_Rect box = {wdx + (int) (tileLength * 7.25 + tileLength * 3 * (n < 5 ? n : n - 5)), wdy + (int) (tileLength * 9.75 + (n < 5 ? 0 : tileLength * 3)), (int) (tileLength * 2.25), (int) (tileLength * 2.25)};
			if (playX + playY * 5 == n && !menuFocus)
				SDL_SetRenderDrawColor(renderer, 128, 0, 255, 255);
			else
				SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
			SDL_RenderFillRect(renderer, &box);

			playOptions[n].paint(wdx + (int) (tileLength * 8.25 + tileLength * 3 * (n < 5 ? n : n - 5)), wdy + tileLength * 10 + (n < 5 ? 0 : tileLength * 3));
		}
		break;
	case 1: // CUSTOM
		for (int n = 0; n < 11; n++) {
			custom[n].text.paint(wdx + tileLength * 8, wdy + (int) (tileLength * (5 + 1.25 * (float) n)), LEFT);
			switch (custom[n].type) {
			case BOOLEAN:
				if (custom[n].currentOption == 0) {
					custom[n].valueText.change("Disabled", (int) (tileLength * .6));
				} else {
					custom[n].valueText.change("Enabled", (int) (tileLength * .6));
				}
				break;
			case INTEGER:
				custom[n].valueText.change(std::to_string(custom[n].currentOption), (int) (tileLength * .6));
				break;
			case FLOAT:
				custom[n].valueText.change((custom[n].currentOption < 0 ? "1 / " : "") + (std::to_string((int) pow(2, abs(custom[n].currentOption)))), (int) (tileLength * .6));
				break;
			case STRING:
				custom[n].valueText.change(custom[n].strings[custom[n].currentOption], (int) (tileLength * .6));
				break;
			default:
				break;
			}
			if (n == currentEditingIndex) {
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
				SDL_Rect box = {wdx + (int) (tileLength * 19.9 - custom[n].valueText.getWidth()), wdy + (int) (tileLength * (4.9 + 1.25 * (float) n)), custom[n].valueText.getWidth() + (int) (tileLength * 0.3), (int) (tileLength * 0.95)};
				SDL_RenderFillRect(renderer, &box);
			}
			custom[n].valueText.paint(wdx + tileLength * 20, wdy + (int) (tileLength * (5 + 1.25 * (float) n)), RIGHT);
		}

		customPlay.paint(wdx + tileLength * 14, wdy + tileLength * 18.75);
		break;
	case 2: // OPTIONS
		for (int n = 0; n < 4; n++) {
			options[n].text.paint(wdx + tileLength * 8, wdy + (int) (tileLength * (5 + 1.25 * (float) n)), LEFT);
			switch (options[n].type) {
			case BOOLEAN:
				if (options[n].currentOption == 0) {
					options[n].valueText.change("Disabled", (int) (tileLength * .6));
				} else {
					options[n].valueText.change("Enabled", (int) (tileLength * .6));
				}
				break;
			case INTEGER:
				options[n].valueText.change(std::to_string(options[n].currentOption), (int) (tileLength * .6));
				break;
			default:
				break;
			}
			if (n == currentEditingIndex) {
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
				SDL_Rect box = {wdx + (int) (tileLength * 19.9 - options[n].valueText.getWidth()), wdx + (int) (tileLength * (4.9 + 1.25 * (float) n)), options[n].valueText.getWidth() + (int) (tileLength * 0.3), (int) (tileLength * 0.95)};
				SDL_RenderFillRect(renderer, &box);
			}
			options[n].valueText.paint(wdx + tileLength * 20, wdy + (int) (tileLength * (5 + 1.25 * (float) n)), RIGHT);
		}
		break;
	case 3: // CONTROL
	{
		std::unordered_set<SDL_Keycode> keys, duplicates;

		for (int n = 0; n < 9; n++) {
			if (std::find(keys.begin(), keys.end(), controls[n].key) != keys.end()) duplicates.insert(controls[n].key);
			keys.insert(controls[n].key);
		}

		for (int n = 0; n < 9; n++) {
			SDL_Color color;
			if (std::find(duplicates.begin(), duplicates.end(), controls[n].key) != duplicates.end())
				color = {255, 128, 128};
			else
				color = {255, 255, 255};

			controls[n].text.paint(wdx + tileLength * 8, wdy + (int) (tileLength * (5 + 1.25 * (float) n)), LEFT);
			controls[n].keyText.change(n == currentEditingIndex ? "" : SDL_GetKeyName(controls[n].key), (int) (tileLength * .6), color);
			controls[n].keyText.paint(wdx + tileLength * 20, wdy + (int) (tileLength * (5 + 1.25 * (float) n)), RIGHT);
		}

		resetControls.paint(wdx + tileLength * 14, wdy + tileLength * 16.25);
		break;
	}
	case 4: // SCORES
		for (int i = 0; i < scoreEntries.size(); i++) {
			scoreEntries[i]->nameT.paint(wdx + tileLength * 8, wdy + (int)(tileLength * (5 + 1.25 * (float)i)), LEFT);
			scoreEntries[i]->scoreT.paint(screenWidth - wdx - tileLength, wdy + (int)(tileLength * (5 + 1.25 * (float)i)), RIGHT);
		}
		break;
	case 5: // EXIT
		break;
	default
		:
			printf
				(
				"u are kill"
				)
				;
			while
				(
				true
				) {
				new
					int
					;
			} // TODO: YES
	}
}

fallState fall() {
	if (!currentShape->fall(grid, true)) {
		isLocking = true;
		return PLACED;
	}
	return FELL;
}

void addShape() {
	for (int x = 0; x < currentShape->data.size(); x++) {
		for (int y = 0; y < currentShape->data.size(); y++) {
			if (currentShape->data[y][x].exists) {
				grid[currentShape->y + y][currentShape->x + x] = currentShape->data[y][x];
			}
		}
	}

	int yMin = 0, linesCleared = 0;
	for (int y = height - 1; y >= yMin;) {
		bool blank = false;

		for (int x = 0; x < width; x++) {
			if (!grid[y][x].exists) {
				blank = true;
				break;
			}
		}

		if (!blank) {
			linesCleared++;
			lines++;

			linesNumT.change(std::to_string(lines));

			yMin++;

			for (int yy = y; yy >= 1; yy--) {
				for (int x = 0; x < width; x++) {
					grid[yy][x] = grid[yy - 1][x];
				}
			}

			for (int x = 0; x < width; x++) {
				grid[0][x].exists = false;
			}
		} else {
			y--;
		}
	}

	if (linesCleared > 0) {
		score += (int) (lineClearPoints[linesCleared - 1] * level * ((linesCleared == 4 && lastClearDifficult) ? lineClearPoints[4] : 1) + lineClearPoints[5] * lineClearCombos * level);

		scoreNumT.change(std::to_string(score));

		lastClearDifficult = linesCleared == 4;
		lineClearCombos++;
		level = startingLevel + lines / (isCustom ? custom[10].currentOption : 10);
		levelNumT.change(std::to_string(level));
		linesNumT.change(std::to_string(lines));
		normalSpeed = (0.3F * (float) pow(level, 1.5F) + 0.7F) * (isCustom ? (float) pow(2, custom[4].currentOption) : 1.0F);
		// printf("%f / %f = %f (%f/s)\n", (float) pow(2, custom[4].currentOption), (0.3F * (float) pow(level, 1.5F) + 0.7F), normalSpeed, 1.0F / normalSpeed);
		lockDelay = (float) (sqrt(level) + 2.0F) / 6.0F;
		fastSpeed = std::max(fastSpeed, normalSpeed);

		if (lastClearDifficult) {
			Mix_PlayChannel(-1, difficult, 0);
		} else {
			Mix_PlayChannel(-1, clear, 0);
		}
	} else {
		lineClearCombos = 0;
		Mix_PlayChannel(-1, placed, 0);
	}

	newShape();
}

void newShape() {
	for (unsigned i = 0; i < shapeIndexes.size(); i++) {
		if (shapeIndexes[i] == -1) continue;

		if (currentShape != NULL) delete currentShape;
		currentShape = new Shape;

		currentShape->data = Shape::shapes[shapeIndexes[i]];
		currentShapeIndex = shapeIndexes[i];
		currentShape->x = (grid[0].size() - currentShape->data.size()) / 2;

		shapeIndexes[i] = -1;

		if (godDammitEthanWhyDidYouNameTheSoundGameOver()) return;

		isFast = false;
		canHold = true;

		return;
	}

	shapeIndexes = nextShapeIndexes;

	for (int i = 0; i < Shape::shapes.size(); i++) nextShapeIndexes[i] = i;

	std::random_shuffle(nextShapeIndexes.begin(), nextShapeIndexes.end());

	newShape();
}

void refreshText() {
	title.change("POLYIS", tileLength * 3, {0, 255, 0});
	mainSubs[0].text.change("Play");
	mainSubs[1].text.change("Custom");
	mainSubs[2].text.change("Options");
	mainSubs[3].text.change("Controls");
	mainSubs[4].text.change("Scores");
	mainSubs[5].text.change("Exit");

	for (int i = 0; i < 6; i++) {
		mainSubs[i].y = 2.0F * (i - selectedMenuIndex);
		mainSubs[i].size = 1;
	}
	mainSubs[selectedMenuIndex].size = 2;

	for (int i = 0; i < 10; i++) {
		playOptions[i].change(playOptions[i].text, tileLength * 1.5);
	}

	custom[0].text.change("Tiles per Polyomino", tileLength * .6);
	custom[1].text.change("Grid Width", tileLength * .6);
	custom[2].text.change("Grid Height", tileLength * .6);
	custom[3].text.change("Grid Shape", tileLength * .6);
	custom[4].text.change("Gravity Multiplier", tileLength * .6);
	custom[5].text.change("Gravity Direction", tileLength * .6);
	custom[6].text.change("Flood Fill Gravity", tileLength * .6);
	custom[7].text.change("Lives", tileLength * .6);
	custom[8].text.change("Show Shapes", tileLength * .6);
	custom[9].text.change("Lock Delay Time", tileLength * .6);
	custom[10].text.change("Lines per Level", tileLength * .6);

	customPlay.change("Play", tileLength * .6);

	options[0].text.change("Master Volume", tileLength * .6);
	options[1].text.change("Music Volume", tileLength * .6);
	options[2].text.change("Sound FX Volume", tileLength * .6);
	options[3].text.change("Ghost Piece", tileLength * .6);

	controls[0].text.change("Move Left", tileLength * .6);
	controls[1].text.change("Move Right", tileLength * .6);
	controls[2].text.change("Soft Drop", tileLength * .6);
	controls[3].text.change("Hard Drop", tileLength * .6);
	controls[4].text.change("Rotate Clockwise", tileLength * .6);
	controls[5].text.change("Rotate Counterclockwise", tileLength * .6);
	controls[6].text.change("Hold Shape", tileLength * .6);
	controls[7].text.change("Pause", tileLength * .6);
	controls[8].text.change("Debug Mode", tileLength * .6);

	resetControls.change("Reset", tileLength * .6);

	for (int i = 0; i < scoreEntries.size(); i++) {
		scoreEntries[i]->nameT.change(scoreEntries[i]->nameT.text, tileLength * .6);
		scoreEntries[i]->scoreT.change(scoreEntries[i]->scoreT.text, tileLength * .6);
	}

	endOptions[0].change("Main Menu", tileLength * .6);
	endOptions[1].change("Play Again", tileLength * .6);

	scoreT.change("Score");
	scoreNumT.change(std::to_string(score));
	linesT.change("Lines");
	linesNumT.change(std::to_string(lines));
	levelT.change("Level");
	levelNumT.change(std::to_string(level));
	nextT.change("Next");
	holdT.change("Hold");
}

void beginGame(int lvl, bool customLevel) {
	grid = gridArray(height, std::vector<Tile>(width));

	currentShape = NULL;
	currentShapeIndex = 0;

	nextShapeIndexes = shapeIndexes = {};

	for (unsigned i = 0; i < Shape::shapes.size(); i++) {
		nextShapeIndexes.push_back(i);
	}

	std::random_shuffle(nextShapeIndexes.begin(), nextShapeIndexes.end());

	newShape();

	heldIndex = -1;

	score = lines = lineClearCombos = 0;
	bool lastClearDifficult = false;

	fastFallTime = normalFallTime = lockTime = 0.0F;
	isFast = isLocking = false;
	canHold = true;

	isCustom = customLevel;

	startingLevel = level = lvl;

	levelNumT.change(std::to_string(level));
	normalSpeed = (0.3F * (float) pow(level, 1.5F) + 0.7F) * (isCustom ? (float) pow(2, custom[4].currentOption) : 1);
	lockDelay = ((float) (sqrt(level) + 2) / 6) * (isCustom ? (float) pow(2, custom[9].currentOption) : 1);
	fastSpeed = std::max(30.0F, normalSpeed);

	Mix_PlayMusic(korobeinki, -1);

	scoreNumT.change(std::to_string(score));
	linesNumT.change(std::to_string(lines));
	levelNumT.change(std::to_string(startingLevel));

	state = PLAYING;
}

void pauseGame(bool pause) {
	if (pause) Mix_PauseMusic(); else Mix_ResumeMusic();

	state = pause ? PAUSED : PLAYING;
}

void returnToMenu() {
	if (Mix_PlayingMusic() != NULL) Mix_HaltMusic();

	menuFocus = true;
	playX = playY = 0;
	selectedMenuIndex = selectedSubmenuIndex = 0;
	currentEditingIndex = -1;

	mainSubs[0].text.change("Play", tileLength * 2);
	mainSubs[1].text.change("Custom");
	mainSubs[2].text.change("Options");
	mainSubs[3].text.change("Controls");
	mainSubs[4].text.change("Scores");
	mainSubs[5].text.change("Exit");

	for (int i = 0; i < 6; i++) {
		mainSubs[i].y = 2.0F * i;
		mainSubs[i].size = 1;
	}
	mainSubs[0].size = 2;

	state = MAIN_MENU;
}

void saveSettings() {
	FILE *settingsFile;

	if (fopen_s(&settingsFile, "settings.cfg", "w") != 0) return;

	for (int n = 0; n < 4; n++) {
		std::string text = options[n].text.text;
		std::replace(text.begin(), text.end(), ' ', '_');
		std::transform(text.begin(), text.end(), text.begin(), ::tolower);
		fprintf(settingsFile, "%s=%i\n", text.c_str(), options[n].currentOption);
	}

	for (int n = 0; n < 9; n++) {
		std::string text = controls[n].text.text;
		std::replace(text.begin(), text.end(), ' ', '_');
		std::transform(text.begin(), text.end(), text.begin(), ::tolower);
		fprintf(settingsFile, "%s=%i\n", text.c_str(), controls[n].key);
	}

	fclose(settingsFile);
}

void loadSettings() {
	FILE *settingsFile;

	if (fopen_s(&settingsFile, "settings.cfg", "r") != 0) return;

	std::string key = "", val = "";
	bool keyFinding = true;
	char c = 0;
	int i = -1;
	int t = 0;
	while (c != EOF) {
		c = fgetc(settingsFile);
		if (c == '=') {
			keyFinding = false;

			for (int n = 0; n < 4; n++) {
				std::string text = options[n].text.text;
				std::replace(text.begin(), text.end(), ' ', '_');
				std::transform(text.begin(), text.end(), text.begin(), tolower);
				if (text == key) {
					i = n;
					t = 2;
					break;
				}
			}
			
			if (t <= 0) {
				for (int n = 0; n < 9; n++) {
					std::string text = controls[n].text.text;
					std::replace(text.begin(), text.end(), ' ', '_');
					std::transform(text.begin(), text.end(), text.begin(), tolower);
					if (text == key) {
						i = n;
						t = 3;
						break;
					}
				}
			}
		} else if (c == '\n') {
			keyFinding = true;

			if (t == 2) options[i].currentOption = stoi(val);
			else if (t == 3) controls[i].key = stoi(val);

			key = "";
			val = "";
			t = 0;
		} else if (keyFinding) {
			key += c;
		} else {
			val += c;
		}
	}

	fclose(settingsFile);

	Mix_VolumeMusic((128 * options[0].currentOption * options[1].currentOption) / 10000);
	Mix_VolumeChunk(move, (128 * options[0].currentOption * options[2].currentOption) / 10000);
	Mix_VolumeChunk(rotate, (128 * options[0].currentOption * options[2].currentOption) / 10000);
	Mix_VolumeChunk(clear, (128 * options[0].currentOption * options[2].currentOption) / 10000);
	Mix_VolumeChunk(difficult, (128 * options[0].currentOption * options[2].currentOption) / 10000);
	Mix_VolumeChunk(placed, (128 * options[0].currentOption * options[2].currentOption) / 10000);
	Mix_VolumeChunk(gameOver, (128 * options[0].currentOption * options[2].currentOption) / 10000);
}

void saveScores() {
	FILE *scoresFile;

	if (fopen_s(&scoresFile, "scores", "w") != 0) return;

	for (int i = 0; i < scoreEntries.size(); i++) {
		fprintf(scoresFile, "%i %s\n", scoreEntries[i]->score, scoreEntries[i]->name.c_str());
	}

	fclose(scoresFile);
}

void loadScores() {
	scoreEntries = {};
	FILE *scoresFile;

	if (fopen_s(&scoresFile, "scores", "r") != 0) return;

	std::string tempScore, name;
	bool scoreFinding = true;
	char c = 0;
	while (c != EOF) {
		c = fgetc(scoresFile);
		if (c == ' ' && scoreFinding) {
			scoreFinding = false;
		} else if (c == '\n') {
			scoreFinding = true;

			
			scoreEntry* newEntry = new scoreEntry;
			newEntry->name = name;
			newEntry->score = std::stoi(tempScore);
			newEntry->nameT.change(name, tileLength * .6);
			newEntry->scoreT.change(tempScore, tileLength * .6);
			scoreEntries.push_back(newEntry);
			
			tempScore = "";
			name = "";
		} else if (scoreFinding) {
			tempScore += c;
		} else {
			name += c;
		}
	}

	fclose(scoresFile);
}

bool init() {
	srand(time(0));

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		printf("Error: Failed to initiate SDL. SDL_Error: %s\n", SDL_GetError());
		return false;
	}

	window = SDL_CreateWindow("Polyis", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		printf("Error: Failed to create window. SDL Error: %s\n", SDL_GetError());
		return false;
	}

	SDL_SetWindowMinimumSize(window, 450, 416);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		printf("Error: Failed to create renderer. SDL Error: %s\n", SDL_GetError());
		return false;
	}

	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {
		printf("Error: Failed to initiate Image Subsystem. SDL_IMG Error: %s\n", IMG_GetError());
		return false;
	}

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 4, 2048) != 0) {
		printf("Error: Failed to initiate Mixer subsystem. SDL_Mixer Error: %s\n", Mix_GetError());
		return false;
	}

	korobeinki = Mix_LoadMUS("resources/korobeinki.wav");
	if (korobeinki == NULL) {
		printf("Failed to load sound \"Korobeinki\". SDL_Mixer Error: %s\n", Mix_GetError());
	}

	move = Mix_LoadWAV("resources/move.wav");
	if (move == NULL) {
		printf("Failed to load sound \"move\". SDL_Mixer Error: %s\n", Mix_GetError());
	}

	rotate = Mix_LoadWAV("resources/rotate.wav");
	if (rotate == NULL) {
		printf("Failed to load sound \"rotate\". SDL_Mixer Error: %s\n", Mix_GetError());
	}

	clear = Mix_LoadWAV("resources/clear.wav");
	if (clear == NULL) {
		printf("Failed to load sound \"clear\". SDL_Mixer Error: %s\n", Mix_GetError());
	}

	difficult = Mix_LoadWAV("resources/maxclear.wav");
	if (difficult == NULL) {
		printf("Failed to load sound \"difficult\". SDL_Mixer Error: %s\n", Mix_GetError());
	}

	placed = Mix_LoadWAV("resources/placed.wav");
	if (placed == NULL) {
		printf("Failed to load sound \"placed\". SDL_Mixer Error: %s\n", Mix_GetError());
	}

	gameOver = Mix_LoadWAV("resources/ended.wav");
	if (gameOver == NULL) {
		printf("Failed to load sound \"gameOver\". SDL_Mixer Error: %s\n", Mix_GetError());
	}

	if (TTF_Init() == -1) {
		printf("Error: Failed to initiate TTF Subsystem. SDL_TTF Error: %s\n", TTF_GetError());
		return false;
	}

	Text::defaultSize = tileLength;
	Text::renderer = renderer;

	SDL_Surface* icon = IMG_Load("resources/icon.png");
	if (icon == NULL) {
		printf("Error: Failed to instatiate icon. SDL_IMG Error: %s\n", IMG_GetError());
	}

	SDL_SetWindowIcon(window, icon);

	title.change("POLYIS", tileLength * 3, {0, 255, 0});
	mainSubs[0].text.change("Play", tileLength * 2);
	mainSubs[1].text.change("Custom");
	mainSubs[2].text.change("Options");
	mainSubs[3].text.change("Controls");
	mainSubs[4].text.change("Scores");
	mainSubs[5].text.change("Exit");

	mainSubs[0].size = 2;
	for (int i = 1; i < 6; i++) {
		mainSubs[i].y = 2.0F * i;
	}

	for (int i = 0; i < 10; i++) {
		playOptions[i].change(std::to_string(i + 1), (int) (tileLength * 1.5));
	}

	custom[0].text.change("Tiles per Polyomino", tileLength * .6);
	custom[1].text.change("Grid Width", tileLength * .6);
	custom[2].text.change("Grid Height", tileLength * .6);
	custom[3].text.change("Grid Shape", tileLength * .6);
	custom[4].text.change("Gravity Multiplier", tileLength * .6);
	custom[5].text.change("Gravity Direction", tileLength * .6);
	custom[6].text.change("Flood Fill Gravity", tileLength * .6);
	custom[7].text.change("Lives", tileLength * .6);
	custom[8].text.change("Show Shapes", tileLength * .6);
	custom[9].text.change("Lock Delay Time", tileLength * .6);
	custom[10].text.change("Lines per Level", tileLength * .6);

	custom[0].type = INTEGER;
	custom[1].type = INTEGER;
	custom[2].type = INTEGER;
	custom[3].type = STRING;
	custom[4].type = FLOAT;
	custom[5].type = STRING;
	custom[6].type = BOOLEAN;
	custom[7].type = INTEGER;
	custom[8].type = STRING;
	custom[9].type = FLOAT;
	custom[10].type = INTEGER;

	custom[0].Imin = 1;
	custom[0].Imax = 15;
	custom[1].Imin = 1;
	custom[1].Imax = 255;
	custom[2].Imin = 1;
	custom[2].Imax = 255;
	custom[3].strings = {"Rectangle", "Circle", "Triangle", "Hexagon", "Arrow"};
	custom[4].Fmin = 0.125F;
	custom[4].Fmax = 16;
	custom[5].strings = {"Down", "Up", "Left", "Right"};
	custom[7].Imax = 15;
	custom[8].strings = {"Always", "On Ground Only", "Falling Only", "After Falling (Momentary)", "Never", "LSD Mode"};
	custom[9].Fmin = 0.125F;
	custom[9].Fmax = 16;
	custom[10].Imin = 2;
	custom[10].Imax = 255;

	custom[0].currentOption = 4;
	custom[1].currentOption = 10;
	custom[2].currentOption = 22;
	custom[10].currentOption = 10;

	customPlay.change("Play", tileLength * .6);

	initSettings(true);
	loadScores();

	resetControls.change("Reset", tileLength * .6);

	endOptions[0].change("Main Menu", tileLength * .6);
	endOptions[1].change("Play Again", tileLength * .6);

	scoreT.change("Score");
	scoreNumT.change("0");
	linesT.change("Lines");
	linesNumT.change("0");
	levelT.change("Level");
	levelNumT.change("1");
	nextT.change("Next");
	holdT.change("Hold");

	return true;
}

void initSettings(bool load) {
	options[0].text.change("Master Volume", tileLength * .6);
	options[1].text.change("Music Volume", tileLength * .6);
	options[2].text.change("Sound FX Volume", tileLength * .6);
	// options[3].text.change("Game Resolution", tileLength * .6);
	options[3].text.change("Ghost Piece", tileLength * .6);

	options[0].type = INTEGER;
	options[1].type = INTEGER;
	options[2].type = INTEGER;
	// options[3].type = INTEGER;
	options[3].type = BOOLEAN;

	options[0].Imax = 100;
	options[1].Imax = 100;
	options[2].Imax = 100;
	// options[3].Imax = 54;
	// options[3].Imin = 3;

	options[0].currentOption = 100;
	options[1].currentOption = 100;
	options[2].currentOption = 100;
	// options[3].currentOption = 34;
	options[3].currentOption = 1;

	controls[0].text.change("Move Left", tileLength * .6);
	controls[1].text.change("Move Right", tileLength * .6);
	controls[2].text.change("Soft Drop", tileLength * .6);
	controls[3].text.change("Hard Drop", tileLength * .6);
	controls[4].text.change("Rotate Clockwise", tileLength * .6);
	controls[5].text.change("Rotate Counterclockwise", tileLength * .6);
	controls[6].text.change("Hold Shape", tileLength * .6);
	controls[7].text.change("Pause", tileLength * .6);
	controls[8].text.change("Debug Mode", tileLength * .6);

	controls[0].key = SDLK_LEFT;
	controls[1].key = SDLK_RIGHT;
	controls[2].key = SDLK_DOWN;
	controls[3].key = SDLK_SPACE;
	controls[4].key = SDLK_UP;
	controls[5].key = SDLK_z;
	controls[6].key = SDLK_c;
	controls[7].key = SDLK_p;
	controls[8].key = SDLK_F7;

	if (load) loadSettings();
	saveSettings();
}

std::string randomStringGenerator(const int len) { // TEMP FUNCTION. TO BE DELETED ONCE PROPER SCORE NAMING IS IMPLEMENTED.
	std::string string;
	std::string alphanum = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < len; ++i) {
		string += alphanum[rand() % alphanum.size()];
	}

	printf("String: %s\n", string.c_str());

	return string;
}
bool scoreSorting(scoreEntry* left, scoreEntry* right) { return left->score > right->score; }
bool godDammitEthanWhyDidYouNameTheSoundGameOver() { // TODO: Rename function to checkGameOver or something like that
	for (int x = 0; x < currentShape->data.size(); x++) {
		for (int y = 0; y < currentShape->data.size(); y++) {
			if (currentShape->data[y][x].exists) {
				if (grid[currentShape->y + y][currentShape->x + x].exists) {
					state = ENDED;
					selectedEndMenuIndex = 0;
					Mix_HaltMusic();
					Mix_PlayChannel(-1, gameOver, 0);

					if (!isCustom) {
						scoreEntry* newEntry = new scoreEntry;
						newEntry->name = randomStringGenerator(5);
						newEntry->score = score;
						newEntry->nameT.change(newEntry->name, tileLength * .6);
						newEntry->scoreT.change(std::to_string(score), tileLength * .6);
						scoreEntries.push_back(newEntry);

						std::sort(scoreEntries.begin(), scoreEntries.end(), scoreSorting);
					}

					return true;
				}
			}
		}
	}

	return false;
}

void close() {
	saveSettings();
	saveScores();

	Mix_FreeMusic(korobeinki);
	korobeinki = NULL;

	Mix_FreeChunk(move);
	Mix_FreeChunk(rotate);
	Mix_FreeChunk(clear);
	Mix_FreeChunk(difficult);
	Mix_FreeChunk(gameOver);
	Mix_FreeChunk(placed);
	move = NULL;
	rotate = NULL;
	clear = NULL;
	difficult = NULL;
	placed = NULL;
	gameOver = NULL;

	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	window = NULL;
	renderer = NULL;

	Mix_Quit();
	IMG_Quit();
	TTF_Quit();
	SDL_Quit();
}