// include/TextureCache.h
#ifndef TEXTURE_CACHE_H
#define TEXTURE_CACHE_H

#include <unordered_map>
#include <string>
#include <mutex>
#include <future>
#include <memory>
#include <iostream>    // ✅ 添加：std::cout, std::endl
#include <thread>      // ✅ 添加：std::this_thread::sleep_for
#include <chrono> 
#include "raylib.h"

class TextureCache {
private:
    std::unordered_map<std::string, Texture2D> cache;
    mutable std::mutex mtx;
    
    // 存储异步加载任务
    std::unordered_map<std::string, std::future<Texture2D>> pendingLoads;
    mutable std::mutex pendingMtx;
    
    TextureCache() = default;
    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;

public:
    static TextureCache& Instance() {
        static TextureCache instance;
        return instance;
    }
    
    // ========== 同步加载 ==========
    Texture2D GetTexture(const std::string& path) {
        std::lock_guard<std::mutex> lock(mtx);
        
        auto it = cache.find(path);
        if (it != cache.end()) {
            return it->second;
        }
        
        Texture2D tex = LoadTexture(path.c_str());
        if (tex.id != 0) {
            cache[path] = tex;
        }
        return tex;
    }
    void AddTexture(const std::string& key, Texture2D tex) {
        std::lock_guard<std::mutex> lock(mtx);
        if (cache.find(key) == cache.end()) {
            cache[key] = tex;
        }
    }
    // ========== 异步加载（使用 packaged_task） ==========
    std::future<Texture2D> LoadTextureAsync(const std::string& path) {
        // 检查是否已经在加载中
        {
            std::lock_guard<std::mutex> lock(pendingMtx);
            auto it = pendingLoads.find(path);
            if (it != pendingLoads.end()) {
                return std::move(it->second);
            }
        }
        
        // 检查缓存
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = cache.find(path);
            if (it != cache.end()) {
                // 已缓存，创建已就绪的 future
                std::promise<Texture2D> promise;
                promise.set_value(it->second);
                return promise.get_future();
            }
        }
        
        // ✅ 使用 std::packaged_task 创建异步任务
        auto task = std::make_shared<std::packaged_task<Texture2D()>>(
            [path]() -> Texture2D {
                // 模拟加载大纹理的耗时
                std::cout << "[Texture Worker] Loading: " << path << std::endl;
                
                // 模拟网络/磁盘延迟
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                
                // 实际加载纹理
                Texture2D tex = LoadTexture(path.c_str());
                
                if (tex.id != 0) {
                    std::cout << "[Texture Worker] Loaded: " << path 
                              << " (" << tex.width << "x" << tex.height << ")" << std::endl;
                } else {
                    std::cout << "[Texture Worker] Failed: " << path << std::endl;
                }
                
                return tex;
            }
        );
        
        // 获取 future
        std::future<Texture2D> future = task->get_future();
        
        // 在新线程中执行
        std::thread([task]() {
            (*task)();  // 执行 packaged_task
        }).detach();
        
        // 保存任务引用
        {
            std::lock_guard<std::mutex> lock(pendingMtx);
            pendingLoads[path] = std::move(future);
        }
        
        return std::move(pendingLoads[path]);
    }
    
    // ========== 检查并应用异步加载结果 ==========
    bool TryApplyAsyncTexture(const std::string& path) {
        std::lock_guard<std::mutex> pendingLock(pendingMtx);
        
        auto it = pendingLoads.find(path);
        if (it == pendingLoads.end()) return false;
        
        // 非阻塞检查
        auto status = it->second.wait_for(std::chrono::seconds(0));
        if (status == std::future_status::ready) {
            Texture2D tex = it->second.get();
            if (tex.id != 0) {
                std::lock_guard<std::mutex> cacheLock(mtx);
                cache[path] = tex;
            }
            pendingLoads.erase(it);
            return true;
        }
        return false;
    }
    
    // 检查纹理是否已缓存
    bool HasTexture(const std::string& path) const {
        std::lock_guard<std::mutex> lock(mtx);
        return cache.find(path) != cache.end();
    }
    
    // 获取缓存数量
    size_t Size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return cache.size();
    }
    
    // 清空所有缓存
    void Clear() {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto& [path, tex] : cache) {
            UnloadTexture(tex);
        }
        cache.clear();
    }
    
    ~TextureCache() {
        Clear();
    }
    
};

#endif // TEXTURE_CACHE_H