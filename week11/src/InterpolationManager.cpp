// InterpolationManager.cpp
#include "InterpolationManager.h"
#include <cstring> 
#include <algorithm>

InterpolationManager::InterpolationManager()
    : previousTime(0), currentTime(0), hasTwoStates(false) {
    memset(&previousState, 0, sizeof(previousState));
    memset(&currentState, 0, sizeof(currentState));
}

void InterpolationManager::AddSnapshot(const GameStateSnapshot& snapshot) {
    previousState = currentState;
    previousTime = currentTime;
    
    currentState = snapshot;
    currentTime = snapshot.timestamp;
    
    // 如果这是第一个快照，直接复制
    if (!hasTwoStates) {
        previousState = currentState;
        previousTime = currentTime;
        hasTwoStates = true;
    }
}

InterpolatedState InterpolationManager::GetInterpolatedState(double renderTime) {
    InterpolatedState result;
    memset(&result, 0, sizeof(result));
    
    if (!hasTwoStates || currentTime <= previousTime) {
        // 没有足够的快照，返回当前状态
        result.ballCount = currentState.ballCount;
        for (int i = 0; i < result.ballCount && i < 10; i++) {
            result.ballX[i] = currentState.ballX[i];
            result.ballY[i] = currentState.ballY[i];
        }
        result.paddleX = currentState.paddle2X;
        result.paddleY = currentState.paddle2Y;
        return result;
    }
    
    // 计算插值因子
    double renderTimestamp = renderTime;
    double t = (renderTimestamp - previousTime) / (currentTime - previousTime);
    
    // 外推处理：如果渲染时间超出当前快照，允许适度外推（最多 50ms）
    if (t > 1.0) {
        double extrapolationLimit = 0.05; // 50ms 外推限制
        double extrapolationTime = renderTimestamp - currentTime;
        if (extrapolationTime > extrapolationLimit) {
            t = 1.0; // 限制外推
        } else {
            // 使用速度进行外推
            result.ballCount = currentState.ballCount;
            for (int i = 0; i < result.ballCount && i < 10; i++) {
                float extrapX = currentState.ballX[i] + currentState.ballSpeedX[i] * extrapolationTime;
                float extrapY = currentState.ballY[i] + currentState.ballSpeedY[i] * extrapolationTime;
                result.ballX[i] = extrapX;
                result.ballY[i] = extrapY;
            }
            result.paddleX = currentState.paddle2X;
            result.paddleY = currentState.paddle2Y;
            return result;
        }
    }
    
    t = (t < 0.0) ? 0.0 : (t > 1.0) ? 1.0 : t;
    
    // 线性插值（对每个球）
    result.ballCount = std::min(previousState.ballCount, currentState.ballCount);
    for (int i = 0; i < result.ballCount && i < 10; i++) {
        result.ballX[i] = previousState.ballX[i] + (currentState.ballX[i] - previousState.ballX[i]) * t;
        result.ballY[i] = previousState.ballY[i] + (currentState.ballY[i] - previousState.ballY[i]) * t;
    }
    
    result.paddleX = previousState.paddle2X + (currentState.paddle2X - previousState.paddle2X) * t;
    result.paddleY = previousState.paddle2Y + (currentState.paddle2Y - previousState.paddle2Y) * t;
    
    return result;
}