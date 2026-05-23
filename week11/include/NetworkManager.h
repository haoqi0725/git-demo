#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <enet/enet.h>
#include <string>
#include <functional>

// 游戏状态快照（支持多球）
#pragma pack(push, 1)
struct GameStateSnapshot {
    // 基础信息
    float paddle1X, paddle1Y;
    float paddle2X, paddle2Y;
    int score;
    int lives;
    int currentLevel;
    double timestamp;
    int frameNumber;           // 添加这个字段！用于确认帧号
    
    // 多球支持（最多10个球）
    int ballCount;             // 球的数量
    float ballX[10];           // 球的X位置
    float ballY[10];           // 球的Y位置
    float ballSpeedX[10];      // 球的X速度
    float ballSpeedY[10];      // 球的Y速度
    bool isFireball[10];       // 火球状态
    bool isAttached[10];
    
    // 游戏状态标志
    bool gameStarted;          // 游戏是否已开始
    int activeBrickCount;      // 活跃砖块数量
};
#pragma pack(pop)

// 玩家输入（添加帧号）
#pragma pack(push, 1)
struct PlayerInput {
    float paddleX;
    float paddleY;
    bool leftPressed;
    bool rightPressed;
    bool spacePressed;
    double timestamp;
    int frameNumber;           // 帧号
};

#pragma pack(pop)
enum class NetworkRole {
    NONE,
    HOST,    // 服务器
    CLIENT   // 客户端
};

class NetworkManager {
public:
    static constexpr int DEFAULT_PORT = 12345;  

    NetworkManager();
    ~NetworkManager();
    
    // 初始化与关闭
    bool Initialize();
    void Shutdown();
    
    // 连接管理
    bool StartHost(int port = DEFAULT_PORT);  // ✅ 使用常量作为默认值
    bool ConnectToHost(const std::string& ip, int port = DEFAULT_PORT);  // ✅ 使用常量
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