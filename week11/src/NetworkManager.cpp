#include "NetworkManager.h"
#include "raylib.h"
#include <iostream>
#include <cstring>

NetworkManager::NetworkManager() 
    : host(nullptr), peer(nullptr), role(NetworkRole::NONE),
      hasNewState(false), hasNewInput(false) {
}

NetworkManager::~NetworkManager() {
    Shutdown();
}

bool NetworkManager::Initialize() {
    if (enet_initialize() != 0) {
        std::cerr << "Failed to initialize ENet!" << std::endl;
        return false;
    }
    return true;
}

void NetworkManager::Shutdown() {
    Disconnect();
    enet_deinitialize();
}

bool NetworkManager::StartHost(int port) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    
    host = enet_host_create(&address,  // 地址
                           2,          // 最大客户端数
                           2,          // 通道数
                           0,          // 输入带宽(0=无限制)
                           0);         // 输出带宽(0=无限制)
    
    if (host == nullptr) {
        std::cerr << "Failed to create host!" << std::endl;
        return false;
    }
    
    role = NetworkRole::HOST;
    TraceLog(LOG_INFO, "Host started on port %d", port);
    return true;
}

bool NetworkManager::ConnectToHost(const std::string& ip, int port) {
    host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (host == nullptr) {
        std::cerr << "Failed to create client!" << std::endl;
        return false;
    }
    
    ENetAddress address;
    enet_address_set_host(&address, ip.c_str());
    address.port = port;
    
    peer = enet_host_connect(host, &address, 2, 0);
    if (peer == nullptr) {
        std::cerr << "Failed to connect to host!" << std::endl;
        enet_host_destroy(host);
        host = nullptr;
        return false;
    }
    
    // 等待连接（最多5秒）
    ENetEvent event;
    if (enet_host_service(host, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        role = NetworkRole::CLIENT;
        TraceLog(LOG_INFO, "Connected to host at %s:%d", ip.c_str(), port);
        return true;
    }
    
    std::cerr << "Connection timeout!" << std::endl;
    enet_peer_reset(peer);
    enet_host_destroy(host);
    host = nullptr;
    peer = nullptr;
    return false;
}

void NetworkManager::Disconnect() {
    if (peer != nullptr) {
        enet_peer_disconnect(peer, 0);
        
        // 等待断开确认
        ENetEvent event;
        while (enet_host_service(host, &event, 1000) > 0) {
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                break;
            }
        }
        
        peer = nullptr;
    }
    
    if (host != nullptr) {
        enet_host_destroy(host);
        host = nullptr;
    }
    
    role = NetworkRole::NONE;
}

void NetworkManager::SendGameState(const GameStateSnapshot& state) {
    if (!IsConnected()) return;
    
    ENetPacket* packet = enet_packet_create(&state, sizeof(state),
                                           ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send(peer, 0, packet);
}

void NetworkManager::SendPlayerInput(const PlayerInput& input) {
    if (!IsConnected()) return;
    
    ENetPacket* packet = enet_packet_create(&input, sizeof(input),
                                           ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send(peer, 0, packet);
}

bool NetworkManager::ReceiveGameState(GameStateSnapshot& state) {
    if (hasNewState) {
        state = lastReceivedState;
        hasNewState = false;
        return true;
    }
    return false;
}

bool NetworkManager::ReceivePlayerInput(PlayerInput& input) {
    if (hasNewInput) {
        input = lastReceivedInput;
        hasNewInput = false;
        return true;
    }
    return false;
}

bool NetworkManager::IsConnected() const {
    return peer != nullptr && host != nullptr;
}

void NetworkManager::Service() {
    if (host == nullptr) return;
    
    ENetEvent event;
    while (enet_host_service(host, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                TraceLog(LOG_INFO, "Client connected from %x:%u",
                        event.peer->address.host,
                        event.peer->address.port);
                peer = event.peer;
                break;
                
            case ENET_EVENT_TYPE_RECEIVE:
                // 根据数据包大小判断类型
                if (event.packet->dataLength == sizeof(GameStateSnapshot)) {
                    memcpy(&lastReceivedState, event.packet->data, sizeof(GameStateSnapshot));
                    hasNewState = true;
                } 
                else if (event.packet->dataLength == sizeof(PlayerInput)) {
                    memcpy(&lastReceivedInput, event.packet->data, sizeof(PlayerInput));
                    hasNewInput = true;
                }
                
                enet_packet_destroy(event.packet);
                break;
                
            case ENET_EVENT_TYPE_DISCONNECT:
                TraceLog(LOG_INFO, "Client disconnected");
                peer = nullptr;
                break;
                
            default:
                break;
        }
    }
}