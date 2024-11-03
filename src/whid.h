#pragma once
#ifndef _HEADER_WHID_H
#define _HEADER_WHID_H

#include <locale.h>
#include <signal.h>
#include "whid_utils.h"
#include "whid_io.h"
#include "whid_struct.h"

#define VERSION L"1.2"

#if defined(_WIN32) || defined(WIN32)
#define CLEAN_SCREEN "cls"
#define VERSION_EXTENDED VERSION L"_win32"
#include <windows.h>
#endif
#ifdef __linux__
#define CLEAN_SCREEN "clear"
#define VERSION_EXTENDED VERSION L"_linux"
#include <unistd.h>
#endif

// Constants definition.
#define MENU_NEW_DAY 1
#define MENU_DEJA_VU 2
#define MENU_NEW_ACTIVITY 1
#define MENU_RESUME_LAST_ACTIVITY 2
#define MENU_RESUME_ACTIVITY 3
#define MENU_TAKE_BREAK 4
#define MENU_EDIT_ACTIVITY 5
#define MENU_SHOW_JOURNEY_SUMMARY 6
#define MENU_SET_LOCATION 7
#define MENU_SHOW_ACTIVITY_SUMMARY 1
#define MENU_SET_TITLE_ACTIVITY 2
#define MENU_DELETE_ACTIVITY 3
#define MENU_SET_TITLE_NEW_ACTIVITY 1
#define MENU_START_ACTIVITY 2
#define MENU_QUIT 9

// Functions part.
void             BreakRunningActivity(int signal);
void             ExitProperly(int codeReturned);
void             SilentKill(int signal);
void             DrawMenuHeader(const wchar_t* p_menuTitle);
void             MenuDawnOfANewDay(void);
void             MenuCreateActivity(void);
struct Activity* MenuChooseActivity(void);
void             MenuEditActivity(struct Activity* activity);
void             MenuRunningActivity(struct Activity* activity);
void             MenuChangeLocation(void);

#if defined(_WIN32) || defined(WIN32)
BOOL WINAPI ConsoleHandler(DWORD signal);
#endif

#endif  // _HEADER_WHID_H