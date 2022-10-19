#pragma once
#ifndef _HEADER_MAIN_H
#define _HEADER_MAIN_H

#include <fcntl.h>
#include <io.h>
#include <locale.h>
#include <signal.h>
#include <windows.h>
#include "whid_utils.h"

#ifndef VERSION
#define VERSION "1.0a"
#endif

#ifdef WIN32
#define CLEAN_SCREEN "cls"
#else
#define CLEAN_SCREEN "clear"
#endif

#define MENU_NEW_DAY 1
#define MENU_CREATE_NEW_TASK 1
#define MENU_RESUME_LAST_ACTIVITY 2
#define MENU_RESUME_ACTIVITY 3
#define MENU_EDIT_ACTIVITY 4
#define MENU_SHOW_JOURNEY_RESUME 5
#define MENU_SHOW_ACTIVITY 1
#define MENU_CHANGE_TITLE 2
#define MENU_REMOVE_ACTIVITY 3
#define MENU_SET_TITLE 1
#define MENU_START_ACTIVITY 2
#define MENU_QUIT 9
#define IS_RUNNING 1

// Functions part
BOOL WINAPI      breakRunningActivity(DWORD signal);
void             drawMenuHeader(wchar_t* menu_title);
void             menuCreateActivity();
void             menuDawnOfANewDay(void);
void             menuEditActivity(struct activity* activity);
void             menuRunningActivity(struct activity* activity);
struct activity* menuChooseActivity(void);
#endif  // _HEADER_MAIN_H
