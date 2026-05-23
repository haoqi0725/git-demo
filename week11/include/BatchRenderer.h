// include/BatchRenderer.h
#ifndef BATCH_RENDERER_H
#define BATCH_RENDERER_H

#include "raylib.h"
#include "rlgl.h"
#include <vector>
#include <cstring>
#include <cmath>

// 批绘制顶点结构（匹配 rlgl 的顶点格式）
struct BatchVertex {
    float x, y;           // 位置
    float texX, texY;     // 纹理坐标（未使用可忽略）
    unsigned char r, g, b, a;  // 颜色
};

class BatchRenderer {
public:
    static constexpr int BATCH_SIZE = 4096;  // 每批最大顶点数
    static constexpr int MAX_BATCHES = 8;    // 最大批次数
    
    BatchRenderer() {
        vertices.resize(BATCH_SIZE);
        Clear();
    }
    
    // 开始批绘制
    void Begin() {
        vertexCount = 0;
        batchCount = 0;
        currentBatchStart = 0;
    }
    
    // 添加矩形（带颜色和轮廓）
    void AddRectangle(float x, float y, float w, float h, Color fillColor) {
        if (vertexCount + 6 >= BATCH_SIZE) {
            FlushBatch();  // 批次满，立即绘制
        }
        
        // 添加4个顶点（两个三角形组成矩形）
        unsigned char r = fillColor.r;
        unsigned char g = fillColor.g;
        unsigned char b = fillColor.b;
        unsigned char a = fillColor.a;
        
        // 矩形顶点顺序（顺时针）
        // 0: (x, y)
        // 1: (x + w, y)
        // 2: (x + w, y + h)
        // 3: (x, y + h)
        
        // 两个三角形组成矩形，统一使用逆时针顺序
        // 第一个三角形: 左下 -> 右下 -> 右上
        AddVertex(x, y, r, g, b, a);           // 0: 左下
        AddVertex(x + w, y, r, g, b, a);       // 1: 右下
        AddVertex(x + w, y + h, r, g, b, a);   // 2: 右上
        
        // 第二个三角形: 左下 -> 右上 -> 左上
        AddVertex(x, y, r, g, b, a);           // 0: 左下
        AddVertex(x + w, y + h, r, g, b, a);   // 2: 右上
        AddVertex(x, y + h, r, g, b, a);       // 3: 左上
        }
    
    // 添加带轮廓的矩形（填充 + 边框）
    void AddRectangleWithBorder(float x, float y, float w, float h, 
                                 Color fillColor, Color borderColor, 
                                 float borderWidth = 1.0f) {
        // 添加填充矩形
        AddRectangle(x, y, w, h, fillColor);
        
        // 添加边框（4条线）
        AddLine(x, y, x + w, y, borderColor, borderWidth);           // 上
        AddLine(x + w, y, x + w, y + h, borderColor, borderWidth);   // 右
        AddLine(x, y + h, x + w, y + h, borderColor, borderWidth);   // 下
        AddLine(x, y, x, y + h, borderColor, borderWidth);           // 左
    }
    
    // 添加线条
    void AddLine(float x1, float y1, float x2, float y2, Color color, float thickness = 1.0f) {
        if (vertexCount + 4 >= BATCH_SIZE) {
            FlushBatch();
        }
        
        // 计算垂直方向向量（用于厚度）
        float dx = x2 - x1;
        float dy = y2 - y1;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 0.0001f) return;
        
        float nx = -dy / len * thickness * 0.5f;
        float ny = dx / len * thickness * 0.5f;
        
        unsigned char r = color.r, g = color.g, b = color.b, a = color.a;
        
        // 两个三角形组成矩形条
        AddVertex(x1 + nx, y1 + ny, r, g, b, a);
        AddVertex(x1 - nx, y1 - ny, r, g, b, a);
        AddVertex(x2 - nx, y2 - ny, r, g, b, a);
        
        AddVertex(x1 + nx, y1 + ny, r, g, b, a);
        AddVertex(x2 - nx, y2 - ny, r, g, b, a);
        AddVertex(x2 + nx, y2 + ny, r, g, b, a);
    }
    
    // 添加圆形（近似用三角形扇）
    void AddCircle(float cx, float cy, float radius, Color color, int segments = 16) {
        if (vertexCount + segments * 3 >= BATCH_SIZE) {
            FlushBatch();
        }
        
        unsigned char r = color.r, g = color.g, b = color.b, a = color.a;
        
        for (int i = 0; i < segments; i++) {
            float angle1 = (float)i / segments * 2 * PI;
            float angle2 = (float)(i + 1) / segments * 2 * PI;
            
            float x1 = cx + cosf(angle1) * radius;
            float y1 = cy + sinf(angle1) * radius;
            float x2 = cx + cosf(angle2) * radius;
            float y2 = cy + sinf(angle2) * radius;
            
            AddVertex(cx, cy, r, g, b, a);
            AddVertex(x1, y1, r, g, b, a);
            AddVertex(x2, y2, r, g, b, a);
        }
    }
    
    // 结束批绘制（提交所有剩余批次）
    void End() {
        if (vertexCount > 0) {
            FlushBatch();
        }
    }
    
    // 清空所有数据
    void Clear() {
        vertexCount = 0;
        batchCount = 0;
        currentBatchStart = 0;
    }
    int GetBatchCount() const { return batchCount; }
    
private:
    std::vector<BatchVertex> vertices;
    int vertexCount;
    int batchCount;
    int currentBatchStart;
    
    void AddVertex(float x, float y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
        vertices[vertexCount].x = x;
        vertices[vertexCount].y = y;
        vertices[vertexCount].texX = 0;
        vertices[vertexCount].texY = 0;
        vertices[vertexCount].r = r;
        vertices[vertexCount].g = g;
        vertices[vertexCount].b = b;
        vertices[vertexCount].a = a;
        vertexCount++;
    }
    
    void FlushBatch() {
        if (vertexCount == 0) return;
        
        // 使用 rlgl 直接绘制顶点数据
        rlEnableTexture(0);  // 不使用纹理
        
        rlPushMatrix();
        rlLoadIdentity();
        
        // 设置顶点格式
        rlBegin(RL_TRIANGLES);
        
        for (int i = 0; i < vertexCount; i++) {
            rlColor4ub(vertices[i].r, vertices[i].g, vertices[i].b, vertices[i].a);
            rlVertex2f(vertices[i].x, vertices[i].y);
        }
        
        rlEnd();
        rlPopMatrix();
        
        batchCount++;
        vertexCount = 0;
        currentBatchStart = 0;
    }
};

#endif // BATCH_RENDERER_H