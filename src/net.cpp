#include "net.h"
#include "types.h"
#include "render.h"
#include "res.h"
#include "game.h"
#include "storage.h"
#include "prng.h"

#include <string.h>
#include <stdlib.h>
#include <memory>
#include <vector>
#include <SDL_ttf.h>

#define CASSERT(EXPRESSION) switch (0) {case 0: case (EXPRESSION):;}

namespace {
Text* createNetText(const std::string& text, SDL_Color color) {
  SDL_Renderer* sdlRenderer = renderer();
  TTF_Font* ttfFont = font();
  if (!sdlRenderer || !ttfFont) {
    return nullptr;
  }
  return new Text(text, color, sdlRenderer, ttfFont);
}

void destroyNetText(Text* text) {
  delete text;
}
}

TCPsocket lanServerSocket;
TCPsocket lanClientSocket;
SDLNet_SocketSet socketSet;

static void CHECK() {
  CASSERT(sizeof(LanPacket) == 4);
}

HandShakePacket createHandShakePacket() {
  HandShakePacket handshakePacket;
  memset(&handshakePacket, 0, sizeof(handshakePacket));
  handshakePacket.seed = (PRNG::rand()*PRNG::rand())&LAN_SEED_MASK;
  return handshakePacket;
}

static inline unsigned packet2unsigned(void* packet) {
  return *static_cast<unsigned*>(packet);
}

static inline void fillPacket(void* packet, unsigned payload) {
  *static_cast<unsigned*>(packet) = payload;
}

void sendPlayerMovePacket(unsigned playerId, unsigned direction) {
  if (!lanClientSocket) return;
  static PlayerMovePacket p;
  p.type = 1;
  p.playerId = playerId;
  p.direction = direction;
  static char buffer[4];
  SDLNet_Write32(packet2unsigned(&p), buffer);
  SDLNet_TCP_Send(lanClientSocket, buffer, sizeof(PlayerMovePacket));
}

unsigned recvLanPacket(LanPacket* dest) {
  if (SDLNet_CheckSockets(socketSet, 0)) {
    static char buffer[sizeof(LanPacket)];
    SDLNet_TCP_Recv(lanClientSocket, buffer, sizeof(LanPacket));
    fillPacket(dest, SDLNet_Read32(buffer));
    return 1;
  }
  return 0;
}

void sendGameOverPacket(unsigned playerId) {
  if (!lanClientSocket) return;
  static GameOverPacket p;
  p.type = 2;
  p.playerId = playerId;
  static char buffer[4];
  SDLNet_Write32(packet2unsigned(&p), buffer);
  SDLNet_TCP_Send(lanClientSocket, buffer, sizeof(GameOverPacket));
}

void hostGame() {
  IPaddress listenIp;
  SDLNet_ResolveHost(&listenIp, INADDR_ANY, LAN_LISTEN_PORT);
  lanServerSocket = SDLNet_TCP_Open(&listenIp);
  if (!lanServerSocket) {
    fprintf(stderr, "hostGame: %s\n", SDLNet_GetError());
    exit(-1);
  }

  extern SDL_Color WHITE;
  Text* listening[4] = {
    createNetText("Listening", WHITE),
    createNetText("Listening.", WHITE),
    createNetText("Listening..", WHITE),
    createNetText("Listening...", WHITE),
    };
  
  SDL_Event e;
  unsigned frameCount = 0;
  while (!lanClientSocket) {
    lanClientSocket = SDLNet_TCP_Accept(lanServerSocket);
    clearRenderer();
    renderCenteredText(listening[(frameCount++)/20%4], SCREEN_WIDTH/2,
                       SCREEN_HEIGHT/2, 2);
    if (SDL_Renderer* sdlRenderer = renderer()) {
      SDL_RenderPresent(sdlRenderer);
    }
    bool quit = false;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT || 
          (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
        quit = true;
      }
    }
    if (quit) break;
  }

  // Clean up after connecting
  SDLNet_TCP_Close(lanServerSocket);
  lanServerSocket = NULL;
  for (unsigned i = 0; i < 4; i++) destroyNetText(listening[i]);

  blackout();

  if (!lanClientSocket) return;

  // Add socket to set
  socketSet = SDLNet_AllocSocketSet(1);
  SDLNet_TCP_AddSocket(socketSet, lanClientSocket);

  // Show the peer ip
  IPaddress* peerIp = SDLNet_TCP_GetPeerAddress(lanClientSocket);

  const char* peerName = SDLNet_ResolveIP(peerIp);
  std::unique_ptr<Text, decltype(&destroyNetText)> peerNameText(
      createNetText(peerName, WHITE), destroyNetText);
  clearRenderer();
  renderCenteredText(peerNameText.get(), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 2);
  if (SDL_Renderer* sdlRenderer = renderer()) {
    SDL_RenderPresent(sdlRenderer);
  }

  HandShakePacket handShakePacket = createHandShakePacket();
  char buffer[sizeof(HandShakePacket)];
  SDLNet_Write32(packet2unsigned(&handShakePacket), buffer);
  SDLNet_TCP_Send(lanClientSocket, buffer, sizeof(HandShakePacket));

  unsigned seed = handShakePacket.seed;
  fprintf(stderr, "seed: %d\n", seed);
  PRNG::srand(seed);

  setLevel(0);
  auto scores = startGame(1, 1, true);
  std::vector<Score*> rawScores;
  rawScores.reserve(scores.size());
  for (auto& score : scores) {
    rawScores.push_back(score.get());
  }
  destroyRanklist(static_cast<int>(rawScores.size()), rawScores.data());

  SDLNet_FreeSocketSet(socketSet);
  socketSet = NULL;
  SDLNet_TCP_Close(lanClientSocket);
  lanClientSocket = NULL;
}

void joinGame(const char* hostname, Uint16 port) {
  IPaddress serverIp;
  if (SDLNet_ResolveHost(&serverIp, hostname, port)) {
    fprintf(stderr, "joinGame: ResolveHost: %s\n", SDLNet_GetError());
    exit(-1);
  }

  extern SDL_Color WHITE;
  Text* connecting[4] = {
    createNetText("Connecting", WHITE),
    createNetText("Connecting.", WHITE),
    createNetText("Connecting..", WHITE),
    createNetText("Connecting...", WHITE),
    };
  
  bool quit = false;
  SDL_Event e;
  unsigned frameCount = 0;
  while (!lanClientSocket) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT || 
          (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
        quit = true;
      }
    }
    if (quit) break;

    lanClientSocket = SDLNet_TCP_Open(&serverIp);
    if (lanClientSocket == NULL) {
      fprintf(stderr, "connect: %s\n", SDL_GetError());
    }
    clearRenderer();
    renderCenteredText(connecting[(frameCount++)/20%4], SCREEN_WIDTH / 2,
                       SCREEN_HEIGHT / 2, 2);
    if (SDL_Renderer* sdlRenderer = renderer()) {
      SDL_RenderPresent(sdlRenderer);
    }
  }

  for (unsigned i = 0; i < 4; i++) destroyNetText(connecting[i]);

  if (!quit) {
    // Add socket to set
    socketSet = SDLNet_AllocSocketSet(1);
    SDLNet_TCP_AddSocket(socketSet, lanClientSocket);

    // Sync seed

    char buffer[sizeof(HandShakePacket)];
    SDLNet_TCP_Recv(lanClientSocket, buffer, sizeof(HandShakePacket));
    HandShakePacket handShakePacket;
    fillPacket(&handShakePacket, SDLNet_Read32(buffer));

    unsigned seed = handShakePacket.seed;
    PRNG::srand(seed);
    fprintf(stderr, "seed: %d\n", seed);

    // Start game
    setLevel(0);
    startGame(1, 1, false);
  }

  if (socketSet) SDLNet_FreeSocketSet(socketSet);
  socketSet = NULL;

  if (lanClientSocket) SDLNet_TCP_Close(lanClientSocket);
  lanClientSocket = NULL;
}
