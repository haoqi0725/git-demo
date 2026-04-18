# 打砖块游戏 — 第三周升级指南

---

## 1. 碰撞检测改进

### 问题分析：当前代码的缺陷

```
球从侧面撞砖块 → 当前代码只会 ReverseY() → 方向错误
球高速移动时    → 可能一帧穿过砖块 → 漏检
挡板碰撞        → position.y 判断条件有逻辑漏洞
```

### 改进后的 Ball::CheckBrickCollision()

在 `Ball.h` 中增加新方法：

```cpp
// Ball.h — 新增方法
enum class CollisionSide { NONE, TOP, BOTTOM, LEFT, RIGHT };
CollisionSide CheckBrickCollision(Rectangle brickRect);
```

```cpp
// Ball.cpp — 实现四方向碰撞判断
CollisionSide Ball::CheckBrickCollision(Rectangle brickRect) {
    if (!CheckCollisionCircleRec(position, radius, brickRect)) {
        return CollisionSide::NONE;
    }

    // 计算球心到砖块各边的距离，判断从哪侧碰撞
    float ballCenterX = position.x;
    float ballCenterY = position.y;

    float brickCenterX = brickRect.x + brickRect.width  / 2.0f;
    float brickCenterY = brickRect.y + brickRect.height / 2.0f;

    float dx = ballCenterX - brickCenterX;  // 正 = 球在砖右侧
    float dy = ballCenterY - brickCenterY;  // 正 = 球在砖下侧

    // 用重叠深度判断主碰撞轴
    float overlapX = (brickRect.width  / 2.0f + radius) - fabs(dx);
    float overlapY = (brickRect.height / 2.0f + radius) - fabs(dy);

    if (overlapX < overlapY) {
        // 水平方向重叠更浅 → 左右碰撞
        return (dx > 0) ? CollisionSide::RIGHT : CollisionSide::LEFT;
    } else {
        // 垂直方向重叠更浅 → 上下碰撞
        return (dy > 0) ? CollisionSide::BOTTOM : CollisionSide::TOP;
    }
}
```

### 在 main.cpp 中使用新碰撞结果

```cpp
// 检查砖块碰撞（替换原有代码段）
for (auto& brick : bricks) {
    if (!brick.IsActive()) continue;

    auto side = ball.CheckBrickCollision(brick.GetRect());
    if (side != CollisionSide::NONE) {
        brick.SetActive(false);
        score += 10;

        // 根据碰撞面精确反弹
        switch (side) {
            case CollisionSide::TOP:
            case CollisionSide::BOTTOM:
                ball.ReverseY();
                break;
            case CollisionSide::LEFT:
            case CollisionSide::RIGHT:
                ball.ReverseX();   // 新增 Ball::ReverseX()
                break;
            default: break;
        }
        break;  // 一帧只处理一块
    }
}
```

在 `Ball.h` 中同时新增：
```cpp
void ReverseX() { speed.x *= -1; }
```

---

## 2. 游戏状态系统

### GameState 枚举

```cpp
// GameState.h
#ifndef GAMESTATE_H
#define GAMESTATE_H

enum class GameState {
    MENU,       // 主菜单
    PLAYING,    // 游戏进行中
    PAUSED,     // 暂停
    GAME_OVER,  // 失败
    WIN         // 胜利
};

#endif
```

### 完整 main.cpp（含生命值与分数）

```cpp
#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "GameState.h"
#include <vector>

// ——— 辅助：重建砖块 ———
std::vector<Brick> BuildBricks() {
    std::vector<Brick> bricks;
    const float brickW = 75.0f, brickH = 25.0f;
    const float startX = 25.0f, startY = 80.0f;
    const float padX   = 10.0f, padY   = 8.0f;
    const int   cols   = 9,     rows   = 4;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            float x = startX + c * (brickW + padX);
            float y = startY + r * (brickH + padY);
            bricks.emplace_back(x, y, brickW, brickH);
        }
    }
    return bricks;
}

int main() {
    const int SW = 800, SH = 600;
    InitWindow(SW, SH, "打砖块 2D — 第三周");
    SetTargetFPS(60);

    // ——— 游戏变量 ———
    GameState state = GameState::MENU;
    int  score = 0;
    int  lives = 3;
    bool newRecord = false;

    Ball    ball({400, 300}, {3, -4}, 10);
    Paddle  paddle(350, 550, 100, 20);
    auto    bricks = BuildBricks();

    // ——— 主循环 ———
    while (!WindowShouldClose()) {

        // ===== UPDATE =====
        switch (state) {

        case GameState::MENU:
            if (IsKeyPressed(KEY_ENTER)) {
                state  = GameState::PLAYING;
                score  = 0;
                lives  = 3;
                bricks = BuildBricks();
                ball.Reset({400, 300}, {3, -4});
            }
            break;

        case GameState::PLAYING: {
            if (IsKeyPressed(KEY_P)) { state = GameState::PAUSED; break; }

            // 移动挡板
            if (IsKeyDown(KEY_LEFT))  paddle.MoveLeft(8);
            if (IsKeyDown(KEY_RIGHT)) paddle.MoveRight(8);

            // 移动球
            ball.Move();
            ball.BounceEdge(SW, SH);

            // 挡板碰撞
            ball.CheckPaddleCollision(paddle.GetRect());

            // 砖块碰撞
            int activeBricks = 0;
            for (auto& brick : bricks) {
                if (!brick.IsActive()) continue;
                activeBricks++;

                auto side = ball.CheckBrickCollision(brick.GetRect());
                if (side != CollisionSide::NONE) {
                    brick.SetActive(false);
                    activeBricks--;
                    score += 10;
                    switch (side) {
                        case CollisionSide::LEFT:
                        case CollisionSide::RIGHT: ball.ReverseX(); break;
                        default:                   ball.ReverseY(); break;
                    }
                    break;
                }
            }

            // 球落底
            if (ball.GetPosition().y + ball.GetRadius() >= SH) {
                lives--;
                if (lives <= 0) {
                    state = GameState::GAME_OVER;
                } else {
                    ball.Reset({400, 300}, {3, -4});
                }
            }

            // 全部砖块清除
            if (activeBricks == 0) {
                state = GameState::WIN;
            }
            break;
        }

        case GameState::PAUSED:
            if (IsKeyPressed(KEY_P)) state = GameState::PLAYING;
            break;

        case GameState::GAME_OVER:
        case GameState::WIN:
            if (IsKeyPressed(KEY_R)) {
                state  = GameState::MENU;
            }
            break;
        }

        // ===== DRAW =====
        BeginDrawing();
        ClearBackground(BLACK);

        if (state == GameState::MENU) {
            DrawText("打砖块 2D",            300, 200, 40, WHITE);
            DrawText("按 ENTER 开始",         290, 270, 24, GRAY);
            DrawText("← → 移动挡板  P 暂停", 230, 310, 20, DARKGRAY);

        } else if (state == GameState::PAUSED) {
            DrawText("已暂停",  340, 280, 36, YELLOW);
            DrawText("按 P 继续", 315, 330, 22, GRAY);

        } else if (state == GameState::GAME_OVER) {
            DrawText("GAME OVER",     270, 230, 48, RED);
            DrawText(TextFormat("得分: %d", score), 340, 290, 28, WHITE);
            DrawText("按 R 返回菜单", 295, 340, 22, GRAY);

        } else if (state == GameState::WIN) {
            DrawText("YOU WIN!",      295, 230, 48, GOLD);
            DrawText(TextFormat("得分: %d", score), 340, 290, 28, WHITE);
            DrawText("按 R 返回菜单", 295, 340, 22, GRAY);

        } else {
            // PLAYING / PAUSED 时绘制游戏对象
            DrawRectangle(0,    0, SW, 5,  DARKGRAY);
            DrawRectangle(0,    0, 5,  SH, DARKGRAY);
            DrawRectangle(SW-5, 0, 5,  SH, DARKGRAY);

            ball.Draw();
            paddle.Draw();
            for (auto& b : bricks) b.Draw();

            // HUD
            DrawText(TextFormat("分数: %d", score), 10,  10, 20, WHITE);
            DrawText(TextFormat("生命: %d", lives), 700, 10, 20, RED);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

---

## 3. 单元测试

### 核心思想

| 概念 | 含义 |
|------|------|
| **测试用例 (Test Case)** | 一个独立的"输入 → 预期输出"验证 |
| **断言 (Assert)** | 检查实际结果是否等于预期 |
| **测试套件 (Test Suite)** | 一组相关的测试用例集合 |
| **回归测试** | 修改代码后重跑全部测试，确保没破坏旧功能 |

### 轻量测试框架（无需第三方库）

```cpp
// test_runner.h — 极简断言宏
#pragma once
#include <iostream>
#include <string>

static int _pass = 0, _fail = 0;

#define ASSERT_EQ(actual, expected, msg)                              \
    if ((actual) == (expected)) {                                     \
        std::cout << "  [PASS] " << (msg) << "\n"; _pass++;          \
    } else {                                                          \
        std::cout << "  [FAIL] " << (msg)                            \
                  << "  (got " << (actual)                           \
                  << ", expected " << (expected) << ")\n"; _fail++;  \
    }

#define ASSERT_TRUE(cond, msg)  ASSERT_EQ((cond), true,  (msg))
#define ASSERT_FALSE(cond, msg) ASSERT_EQ((cond), false, (msg))

#define TEST_SUMMARY()                                                 \
    std::cout << "\n结果: " << _pass << " 通过 / "                    \
              << _fail << " 失败\n";                                   \
    return (_fail > 0) ? 1 : 0;
```

### 针对 Brick 的测试用例

```cpp
// test_brick.cpp
#include "test_runner.h"
#include "Brick.h"
#include <iostream>

void TestBrickInitialState() {
    std::cout << "=== Brick 初始状态 ===\n";
    Brick b(10, 20, 80, 30);
    ASSERT_TRUE(b.IsActive(), "新砖块默认为激活状态");

    Rectangle r = b.GetRect();
    ASSERT_EQ(r.x,      10.0f, "砖块 x 坐标");
    ASSERT_EQ(r.y,      20.0f, "砖块 y 坐标");
    ASSERT_EQ(r.width,  80.0f, "砖块宽度");
    ASSERT_EQ(r.height, 30.0f, "砖块高度");
}

void TestBrickSetActive() {
    std::cout << "=== Brick 激活/停用 ===\n";
    Brick b(0, 0, 80, 30);
    b.SetActive(false);
    ASSERT_FALSE(b.IsActive(), "SetActive(false) 后应为 false");

    // 停用的砖块不应检测碰撞
    Rectangle overlap = {0, 0, 80, 30};
    ASSERT_FALSE(b.CheckCollision(overlap), "非激活砖块不参与碰撞");

    b.SetActive(true);
    ASSERT_TRUE(b.CheckCollision(overlap), "重新激活后可碰撞");
}

void TestBrickCollisionDetection() {
    std::cout << "=== Brick 碰撞检测 ===\n";
    Brick b(100, 100, 80, 30);

    Rectangle inside  = {110, 105, 20, 10};  // 完全在砖块内
    Rectangle outside = {300, 300, 20, 10};  // 完全在砖块外
    Rectangle edge    = {175, 100, 20, 30};  // 刚好触碰右边缘

    ASSERT_TRUE (b.CheckCollision(inside),  "内部矩形应碰撞");
    ASSERT_FALSE(b.CheckCollision(outside), "外部矩形不应碰撞");
    ASSERT_TRUE (b.CheckCollision(edge),    "边缘矩形应碰撞");
}

int main() {
    TestBrickInitialState();
    TestBrickSetActive();
    TestBrickCollisionDetection();
    TEST_SUMMARY();
}
```

### 编译与运行测试

```bash
# 编译测试（不需要 raylib 的窗口部分，但 Brick.h/cpp 依赖 raylib 类型）
g++ test_brick.cpp Brick.cpp -I. -lraylib -o test_brick
./test_brick

# 预期输出：
# === Brick 初始状态 ===
#   [PASS] 新砖块默认为激活状态
#   [PASS] 砖块 x 坐标
#   ...
# 结果: 7 通过 / 0 失败
```

---

## 4. Git 分支管理工作流

### 本周推荐的分支结构

```
main          ← 只放稳定、可运行的版本
  └── dev     ← 日常开发集成分支
        ├── feature/game-state    ← 生命值 & 分数功能
        ├── feature/collision     ← 碰撞改进
        └── feature/unit-tests   ← 单元测试
```

### 完整操作命令

```bash
# ① 从 main 创建 dev 分支
git checkout main
git checkout -b dev

# ② 为每个功能创建独立分支
git checkout -b feature/game-state dev
# ... 编写代码 ...
git add GameState.h main.cpp
git commit -m "feat: 添加 GameState 枚举与生命值/分数系统"

# ③ 完成后合并回 dev
git checkout dev
git merge feature/game-state
git branch -d feature/game-state   # 删除已合并的功能分支

# ④ 碰撞改进分支
git checkout -b feature/collision dev
# ... 修改 Ball.cpp/Ball.h ...
git add Ball.h Ball.cpp
git commit -m "fix: 使用重叠深度算法实现四方向砖块碰撞判断"
git commit -m "feat: 新增 Ball::ReverseX() 方法"
git checkout dev
git merge feature/collision

# ⑤ 单元测试分支
git checkout -b feature/unit-tests dev
git add test_runner.h test_brick.cpp
git commit -m "test: 添加 Brick 类单元测试（7个用例）"
git checkout dev
git merge feature/unit-tests

# ⑥ dev 测试稳定后合并到 main
git checkout main
git merge dev
git tag v0.3.0 -m "第三周：碰撞改进 + 游戏状态 + 单元测试"
git push origin main --tags
```

### Commit Message 规范（Conventional Commits）

| 前缀 | 用途 | 示例 |
|------|------|------|
| `feat:` | 新功能 | `feat: 添加暂停功能` |
| `fix:` | 修复 bug | `fix: 球穿透砖块侧面问题` |
| `test:` | 测试相关 | `test: 新增 Ball 碰撞测试` |
| `refactor:` | 重构（不改功能） | `refactor: 提取 BuildBricks 函数` |
| `docs:` | 文档 | `docs: 更新 README` |

---

## 5. 本周任务检查清单

```
碰撞检测
  [ ] Ball::CheckBrickCollision() 返回 CollisionSide
  [ ] 左右碰砖触发 ReverseX()，上下碰砖触发 ReverseY()
  [ ] 挡板碰撞的 position.y 判断已修正

游戏状态
  [ ] GameState 枚举含 5 个状态
  [ ] 生命值 = 3，球落底 -1，归零进入 GAME_OVER
  [ ] 砖块清零进入 WIN 状态
  [ ] 分数每消砖 +10，显示在 HUD

单元测试
  [ ] test_runner.h 断言宏可用
  [ ] Brick 测试 7 个用例全部通过
  [ ] 理解"回归测试"的意义

Git 分支
  [ ] 创建了 dev + 3 个 feature 分支
  [ ] 每个 feature 独立 commit，commit message 符合规范
  [ ] feature 分支合并回 dev 后删除
  [ ] dev 稳定后合并到 main 并打 tag v0.3.0
```
