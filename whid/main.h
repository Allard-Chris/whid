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
#define VERSION 0.1
#endif

#ifdef WIN32
#define CLEAN_SCREEN "CLS"
#else
#define CLEAN_SCREEN "clear"
#endif

#define MENU_NEW_DAY 1
#define MENU_OPEN_DAY 2
#define MENU_CREATE_NEW_EVENT 1
#define MENU_CREATE_NEW_TASK 2
#define MENU_RESUME_LAST_ACTIVITY 3
#define MENU_RESUME_ACTIVITY 4
#define MENU_SET_TITLE 1
#define MENU_START_ACTIVITY 2
#define MENU_QUIT 9
#define IS_RUNNING 1

// Functions part
void        menuDawnOfANewDay(void);
void        menuCreateActivity(int type);
void        menuRunningActivity(struct activity* activity);
void        menuChooseActivity(void);
BOOL WINAPI breakRunningActivity(DWORD signal);
void        drawMenuHeader(wchar_t* menu_title);

#endif  // _HEADER_MAIN_H
