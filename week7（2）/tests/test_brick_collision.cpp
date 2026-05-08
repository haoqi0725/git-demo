#include "../include/test_runner.h"
#include "../include/Ball.h"
#include <cmath>

// 测试1：球从顶部碰撞砖块
void TestBrickCollisionTop() {
    // 砖块：位置(350, 280)，宽80，高20
    Rectangle brick = {350, 280, 80, 20};
    // 砖块顶部 y = 280，球半径10，球心 y = 290 时球刚好接触砖块底部？
    // 实际上：球从顶部碰撞，意味着球在砖块上方，向下移动
    // 球心应该在砖块顶部上方半径距离处：y = 280 - 10 = 270
    Ball ball({400, 270}, {0, 5}, 10);
    
    auto side = ball.CheckBrickCollision(brick);
    ASSERT_EQ((int)side, (int)CollisionSide::TOP, "球从顶部碰撞砖块");
}

// 测试2：球从底部碰撞砖块
void TestBrickCollisionBottom() {
    Rectangle brick = {350, 280, 80, 20};
    // 砖块底部 y = 300，球心在 y = 310 时刚好接触砖块底部
    Ball ball({400, 310}, {0, -5}, 10);
    
    auto side = ball.CheckBrickCollision(brick);
    ASSERT_EQ((int)side, (int)CollisionSide::BOTTOM, "球从底部碰撞砖块");
}

// 测试3：球从左侧碰撞砖块
void TestBrickCollisionLeft() {
    Rectangle brick = {350, 280, 80, 20};
    // 砖块左侧 x = 350，球心在 x = 340 时刚好接触砖块左侧
    Ball ball({340, 290}, {5, 0}, 10);
    
    auto side = ball.CheckBrickCollision(brick);
    ASSERT_EQ((int)side, (int)CollisionSide::LEFT, "球从左侧碰撞砖块");
}

// 测试4：球从右侧碰撞砖块
void TestBrickCollisionRight() {
    Rectangle brick = {350, 280, 80, 20};
    // 砖块右侧 x = 430，球心在 x = 440 时刚好接触砖块右侧
    Ball ball({440, 290}, {-5, 0}, 10);
    
    auto side = ball.CheckBrickCollision(brick);
    ASSERT_EQ((int)side, (int)CollisionSide::RIGHT, "球从右侧碰撞砖块");
}

// 测试5：无碰撞
void TestBrickCollisionNone() {
    Rectangle brick = {350, 280, 80, 20};
    Ball ball({100, 100}, {0, 0}, 10);
    
    auto side = ball.CheckBrickCollision(brick);
    ASSERT_EQ((int)side, (int)CollisionSide::NONE, "无碰撞时应返回 NONE");
}

int main() {
    std::cout << "=== Ball::CheckBrickCollision 单元测试 ===" << std::endl;
    std::cout << std::endl;
    
    TestBrickCollisionTop();
    TestBrickCollisionBottom();
    TestBrickCollisionLeft();
    TestBrickCollisionRight();
    TestBrickCollisionNone();
    
    TEST_SUMMARY();
}
