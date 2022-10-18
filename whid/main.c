#include "main.h"

// Global variables
struct journey*  g_journey = NULL;
wchar_t*         g_title = NULL;
wchar_t*         g_reason = NULL;
int              g_current_state = STOPPED;
struct activity* g_last_activity = NULL;
u8               g_id = 0;
FILE*            g_xml_file;

// main function.
int main(int argc, const char** argv) {
  // set console ouput format. Handle accent.
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
    wprintf(L"9. Exit.\n");
    menu_choice = getUserChoice();

    switch (menu_choice) {
      case MENU_NEW_DAY:
        menuDawnOfANewDay();
        break;

      case MENU_QUIT:
        // Create last activity when quitting.
        if (g_journey != NULL) {
          wchar_t* str_out;
          str_out = setTitle(L"Pointage sortie", false);
          createActivity(str_out, false, g_id, &g_journey->activities, &g_last_activity);
          g_journey->nb_task += 1;
          g_id += 1;
          g_journey->last_activity = g_last_activity;
        }
        break;
    }
  } while (menu_choice != 9);

  // End program, export to file and print all activities.
  exportToFile(g_xml_file, g_journey);
  printJourney(g_journey);
  freeJourney(g_journey);
  return 0;
}

// Breaks loop during activity.
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
    wprintf(L"+------------------------------------------------------------------------------+\n");
    wprintf(L"| What Have I Done                                                             |\n");
    wprintf(L"| Version: %hs                                                                |\n", VERSION);
    wprintf(L"| ---                                                                          |\n");
    if (g_journey != NULL) {
        wprintf(L"| %-76ls |\n", menu_title);
        wprintf(L"| ---                                                                          |\n");
        wprintf(L"| Total Tasks:  %-62d |\n", g_journey->nb_task);
        wprintf(L"| Total activities duration: %-50ls|\n", convertSecondsToTime(g_journey->duration, s_time));
    }
    else {
        wprintf(L"| %-77ls|\n", menu_title);
    }
    wprintf(L"+------------------------------------------------------------------------------+\n");
  // clang-format on
  free(s_time);
}

void menuCreateActivity() {
  u8       menu_choice = 0;
  wchar_t* title = NULL;

  do {
    drawMenuHeader(L"Menu: Create a new task activity");
    if (g_title != NULL) {
      wprintf(L"1. Set title: \"%ls\".\n", g_title);
    } else {
      wprintf(L"1. Set title: \"\".\n");
    }
    wprintf(L"2. Start activity.\n");
    wprintf(L"9. Go back.\n");
    menu_choice = getUserChoice();

    switch (menu_choice) {
      case MENU_SET_TITLE:
        title = setTitle(NULL, true);
        g_title = title;
        break;

      case MENU_START_ACTIVITY:
        if (title) {
          createActivity(title, true, g_id, &g_journey->activities, &g_last_activity);
          g_journey->last_activity = g_last_activity;
          g_journey->nb_task += 1;
          g_id += 1;
          menuRunningActivity(g_journey->last_activity);
          menu_choice = 9;
        } else {
          wprintf(L"No title was set.\n");
          waitUserInput();
        }
        break;
    }
  } while (menu_choice != 9);

  if (g_title) {
    g_title = NULL;
  }
}

void menuDawnOfANewDay(void) {
  u8 menu_choice = 0;

  if (g_journey == NULL) {
    // generate filename with current date.
    char* s_date = calloc(MAX_STRING_SIZE, sizeof(char));
    if (!s_date) {
      return;
    }
    struct tm  tm_current_time;
    __time64_t current_time;
    _time64(&current_time);
    _localtime64_s(&tm_current_time, &current_time);
    sprintf_s(s_date, MAX_STRING_SIZE, "%02d%02d%02d_logs.xml", tm_current_time.tm_year + 1900, tm_current_time.tm_mon + 1, tm_current_time.tm_mday);

    // create file.
    if (fopen_s(&g_xml_file, s_date, "w") != 0) {
      wprintf(L"Can't create file\n");
      free(s_date);
      return;
    }
    free(s_date);

    // create a new journey.
    g_journey = createJourney();
    wchar_t* str_in;
    str_in = setTitle(L"Pointage entrée", false);
    createActivity(str_in, false, g_id, &g_journey->activities, &g_last_activity);
    g_journey->nb_task += 1;
    g_id += 1;
    g_journey->first_activity = g_last_activity;
  }

  do {
    drawMenuHeader(L"Menu: Dawn of a new day");
    wprintf(L"1. Create a new task activity.\n");
    wprintf(L"2. Resume last activity.\n");
    wprintf(L"3. Resume an activity.\n");
    wprintf(L"4. Edit an activity.\n");
    wprintf(L"5. Show journey summary.\n");
    wprintf(L"9. Go back.\n");
    menu_choice = getUserChoice();

    switch (menu_choice) {
      case MENU_CREATE_NEW_TASK:
        menuCreateActivity();
        break;

      case MENU_RESUME_LAST_ACTIVITY:
        if (g_journey->last_activity != NULL) {
          if (g_journey->last_activity->events != EVENT) {
            createEvent(RESUME, NULL, &g_journey->last_activity->events);
            menuRunningActivity(g_journey->last_activity);
          }
        }
        break;

      case MENU_RESUME_ACTIVITY:
        if (g_journey->activities != NULL) {
          struct activity* activity = menuChooseActivity();
          if (activity != NULL) {
            createEvent(RESUME, NULL, &activity->events);
            menuRunningActivity(activity);
          }
        }
        break;

      case MENU_EDIT_ACTIVITY:
        if (g_journey->activities != NULL) {
          struct activity* activity = menuChooseActivity();
          if (activity != NULL) {
            menuEditActivity(activity);
          }
        }
        break;

      case MENU_SHOW_JOURNEY_RESUME:
        printJourney(g_journey);
        break;
    }
  } while (menu_choice != 9);
}

void menuEditActivity(struct activity* activity) {
  u8       menu_choice = 0;
  wchar_t* new_title = NULL;

  do {
    drawMenuHeader(L"Menu: Edit activity");
    wprintf(L"1. Show activity summary.\n");
    wprintf(L"2. Change title.\n");
    wprintf(L"3. Remove activity.\n");
    wprintf(L"9. Go back.\n");
    menu_choice = getUserChoice();

    switch (menu_choice) {
      case MENU_SHOW_ACTIVITY:
        wprintf(L"---\n");
        printActivity(activity);
        waitUserInput();
        break;

      case MENU_CHANGE_TITLE:
        g_title = activity->title;
        wprintf(L"Old title is: \"%ls\"\n", g_title);
        new_title = setTitle(NULL, true);
        free(g_title);
        activity->title = new_title;
        break;

      case MENU_REMOVE_ACTIVITY:
        removeActivity(activity->id, &g_journey->activities);
        g_journey->nb_task -= 1;
        menu_choice = 9;
        break;
    }
  } while (menu_choice != 9);
}

void menuRunningActivity(struct activity* activity) {
  time_t   t_current_activity = time(NULL);
  u32      total_seconds = 0;
  u32      old_duration = computeActivityDuration(activity->events, activity->started_at);
  wchar_t* ws_buffer = malloc(MAX_WSTRING_BUFFER_SIZE);

  if (!ws_buffer) {
    free(ws_buffer);
    return;
  }

  drawMenuHeader(L"Menu: Running activity");
  wprintf(L"Title: %ls\n", activity->title);
  wprintf(L"Started at: %ls\n", ctimeToWstring(ws_buffer, &activity->started_at));

  // if we have resumed the activity.
  if (old_duration != 0) {
    wprintf(L"Last activity duration: %ls\n", convertSecondsToTime(old_duration, ws_buffer));
  }

  // Running activity until hang from user.
  wprintf(L"\n");
  wprintf(L"Ctrl+c to stop current activity.\n");
  g_current_state = IS_RUNNING;
  while (g_current_state != STOPPED) {
    total_seconds = (u32)difftime(time(NULL), t_current_activity);
    wprintf(L"\r%ls", convertSecondsToTime(total_seconds, ws_buffer));
    fflush(stdout);  // permit to print on the same line.
    Sleep(1000);
  }

  // create a new event when we stop.
  wprintf(L"\n");
  g_reason = setTitle(NULL, false);
  createEvent(STOPPED, g_reason, &activity->events);

  // update total_duration for the journey
  g_journey->duration = computeJourneyDuration(g_journey->activities);

  free(ws_buffer);
}

struct activity* menuChooseActivity(void) {
  u8               menu_choice = 0;
  u8               nb_activity = 0;
  struct activity* all_activities[MAX_SIZE];
  struct activity* head_activity = g_journey->activities;

  do {
    drawMenuHeader(L"Menu: Choose activities");

    // print all tasks.
    while (head_activity != NULL) {
      if (head_activity->runnable) {
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
      menu_choice = 0;
    }
  } while (menu_choice != 0);

  return head_activity;
}
