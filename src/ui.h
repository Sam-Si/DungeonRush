#ifndef SNAKE_UI_H_
#define SNAKE_UI_H_
#include <SDL.h>

#include <optional>
#include <string>

#include "types.h"
#define UI_MAIN_GAP 40
#define UI_MAIN_GAP_ALT 22
int chooseOptions(int optsNum, Text** options);
void baseUi(int w, int h);
void mainUi();
void rankListUi(int count, Score** scores);
void localRankListUi();
std::optional<std::string> inputUi();
void launchLanGame();
void launchLocalGame(int localPlayerNum);
bool chooseLevelUi();
int chooseOnLanUi();
#endif
