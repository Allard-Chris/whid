#include "whid.h"

static struct Journey* gp_journey;
static wchar_t*        gp_title;
static wchar_t*        gp_reason;
static bool            gp_silentKill;
static wchar_t*        gp_filename;

#if defined(_WIN32) || defined(WIN32)
BOOL WINAPI ConsoleHandler(DWORD signal) {
  if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT) {
    BreakRunningActivity(0);
    return true;
  } else if (signal == CTRL_CLOSE_EVENT || signal == CTRL_LOGOFF_EVENT || signal == CTRL_SHUTDOWN_EVENT) {
    SilentKill(0);
    return true;
  }
  return false;
}
#endif

// Catch SIGINT signal.
void BreakRunningActivity(int signal) {
  if (gp_journey != NULL) {
    gp_journey->currentStatus = STATE_STOPPED;
  }
}

void SilentKill(int signal) {
  BreakRunningActivity(0);
  gp_silentKill = true;
}

// main function.
int main(int argc, const char** argv) {
  u8 menuChoice = 0;

  // Init global variables.
  gp_journey = NULL;
  gp_title = NULL;
  gp_reason = NULL;
  gp_silentKill = false;
  gp_filename = NULL;

#if defined(_WIN32) || defined(WIN32)
  // set console ouput format. Handle accent.
  _setmode(_fileno(stdin), _O_WTEXT);

  if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
    ExitProperly(-1);
  }
#endif
#ifdef __linux__
  if (signal(SIGINT, BreakRunningActivity) == SIG_ERR) {
    ExitProperly(-1);
  }

  if (signal(SIGTERM, SilentKill) == SIG_ERR) {
    ExitProperly(-1);
  }
#endif
  setlocale(LC_ALL, "");
  // Check if we want to reopen a journey through an argument.
  if (argc > 1) {
    gp_filename = Utf8ToWchar(argv[1]);
    gp_journey = ImportFromFile(argv[1]);
    if (gp_journey != NULL) {
      MenuDawnOfANewDay();
    } else {
      gp_filename = NULL;
    }
  }

  do {
    DrawMenuHeader(L"MAIN MENU");
    wprintf(L"1. DAWN OF A NEW DAY.\n");
    wprintf(L"2. I HAVE A DEJA VU.\n");
    wprintf(L"9. EXIT.\n");
    menuChoice = GetUserChoice();

    switch (menuChoice) {
      case MENU_NEW_DAY:
        MenuDawnOfANewDay();
        break;

      case MENU_DEJA_VU:
        gp_filename = AskUserFilename();
        if (gp_journey != NULL) {
          FreeJourney(gp_journey);
        }
        gp_journey = ImportFromFile(WcharToUtf8(gp_filename));
        if (gp_journey != NULL) {
          gp_journey->duration = ComputeJourneyDuration(gp_journey->p_listActivities);
          MenuDawnOfANewDay();
        } else {
          gp_filename = NULL;
        }
        break;

      case MENU_QUIT:
        break;
    }
  } while (menuChoice != 9);

  ExitProperly(0);
}

void ExitProperly(int codeReturned) {
  if (gp_journey != NULL) {
    gp_journey->checkOut = time(NULL);
  }

  if (gp_silentKill == false) {
    PrintJourney(gp_journey);
  }

  ExportToFile(WcharToUtf8(gp_filename), gp_journey);
  FreeJourney(gp_journey);
  exit(codeReturned);
}

void DrawMenuHeader(const wchar_t* p_menuTitle) {
  u8         cr = system(CLEAN_SCREEN);
  wchar_t    buffer[MAX_STRING_SIZE];
  struct tm  timeInfo;
  struct tm* p_timeInfo = &timeInfo;

  if (cr != 0) {
    wprintf(L"Cannot execute: %hs\n", (char*)CLEAN_SCREEN);
  }

  // clang-format off
  wprintf(L"+------------------------------------------------------------------------------+\n");
  wprintf(L"What Have I Done\n");
  wprintf(L"Version: ");
  wprintf(L"%ls\n", VERSION_EXTENDED);
  wprintf(L"---\n");
  wprintf(L"");
  SET_TERMINAL_ATTRIBUTE(FG_WHITE, BOLD, L"");
  wprintf(L"%ls", p_menuTitle);
  RESET_TERMINAL_ATTRIBUTE();
  wprintf(L"\n");

  if (gp_journey != NULL) {
     wprintf(L"Started at: ");
     SET_TERMINAL_ATTRIBUTE(BOLD, L"", L"");
     //localtime_s(&p_timeInfo, &gp_journey->checkIn);
     UniLocaltime(&p_timeInfo, &gp_journey->checkIn);
     wcsftime(buffer, MAX_STRING_SIZE, STRCTIME, p_timeInfo);
     wprintf(L"%ls", buffer);
     RESET_TERMINAL_ATTRIBUTE();
     wprintf(L"\n");
     wprintf(L"Total Tasks: ");
     SET_TERMINAL_ATTRIBUTE(BOLD, L"", L"");
     if (gp_journey->p_listActivities != NULL) {
       wprintf(L"%d", GetNbActivities(&gp_journey->p_listActivities));
     } else {
       wprintf(L"%d", 0);
     }

     RESET_TERMINAL_ATTRIBUTE();
     wprintf(L"\n");
     wprintf(L"Total activities duration: ");
     SET_TERMINAL_ATTRIBUTE(BOLD, L"", L"");
     if (gp_journey->p_listActivities != NULL) {
       gp_journey->duration = ComputeJourneyDuration(gp_journey->p_listActivities);
       ConvertSecondsToTime(gp_journey->duration, buffer);
       wprintf(L"%ls", buffer);
     } else {
       wprintf(L"%d", 0);
     }

     RESET_TERMINAL_ATTRIBUTE();
     wprintf(L"\n");
     wprintf(L"Location: ");
     SET_TERMINAL_ATTRIBUTE(BOLD, L"", L"");
     wprintf(L"%ls", (gp_journey->location == LOCATION_OFFICE) ? L"Office" : L"Home");
     RESET_TERMINAL_ATTRIBUTE();
     wprintf(L"\n");

     wprintf(L"Current state: ");
     switch (gp_journey->currentStatus) {
       case STATE_STOPPED:
         SET_TERMINAL_ATTRIBUTE(BLINK, FG_WHITE, FG_ORANGE);
         SET_TERMINAL_ATTRIBUTE(BOLD, L"", L"");
         wprintf(L"IDLE");
         RESET_TERMINAL_ATTRIBUTE();
         wprintf(L"%ls\n", L"");
         break;

       case STATE_RUNNING:
         SET_TERMINAL_ATTRIBUTE(BLINK, FG_WHITE, BG_RED);
         SET_TERMINAL_ATTRIBUTE(BOLD, L"", L"");
         wprintf(L"RUNNING");
         RESET_TERMINAL_ATTRIBUTE();
         wprintf(L"%ls\n", L"");
         break;

       case STATE_BREAK:
         SET_TERMINAL_ATTRIBUTE(BLINK, FG_WHITE, BG_GREEN);
         SET_TERMINAL_ATTRIBUTE(BOLD, L"", L"");
         wprintf(L"TAKE A BREAK");
         RESET_TERMINAL_ATTRIBUTE();
         wprintf(L"%ls\n", L"");
         break;
    }
  }
   wprintf(L"+------------------------------------------------------------------------------+\n");
  // clang-format on
  return;
}

void MenuDawnOfANewDay(void) {
  u8               menuChoice = 0;
  wchar_t*         p_breakTitle;
  struct Activity* p_activity;

  if (gp_journey == NULL) {
    gp_journey = CreateJourney();

    if (gp_journey == NULL) {
      ExitProperly(-1);
    } else {
      gp_journey->checkIn = time(NULL);
    }
  }

  if (gp_journey != NULL) {
    do {
      DrawMenuHeader(L"MENU - DAWN OF A NEW DAY");
      wprintf(L"1. NEW ACTIVITY.\n");

      if (gp_journey->p_lastActivity != NULL) {
        wprintf(L"2. RESUME LAST ACTIVITY: ");
        SET_TERMINAL_ATTRIBUTE(BOLD, FG_WHITE, L"");
        wprintf(L"\"%ls\"\n", gp_journey->p_lastActivity->p_title);
        RESET_TERMINAL_ATTRIBUTE();
      } else {
        SET_TERMINAL_ATTRIBUTE(STRIKE, L"", L"");
        wprintf(L"2. RESUME LAST ACTIVITY\n");
        RESET_TERMINAL_ATTRIBUTE();
      }
      wprintf(L"3. RESUME ACTIVITY.\n");
      wprintf(L"4. TAKE A BREAK.\n");
      wprintf(L"5. EDIT ACTIVITY.\n");
      wprintf(L"6. SHOW JOURNEY SUMMARY.\n");
      wprintf(L"7. SET LOCATION.\n");
      wprintf(L"9. GO BACK.\n");

      menuChoice = GetUserChoice();
      switch (menuChoice) {
        case MENU_NEW_ACTIVITY:
          MenuCreateActivity();
          break;

        case MENU_TAKE_BREAK:
          if (gp_journey->p_breakTime == NULL) {
            p_breakTitle = SetTitle(L"Take a break", false);
            if (p_breakTitle == NULL) {
              ExitProperly(-1);
            }
            p_activity = CreateActivity(p_breakTitle, true, 0, NULL);
            gp_journey->p_breakTime = p_activity;
          } else {
            p_activity = gp_journey->p_breakTime;
            CreateEvent(EVENT_TYPE_RESUME, NULL, 0, &gp_journey->p_breakTime->p_listEvents);
          }
          MenuRunningActivity(p_activity);

          break;

        case MENU_RESUME_LAST_ACTIVITY:
          if (gp_journey->p_lastActivity != NULL) {
            CreateEvent(EVENT_TYPE_RESUME, NULL, 0, &gp_journey->p_lastActivity->p_listEvents);
            MenuRunningActivity(gp_journey->p_lastActivity);
          }
          break;

        case MENU_RESUME_ACTIVITY:
          if (gp_journey->p_listActivities != NULL) {
            p_activity = MenuChooseActivity();
            if (p_activity != NULL) {
              gp_journey->p_lastActivity = p_activity;
              CreateEvent(EVENT_TYPE_RESUME, NULL, 0, &p_activity->p_listEvents);
              MenuRunningActivity(p_activity);
            }
          }
          break;

        case MENU_EDIT_ACTIVITY:
          if (gp_journey->p_listActivities != NULL) {
            p_activity = MenuChooseActivity();
            if (p_activity != NULL) {
              MenuEditActivity(p_activity);
            }
          }
          break;

        case MENU_SHOW_JOURNEY_SUMMARY:
          PrintJourney(gp_journey);
          break;

        case MENU_SET_LOCATION:
          MenuChangeLocation();
          break;
      }
    } while (menuChoice != 9);
  }
  return;
}

void MenuCreateActivity(void) {
  u8       menuChoice = 1;
  wchar_t* p_title = NULL;

  do {
    DrawMenuHeader(L"MENU - NEW ACTIVITY");
    if (gp_title != NULL) {
      wprintf(L"1. SET TITLE: \"%ls\".\n", gp_title);
    } else {
      wprintf(L"1. SET TITLE: \"\".\n");
    }
    wprintf(L"2. START.\n");
    wprintf(L"9. GO BACK.\n");

    menuChoice = GetUserChoice();
    switch (menuChoice) {
      case MENU_SET_TITLE_NEW_ACTIVITY:
        p_title = SetTitle(NULL, true);
        if (p_title == NULL) {
          ExitProperly(-1);
        }
        if (IsActivityExists(p_title, &gp_journey->p_listActivities)) {
          wprintf(L"This title already exists !\n");
          WaitUserInput();
          break;
        } else {
          gp_title = p_title;
        }
        break;

      case MENU_START_ACTIVITY:
        if (p_title) {
          gp_journey->p_lastActivity = CreateActivity(p_title, false, 0, &gp_journey->p_listActivities);
          MenuRunningActivity(gp_journey->p_lastActivity);
          menuChoice = 9;
        } else {
          wprintf(L"No title was set.\n");
          WaitUserInput();
        }
        break;
    }
  } while (menuChoice != 9);

  if (gp_title) {
    gp_title = NULL;
  }
}

struct Activity* MenuChooseActivity(void) {
  u8               menuChoice;
  u8               nbActivities;
  struct Activity* p_allActivities[MAX_SIZE - 1];
  struct Activity* p_currentActivity;

  do {
    DrawMenuHeader(L"MENU - SELECT ACTIVITIES");
    nbActivities = 0;
    p_currentActivity = gp_journey->p_listActivities;

    // print all tasks.
    while (p_currentActivity != NULL) {
      wprintf(L"[%d] - %ls\n", nbActivities + 1, p_currentActivity->p_title);
      p_allActivities[nbActivities] = p_currentActivity;
      nbActivities += 1;
      p_currentActivity = p_currentActivity->p_nextActivity;
    }
    wprintf(L"0. GO BACK.\n");

    menuChoice = GetUserChoice();
    if ((menuChoice > 0) && (menuChoice < nbActivities + 1)) {
      p_currentActivity = p_allActivities[menuChoice - 1];
      menuChoice = 0;
    }
  } while (menuChoice != 0);

  return p_currentActivity;
}

void MenuEditActivity(struct Activity* p_activity) {
  u8       menuChoice = 0;
  wchar_t* p_newTitle = NULL;

  do {
    DrawMenuHeader(L"MENU - EDIT ACTIVITY");
    wprintf(L"1. SHOW ACTIVITY SUMMARY.\n");
    wprintf(L"2. SET TITLE.\n");
    wprintf(L"3. DELETE ACTIVITY.\n");
    wprintf(L"9. GO BACK.\n");

    menuChoice = GetUserChoice();
    switch (menuChoice) {
      case MENU_SHOW_ACTIVITY_SUMMARY:
        wprintf(L"---\n");
        PrintActivity(p_activity, false);
        WaitUserInput();
        break;

      case MENU_SET_TITLE_ACTIVITY:
        gp_title = p_activity->p_title;
        wprintf(L"Old title is: \"%ls\"\n", gp_title);
        p_newTitle = SetTitle(NULL, true);
        if (p_newTitle == NULL) {
          ExitProperly(-1);
        }
        free(gp_title);
        p_activity->p_title = p_newTitle;
        break;

      case MENU_DELETE_ACTIVITY:
        if (gp_journey->p_lastActivity != NULL) {
          if (gp_journey->p_lastActivity->id == p_activity->id) {
            gp_journey->p_lastActivity = NULL;
          }
        }
        RemoveActivity(p_activity->id, &gp_journey->p_listActivities);
        menuChoice = 9;
        break;
    }
  } while (menuChoice != 9);
}

void MenuRunningActivity(struct Activity* p_activity) {
  time_t     now = time(NULL);
  time_t     atTime = 0;
  u32        totalSeconds = 0;
  u32        oldDuration = ComputeActivityDuration(p_activity->p_listEvents, p_activity->startedAt);
  wchar_t    buffer[MAX_STRING_SIZE];
  struct tm  timeInfo;
  struct tm* p_timeInfo = &timeInfo;

  if (p_activity->id == BREAK_TIME_ID) {
    gp_journey->currentStatus = STATE_BREAK;
  } else {
    gp_journey->currentStatus = STATE_RUNNING;
  }

  DrawMenuHeader(L"MENU - RUNNING ACTIVITY");
  wprintf(L"Title: ");
  SET_TERMINAL_ATTRIBUTE(BOLD, FG_WHITE, L"");
  wprintf(L"\"%ls\"\n", p_activity->p_title);
  RESET_TERMINAL_ATTRIBUTE();
  wprintf(L"Started at: ");
  SET_TERMINAL_ATTRIBUTE(BOLD, FG_WHITE, L"");
  UniLocaltime(&p_timeInfo, &p_activity->startedAt);
  wcsftime(buffer, MAX_STRING_SIZE, STRCTIME_SHORT, p_timeInfo);
  wprintf(L"%ls\n", buffer);
  RESET_TERMINAL_ATTRIBUTE();

  // Running activity until hang from user.
  wprintf(L"\n");
  SET_TERMINAL_ATTRIBUTE(ITALIC, L"", L"");
  wprintf(L"Press Ctrl+c to stop.\n");
  RESET_TERMINAL_ATTRIBUTE();

  while (gp_journey->currentStatus != STATE_STOPPED) {
    RESET_TERMINAL_ATTRIBUTE();
    wprintf(L"\rRunning activity: ");
    SET_TERMINAL_ATTRIBUTE(BOLD, L"", L"");
    totalSeconds = (u32)difftime(time(NULL), now);
    ConvertSecondsToTime((totalSeconds + oldDuration), buffer);
    wprintf(L"%ls", buffer);
    fflush(stdout);  // permit to print on the same line.
#if defined(WIN32) || defined(_WIN32)
    Sleep(500);
#endif
#ifdef __linux__
    usleep(500);
#endif
  }
  RESET_TERMINAL_ATTRIBUTE();

  // create a new event when we stop.
  atTime = time(NULL);
  wprintf(L"\n");

  if (gp_silentKill == true) {
    gp_reason = SetTitle(L"Program stopped by system KILL.", false);
  } else {
    gp_reason = SetTitle(NULL, false);
  }
  CreateEvent(STATE_STOPPED, gp_reason, atTime, &p_activity->p_listEvents);

  if (gp_silentKill == true) {
    ExitProperly(0);
  }

  // update total_duration for the journey.
  gp_journey->duration = ComputeJourneyDuration(gp_journey->p_listActivities);
}

void MenuChangeLocation(void) {
  u8 menuChoice = 0;

  do {
    DrawMenuHeader(L"MENU - SET LOCATION");
    wprintf(L"1. Location: office.\n");
    wprintf(L"2. Location: home.\n");
    wprintf(L"9. Go back.\n");

    menuChoice = GetUserChoice();
    switch (menuChoice) {
      case LOCATION_OFFICE:
        gp_journey->location = LOCATION_OFFICE;
        menuChoice = 9;
        break;

      case LOCATION_HOME:
        gp_journey->location = LOCATION_HOME;
        menuChoice = 9;
        break;
    }
  } while (menuChoice != 9);
}
