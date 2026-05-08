#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <enet/enet.h>
#include <string>
#include <functional>

// 游戏状态快照（用于网络同步）
#pragma pack(push, 1)  // 确保字节对齐
struct GameStateSnapshot {
    float ballX, ballY;
    float ballSpeedX, ballSpeedY;
    float paddle1X, paddle1Y;  // 主机挡板
    float paddle2X, paddle2Y;  // 客户端挡板
    int score;
    int lives;
    int currentLevel;
    bool isFireball;
    double timestamp;
};
#pragma pack(pop)

// 玩家输入
#pragma pack(push, 1)
struct PlayerInput {
    float paddleX;
    float paddleY;
    bool leftPressed;
    bool rightPressed;
    bool spacePressed;
    double timestamp;
};
#pragma pack(pop)

enum class NetworkRole {
    NONE,
    HOST,    // 服务器
    CLIENT   // 客户端
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();
    
    // 初始化与关闭
    bool Initialize();
    void Shutdown();
    
    // 连接管理
    bool StartHost(int port = 1234);
    bool ConnectToHost(const std::string& ip, int port = 1234);
    void Disconnect();
    
    // 消息发送
    void SendGameState(const GameStateSnapshot& state);
    void SendPlayerInput(const PlayerInput& input);
    
    // 消息接收
    bool ReceiveGameState(GameStateSnapshot& state);
    bool ReceivePlayerInput(PlayerInput& input);
    
    // 状态查询
    bool IsConnected() const;
    NetworkRole GetRole() const { return role; }
    
    // 事件处理（需要每帧调用）
    void Service();
    
private:
    ENetHost* host;
    ENetPeer* peer;
    NetworkRole role;
    
    // 接收缓冲区
    GameStateSnapshot lastReceivedState;
    PlayerInput lastReceivedInput;
    bool hasNewState;
    bool hasNewInput;
};

#endif