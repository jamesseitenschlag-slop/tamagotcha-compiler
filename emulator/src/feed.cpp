// feed.cpp
// erstellt: 08. Feb. 2025
// zuletzt geändert: 12. Feb. 2025
// James Ezra Seitenschlag
// feed.cpp - tiny non-blocking HTTP server that exposes the current display
//            state as JSON (see feed.hpp for the endpoint documentation).
// ---------------------------------------------------------------------------
#include "feed.hpp"
#include "cpu.hpp"
#include "display.hpp"
#include "tama_icons.inc"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cerrno>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET sock_t;
#define INVALID_SOCK INVALID_SOCKET
#define CLOSE_SOCK(s) closesocket(s)
#define SOCK_ERR(e) ((e) == SOCKET_ERROR)
#define LAST_ERR() (WSAGetLastError())
#define WOULD_BLOCK WSAEWOULDBLOCK
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
typedef int sock_t;
#define INVALID_SOCK (-1)
#define CLOSE_SOCK(s) close(s)
#define SOCK_ERR(e) ((e) < 0)
#define LAST_ERR() (errno)
#define WOULD_BLOCK EWOULDBLOCK
#endif

// ---------------------------------------------------------------------------
// Connection state machine (READING -> RESPONDING -> done)
// ---------------------------------------------------------------------------
namespace {
constexpr size_t MAX_CONN = 16;
constexpr size_t MAX_REQ  = 8192;

struct Conn {
    sock_t sock = INVALID_SOCK;
    std::string req;
    std::string resp;       // filled once the request line/headers arrived
    size_t sent = 0;
};

char toUpper(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c; }
}

struct FeedServer::Impl {
    sock_t listenSock = INVALID_SOCK;
    std::vector<Conn> conns;
    std::deque<std::string> actions;
#ifdef _WIN32
    WSADATA wsa{};
    bool wsaInit = false;
#endif
};

FeedServer::~FeedServer() { stop(); }

bool FeedServer::start(unsigned short port) {
    stop();
    impl = new Impl;

#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &impl->wsa) != 0) { stop(); return false; }
    impl->wsaInit = true;
#endif

    impl->listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (impl->listenSock == INVALID_SOCK) { stop(); return false; }

    int yes = 1;
    setsockopt(impl->listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof yes);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   // localhost only
    addr.sin_port = htons(port);
    if (bind(impl->listenSock, (sockaddr*)&addr, sizeof addr) == SOCKET_ERROR) {
        stop(); return false;
    }
    if (listen(impl->listenSock, 8) == SOCKET_ERROR) { stop(); return false; }

    // Non-blocking accept/recv/send so poll() never stalls the emulator.
#ifdef _WIN32
    u_long nb = 1;
    ioctlsocket(impl->listenSock, FIONBIO, &nb);
#else
    int fl = fcntl(impl->listenSock, F_GETFL, 0);
    fcntl(impl->listenSock, F_SETFL, fl | O_NONBLOCK);
#endif

    running = true;
    return true;
}

void FeedServer::stop() {
    running = false;
    if (impl) {
        for (Conn& c : impl->conns)
            if (c.sock != INVALID_SOCK) CLOSE_SOCK(c.sock);
        impl->conns.clear();
        if (impl->listenSock != INVALID_SOCK) { CLOSE_SOCK(impl->listenSock); impl->listenSock = INVALID_SOCK; }
#ifdef _WIN32
        if (impl->wsaInit) { WSACleanup(); impl->wsaInit = false; }
#endif
        delete impl;
        impl = nullptr;
    }
}

void FeedServer::enqueueAction(const std::string& action) {
    if (impl) impl->actions.push_back(action);
}

bool FeedServer::popAction(std::string& out) {
    if (!impl || impl->actions.empty()) return false;
    out = impl->actions.front();
    impl->actions.pop_front();
    return true;
}

// ---------------------------------------------------------------------------
// JSON snapshot of the current display state
// ---------------------------------------------------------------------------
static std::string buildSnapshotJson(const std::uint8_t ram[4096]) {
    const Byte* bytes = reinterpret_cast<const Byte*>(ram);
    char rowHex[16][9];
    for (int y = 0; y < 16; y++) {
        unsigned v = 0;
        for (int x = 0; x < 32; x++)
            if (display.getPixel(bytes, x, y)) v |= (1u << (31 - x));
        std::snprintf(rowHex[y], 9, "%08X", v);
    }

    char icons[8];
    for (int i = 0; i < 8; i++) icons[i] = display.getIcon(bytes, i) ? '1' : '0';

    std::string out = "{\"cols\":32,\"rows\":16,\"hex\":[";
    for (int y = 0; y < 16; y++) {
        out += y ? ",\"" : "\"";
        out += rowHex[y];
        out += '"';
    }
    out += "],\"icons\":[";
    for (int i = 0; i < 8; i++) { out += i ? "," : ""; out += icons[i]; }
    out += "],\"buzzer\":"; out += (buzzerOn ? "1" : "0");
    out += ",\"speed\":";  out += std::to_string(hostSpeed);
    out += ",\"paused\":"; out += (hostPaused ? "true" : "false");
    out += ",\"cycles\":"; out += std::to_string(emuTotalCycles);
    out += "}";
    return out;
}

// ---------------------------------------------------------------------------
// Static demo page: polls /feed and draws the LCD on a <canvas>.
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Demo page. Builds the icon bitmaps (same data as the SDL renderer uses)
// into a readable JS array so the page can draw the real icons.
// ---------------------------------------------------------------------------
static std::string iconDataJs() {
    std::string out = "[\n";
    for (int i = 0; i < 8; i++) {
        out += "    [ ";
        for (int y = 0; y < 8; y++) {
            out += '"';
            for (int x = 0; x < 8; x++) out += tama_icons[i][y][x] ? '1' : '0';
            out += '"';
            if (y < 7) out += ", ";
        }
        out += " ]";
        if (i < 7) out += ",";
        out += "\n";
    }
    out += "  ]";
    return out;
}

static const char* DEMO_HEAD = R"HTML(<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="utf-8">
  <title>Tamagotchi P1 - Live</title>
  <style>
    body {
      background: #f2efe4;
      color: #333;
      font-family: Georgia, serif;
      margin: 2em auto;
      max-width: 900px;
      padding: 0 1em;
    }
    h1 { font-size: 1.4em; margin: 0 0 .2em 0; }
    p  { color: #666; margin: .5em 0; }

    #rahmen {
      background: #b8cf9c;
      display: inline-block;
      padding: 10px;
      border: 2px solid #97b07d;
      border-radius: 6px;
    }
    canvas { display: block; }

    /* Die drei Tasten wie am Gerät: links, Mitte, rechts */
    .tasten { width: 528px; margin-top: 14px; font-size: 0; }
    .tasten button {
      font-family: inherit;
      font-size: 22px;
      width: 130px;
      height: 48px;
      cursor: pointer;
    }
    .mitte { margin: 0 69px; }
    #hinweis { color: #999; font-size: .8em; }
  </style>
</head>
<body>

  <h1>Tamagotchi P1</h1>
  <p>Das Display direkt aus dem Emulator - aktualisiert sich von selbst.</p>

  <div id="rahmen">
    <canvas id="lcd" width="528" height="240"></canvas>
  </div>

  <div class="tasten">
    <button onclick="akt('A')" title="nach links">&#9664; A</button>
    <button class="mitte" onclick="akt('B')" title="best&auml;tigen">B</button>
    <button onclick="akt('C')" title="nach rechts">C &#9654;</button>
  </div>
  <p>Mit A und C durch die Men&uuml;s bewegen, mit B best&auml;tigen.</p>
  <p id="status">Verbinde zum Emulator ...</p>
  <p id="hinweis">API: GET /feed liefert das Display als JSON, GET /action?key=A|B|C steuert die Tasten.</p>

  <script>
    // Schlicht gehalten, ohne Framework.
    var lcd = document.getElementById('lcd');
    var ctx = lcd.getContext('2d');

    var ZELLE = 12;  // Groesse eines LCD-Pixels
    var ICONP = 5;   // Groesse eines Icon-Pixels
    var LCD_X = 72, LCD_Y = 24;

    var DUNKEL = '#333';
    var HELL   = '#b8cf9c';
    var GITTER = '#a9c28e';

    // Die Icon-Bitmaps (8x8, '1' = Pixel an). Gleiche Daten wie im Emulator.
    var ICONS = )HTML";

static const char* DEMO_TAIL = R"HTML(;

    function zeichne(data) {
      // erst alles hell machen
      ctx.fillStyle = HELL;
      ctx.fillRect(0, 0, lcd.width, lcd.height);

      // 16 Zeilen, 32 Spalten aus dem Feed zeichnen
      for (var y = 0; y < 16; y++) {
        var zeile = data.hex[y];
        for (var x = 0; x < 32; x++) {
          // 8 Hex-Ziffern je Zeile, jede Ziffer haelt 4 Pixel
          var ziffer = parseInt(zeile.charAt(x >> 2), 16);
          var an = ((ziffer >> (3 - (x & 3))) & 1) == 1;
          ctx.fillStyle = an ? DUNKEL : HELL;
          ctx.fillRect(LCD_X + x * ZELLE, LCD_Y + y * ZELLE, ZELLE - 1, ZELLE - 1);
        }
      }

      // Icons links (0..3) und rechts (4..7) neben dem Bildschirm zeichnen
      for (var i = 0; i < 8; i++) {
        var x0 = (i < 4) ? 8 : 472;
        var y0 = 25 + (i % 4) * 50;
        for (var py = 0; py < 8; py++) {
          for (var px = 0; px < 8; px++) {
            if (ICONS[i][py].charAt(px) != '1') continue;
            ctx.fillStyle = (data.icons[i] == 1) ? DUNKEL : GITTER;
            ctx.fillRect(x0 + px * ICONP, y0 + py * ICONP, ICONP - 1, ICONP - 1);
          }
        }
      }

      // kurzer Statustext
      var tempo = data.paused ? 'Pause' : data.speed + 'x';
      var ton   = data.buzzer ? 'an' : 'aus';
      document.getElementById('status').textContent =
        'Verbunden - Tempo: ' + tempo + ' - Buzzer: ' + ton;
    }

    function akt(taste) {
      fetch('/action?key=' + taste); // einmal druecken, Feuer und vergiss
    }

    function abrufen() {
      fetch('/feed', { cache: 'no-store' })
        .then(function (r) { return r.json(); })
        .then(zeichne)
        .catch(function () {
          document.getElementById('status').textContent =
            'Emulator nicht erreichbar - laeuft er?';
        });
    }

    setInterval(abrufen, 250);
    abrufen();
  </script>
</body>
</html>)HTML";

static std::string demoHtml() {
    std::string page = DEMO_HEAD;
    page += iconDataJs();
    page += DEMO_TAIL;
    return page;
}

// ---------------------------------------------------------------------------
// Tiny request parsing (GET only, enough for a local feed)
// ---------------------------------------------------------------------------
static void respond(Conn& c, const std::string& body, const char* type, int code = 200) {
    char head[256];
    std::snprintf(head, sizeof head,
        "HTTP/1.1 %d %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Cache-Control: no-store\r\n"
        "Content-Type: %s; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        code, code == 200 ? "OK" : (code == 404 ? "Not Found" : "Bad Request"),
        type, body.size());
    c.resp = head + body;
    c.sent = 0;
}

static void handleRequest(Conn& c, const std::string& method, const std::string& pathQ,
                          const std::uint8_t ram[4096]) {
    if (method != "GET") { respond(c, "{\"error\":\"method\"}", "application/json", 400); return; }

    std::string path = pathQ, query;
    size_t q = pathQ.find('?');
    if (q != std::string::npos) { path = pathQ.substr(0, q); query = pathQ.substr(q + 1); }
    if (path.empty() || path[0] != '/') path = "/" + path;

    auto getParam = [&](const char* name) -> std::string {
        std::string key = std::string(name) + "=";
        size_t p = query.find(key);
        if (p == std::string::npos) return "";
        size_t v = p + key.size();
        size_t e = query.find('&', v);
        return query.substr(v, e == std::string::npos ? std::string::npos : e - v);
    };

    if (path == "/" || path == "/index.html") {
        respond(c, demoHtml(), "text/html");
    } else if (path == "/feed" || path == "/display.json" || path == "/api/display") {
        respond(c, buildSnapshotJson(ram), "application/json");
    } else if (path == "/action") {
        std::string k = getParam("key");
        if (k.empty()) k = getParam("button");
        if (k.empty()) { respond(c, "{\"error\":\"missing key\"}", "application/json", 400); return; }
        for (char& ch : k) ch = toUpper(ch);
        extern FeedServer g_feed;
        g_feed.enqueueAction(k);
        respond(c, "{\"ok\":true,\"key\":\"" + k + "\"}", "application/json");
    } else if (path == "/favicon.ico") {
        respond(c, "", "image/x-icon", 200);
    } else {
        respond(c, "{\"error\":\"not found\"}", "application/json", 404);
    }
}

// ---------------------------------------------------------------------------
// Non-blocking poll: accept new clients, read requests, write responses.
// ---------------------------------------------------------------------------
void FeedServer::poll(const std::uint8_t ram[4096]) {
    if (!running || !impl) return;

    // Accept new connections.
    while ((int)impl->conns.size() < (int)MAX_CONN) {
        sockaddr_in peer{};
#ifdef _WIN32
        int plen = sizeof peer;
#else
        socklen_t plen = sizeof peer;
#endif
        sock_t s = accept(impl->listenSock, (sockaddr*)&peer, &plen);
        if (s == INVALID_SOCK) {
            int e = LAST_ERR();
            if (e == WOULD_BLOCK
#ifdef EAGAIN
                || e == EAGAIN
#endif
            ) break;                       // no more pending clients
            break;
        }
#ifdef _WIN32
        u_long nb = 1; ioctlsocket(s, FIONBIO, &nb);
#else
        int fl = fcntl(s, F_GETFL, 0); fcntl(s, F_SETFL, fl | O_NONBLOCK);
#endif
        impl->conns.push_back({ s });
    }

    // Read requests / write responses.
    for (size_t i = 0; i < impl->conns.size();) {
        Conn& c = impl->conns[i];

        if (c.resp.empty()) {
            // --- reading phase ---
            char buf[2048];
            int n = (int)recv(c.sock, buf, sizeof buf, 0);
            if (n > 0) {
                c.req.append(buf, (size_t)n);
                if (c.req.size() > MAX_REQ) {          // too large / garbage
                    respond(c, "{\"error\":\"request too large\"}", "application/json", 400);
                    c.req.clear();
                }
            } else {
                int e = (n == 0) ? 0 : LAST_ERR();
                bool fatal = (n == 0)                    // peer closed the connection
#ifdef EAGAIN
                    || (e != WOULD_BLOCK && e != EAGAIN) // hard socket error
#else
                    || (e != WOULD_BLOCK)
#endif
                    ;
                if (fatal) {
                    CLOSE_SOCK(c.sock); c.sock = INVALID_SOCK;
                    c.req.clear();
                }
            }

            // Request header complete?
            size_t hEnd = c.req.find("\r\n\r\n");
            if (hEnd != std::string::npos && c.resp.empty() && c.sock != INVALID_SOCK) {
                std::string head = c.req.substr(0, hEnd);
                size_t eol = head.find("\r\n");
                std::string requestLine = (eol == std::string::npos) ? head : head.substr(0, eol);
                std::string method, pathQ, proto;
                std::istringstream ls(requestLine);
                ls >> method >> pathQ >> proto;
                handleRequest(c, method, pathQ, ram);
                c.req.clear();
            }
        }

        if (!c.resp.empty() && c.sock != INVALID_SOCK) {
            // --- writing phase ---
            int n = (int)send(c.sock, c.resp.data() + c.sent, c.resp.size() - c.sent, 0);
            if (n > 0) c.sent += (size_t)n;
            else if (SOCK_ERR(n)) {
                int e = LAST_ERR();
                if (e != WOULD_BLOCK
#ifdef EAGAIN
                    && e != EAGAIN
#endif
                ) { c.sock = INVALID_SOCK; }
            }
        }

        if (!c.resp.empty() && c.sock != INVALID_SOCK && c.sent >= c.resp.size()) {
            CLOSE_SOCK(c.sock); c.sock = INVALID_SOCK;   // response fully sent
        }

        if (c.sock == INVALID_SOCK) {
            impl->conns.erase(impl->conns.begin() + (long)i);
        } else {
            i++;
        }
    }
}
