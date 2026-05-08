#ifndef INTERPOLATION_MANAGER_H
#define INTERPOLATION_MANAGER_H

#include "NetworkManager.h"

struct InterpolatedState {
    float ballX, ballY;
    float paddleX, paddleY;
};

class InterpolationManager {
public:
    InterpolationManager();
    
    void AddSnapshot(const GameStateSnapshot& snapshot);
    InterpolatedState GetInterpolatedState(double currentTime);
    
private:
    GameStateSnapshot previousState;
    GameStateSnapshot currentState;
    double previousTime;
    double currentTime;
    bool hasTwoStates;
};

#endif