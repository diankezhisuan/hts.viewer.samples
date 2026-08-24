#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <HtsViewerSdk.h>

#include <array>
#include <iostream>
#include <string>

namespace hts::viewer::samples
{
class HotkeyState
{
public:
    bool pressed(int virtualKey)
    {
        if (virtualKey < 0 || virtualKey >= static_cast<int>(m_Down.size())) return false;
        const bool down = (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
        const bool edge = down && !m_Down[static_cast<std::size_t>(virtualKey)];
        m_Down[static_cast<std::size_t>(virtualKey)] = down;
        return edge;
    }
private:
    std::array<bool, 256> m_Down{};
};

inline void printSampleHelp(const std::string& sampleName, const std::string& commands)
{
    std::cout << "\n========================================\n " << sampleName
              << "\n========================================\n" << commands
              << "\nH  Print this help again\nClose the Viewer window to exit.\n\n";
}

template <typename Handler>
int runInteractive(HtsViewerSdk& viewer, const std::string& sampleName,
                   const std::string& commands, Handler&& handler)
{
    printSampleHelp(sampleName, commands);
    HotkeyState hotkeys;
    while (viewer.frame()) {
        if (hotkeys.pressed('H')) printSampleHelp(sampleName, commands);
        handler(hotkeys);
    }
    return 0;
}
}
