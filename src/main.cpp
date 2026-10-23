#include "game/init.h"
#include "game/main_loop.h"
#include "game/mouse/mouse.h"
#include "game/player_name.h"
#include "game/train.h"
#include "graphics/vga.h"
#include "system/active_sleep.h"
#include "system/analytics.h"
#include "system/filesystem.h"
#include "system/keyboard.h"
#include "system/time.h"
#include "tasks/task.h"
#include <system/driver/driver.h>

#include <SDL_main.h>

#include <cstdio>
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

using namespace resl;

namespace {

// Helper task for handling SDL events.
Task sdlLoop()
{
    for (;;) {
        Driver::instance().vga().flush();
        co_await resl::sleep(2);
    }
}

int usage(int /* argc */, char* argv[], int unknownArg = -1)
{
    if (unknownArg != -1)
        std::cerr << "Unknown command line argument \"" << argv[unknownArg] << "\"\n"
                  << std::endl;
    std::cerr << "Usage: " << argv[0]
              << " [--debug-graphics] [--windowed] [playerName] [S<random seed>]\n"
                 "\n"
                 "The following options are available:\n"
                 "  --debug-graphics  enable debug video mode. In this mode, you will see the entire content of the video memory including invisible areas\n"
                 "  --windowed        run the game in windowed mode, do not expand to full screen\n"
                 "  --dump-frames DIR save selected final frames as BMP files in DIR\n"
                 "  --dump-at N,...   frame numbers to save (requires --dump-frames)\n"
                 "  --dump-split N    save N frames during the next split-screen transition\n"
                 "  --exit-after N    exit after N vertical retraces\n"
                 "  --autostart-demo  start the first demo without waiting for menu timeout\n"
                 "  --autostart-game  start a new game without waiting for menu input\n"
                 "  --skip-intro      dismiss the opening poster after frame 12\n"
                 "  --pause-at N      press the pause-menu shortcut at frame N (with --autostart-game)\n"
                 "  --perf-log        print frame rate and classic upload cost once per second\n"
                 "  --help            show this help\n"
                 "\n"
              << std::endl;
    return EXIT_FAILURE;
}

} // namespace

namespace resl {

/* 1c75:027c */
static void initTasks(/* void* taskStacksMemory */)
{
    /*
       The original game uses a platform-specific stack-based coroutine
       implementation. But reSL uses a portable approach based on C++20 coroutines.
       So, this implementation of the function is different (but logically the same).
    */

    addTask(taskSpawnTrains());
    addTask(taskMouseEventHandling());
    g_taskGameMainLoop = addTask(taskGameMainLoop());

    runScheduler();
}

// main function of the original game
/* 15ab:0018 */
int main(int argc, char* argv[])
{
    const unsigned seed = static_cast<unsigned>(std::time(nullptr));
    std::srand(seed);
    if (argc == 3 && argv[2][0] == 'S') {
        std::srand(static_cast<unsigned>(argv[2][1]));
        std::printf("Seed = %d\n", static_cast<int>(argv[2][1]));
    }
    if (argc > 1)
        std::strcpy(g_playerName, argv[1]);

    trackEvent("run_game", "app_name", "reSL");

    /*
        The original game performs initialization here:
            15ab:0096 > mouse driver initialization
            15ab:00b2 > allocates memory for task stacks
            15ab:00d5 > allocates memory for g_pageBuffer (262d:21d8)
            15ab:00f3 > sets the video mode (VGA 640x350, 16 colors)
            15ab:00fe > sets the number of bytes in a VGA scanline
    */

    vga::setVideoModeR0W2();
    initGameData();

    /*
        The original game sets interruption handlers here:
            15ab:0136 > DIV/0 handler
            15ab:015c > timer handler
    */
    initTimer();
    calibrateActiveSleep();

    Driver::instance().setKeyboardHandler(&keyboardInterruptionHandler);
    MouseDriver::HandlerHolder mouseHandler =
        Driver::instance().mouse().addHandler(&handleMouseInput);

    initTasks(/* taskStacksMemory */);

    return EXIT_SUCCESS;
}

} // namespace resl

#if defined(__IPHONEOS__) || defined(ANDROID)
extern "C" {
#endif

int main(int argc, char* argv[])
{
    bool debugGraphics = false;
    bool fullscreen = true;
    bool autostartDemo = false;
    bool autostartGame = false;
    bool skipIntro = false;
    bool perfLog = false;
    std::string dumpDir;
    std::vector<unsigned> dumpFrames;
    unsigned exitAfter = 0;
    unsigned pauseAt = 0;
    unsigned dumpSplit = 0;

    const auto parseUnsigned = [](const char* begin, const char* end, unsigned& value) {
        const auto result = std::from_chars(begin, end, value);
        return result.ec == std::errc{} && result.ptr == end;
    };

    char* origGameArgv[3];
    origGameArgv[0] = argv[1];
    int origGameArgc = 1;

    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--debug-graphics"))
            debugGraphics = true;
        else if (!std::strcmp(argv[i], "--windowed"))
            fullscreen = false;
        else if (!std::strcmp(argv[i], "--autostart-demo"))
            autostartDemo = true;
        else if (!std::strcmp(argv[i], "--autostart-game"))
            autostartGame = true;
        else if (!std::strcmp(argv[i], "--skip-intro"))
            skipIntro = true;
        else if (!std::strcmp(argv[i], "--perf-log"))
            perfLog = true;
        else if (!std::strcmp(argv[i], "--dump-frames") && i + 1 < argc)
            dumpDir = argv[++i];
        else if (!std::strcmp(argv[i], "--exit-after") && i + 1 < argc) {
            const char* value = argv[++i];
            if (!parseUnsigned(value, value + std::strlen(value), exitAfter) || !exitAfter)
                return usage(argc, argv, i);
        } else if (!std::strcmp(argv[i], "--pause-at") && i + 1 < argc) {
            const char* value = argv[++i];
            if (!parseUnsigned(value, value + std::strlen(value), pauseAt) || !pauseAt)
                return usage(argc, argv, i);
        } else if (!std::strcmp(argv[i], "--dump-split") && i + 1 < argc) {
            const char* value = argv[++i];
            if (!parseUnsigned(value, value + std::strlen(value), dumpSplit) || !dumpSplit)
                return usage(argc, argv, i);
        } else if (!std::strcmp(argv[i], "--dump-at") && i + 1 < argc) {
            const char* value = argv[++i];
            do {
                const char* comma = std::strchr(value, ',');
                const char* end = comma ? comma : value + std::strlen(value);
                unsigned frame = 0;
                if (!parseUnsigned(value, end, frame) || !frame)
                    return usage(argc, argv, i);
                dumpFrames.push_back(frame);
                value = comma ? comma + 1 : nullptr;
            } while (value);
        }
        else if (!std::strcmp(argv[i], "--help"))
            return usage(argc, argv);
        else if (argv[i][0] != '-' && origGameArgc < 3)
            origGameArgv[origGameArgc++] = argv[i];
        else
            return usage(argc, argv, i);
    }
    if ((!dumpFrames.empty() || dumpSplit) && dumpDir.empty())
        return usage(argc, argv);
    if ((autostartDemo && autostartGame) || (pauseAt && !autostartGame))
        return usage(argc, argv);

    initFS();

    // Driver should be destroyed after scheduler => we have to initialize it first
    Driver::instance();
    Driver::instance().vga().configureDiagnostics(
        std::move(dumpDir), std::move(dumpFrames), exitAfter, perfLog,
        autostartDemo, autostartGame, skipIntro, pauseAt, dumpSplit);

    if (debugGraphics)
        Driver::instance().vga().setDebugMode(true);
    if (fullscreen)
        Driver::instance().vga().setFullscreenMode(true);

    addTask(sdlLoop());
    return resl::main(origGameArgc, origGameArgv);
}

#if defined(__IPHONEOS__) || defined(ANDROID)
} // extern "C"
#endif
