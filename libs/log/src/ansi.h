#ifndef HYPHA_ANSI_H
#define HYPHA_ANSI_H

// TODO(@s0cks): need to use isatty(STDOUT_FILENO) to verify TTY out for stdout
#define _ANSI_COLOR(Code) "\x1b[" #Code "m"

#define ANSI_RESET        _ANSI_COLOR(0)
#define ANSI_BOLD         _ANSI_COLOR(1)

#define ANSI_RED          _ANSI_COLOR(31)
#define ANSI_GREEN        _ANSI_COLOR(32)
#define ANSI_YELLOW       _ANSI_COLOR(33)
#define ANSI_BLUE         _ANSI_COLOR(34)
#define ANSI_PURPLE       _ANSI_COLOR(35)
#define ANSI_CYAN         _ANSI_COLOR(36)
#define ANSI_WHITE        _ANSI_COLOR(37)

#endif  // HYPHA_ANSI_H
