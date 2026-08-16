// gamepad.h — 手柄输入轮询接口（实验性功能：手柄键位显示）
// Windows: XInput（Xbox 及兼容手柄，动态加载 xinput1_4/1_3/9_1_0）+ DirectInput（旧式/杂牌手柄）轮询，零新依赖
// 其他平台: gamepad_stub.cpp 空实现——保证 macOS/Linux 构建链接通过
#ifndef GAMEPAD_H
#define GAMEPAD_H

// 手柄按键位（XInput 语义；DirectInput 设备按通用布局尽力映射，见 gamepad_win.cpp）
enum GamepadButton
{
    GP_A = 0,
    GP_B,
    GP_X,
    GP_Y,
    GP_DPAD_UP,
    GP_DPAD_DOWN,
    GP_DPAD_LEFT,
    GP_DPAD_RIGHT,
    GP_LB,
    GP_RB,
    GP_L3,
    GP_R3,
    GP_START,
    GP_BACK,
    GP_LT, // 左扳机（模拟值超阈值视为按下）
    GP_RT, // 右扳机
    GP_LS_UP,
    GP_LS_DOWN,
    GP_LS_LEFT,
    GP_LS_RIGHT, // 左摇杆方向
    GP_RS_UP,
    GP_RS_DOWN,
    GP_RS_LEFT,
    GP_RS_RIGHT, // 右摇杆方向
    GP_COUNT
};

struct GamepadState
{
    bool buttons[GP_COUNT] = {false};
    int controllerIndex = -1; // 当前手柄编号（XInput: 0..3；DirectInput: -2 表示 DI 设备）
    bool isXInput = true;     // 当前输入来源：XInput 或 DirectInput
    bool connected = false;   // 是否检测到可用手柄
};

// 初始化后端（加载 XInput DLL、创建 DirectInput 设备）。返回是否有可用后端。
bool initGamepad();
void shutdownGamepad();
// 轮询一次当前按键状态。成功读到手柄状态返回 true（out.connected 表示手柄是否存在），
// 无可用手柄/后端时返回 false。
bool pollGamepad(GamepadState &out);

#endif // GAMEPAD_H
