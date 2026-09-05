// feed.hpp
// erstellt: 31. Jan. 2025
// zuletzt geändert: 12. Feb. 2025
// James Ezra Seitenschlag
#pragma once
#include <cstdint>
#include <string>
#include <deque>

// ---------------------------------------------------------------------------
// Embedded JSON feed server (display state) for website integration.
//
// Serves on http://127.0.0.1:<port>  (default 8017, disable via --no-feed)
//   GET /                  -> small demo HTML page that polls the feed
//   GET /feed              -> JSON snapshot of the current display
//                              (pixels, icons, buzzer, speed, cycles, ...)
//   GET /action?key=<x>    -> trigger a host action / button from the web
//                              key: A B C hatch skip save load reset pause
//                                   slower faster
//
// Pure BSD/Winsock sockets -> cross platform (Windows / macOS / Linux).
// Non-blocking, polled from the emulator main loop (no extra thread).
// ---------------------------------------------------------------------------
class FeedServer {
public:
    FeedServer() = default;
    ~FeedServer();

    FeedServer(const FeedServer&) = delete;
    FeedServer& operator=(const FeedServer&) = delete;

    // Binds 127.0.0.1:port and starts listening. Returns false on failure.
    bool start(unsigned short port);
    void stop();
    bool isRunning() const { return running; }

    // Serve pending HTTP requests with the *current* display state.
    void poll(const std::uint8_t ramNibbles[4096]);

    // Web -> emulator action queue (drained by the main loop).
    void enqueueAction(const std::string& action);
    bool popAction(std::string& out);

private:
    struct Impl;
    Impl* impl = nullptr;
    bool running = false;
};
