// gamepad_win.cpp — Windows 手柄输入轮询（实验性功能：手柄键位显示）
// 零新依赖策略：
//   - XInput：动态加载系统 DLL（xinput1_4 → xinput1_3 → xinput9_1_0），不链接任何库
//   - DirectInput：dinput8.dll 为 Windows 自带，仅链接 SDK 的 dinput8.lib + dxguid.lib
// 优先 XInput（有标准按键布局）；仅当 XInput 未检测到手柄时才走 DirectInput 通用映射。
#include "gamepad.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <xinput.h>
#include <cstdlib>

namespace
{
// ── XInput 动态加载 ────────────────────────────────────────────────
using XInputGetStateFn = DWORD(WINAPI *)(DWORD, XINPUT_STATE *);

HMODULE s_xiDll = nullptr;
XInputGetStateFn s_xiGetState = nullptr;

bool loadXInput()
{
    if (s_xiGetState)
        return true;
    // Win8+ 自带 xinput1_4；Win7 需要 1_3 或 9_1_0（9_1_0 系统自带但功能最简）
    const wchar_t *names[] = {L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"};
    for (const wchar_t *name : names)
    {
        s_xiDll = LoadLibraryW(name);
        if (!s_xiDll)
            continue;
        s_xiGetState = reinterpret_cast<XInputGetStateFn>(GetProcAddress(s_xiDll, "XInputGetState"));
        if (s_xiGetState)
            return true;
        FreeLibrary(s_xiDll);
        s_xiDll = nullptr;
    }
    return false;
}

// ── DirectInput ────────────────────────────────────────────────────
LPDIRECTINPUT8 s_dinput = nullptr;
LPDIRECTINPUTDEVICE8 s_diDevice = nullptr;
// 健康判定：见过轴在标称范围内的正常数据才算真手柄（幽灵/虚拟设备轴恒满偏
// 32767 或恒 0 以外的异常值，永不健康——诊断确认部分机器上存在此类设备）
bool s_diHealthy = false;
// DI 轴标称量程约 -1000..1000；留 8 倍余量容忍真实设备差异，满偏 32767 不在此范围内
constexpr LONG kDiAxisMax = 8000;

BOOL CALLBACK enumGameCtrl(LPCDIDEVICEINSTANCE inst, LPVOID context)
{
    // 只接受真实游戏设备类型：跳过 SUPPLEMENTAL/DEVICECTRL 等系统虚拟设备
    const DWORD t = inst->dwDevType & 0xFF;
    if (t != DI8DEVTYPE_GAMEPAD && t != DI8DEVTYPE_JOYSTICK && t != DI8DEVTYPE_DRIVING && t != DI8DEVTYPE_FLIGHT &&
        t != DI8DEVTYPE_1STPERSON && t != DI8DEVTYPE_REMOTE)
        return DIENUM_CONTINUE;
    auto *guid = static_cast<GUID *>(context);
    *guid = inst->guidInstance;
    return DIENUM_STOP; // 取第一个已连接的游戏控制器
}

bool initDirectInput()
{
    if (s_diDevice)
        return true;
    HRESULT hr = DirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<void **>(&s_dinput), nullptr);
    if (FAILED(hr) || !s_dinput)
        return false;
    GUID guid = GUID_NULL;
    s_dinput->EnumDevices(DI8DEVCLASS_GAMECTRL, enumGameCtrl, &guid, DIEDFL_ATTACHEDONLY);
    if (guid == GUID_NULL)
        return false;
    hr = s_dinput->CreateDevice(guid, &s_diDevice, nullptr);
    if (FAILED(hr) || !s_diDevice)
        return false;
    // 后台 + 非独占：不干扰游戏本身读手柄
    hr = s_diDevice->SetDataFormat(&c_dfDIJoystick2);
    if (SUCCEEDED(hr))
        hr = s_diDevice->SetCooperativeLevel(nullptr, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    if (SUCCEEDED(hr))
        hr = s_diDevice->Acquire();
    if (FAILED(hr))
    {
        s_diDevice->Release();
        s_diDevice = nullptr;
        return false;
    }
    return true;
}

// 摇杆/轴死区：DI 轴范围约 -1000..1000，20% 以下视为静止
constexpr LONG kDiDeadzone = 200;

void mapDiAxes(const DIJOYSTATE2 &js, GamepadState &out)
{
    out.buttons[GP_LS_LEFT] = js.lX < -kDiDeadzone;
    out.buttons[GP_LS_RIGHT] = js.lX > kDiDeadzone;
    out.buttons[GP_LS_UP] = js.lY < -kDiDeadzone;
    out.buttons[GP_LS_DOWN] = js.lY > kDiDeadzone;
    out.buttons[GP_RS_LEFT] = js.lRx < -kDiDeadzone;
    out.buttons[GP_RS_RIGHT] = js.lRx > kDiDeadzone;
    out.buttons[GP_RS_UP] = js.lRy < -kDiDeadzone;
    out.buttons[GP_RS_DOWN] = js.lRy > kDiDeadzone;
    out.buttons[GP_LT] = js.lZ > kDiDeadzone;
    out.buttons[GP_RT] = js.lRz > kDiDeadzone;
    // POV 帽 → 方向键（0=上 9000=右 18000=下 27000=左，0xFFFF=居中）
    DWORD pov = js.rgdwPOV[0];
    out.buttons[GP_DPAD_UP] = pov == 0 || pov == 31500 || pov == 4500;
    out.buttons[GP_DPAD_RIGHT] = pov == 9000 || pov == 4500 || pov == 13500;
    out.buttons[GP_DPAD_DOWN] = pov == 18000 || pov == 13500 || pov == 22500;
    out.buttons[GP_DPAD_LEFT] = pov == 27000 || pov == 22500 || pov == 31500;
}

// DirectInput 通用按键映射：0-3→ABXY，4→LB，5→RB，6→Back，7→Start，8→L3，9→R3
// （不同厂商布局不一，实验功能按最常见的排列尽力映射）
void mapDiButtons(const BYTE *rgb, GamepadState &out)
{
    static const int map[10] = {GP_A, GP_B, GP_X, GP_Y, GP_LB, GP_RB, GP_BACK, GP_START, GP_L3, GP_R3};
    for (int i = 0; i < 10; ++i)
        out.buttons[map[i]] = (rgb[i] & 0x80) != 0;
}

bool pollDirectInput(GamepadState &out)
{
    if (!s_diDevice)
        return false;
    HRESULT hr = s_diDevice->Poll();
    if (hr == DIERR_INPUTLOST)
        hr = s_diDevice->Acquire();
    DIJOYSTATE2 js = {};
    if (FAILED(hr) || FAILED(s_diDevice->GetDeviceState(sizeof(js), &js)))
        return false;
    // 健康判定：任一轴在标称范围（含空闲 0）即健康，之后保持（容忍真实手柄满推）。
    // 幽灵设备轴恒满偏 32767 且从未给过正常数据 → 永不健康 → 视为无手柄。
    const bool axisOk = abs(js.lX) <= kDiAxisMax && abs(js.lY) <= kDiAxisMax && abs(js.lZ) <= kDiAxisMax &&
                        abs(js.lRx) <= kDiAxisMax && abs(js.lRy) <= kDiAxisMax && abs(js.lRz) <= kDiAxisMax;
    if (!axisOk && !s_diHealthy)
        return false;
    if (axisOk)
        s_diHealthy = true;
    mapDiButtons(js.rgbButtons, out);
    mapDiAxes(js, out);
    out.controllerIndex = -2;
    out.isXInput = false;
    out.connected = true;
    return true;
}

// ── XInput 状态映射 ────────────────────────────────────────────────
bool xiHasInput(const XINPUT_STATE &st)
{
    if (st.Gamepad.wButtons != 0)
        return true;
    if (st.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD || st.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        return true;
    return abs(st.Gamepad.sThumbLX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
           abs(st.Gamepad.sThumbLY) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
           abs(st.Gamepad.sThumbRX) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ||
           abs(st.Gamepad.sThumbRY) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
}

void mapXInput(const XINPUT_STATE &st, GamepadState &out)
{
    const WORD b = st.Gamepad.wButtons;
    out.buttons[GP_A] = b & XINPUT_GAMEPAD_A;
    out.buttons[GP_B] = b & XINPUT_GAMEPAD_B;
    out.buttons[GP_X] = b & XINPUT_GAMEPAD_X;
    out.buttons[GP_Y] = b & XINPUT_GAMEPAD_Y;
    out.buttons[GP_DPAD_UP] = b & XINPUT_GAMEPAD_DPAD_UP;
    out.buttons[GP_DPAD_DOWN] = b & XINPUT_GAMEPAD_DPAD_DOWN;
    out.buttons[GP_DPAD_LEFT] = b & XINPUT_GAMEPAD_DPAD_LEFT;
    out.buttons[GP_DPAD_RIGHT] = b & XINPUT_GAMEPAD_DPAD_RIGHT;
    out.buttons[GP_LB] = b & XINPUT_GAMEPAD_LEFT_SHOULDER;
    out.buttons[GP_RB] = b & XINPUT_GAMEPAD_RIGHT_SHOULDER;
    out.buttons[GP_L3] = b & XINPUT_GAMEPAD_LEFT_THUMB;
    out.buttons[GP_R3] = b & XINPUT_GAMEPAD_RIGHT_THUMB;
    out.buttons[GP_START] = b & XINPUT_GAMEPAD_START;
    out.buttons[GP_BACK] = b & XINPUT_GAMEPAD_BACK;
    out.buttons[GP_LT] = st.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    out.buttons[GP_RT] = st.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    out.buttons[GP_LS_LEFT] = st.Gamepad.sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    out.buttons[GP_LS_RIGHT] = st.Gamepad.sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    out.buttons[GP_LS_UP] = st.Gamepad.sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    out.buttons[GP_LS_DOWN] = st.Gamepad.sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    out.buttons[GP_RS_LEFT] = st.Gamepad.sThumbRX < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    out.buttons[GP_RS_RIGHT] = st.Gamepad.sThumbRX > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    out.buttons[GP_RS_UP] = st.Gamepad.sThumbRY > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    out.buttons[GP_RS_DOWN] = st.Gamepad.sThumbRY < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
}

// 上次活跃的手柄编号：双柄场景下优先保持显示当前正在操作的那只
DWORD s_activePad = static_cast<DWORD>(-1);
// 上一轮是否有手柄在线：断开后的首次轮询返回一次 connected=false 的"断开事件"，
// 供 UI 显示"手柄未连接"提示后隐藏
bool s_wasConnected = false;

// 断开事件：仅在线→离线切换时返回 true（out 已重置为默认 disconnected 状态）
bool emitDisconnectEvent()
{
    if (s_wasConnected)
    {
        s_wasConnected = false;
        return true;
    }
    return false;
}
} // namespace

bool initGamepad()
{
    bool ok = loadXInput();
    ok = initDirectInput() || ok;
    return ok;
}

void shutdownGamepad()
{
    if (s_diDevice)
    {
        s_diDevice->Unacquire();
        s_diDevice->Release();
        s_diDevice = nullptr;
    }
    if (s_dinput)
    {
        s_dinput->Release();
        s_dinput = nullptr;
    }
    if (s_xiDll)
    {
        FreeLibrary(s_xiDll);
        s_xiDll = nullptr;
    }
    s_xiGetState = nullptr;
    s_activePad = static_cast<DWORD>(-1);
    s_wasConnected = false;
    s_diHealthy = false; // 设备更换后重新做健康判定
}

bool pollGamepad(GamepadState &out)
{
    out = GamepadState();
    if (!s_xiGetState)
    {
        if (pollDirectInput(out))
        {
            s_wasConnected = true;
            return true;
        }
        return emitDisconnectEvent();
    }

    bool anyConnected = false;
    bool activeHasInput = false;
    DWORD firstConnected = static_cast<DWORD>(-1);
    XINPUT_STATE states[XUSER_MAX_COUNT] = {};

    for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
    {
        if (s_xiGetState(i, &states[i]) == ERROR_SUCCESS)
        {
            if (firstConnected == static_cast<DWORD>(-1))
                firstConnected = i;
            anyConnected = true;
            if (i == s_activePad && xiHasInput(states[i]))
                activeHasInput = true;
        }
    }

    if (!anyConnected)
    {
        if (pollDirectInput(out))
        {
            s_wasConnected = true;
            return true;
        }
        return emitDisconnectEvent();
    }

    // 选中策略：活跃手柄仍有输入 → 继续；否则选任意有输入的手柄；再否则选最近活跃或第一个连接的
    DWORD chosen = static_cast<DWORD>(-1);
    if (s_activePad != static_cast<DWORD>(-1) && activeHasInput)
    {
        chosen = s_activePad;
    }
    else
    {
        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
        {
            if (xiHasInput(states[i]))
            {
                chosen = i;
                break;
            }
        }
        if (chosen == static_cast<DWORD>(-1))
        {
            bool activeStillConnected = s_activePad != static_cast<DWORD>(-1) &&
                                        s_xiGetState(s_activePad, &states[s_activePad]) == ERROR_SUCCESS;
            chosen = activeStillConnected ? s_activePad : firstConnected;
        }
    }
    if (chosen == static_cast<DWORD>(-1))
        return false;
    s_activePad = chosen;
    mapXInput(states[chosen], out);
    out.controllerIndex = static_cast<int>(chosen);
    out.isXInput = true;
    out.connected = true;
    s_wasConnected = true;
    return true;
}
