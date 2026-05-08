#include "InterpolationManager.h"
#include <cstring> 

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
    
    if (!hasTwoStates || currentTime <= previousTime) {
        // 没有足够的快照，返回当前状态
        result.ballX = currentState.ballX;
        result.ballY = currentState.ballY;
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
            float extrapX = currentState.ballX + currentState.ballSpeedX * extrapolationTime;
            float extrapY = currentState.ballY + currentState.ballSpeedY * extrapolationTime;
            result.ballX = extrapX;
            result.ballY = extrapY;
            result.paddleX = currentState.paddle2X;
            result.paddleY = currentState.paddle2Y;
            return result;
        }
    }
    
    t = (t < 0.0) ? 0.0 : (t > 1.0) ? 1.0 : t;
    
    // 线性插值
    result.ballX = previousState.ballX + (currentState.ballX - previousState.ballX) * t;
    result.ballY = previousState.ballY + (currentState.ballY - previousState.ballY) * t;
    result.paddleX = previousState.paddle2X + (currentState.paddle2X - previousState.paddle2X) * t;
    result.paddleY = previousState.paddle2Y + (currentState.paddle2Y - previousState.paddle2Y) * t;
    
    return result;
}