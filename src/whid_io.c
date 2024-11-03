#include "whid_io.h"

// Functions for import/export journey.
void ExportToFile(const char* p_filename, struct Journey* p_journey) {
  wchar_t          buffer[MAX_STRING_SIZE];
  struct Activity* p_currentActivity = NULL;
  struct Event*    p_currentEvent = NULL;
  cJSON*           p_cJSONJourney = NULL;
  cJSON*           p_cJSONActivities = NULL;
  cJSON*           p_cJSONActivity = NULL;
  cJSON*           p_cJSONEvents = NULL;
  cJSON*           p_cJSONEvent = NULL;
  cJSON*           p_cJSONBreaks = NULL;
  char*            p_jsonOutput = NULL;
  char*            p_jsonBuffer = NULL;
  size_t           length = 0;
  time_t           currentTime = time(NULL);
  FILE*            p_file = NULL;
  struct tm*       p_timeInfo;

  if (p_journey == NULL) {
    return;
  }

  // Create new json file.
  p_timeInfo = localtime(&currentTime);
  UniSwprintf(buffer, MAX_STRING_SIZE, L"%02d%02d%02d_logs.json", p_timeInfo->tm_year + 1900, p_timeInfo->tm_mon + 1, p_timeInfo->tm_mday);
  if (p_filename == NULL) {
    UniFopen(&p_file, WcharToUtf8(buffer), "w");
  } else {
    UniFopen(&p_file, p_filename, "w");
  }

  if (p_file == NULL) {
    UniFopen(&p_file, "crash.json", "w");
    if (p_file == NULL) {
      return;
    }
  }

  // Print journey's information.
  p_cJSONJourney = cJSON_CreateObject();

  p_timeInfo = localtime(&p_journey->checkIn);
  wcsftime(buffer, sizeof(buffer), STRCTIME, p_timeInfo);
  cJSON_AddItemToObject(p_cJSONJourney, "check_in", cJSON_CreateString(WcharToUtf8(buffer)));
  cJSON_AddItemToObject(p_cJSONJourney, "check_in_timestamp", cJSON_CreateNumber((long)p_journey->checkIn));

  p_timeInfo = localtime(&p_journey->checkOut);
  wcsftime(buffer, sizeof(buffer), STRCTIME, p_timeInfo);
  cJSON_AddItemToObject(p_cJSONJourney, "check_out", cJSON_CreateString(WcharToUtf8(buffer)));
  cJSON_AddItemToObject(p_cJSONJourney, "check_out_timestamp", cJSON_CreateNumber((long)p_journey->checkOut));

  cJSON_AddItemToObject(p_cJSONJourney, "location", cJSON_CreateString((p_journey->location == LOCATION_OFFICE) ? "Office" : "Home"));

  ConvertSecondsToTime(ComputeJourneyDuration(p_journey->p_listActivities), buffer);
  cJSON_AddItemToObject(p_cJSONJourney, "total_activities_duration", cJSON_CreateString(WcharToUtf8(buffer)));

  if ((p_journey->p_breakTime != NULL) && (p_journey->p_breakTime->p_listEvents != NULL)) {
    ConvertSecondsToTime(ComputeActivityDuration(p_journey->p_breakTime->p_listEvents, p_journey->p_breakTime->startedAt), buffer);
    cJSON_AddItemToObject(p_cJSONJourney, "total_breaks_duration", cJSON_CreateString(WcharToUtf8(buffer)));
  }

  if (p_journey->checkOut != (time_t)-1) {
    ConvertSecondsToTime(ComputeIdleTimeDuration(p_journey), buffer);
    cJSON_AddItemToObject(p_cJSONJourney, "total_idle_duration", cJSON_CreateString(WcharToUtf8(buffer)));
  }
  cJSON_AddItemToObject(p_cJSONJourney, "nb_activities", cJSON_CreateNumber(GetNbActivities(&p_journey->p_listActivities)));

  // Print journey's activities.
  if (p_journey->p_listActivities != NULL) {
    p_cJSONActivities = cJSON_CreateArray();
    cJSON_AddItemToObject(p_cJSONJourney, "activities", p_cJSONActivities);
    p_currentActivity = p_journey->p_listActivities;
    while (p_currentActivity) {
      p_cJSONActivity = cJSON_CreateObject();
      cJSON_AddItemToArray(p_cJSONActivities, p_cJSONActivity);
      cJSON_AddItemToObject(p_cJSONActivity, "name", cJSON_CreateString(WcharToUtf8(p_currentActivity->p_title)));
      p_timeInfo = localtime(&p_currentActivity->startedAt);
      wcsftime(buffer, sizeof(buffer), STRCTIME_SHORT, p_timeInfo);
      cJSON_AddItemToObject(p_cJSONActivity, "started_at", cJSON_CreateString(WcharToUtf8(buffer)));
      cJSON_AddItemToObject(p_cJSONActivity, "timestamp", cJSON_CreateNumber((long)p_currentActivity->startedAt));
      ConvertSecondsToTime(ComputeActivityDuration(p_currentActivity->p_listEvents, p_currentActivity->startedAt), buffer);
      cJSON_AddItemToObject(p_cJSONActivity, "total_time", cJSON_CreateString(WcharToUtf8(buffer)));

      // Print activity's events.
      p_cJSONEvents = cJSON_CreateArray();
      cJSON_AddItemToObject(p_cJSONActivity, "events", p_cJSONEvents);

      p_currentEvent = p_currentActivity->p_listEvents;
      while (p_currentEvent) {
        p_cJSONEvent = cJSON_CreateObject();
        cJSON_AddItemToArray(p_cJSONEvents, p_cJSONEvent);
        cJSON_AddItemToObject(p_cJSONEvent, "type", cJSON_CreateString((p_currentEvent->type == EVENT_TYPE_RESUME) ? "RESUME" : "STOPPED"));
        cJSON_AddItemToObject(p_cJSONEvent, "reason", cJSON_CreateString(WcharToUtf8(p_currentEvent->p_reason)));
        p_timeInfo = localtime(&p_currentEvent->at);
        wcsftime(buffer, sizeof(buffer), STRCTIME_SHORT, p_timeInfo);
        cJSON_AddItemToObject(p_cJSONEvent, "at", cJSON_CreateString(WcharToUtf8(buffer)));
        cJSON_AddItemToObject(p_cJSONEvent, "timestamp", cJSON_CreateNumber((long)p_currentEvent->at));
        p_currentEvent = p_currentEvent->p_nextEvent;
      }
      p_currentActivity = p_currentActivity->p_nextActivity;
    }
  }

  // Part for break activity.
  if (p_journey->p_breakTime != NULL) {
    p_cJSONBreaks = cJSON_CreateObject();
    cJSON_AddItemToObject(p_cJSONJourney, "breaks", p_cJSONBreaks);
    cJSON_AddItemToObject(p_cJSONBreaks, "name", cJSON_CreateString(WcharToUtf8(p_journey->p_breakTime->p_title)));
    p_timeInfo = localtime(&p_journey->p_breakTime->startedAt);
    wcsftime(buffer, sizeof(buffer), STRCTIME_SHORT, p_timeInfo);
    cJSON_AddItemToObject(p_cJSONBreaks, "started_at", cJSON_CreateString(WcharToUtf8(buffer)));
    cJSON_AddItemToObject(p_cJSONBreaks, "timestamp", cJSON_CreateNumber((long)p_journey->p_breakTime->startedAt));
    ConvertSecondsToTime(ComputeActivityDuration(p_journey->p_breakTime->p_listEvents, p_journey->p_breakTime->startedAt), buffer);
    cJSON_AddItemToObject(p_cJSONBreaks, "total_time", cJSON_CreateString(WcharToUtf8(buffer)));

    p_cJSONEvents = cJSON_CreateArray();
    cJSON_AddItemToObject(p_cJSONBreaks, "events", p_cJSONEvents);
    p_currentEvent = p_journey->p_breakTime->p_listEvents;
    while (p_currentEvent) {
      p_cJSONEvent = cJSON_CreateObject();
      cJSON_AddItemToArray(p_cJSONEvents, p_cJSONEvent);
      cJSON_AddItemToObject(p_cJSONEvent, "type", cJSON_CreateString((p_currentEvent->type == EVENT_TYPE_RESUME) ? "RESUME" : "STOPPED"));
      cJSON_AddItemToObject(p_cJSONEvent, "reason", cJSON_CreateString(WcharToUtf8(p_currentEvent->p_reason)));
      p_timeInfo = localtime(&p_currentEvent->at);
      wcsftime(buffer, sizeof(buffer), STRCTIME_SHORT, p_timeInfo);
      cJSON_AddItemToObject(p_cJSONEvent, "at", cJSON_CreateString(WcharToUtf8(buffer)));
      cJSON_AddItemToObject(p_cJSONEvent, "timestamp", cJSON_CreateNumber((long)p_currentEvent->at));
      p_currentEvent = p_currentEvent->p_nextEvent;
    }
  }

  p_jsonOutput = cJSON_Print(p_cJSONJourney);
  length = strlen(p_jsonOutput) + 5;
  p_jsonBuffer = (char*)malloc(length);

  if (p_jsonBuffer == NULL) {
    return;
  }

  cJSON_PrintPreallocated(p_cJSONJourney, p_jsonBuffer, (int)length, 1);
  // ici
  fprintf(p_file, "%s\n", p_jsonBuffer);
  cJSON_Delete(p_cJSONJourney);
  free(p_jsonBuffer);
  free(p_jsonOutput);
  fclose(p_file);

  return;
}

struct Journey* ImportFromFile(const char* p_filename) {
  struct Journey* p_newJourney = NULL;
  cJSON*          p_cJSONJourney = NULL;
  char*           buffer = NULL;
  const char*     p_err = NULL;
  FILE*           p_file = NULL;
  long            length = 0;
  size_t          read = 0;

  UniFopen(&p_file, p_filename, "r");
  if (p_file == NULL) {
    return NULL;
  }

  // Get the file size
  fseek(p_file, 0, SEEK_END);
  length = ftell(p_file);
  fseek(p_file, 0, SEEK_SET);

  buffer = (char*)malloc(length + 1);
  read = fread(buffer, 1, length, p_file);
  if (read > 0) {
    buffer[length] = L'\0';  // Null-terminate the string
  }

  fclose(p_file);

  p_cJSONJourney = cJSON_Parse(buffer);

  if (p_cJSONJourney == NULL) {
    p_err = cJSON_GetErrorPtr();
    if (p_err != NULL) {
      wprintf(L"Error before: %s\n", p_err);
      WaitUserInput();
    }
    cJSON_Delete(p_cJSONJourney);
    free(buffer);
    return NULL;
  }

  p_newJourney = ParseJourney(p_cJSONJourney);
  cJSON_Delete(p_cJSONJourney);
  free(buffer);

  return p_newJourney;
}

struct Journey* ParseJourney(cJSON* p_cJSONJourney) {
  struct Journey*  p_newJourney = CreateJourney();
  struct Activity* p_newActivity = NULL;
  struct Activity* p_newBreakActivity = NULL;
  cJSON*           p_cJSONCheckInTimestamp;
  cJSON*           p_cJSONLocation;
  cJSON*           p_cJSONActivities;
  cJSON*           p_cJSONActivityName;
  cJSON*           p_cJSONActivityStartedAtTimestamp;
  cJSON*           p_cJSONEvents;
  cJSON*           p_cJSONEventType;
  cJSON*           p_cJSONEventReason;
  cJSON*           p_cJSONEventAtTimestamp;
  cJSON*           p_cJSONActivityIterator = NULL;
  cJSON*           p_cJSONEventIterator = NULL;
  wchar_t*         p_newTitle = NULL;
  wchar_t*         p_newReason = NULL;
  i32              cmpValue = 0;
  u8               eventType = 0;

  // Import journey's part.
  p_cJSONCheckInTimestamp = cJSON_GetObjectItemCaseSensitive(p_cJSONJourney, "check_in_timestamp");
  p_cJSONLocation = cJSON_GetObjectItemCaseSensitive(p_cJSONJourney, "location");

  if ((!cJSON_IsNumber(p_cJSONCheckInTimestamp)) || (!cJSON_IsString(p_cJSONLocation)) || (p_cJSONLocation->valuestring == NULL)) {
    goto _error;
  } else {
    p_newJourney->checkIn = p_cJSONCheckInTimestamp->valuedouble;
    cmpValue = strcmp(p_cJSONLocation->valuestring, "Office");
    if (cmpValue == 0) {
      p_newJourney->location = LOCATION_OFFICE;
    } else {
      p_newJourney->location = LOCATION_HOME;
    }
  }

  // Import activities part.
  p_cJSONActivities = cJSON_GetObjectItem(p_cJSONJourney, "activities");
  cJSON_ArrayForEach(p_cJSONActivityIterator, p_cJSONActivities) {
    p_cJSONActivityName = cJSON_GetObjectItemCaseSensitive(p_cJSONActivityIterator, "name");
    p_cJSONActivityStartedAtTimestamp = cJSON_GetObjectItemCaseSensitive(p_cJSONActivityIterator, "timestamp");

    if ((!cJSON_IsString(p_cJSONActivityName)) || (!cJSON_IsNumber(p_cJSONActivityStartedAtTimestamp))) {
      goto _error;
    } else {
      p_newTitle = malloc(MAX_STRING_BUFFER_SIZE);
      if (p_newTitle != NULL) {
        StrncpyTruncate(p_newTitle, MAX_STRING_SIZE, Utf8ToWchar(p_cJSONActivityName->valuestring));
        p_newActivity = CreateActivity(p_newTitle, false, p_cJSONActivityStartedAtTimestamp->valuedouble, &p_newJourney->p_listActivities);
      } else {
        goto _error;
      }
    }

    // Import events part.
    p_cJSONEvents = cJSON_GetObjectItem(p_cJSONActivityIterator, "events");
    cJSON_ArrayForEach(p_cJSONEventIterator, p_cJSONEvents) {
      p_cJSONEventType = cJSON_GetObjectItemCaseSensitive(p_cJSONEventIterator, "type");
      p_cJSONEventReason = cJSON_GetObjectItemCaseSensitive(p_cJSONEventIterator, "reason");
      p_cJSONEventAtTimestamp = cJSON_GetObjectItemCaseSensitive(p_cJSONEventIterator, "timestamp");

      if ((!cJSON_IsString(p_cJSONEventType)) || (!cJSON_IsString(p_cJSONEventReason)) || (!cJSON_IsNumber(p_cJSONEventAtTimestamp))) {
        goto _error;
      } else {
        p_newReason = SetTitle(Utf8ToWchar(p_cJSONEventReason->valuestring), false);

        cmpValue = strcmp(p_cJSONEventType->valuestring, "STOPPED");
        if (cmpValue == 0) {
          eventType = EVENT_TYPE_STOPPED;
        } else {
          eventType = EVENT_TYPE_RESUME;
        }
        CreateEvent(eventType, p_newReason, p_cJSONEventAtTimestamp->valuedouble, &p_newActivity->p_listEvents);
      }
    }
  }

  // Import BREAK part.
  p_cJSONActivities = cJSON_GetObjectItem(p_cJSONJourney, "breaks");
  p_cJSONActivityStartedAtTimestamp = cJSON_GetObjectItemCaseSensitive(p_cJSONActivities, "timestamp");

  if (!cJSON_IsNumber(p_cJSONActivityStartedAtTimestamp)) {
    goto _error;
  } else {
    p_newTitle = SetTitle(L"Take a break", false);
    p_newBreakActivity = CreateActivity(p_newTitle, false, p_cJSONActivityStartedAtTimestamp->valuedouble, &p_newJourney->p_breakTime);
  }

  // Import break's events part.
  p_cJSONEvents = cJSON_GetObjectItem(p_cJSONActivities, "events");
  cJSON_ArrayForEach(p_cJSONEventIterator, p_cJSONEvents) {
    p_cJSONEventType = cJSON_GetObjectItemCaseSensitive(p_cJSONEventIterator, "type");
    p_cJSONEventReason = cJSON_GetObjectItemCaseSensitive(p_cJSONEventIterator, "reason");
    p_cJSONEventAtTimestamp = cJSON_GetObjectItemCaseSensitive(p_cJSONEventIterator, "timestamp");

    if ((!cJSON_IsString(p_cJSONEventType)) || (!cJSON_IsString(p_cJSONEventReason)) || (!cJSON_IsNumber(p_cJSONEventAtTimestamp))) {
      goto _error;
    } else {
      p_newReason = SetTitle(Utf8ToWchar(p_cJSONEventReason->valuestring), false);
      cmpValue = strcmp(p_cJSONEventType->valuestring, "STOPPED");
      if (cmpValue == 0) {
        eventType = EVENT_TYPE_STOPPED;
      } else {
        eventType = EVENT_TYPE_RESUME;
      }
      CreateEvent(eventType, p_newReason, p_cJSONEventAtTimestamp->valuedouble, &p_newBreakActivity->p_listEvents);
    }
  }

  return p_newJourney;

_error:
  FreeJourney(p_newJourney);
  return NULL;
}

// Functions for U/I stuff.

i8 GetUserChoice(void) {
  wchar_t  buffer[MAX_STRING_SIZE];
  wchar_t* p_endptr;
  u8       inputSuccess;
  u8       userChoice = 0;

  wprintf(L"\n");
  wprintf(L"Enter a value:\n");
  do {
    wprintf(L"> ");
    if (fgetws(buffer, MAX_STRING_SIZE, stdin) == NULL) {
      wprintf(L"Cannot retrieve value from stdin\n");
      return -1;
    }

    errno = 0;
    userChoice = (u8)wcstol(buffer, &p_endptr, 10);
    if (errno == ERANGE) {
      wprintf(L"Sorry, this number is too small or too large.\n");
      inputSuccess = 0;
    } else if (p_endptr == buffer) {  // nothing was read.
      inputSuccess = 0;
    } else if (*p_endptr && *p_endptr != '\n') {
      inputSuccess = 0;
    } else {
      inputSuccess = 1;
    }
    fflush(stdout);
  } while (inputSuccess == 0);
  return userChoice;
}

wchar_t* SetTitle(wchar_t* p_title, bool mandatory) {
  wchar_t* p_newTitle = malloc(MAX_STRING_BUFFER_SIZE);
  wchar_t  buffer[MAX_STRING_SIZE];
  size_t   szBuffer;

  if (p_newTitle == NULL) {
    return NULL;
  }

  if (p_title == NULL) {
    do {
      wprintf(L"Enter a title:\n> ");
      if (fgetws(buffer, MAX_STRING_SIZE, stdin) == NULL) {
        free(p_newTitle);
        return NULL;
      }
#if defined(WIN32) || defined(_WIN32)
      sz_buffer = wcsnlen(str_buffer, MAX_STRING_SIZE - 1);
#endif
#ifdef __linux__
      szBuffer = SafeCsnlen(buffer, MAX_STRING_SIZE - 1);
#endif
    } while ((mandatory) && (szBuffer < 2));

    if (szBuffer < 2) {
      free(p_newTitle);
      return NULL;
    } else {
      RemoveCR(buffer);
      StrncpyTruncate(p_newTitle, MAX_STRING_SIZE, buffer);
      return p_newTitle;
    }
  } else {
    StrncpyTruncate(p_newTitle, MAX_STRING_SIZE, p_title);
    return p_newTitle;
  }
}

wchar_t* AskUserFilename() {
  wchar_t* p_filename = malloc(FILENAME_MAX);
  wchar_t  buffer[MAX_STRING_SIZE];
  size_t   szBuffer;

  do {
    wprintf(L"Enter a filename (with full path):\n> ");
    if (fgetws(buffer, MAX_STRING_SIZE, stdin) == NULL) {
      free(p_filename);
      return NULL;
    }
    szBuffer = SafeCsnlen(buffer, MAX_STRING_SIZE - 1);
  } while (szBuffer < 2);

  if (szBuffer < 2) {
    free(p_filename);
    p_filename = NULL;
    return p_filename;
  } else {
    RemoveCR(buffer);
    StrncpyTruncate(p_filename, MAX_STRING_SIZE, buffer);
    return p_filename;
  }
}

void WaitUserInput(void) {
  wchar_t buffer[MAX_STRING_SIZE];

  wprintf(L"Press any key to continue.\n> ");
  if (fgetws(buffer, MAX_STRING_SIZE, stdin) == NULL) {
    wprintf(L"Cannot retrieve value from stdin\n");
    return;
  }

  return;
}

void PrintJourney(struct Journey* p_journey) {
  wchar_t    buffer[MAX_STRING_SIZE];
  struct tm* p_timeInfo;

  if ((p_journey == NULL) || (p_journey->p_listActivities == NULL)) {
    return;
  }

  wprintf(L"\n---\n");

  wprintf(L"Check-in: ");
  SET_TERMINAL_ATTRIBUTE(BOLD, L"", L"");
  p_timeInfo = localtime(&p_journey->checkIn);
  wcsftime(buffer, sizeof(buffer), STRCTIME, p_timeInfo);
  wprintf(L"%ls\n", buffer);
  RESET_TERMINAL_ATTRIBUTE();

  if (p_journey->checkOut != (time_t)-1) {
    wprintf(L"Check-out: ");
    SET_TERMINAL_ATTRIBUTE(BOLD, L"", L"");
    p_timeInfo = localtime(&p_journey->checkOut);
    wcsftime(buffer, sizeof(buffer), STRCTIME, p_timeInfo);
    wprintf(L"%ls\n", buffer);
    RESET_TERMINAL_ATTRIBUTE();
  }

  wprintf(L"Location: ");
  SET_TERMINAL_ATTRIBUTE(BOLD, L"", L"");
  wprintf(L"%ls\n", (p_journey->location == LOCATION_OFFICE) ? L"Office" : L"Home");
  RESET_TERMINAL_ATTRIBUTE();

  wprintf(L"Total activities duration: ");
  SET_TERMINAL_ATTRIBUTE(BOLD, FG_WHITE, L"");
  ConvertSecondsToTime(p_journey->duration, buffer);
  wprintf(L"%ls\n", buffer);
  RESET_TERMINAL_ATTRIBUTE();

  wprintf(L"Total breaks duration: ");
  SET_TERMINAL_ATTRIBUTE(BOLD, FG_WHITE, L"");
  if (p_journey->p_breakTime != NULL) {
    ConvertSecondsToTime(ComputeActivityDuration(p_journey->p_breakTime->p_listEvents, p_journey->p_breakTime->startedAt), buffer);
    wprintf(L"%ls\n", buffer);
  } else {
    wprintf(L"No breaks, no kitkat :(\n");
  }
  RESET_TERMINAL_ATTRIBUTE();

  wprintf(L"Total idle duration: ");
  SET_TERMINAL_ATTRIBUTE(BOLD, FG_WHITE, L"");
  ConvertSecondsToTime(ComputeIdleTimeDuration(p_journey), buffer);
  wprintf(L"%ls\n", buffer);
  RESET_TERMINAL_ATTRIBUTE();

  wprintf(L"Total Tasks: ");
  SET_TERMINAL_ATTRIBUTE(BOLD, L"", L"");
  wprintf(L"%d\n", GetNbActivities(&p_journey->p_listActivities));
  RESET_TERMINAL_ATTRIBUTE();

  struct Activity* p_currentActivity = p_journey->p_listActivities;
  while (p_currentActivity != NULL) {
    PrintActivity(p_currentActivity, false);
    p_currentActivity = p_currentActivity->p_nextActivity;
  }

  if (p_journey->p_breakTime != NULL) {
    PrintActivity(p_journey->p_breakTime, true);
  }

  WaitUserInput();
  return;
};

void PrintActivity(struct Activity* p_activity, bool isBreakTime) {
  wchar_t       buffer[MAX_STRING_SIZE];
  struct Event* p_currentEvent = p_activity->p_listEvents;
  struct tm*    p_timeInfo;

  wprintf(L"\n");
  wprintf(L"\tTitle: ");
  if (isBreakTime == true) {
    SET_TERMINAL_ATTRIBUTE(BOLD, FG_GREEN, L"");
  } else {
    SET_TERMINAL_ATTRIBUTE(BOLD, FG_WHITE, L"");
  }

  wprintf(L"%ls\n", p_activity->p_title);
  RESET_TERMINAL_ATTRIBUTE();

  wprintf(L"\tStarted at: ");
  SET_TERMINAL_ATTRIBUTE(BOLD, FG_WHITE, L"");
  p_timeInfo = localtime(&p_activity->startedAt);
  wcsftime(buffer, sizeof(buffer), STRCTIME_SHORT, p_timeInfo);
  wprintf(L"%ls\n", buffer);
  RESET_TERMINAL_ATTRIBUTE();

  wprintf(L"\tDuration: ");
  SET_TERMINAL_ATTRIBUTE(BOLD, FG_WHITE, L"");
  ConvertSecondsToTime(ComputeActivityDuration(p_activity->p_listEvents, p_activity->startedAt), buffer);
  wprintf(L"%ls\n", buffer);
  RESET_TERMINAL_ATTRIBUTE();

  if (p_currentEvent == NULL) {
    wprintf(L"\tNo events\n");
  } else {
    wprintf(L"\tEvents:\n");
    while (p_currentEvent) {
      PrintEvent(p_currentEvent);
      p_currentEvent = p_currentEvent->p_nextEvent;
    }
  }

  return;
};

void PrintEvent(struct Event* p_event) {
  wchar_t    buffer[MAX_STRING_SIZE];
  struct tm* p_timeInfo;

  if (p_event == NULL) {
    return;
  }

  if (p_event->type == EVENT_TYPE_RESUME) {
    p_timeInfo = localtime(&p_event->at);
    wcsftime(buffer, sizeof(buffer), STRCTIME_SHORT, p_timeInfo);
    wprintf(L"\t\tType: RESUME, at %ls\n", buffer);
  } else {
    if (p_event->p_reason) {
      p_timeInfo = localtime(&p_event->at);
      wcsftime(buffer, sizeof(buffer), STRCTIME_SHORT, p_timeInfo);
      wprintf(L"\t\tType: STOPPED, at %ls, reason: %ls\n", buffer, p_event->p_reason);
    } else {
      p_timeInfo = localtime(&p_event->at);
      wcsftime(buffer, sizeof(buffer), STRCTIME_SHORT, p_timeInfo);
      wprintf(L"\t\tType: STOPPED, at %ls, no reason\n", buffer);
    }
  }
  return;
};
