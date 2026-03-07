#include "storage.h"

#include <string.h>

namespace {
void readScore(FILE* f, Score* score) {
  if (!score) {
    return;
  }
  int damage = 0;
  int stand = 0;
  int killed = 0;
  int got = 0;
  fscanf(f, "%d %d %d %d\n", &damage, &stand, &killed, &got);
  score->addDamage(damage);
  score->addStand(stand);
  score->addKilled(killed);
  score->addGot(got);
  score->calcScore(0);
}

void writeScore(FILE* f, const Score* score) {
  if (!score) {
    return;
  }
  fprintf(f, "%d %d %d %d\n", score->damage(), score->stand(),
          score->killed(), score->got());
}

Score* cloneScore(const Score* score) {
  if (!score) {
    return new Score();
  }
  auto* clone = new Score();
  clone->addDamage(score->damage());
  clone->addStand(score->stand());
  clone->addKilled(score->killed());
  clone->addGot(score->got());
  clone->calcScore(0);
  return clone;
}
}  // namespace

void destroyRanklist(int n, Score** scores) {
  if (!scores) {
    return;
  }
  for (int i = 0; i < n; i++) {
    delete scores[i];
  }
}
Score** insertScoreToRanklist(Score* score, int* n, Score** scores) {
  if (!score || !n) {
    return scores;
  }
  for (int i = 0; i < *n; i++) {
    if (scores[i]->rank() < score->rank()) {
      if (*n < STORAGE_RANKLIST_NUM) {
        scores = static_cast<Score**>(realloc(scores, sizeof(Score*) * (++*n)));
      }
      for (int j = *n - 1; j > i; j--) {
        scores[j] = scores[j - 1];
      }
      scores[i] = cloneScore(score);
      return scores;
    }
  }
  return scores;
}

void writeRanklist(const char* path, int n, Score** scores) {
  FILE* f = fopen(path, "w");
  if (f == NULL) {
    fprintf(stderr, "writeRanklist: Can not create file\n");
    return;
  } 
  fprintf(f, "%d\n", n);
  for (int i = 0; i < n; i++) writeScore(f, scores[i]);
  fclose(f);
}
Score** readRanklist(const char* path, int* n) {
  FILE* f = fopen(path, "r");
  if (!f) {
    *n = 1;
    Score** scores = static_cast<Score**>(malloc(sizeof(Score*) * (*n)));
    scores[0] = new Score();
    return scores;
  }
  fscanf(f, "%d", n);
  Score** scores = static_cast<Score**>(malloc(sizeof(Score*) * (*n)));
  for (int i = 0; i < *n; i++) {
    scores[i] = new Score();
    readScore(f, scores[i]);
  }
  fclose(f);
  return scores;
}
void updateLocalRanklist(Score* score) {
  int n;
  Score** scores = readRanklist(STORAGE_PATH, &n);
  scores = insertScoreToRanklist(score, &n, scores);
  writeRanklist(STORAGE_PATH, n, scores);
  destroyRanklist(n, scores);
  free(scores);
}
