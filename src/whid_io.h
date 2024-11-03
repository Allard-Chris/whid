#pragma once
#ifndef _HEADER_WHID_IO_H
#define _HEADER_WHID_IO_H

#include "cJSON.h"
#include <errno.h>
#include "whid_struct.h"
#include "whid_utils.h"

// Text color codes.
#define FG_GREEN L"\x1b[32m"
#define FG_WHITE L"\x1b[37m"
#define FG_ORANGE L"\x1b[48:5:208m"

// Background color codes.
#define BG_RED L"\x1b[41m"
#define BG_GREEN L"\x1b[42m"

// Text attributes.
#define RESET L"\x1b[0m"
#define BOLD L"\x1b[1m"
#define ITALIC L"\x1b[3m"
#define BLINK L"\x1b[5m"
#define STRIKE L"\x1b[9m"

#define STRCTIME L"%A %d %B %Y (Semaine %V) - %T"
#define STRCTIME_SHORT L"%d/%m/%y - %T"

// Macro for colored printing with attributes.
#define SET_TERMINAL_ATTRIBUTE(attr, color, fmt, ...) wprintf(attr color fmt __VA_ARGS__);
#define RESET_TERMINAL_ATTRIBUTE() wprintf(RESET);

// Functions for import/export journey.
struct Journey*  ImportFromFile(const char* p_filename);
void             ExportToFile(const char* p_filename, struct Journey* p_journey);
struct Journey*  ParseJourney(cJSON* p_cJSONJourney);

// Functions for U/I stuff.
wchar_t* SetTitle(wchar_t* p_title, bool mandatory);
wchar_t* AskUserFilename();
i8       GetUserChoice(void);
void     WaitUserInput(void);
void     PrintJourney(struct Journey* p_journey);
void     PrintActivity(struct Activity* p_activity, bool isBreakTime);
void     PrintEvent(struct Event* p_event);

#endif  // _HEADER_WHID_IO_H