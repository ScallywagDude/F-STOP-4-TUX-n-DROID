#ifndef GAMECONSOLE_SHIM_H
#define GAMECONSOLE_SHIM_H
struct CGameConsoleShim { bool IsConsoleVisible() { return false; } };
inline CGameConsoleShim &GameConsole() { static CGameConsoleShim s; return s; }
#endif
