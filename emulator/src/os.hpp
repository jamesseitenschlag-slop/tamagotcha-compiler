// os.hpp
// erstellt: 05. Feb. 2025
// zuletzt geändert: 12. Feb. 2025
// James Ezra Seitenschlag
// ============================================================================
// os.hpp - Host-OS: Syscall-Schnittstelle ("externe Interrupts" fuer Gäste)
//
// Der Gast (z.B. E0C6S46-Assembly- oder Compiler-Programm) meldet einen
// Systemdienst, indem er die Syscall-Mailbox in DATA-RAM befuellt und dann
// das Trigger-Register OS_TRIGGER schreibt (write-only, self-clearing).
// Der Emulator behandelt das wie einen *externen Interrupt* an den Host und
// fuehrt den Dienst aus (Dateisystem auf einer echten Verzeichnisstruktur).
//
// Mailbox (DATA-RAM, je Zelle = 1 Nibble):
//   0x2C0  FN         Dienstnummer (siehe SysFn)
//   0x2C1  Namenslaenge (Zeichen, 0..8)
//   0x2C2..0x2D1 Name (max. 8 Bytes als Nibble-Paare, low bei gerader Adresse)
//   0x2D2..0x2D4      Daten-Pointer (12 Bit: hi, mid, lo)
//   0x2D5..0x2D6      Laenge des Datenbereichs (lo, hi) -> 0..255 Bytes
//   0x2D7  STATUS     Ergebnis (Host -> Gast, siehe SysStatus)
//   0x2D8  KEY_READY  Eingabe: 1 = Zeichen wartet
//   0x2D9..0x2DA      Eingabe-Zeichen (lo, hi), ASCII 0..255
//
// Trigger-Register (I/O-Adresse, wie der DBG-Kanal 0xF0E):
//   0xF0D  OS_TRIGGER - Schreiben loest os::syscall() aus.
//
// Datenformat im Gast-RAM: Byte b liegt als Nibble-Paar vor, low-Teil
// (b & 0xF) an Adresse P, High-Teil (b >> 4) an P+1 - identisch zum
// Debug-Kanal (0x0C0) und set_text() des Compilers.
// ============================================================================
#pragma once

namespace os {

// --- Systemdienste (FN) ---
enum SysFn {
    FS_LIST   = 1,   // Verzeichnisinhalt auf der Konsole ausgeben
    FS_READ   = 2,   // Datei -> Gast-RAM (Ptr/Laenge in der Mailbox)
    FS_WRITE  = 3,   // Gast-RAM -> Datei (anlegen/ueberschreiben)
    FS_DELETE = 4,   // Datei loeschen
    FS_STAT   = 5,   // Dateiinfo auf der Konsole ausgeben
};

// --- Ergebnisstatus ---
enum SysStatus {
    OK       = 0,
    NOT_FOUND= 1,
    BAD_FN   = 2,
    IO_ERR   = 3,
    TOO_BIG  = 4,
    BAD_NAME = 5,
};

// --- Mailbox-Adressen (DATA-RAM-Nibbles) ---
constexpr unsigned MAIL_FN      = 0x2C0;
constexpr unsigned MAIL_NAME_LEN= 0x2C1;
constexpr unsigned MAIL_NAME    = 0x2C2;   // max. 8 Zeichen -> 0x2C2..0x2D1
constexpr unsigned MAIL_PTR     = 0x2D2;   // 3 Nibbles: hi, mid, lo  (0x2D2..)
constexpr unsigned MAIL_LEN     = 0x2D5;   // 2 Nibbles: lo, hi
constexpr unsigned MAIL_STATUS  = 0x2D7;
constexpr unsigned MAIL_KEYRDY  = 0x2D8;
constexpr unsigned MAIL_KEY     = 0x2D9;   // lo, hi
constexpr unsigned OS_TRIGGER   = 0xF0D;   // I/O: Schreiben = Syscall
constexpr unsigned OS_DISK_MAX  = 8;       // max. Dateinamenlaenge
constexpr unsigned OS_DATA_MAX  = 255;     // max. Bytes pro Transaktion

// --- API ---
// Schaltet die SDL-Tastatureingabe -> Gast-Mailbox frei (und Echo).
inline bool keyEnabled = true;
// Host-Echo-Zustand: 1 = getippte Zeichen stehen auf der offenen Zeile.
// Gast-Meldungen (logf/debugFlush) setzen davor eine neue Zeile.
inline bool echoLineOpen = false;

// Host-Dienst-Dispatcher; wird vom CPU-Schreib-Hook aufgerufen.
void syscall();

// Ein eingegebenes Zeichen (SDL) an den Gast weiterreichen.
void feedKey(unsigned char c);

// Konsole/Log-Ausgabe des OS (frei verwendbar, wird auch fuer LIST genutzt).
void logf(const char* fmt, ...);   // siehe os.cpp (va_list)

// Wurzelverzeichnis (Default: <cwd>/osdisk); bei Bedarf anpassen.
const char* diskRoot();
}  // namespace os
