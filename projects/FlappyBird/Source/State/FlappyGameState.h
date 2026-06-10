#ifndef FLAPPY_BIRD_GAME_STATE_H
#define FLAPPY_BIRD_GAME_STATE_H

struct FlappyGameState
{
    int score = 0;
    int bestScore = 0;
    bool gameOver = false;
};

#endif // FLAPPY_BIRD_GAME_STATE_H
