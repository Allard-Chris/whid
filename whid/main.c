#include "main.h"

// Global variables
struct journey*  g_journey = NULL;
wchar_t*         g_last_title = NULL;
int              g_current_state = STOPPED;
struct activity* g_last_activity = NULL;
FILE*            g_xml_file;

// main function.
int main(int argc, const char** argv) {
  // set console ouput format
  _setmode(_fileno(stdin), _O_WTEXT);
  setlocale(LC_ALL, "");

  // user's signals handler.
  if (!SetConsoleCtrlHandler(breakRunningActivity, TRUE)) {
    wprintf(L"Failed SetConsoleCtrHandler");
    freeJourney(g_journey);
    return -1;
  }

  u8 menu_choice = 0;
  do {
    drawMenuHeader(L"Main menu");
    wprintf(L"1. Dawn of a new day.\n");
    wprintf(L"2. Open a day.\n");
    wprintf(L"9. Exit.\n");
    menu_choice = getUserChoice();

    switch (menu_choice) {
      case MENU_NEW_DAY:
        menuDawnOfANewDay();
        break;

      case MENU_OPEN_DAY:
        break;

      case MENU_QUIT:
        if (g_journey != NULL) {
          wchar_t* str_out = (wchar_t*)malloc(MAX_WSTRING_BUFFER_SIZE);
          strncpyTruncate(str_out, MAX_WSTRING_BUFFER_SIZE, L"Pointage sortie");
          createActivity(str_out, EVENT, &g_journey->activities, &g_last_activity);
          g_journey->last_activity = g_last_activity;
        }
        break;
    }
  } while (menu_choice != 9);
  exportToFile(g_xml_file, g_journey);
  printJourney(g_journey);
  freeJourney(g_journey);
  return 0;
}

void menuDawnOfANewDay(void) {
  u8 menu_choice = 0;

  if (g_journey == NULL) {
    // create file with current date.
    char* s_date = calloc(MAX_STRING_SIZE, sizeof(char));
    if (!s_date) {
      return;
    }
    struct tm  tm_current_time;
    __time64_t current_time;
    _time64(&current_time);
    _localtime64_s(&tm_current_time, &current_time);
    sprintf_s(s_date, MAX_STRING_SIZE, "%02d%02d%02d_logs.xml", tm_current_time.tm_year + 1900, tm_current_time.tm_mon + 1, tm_current_time.tm_mday);

    if (fopen_s(&g_xml_file, s_date, "w") != 0) {
      wprintf(L"Can't create file\n");
      free(s_date);
      return;
    }
    free(s_date);

    g_journey = createJourney();
    wchar_t* str_in = (wchar_t*)malloc(MAX_WSTRING_BUFFER_SIZE);
    strncpyTruncate(str_in, MAX_WSTRING_BUFFER_SIZE, L"Pointage entrée");
    createActivity(str_in, EVENT, &g_journey->activities, &g_last_activity);
    g_journey->first_activity = g_last_activity;
  }

  do {
    drawMenuHeader(L"1. Dawn of a new day");
    wprintf(L"1. Create a new event activity.\n");
    wprintf(L"2. Create a new task activity.\n");
    wprintf(L"3. Resume last activity.\n");
    wprintf(L"4. Resume an activity.\n");
    wprintf(L"9. Go back.\n");
    menu_choice = getUserChoice();

    switch (menu_choice) {
      case MENU_CREATE_NEW_EVENT:
        menuCreateActivity(EVENT);
        break;

      case MENU_CREATE_NEW_TASK:
        menuCreateActivity(TASK);
        break;

      case MENU_RESUME_LAST_ACTIVITY:
        if (g_journey->last_activity != NULL) {
          if (g_journey->last_activity->events != EVENT) {
            createEvent(RESUME, &g_journey->last_activity->events);
            menuRunningActivity(g_journey->last_activity);
          }
        }
        break;

      case MENU_RESUME_ACTIVITY:
        if (g_journey->activities != NULL) {
          menuChooseActivity();
        }
        break;
    }
  } while (menu_choice != 9);
}

void menuCreateActivity(int type) {
  u8       menu_choice = 0;
  wchar_t* title = NULL;

  do {
    if (type == EVENT) {
      drawMenuHeader(L"1-1. Create a new event activity");
    } else {
      drawMenuHeader(L"1-2. Create a new task activity");
    }
    if (g_last_title != NULL) {
      wprintf(L"1. Set title: \"%ls\".\n", g_last_title);
    } else {
      wprintf(L"1. Set title: \"\".\n");
    }
    if (type == EVENT) {
      wprintf(L"2. Create event.\n");
    } else {
      wprintf(L"2. Start activity.\n");
    }
    wprintf(L"9. Go back.\n");
    menu_choice = getUserChoice();

    switch (menu_choice) {
      case MENU_SET_TITLE:
        title = setTitle(NULL);
        g_last_title = title;
        break;

      case MENU_START_ACTIVITY:
        if (type == TASK) {
          createActivity(title, type, &g_journey->activities, &g_last_activity);
          g_journey->last_activity = g_last_activity;
          g_journey->nb_task += 1;
          menuRunningActivity(g_journey->last_activity);
        } else {
          createActivity(title, type, &g_journey->activities, NULL);
        }

        menu_choice = 9;
        break;
    }
  } while (menu_choice != 9);

  if (g_last_title) {
    g_last_title = NULL;
  }
}

void menuRunningActivity(struct activity* activity) {
  time_t   t_current_activity = time(NULL);
  u32      total_seconds = 0;
  u32      old_duration = computeActivityDuration(activity->events, activity->started_at);
  wchar_t* ws_buffer = malloc(MAX_WSTRING_BUFFER_SIZE);
  char*    s_buffer = malloc(MAX_STRING_BUFFER_SIZE);

  if (!ws_buffer || !s_buffer) {
    free(ws_buffer);
    free(s_buffer);
    return;
  }

  drawMenuHeader(L"Running activity");
  wprintf(L"Title: %ls\n", activity->title);
  ctime_s(s_buffer, MAX_STRING_SIZE, &activity->started_at);
  mbstowcs_s(NULL, ws_buffer, strlen(s_buffer) + 1, s_buffer, strlen(s_buffer));
  wprintf(L"Started at: %ls\n", ws_buffer);

  // if we have resumed the activity.
  if (old_duration != 0) {
    wprintf(L"Last duration: %ls\n", convertSecondsToTime(old_duration, ws_buffer));
  }

  // Running activity until hang from user.
  wprintf(L"Ctrl+c to stop current activity.\n");
  g_current_state = IS_RUNNING;
  while (g_current_state != STOPPED) {
    total_seconds = (u32)difftime(time(NULL), t_current_activity);
    wprintf(L"\r%ls", convertSecondsToTime(total_seconds, ws_buffer));
    fflush(stdout);  // permit to print on the same line.
    Sleep(1000);
  }

  // create a new event when we stop.
  createEvent(STOPPED, &activity->events);

  // update total_duration for the journey
  g_journey->duration = computeJourneyDuration(g_journey->activities);

  free(ws_buffer);
  free(s_buffer);
}

void menuChooseActivity(void) {
  u8               menu_choice = 0;
  u8               nb_activity = 0;
  struct activity* all_activities[MAX_SIZE];
  struct activity* head_activity = g_journey->activities;

  do {
    drawMenuHeader(L"Choose activities");

    // print all tasks.
    while (head_activity != NULL) {
      if (head_activity->type == TASK) {
        wprintf(L"[%d] - %ls\n", nb_activity + 1, head_activity->title);
        all_activities[nb_activity] = head_activity;
        nb_activity += 1;
      }
      head_activity = head_activity->next_activity;
    }
    wprintf(L"0. Go back.\n");
    menu_choice = getUserChoice();

    if ((menu_choice > 0) && (menu_choice < nb_activity + 1)) {
      head_activity = all_activities[menu_choice - 1];
      createEvent(RESUME, &head_activity->events);
      menuRunningActivity(head_activity);

      menu_choice = 0;
    }
  } while (menu_choice != 0);
}

BOOL WINAPI breakRunningActivity(DWORD signal) {
  switch (signal) {
    case CTRL_C_EVENT:
      g_current_state = STOPPED;
      return TRUE;
    default:
      return FALSE;
  }
}

void drawMenuHeader(wchar_t* menu_title) {
  wchar_t* s_time = malloc(MAX_WSTRING_BUFFER_SIZE);
  u8       cr = system(CLEAN_SCREEN);
  if (cr != 0) {
    wprintf(L"Cannot execute: %hs\n", (char*)CLEAN_SCREEN);
  }

  // clang-format off
    wprintf(L"╔══════════════════════════════════════════════════════════════════════════════╗\n");
    wprintf(L"║ What Have I Done                                                             ║\n");
    wprintf(L"║ Version: %1.2f                                                                ║\n", VERSION);
    wprintf(L"║                                                                              ║\n");
    if (g_journey != NULL) {
        wprintf(L"║ %-35ls Total Tasks: %d, Total duration: %ls ║\n", menu_title, g_journey->nb_task, convertSecondsToTime(g_journey->duration, s_time));
    }
    else {
        wprintf(L"║ %-77ls║\n", menu_title);
    }
    wprintf(L"╚══════════════════════════════════════════════════════════════════════════════╝\n");
  // clang-format on
  free(s_time);
}
