// InterpolationManager.h
#ifndef INTERPOLATION_MANAGER_H
#define INTERPOLATION_MANAGER_H

#include "NetworkManager.h"

struct InterpolatedState {
    // 多球支持（最多10个球）
    int ballCount;
    float ballX[10];
    float ballY[10];
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