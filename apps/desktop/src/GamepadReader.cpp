#include "GamepadReader.hpp"

#ifdef Q_OS_WIN
#include <windows.h>
#include <xinput.h>
#endif

namespace {

#ifdef Q_OS_WIN
// Bit per control, in ControllerMapping::presetControlIds() order:
// UP DOWN LEFT RIGHT A B X Y L R START SELECT.
constexpr WORD ControlMasks[] = {
    XINPUT_GAMEPAD_DPAD_UP,
    XINPUT_GAMEPAD_DPAD_DOWN,
    XINPUT_GAMEPAD_DPAD_LEFT,
    XINPUT_GAMEPAD_DPAD_RIGHT,
    XINPUT_GAMEPAD_A,
    XINPUT_GAMEPAD_B,
    XINPUT_GAMEPAD_X,
    XINPUT_GAMEPAD_Y,
    XINPUT_GAMEPAD_LEFT_SHOULDER,
    XINPUT_GAMEPAD_RIGHT_SHOULDER,
    XINPUT_GAMEPAD_START,
    XINPUT_GAMEPAD_BACK
};
constexpr int ControlCount = static_cast<int>(sizeof(ControlMasks) / sizeof(ControlMasks[0]));

// Loaded at runtime: the import library is not the same name across Windows SDKs
// and a missing DLL should degrade to "no pad", never to a failed start.
using XInputGetStateFn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);

XInputGetStateFn resolveXInput()
{
    static XInputGetStateFn function = [] () -> XInputGetStateFn {
        for (const wchar_t* name : {L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"}) {
            if (HMODULE module = LoadLibraryW(name)) {
                if (auto* symbol = reinterpret_cast<XInputGetStateFn>(GetProcAddress(module, "XInputGetState"))) {
                    return symbol;
                }
            }
        }
        return nullptr;
    }();
    return function;
}
#endif

} // namespace

namespace Pocket::App {

GamepadReader::GamepadReader(QObject* parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &GamepadReader::poll);
    // ~120 Hz while a pad is present: below one emulated frame, so no input is
    // swallowed between frames. Drops to a slow scan when nothing is plugged in.
    m_timer.start(8);
}

void GamepadReader::setActive(bool active)
{
    if (active == m_timer.isActive()) return;
    if (active) {
        m_idleTicks = 0;
        m_timer.start(8);
        return;
    }
    // Release anything held so a stopped reader cannot leave a button stuck down.
    for (int index = 0; index < 32; ++index) {
        if (m_previous & (1u << index)) emit buttonChanged(index, false);
    }
    m_previous = 0;
    m_timer.stop();
}

void GamepadReader::poll()
{
#ifdef Q_OS_WIN
    auto* getState = resolveXInput();
    if (!getState) {
        m_timer.stop();
        return;
    }

    XINPUT_STATE state{};
    if (m_pad < 0) {
        // Scanning for a pad is a system call per slot: once a second is plenty.
        if (++m_idleTicks < 125) return;
        m_idleTicks = 0;
        for (DWORD slot = 0; slot < XUSER_MAX_COUNT; ++slot) {
            if (getState(slot, &state) == ERROR_SUCCESS) {
                m_pad = static_cast<int>(slot);
                m_previous = 0;
                m_connected = true;
                emit connectionChanged(true);
                break;
            }
        }
        if (m_pad < 0) return;
    } else if (getState(static_cast<DWORD>(m_pad), &state) != ERROR_SUCCESS) {
        // Unplugged: release whatever was held so the game does not keep running.
        for (int index = 0; index < ControlCount; ++index) {
            if (m_previous & (1u << index)) emit buttonChanged(index, false);
        }
        m_previous = 0;
        m_pad = -1;
        m_connected = false;
        emit connectionChanged(false);
        return;
    }

    quint32 current = 0;
    for (int index = 0; index < ControlCount; ++index) {
        if (state.Gamepad.wButtons & ControlMasks[index]) current |= (1u << index);
    }
    // The left stick doubles as the d-pad: generic pads often default to it.
    const SHORT deadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    if (state.Gamepad.sThumbLY > deadzone) current |= 1u << 0;
    if (state.Gamepad.sThumbLY < -deadzone) current |= 1u << 1;
    if (state.Gamepad.sThumbLX < -deadzone) current |= 1u << 2;
    if (state.Gamepad.sThumbLX > deadzone) current |= 1u << 3;

    const quint32 changed = current ^ m_previous;
    if (changed) {
        for (int index = 0; index < ControlCount; ++index) {
            if (changed & (1u << index)) emit buttonChanged(index, current & (1u << index));
        }
        m_previous = current;
    }
#endif
}

} // namespace Pocket::App
