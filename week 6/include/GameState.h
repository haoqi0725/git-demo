#pragma once

enum class GameState {
    MENU,       // 主菜单
    PLAYING,    // 游戏中
    PAUSED,     // 暂停
    GAMEOVER,   // 游戏结束（失败）
    VICTORY,    // 胜利
    LEADERBOARD // 排行榜界面
};
