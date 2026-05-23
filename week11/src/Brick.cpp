// Brick.cpp
#include "../include/Brick.h"
#include "../include/BatchRenderer.h"  // ✅ 新增：包含批绘制器头文件

// ✅ 新增：定义静态成员
BatchRenderer* Brick::s_batchRenderer = nullptr;
// 修复构造函数，添加 level 参数并正确初始化所有成员
Brick::Brick(float x, float y, float w, float h, Color color, int points, int hp, int level) 
    : rect({x, y, w, h}), 
      active(true),
      hitThisFrame(false),
      health(hp),          // 初始血量
      maxHealth(hp),       // 最大血量
      brickLevel(level),   // 保存关卡编号
      color(color),
      points(points)
{
    // 初始化颜色
    UpdateColor();
}
// Brick.cpp
void Brick::UpdateColor() {
    // 使用成员变量 health 和 brickLevel
    if (health >= 999) { 
        color = (Color){80, 85, 90, 255}; // 石头色
        return; 
    }

    if (brickLevel == 1) { // 森林绿色系
        if (health >= 3) color = (Color){0, 100, 0, 255};
        else if (health == 2) color = DARKGREEN;
        else color = LIME;
    } 
    else if (brickLevel == 2) { // 落日橙色系
        if (health >= 4) color = (Color){139, 69, 19, 255};
        else if (health == 3) color = (Color){180, 80, 20, 255};
        else if (health == 2) color = ORANGE;
        else color = YELLOW;
    } 
    else if (brickLevel == 3) { // 深海蓝色系
        if (health >= 4) color = (Color){0, 0, 139, 255};
        else if (health == 3) color = (Color){0, 80, 180, 255};
        else if (health == 2) color = BLUE;
        else color = SKYBLUE;
    }
}

bool Brick::Hit() {
    // 无敌砖块不扣血
    if (health >= 999) return false; 

    // 已经破碎
    if (health <= 0) return true;

    // 扣血
    health--;

    // 检查是否完全破碎
    if (health <= 0) {
        active = false;
        return true; 
    }

    // 更新外观
    UpdateColor();
    return false;
}

void Brick::Draw() {
    if (!active) return;

    if (health >= 999) {
        // 石头无敌砖块样式
        Color stoneHigh = { 130, 135, 140, 255 };
        Color stoneShadow = { 40, 40, 45, 255 };

        DrawRectangleRec(rect, color);

        // 高光边（左上）
        DrawLineEx({rect.x, rect.y}, {rect.x + rect.width, rect.y}, 2, stoneHigh);
        DrawLineEx({rect.x, rect.y}, {rect.x, rect.y + rect.height}, 2, stoneHigh);
        
        // 阴影边（右下）
        DrawLineEx({rect.x + rect.width, rect.y}, {rect.x + rect.width, rect.y + rect.height}, 2, stoneShadow);
        DrawLineEx({rect.x, rect.y + rect.height}, {rect.x + rect.width, rect.y + rect.height}, 2, stoneShadow);

        // 黑色轮廓
        DrawRectangleLinesEx(rect, 1, (Color){0, 0, 0, 150});
    } 
    else {
        // 普通砖块
        DrawRectangleRec(rect, color);
        
        // 亮色内边框
        DrawRectangleLinesEx(rect, 1, (Color){255, 255, 255, 40}); 
        // 黑色外轮廓
        DrawRectangleLinesEx(rect, 1, (Color){0, 0, 0, 80});
    }
}
void Brick::DrawBatch() {
    if (!active) return;
    if (!s_batchRenderer) {
        Draw();  // 降级到普通绘制
        return;
    }
    
    if (health >= 999) {
        Color stoneHigh = { 130, 135, 140, 255 };
        Color stoneShadow = { 40, 40, 45, 255 };
        
        // 填充主体
        s_batchRenderer->AddRectangle(rect.x, rect.y, rect.width, rect.height, color);
        
        // 高光边（左上）
        s_batchRenderer->AddLine(
            rect.x, rect.y, 
            rect.x + rect.width, rect.y, 
            stoneHigh, 2.0f
        );
        s_batchRenderer->AddLine(
            rect.x, rect.y, 
            rect.x, rect.y + rect.height, 
            stoneHigh, 2.0f
        );
        
        // 阴影边（右下）
        s_batchRenderer->AddLine(
            rect.x + rect.width, rect.y,
            rect.x + rect.width, rect.y + rect.height,
            stoneShadow, 2.0f
        );
        s_batchRenderer->AddLine(
            rect.x, rect.y + rect.height,
            rect.x + rect.width, rect.y + rect.height,
            stoneShadow, 2.0f
        );
        
        // 黑色轮廓
        s_batchRenderer->AddRectangleWithBorder(
            rect.x, rect.y, rect.width, rect.height,
            color, (Color){0, 0, 0, 150}, 1.0f
        );
    } else {
        // 普通砖块 - 内边框（亮色）
        s_batchRenderer->AddRectangleWithBorder(
            rect.x, rect.y, rect.width, rect.height,
            color, (Color){255, 255, 255, 40}, 1.0f
        );
        // 普通砖块 - 外边框（黑色）
        s_batchRenderer->AddRectangleWithBorder(
            rect.x, rect.y, rect.width, rect.height,
            color, (Color){0, 0, 0, 80}, 1.0f
        );
    }
}