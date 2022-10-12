#include "whid_utils.h"

struct journey* createJourney(void) {
  struct journey* new_journey = (struct journey*)malloc(sizeof(struct journey));

  if (!new_journey) {
    return NULL;
  }

  new_journey->first_activity = NULL;
  new_journey->last_activity = NULL;
  new_journey->duration = 0;
  new_journey->nb_task = 0;
  new_journey->activities = NULL;

  return new_journey;
}

void createActivity(wchar_t* title, u8 type, struct activity** head_activity, struct activity** last_activity) {
  struct activity* new_activity = (struct activity*)malloc(sizeof(struct activity));
  struct activity* next_head_activity = *head_activity;
  if (!new_activity) {
    return;
  }

  new_activity->title = title;
  new_activity->type = type;
  new_activity->started_at = time(NULL);
  new_activity->events = NULL;
  new_activity->next_activity = NULL;

  if (type == EVENT) {
    createEvent(STOPPED, &new_activity->events);
  }

  if (*last_activity != NULL) {
    *last_activity = new_activity;
  }

  if (!*head_activity) {
    *head_activity = new_activity;
    return;
  }

  while (next_head_activity->next_activity != NULL) {
    next_head_activity = next_head_activity->next_activity;
  }
  next_head_activity->next_activity = new_activity;
  return;
}

void createEvent(u8 type, struct event** head_event) {
  struct event* new_event = malloc(sizeof(event));
  struct event* next_head_event = *head_event;

  if (!new_event) {
    return;
  }

  new_event->type = type;
  new_event->at = time(NULL);
  new_event->next_event = NULL;

  if (!*head_event) {
    *head_event = new_event;
    return;
  }

  while (next_head_event->next_event != NULL) {
    next_head_event = next_head_event->next_event;
  }
  next_head_event->next_event = new_event;
  return;
}

wchar_t* convertSecondsToTime(u32 total_seconds, wchar_t* buffer) {
  u32 remaining_seconds = total_seconds;
  u32 hours, minutes, seconds;

  hours = remaining_seconds / 3600;
  remaining_seconds = remaining_seconds - (hours * 3600);
  minutes = remaining_seconds / 60;
  remaining_seconds = remaining_seconds - (minutes * 60);
  seconds = remaining_seconds;

  swprintf_s(buffer, MAX_STRING_SIZE, L"%02d:%02d:%02d", (int)hours, (int)minutes, (int)seconds);
  return buffer;
}

u32 computeActivityDuration(struct event* head_event, time_t beginning) {
  u32           duration = 0;
  time_t        started_at = beginning;
  struct event* current_event = head_event;

  if (!current_event) {
    return duration;
  }

  while (current_event) {
    if (current_event->type == RESUME) {
      started_at = current_event->at;
    } else {
      duration += (u32)difftime(current_event->at, started_at);
    }
    current_event = current_event->next_event;
  }

  return duration;
}

u32 computeJourneyDuration(struct activity* head_activities) {
  u32              duration = 0;
  struct activity* current_activity = head_activities;

  if (!current_activity) {
    return duration;
  }

  while (current_activity) {
    duration += computeActivityDuration(current_activity->events, current_activity->started_at);
    current_activity = current_activity->next_activity;
  }

  return duration;
}

i8 getUserChoice() {
  wchar_t  buffer[MAX_STRING_SIZE];
  wchar_t* endptr;
  u8       input_success;
  u8       user_choice = 0;

  // loop until we have a good input from the user.
  do {
    wprintf(L"Enter a value:\n> ");
    if (!fgetws(buffer, MAX_STRING_SIZE, stdin)) {
      wprintf(L"Cannot retrieve value from stdin\n");
      return -1;
    }

    errno = 0;
    user_choice = (u8)wcstol(buffer, &endptr, 10);
    if (errno == ERANGE) {
      wprintf(L"Sorry, this number is too small or too large.\n");
      input_success = 0;
    } else if (endptr == buffer) {  // nothing was read.
      input_success = 0;
    } else if (*endptr && *endptr != '\n') {
      input_success = 0;
    } else {
      input_success = 1;
    }
  } while (!input_success);
  return user_choice;
}

wchar_t* setTitle(wchar_t* pre_title) {
  wchar_t* new_title = malloc(MAX_WSTRING_BUFFER_SIZE);
  wchar_t  buffer[MAX_STRING_SIZE];

  if (!new_title) {
    return NULL;
  }

  if (!pre_title) {
    wprintf(L"Enter a title:\n> ");
    if (!fgetws(buffer, MAX_STRING_SIZE, stdin)) {
      free(new_title);
      return NULL;
    }

    int cursor = 0;
    for (wchar_t* head_c = buffer; *head_c != L'\0'; head_c++) {
      if (buffer[cursor] == L'\n') {
        buffer[cursor] = L'\0';
      }
      cursor += 1;
    }
    strncpyTruncate(new_title, MAX_STRING_SIZE, buffer);
  } else {
    strncpyTruncate(new_title, MAX_STRING_SIZE, pre_title);
  }

  return new_title;
}

wchar_t* strncpyTruncate(wchar_t* dest, size_t sz_dest, const wchar_t* src) {
  assert(sz_dest > 0);
  // get size of the src (we truncate to force null caracter at the end).
  size_t sz_src = wcsnlen_s(src, sz_dest - 1);
  memmove_s(dest, sz_dest, src, sz_src * sizeof(wchar_t));
  dest[sz_src] = '\0';
  return dest;
}

void printJourney(struct journey* journey) {
  wchar_t* ws_buffer = malloc(MAX_WSTRING_BUFFER_SIZE);
  char*    s_buffer = malloc(MAX_STRING_BUFFER_SIZE);

  if (!ws_buffer || !s_buffer) {
    free(ws_buffer);
    free(s_buffer);
    return;
  }

  if ((!journey) || (!journey->activities)) {
    free(ws_buffer);
    free(s_buffer);
    return;
  }

  if (journey->first_activity) {
    wprintf(L"First activity: %ls\n", journey->first_activity->title);
  }

  if (journey->last_activity) {
    wprintf(L"Last activity: %ls\n", journey->last_activity->title);
  }

  wprintf(L"Total duration: %ls\n", convertSecondsToTime(journey->duration, ws_buffer));

  struct activity* current_activity = journey->activities;
  struct event*    current_event = NULL;
  while (current_activity) {
    wprintf(L"\n");
    wprintf(L"\tTitle: %ls\n", current_activity->title);
    ctime_s(s_buffer, MAX_STRING_SIZE, &current_activity->started_at);
    mbstowcs_s(NULL, ws_buffer, strlen(s_buffer) + 1, s_buffer, strlen(s_buffer));
    wprintf(L"\tStarted at: %ls", ws_buffer);
    wprintf(L"\tTotal time: %ls\n", convertSecondsToTime(computeActivityDuration(current_activity->events, current_activity->started_at), ws_buffer));

    current_event = current_activity->events;
    while (current_event) {
      if (current_event->type == RESUME) {
        wprintf(L"\t\tType: RESUME\n");
      } else {
        wprintf(L"\t\tType: STOPPED\n");
      }
      ctime_s(s_buffer, MAX_STRING_SIZE, &current_event->at);
      wprintf(L"\t\tat %hs", s_buffer);
      current_event = current_event->next_event;
    }
    current_activity = current_activity->next_activity;
  }

  free(ws_buffer);
  free(s_buffer);
  return;
}

void exportToFile(FILE* xml_file, struct journey* journey) {
  wchar_t* ws_buffer = malloc(MAX_WSTRING_BUFFER_SIZE);
  char*    s_buffer = malloc(MAX_STRING_BUFFER_SIZE);

  if (!ws_buffer || !s_buffer) {
    free(ws_buffer);
    free(s_buffer);
    return;
  }

  if (journey == NULL) {
    free(ws_buffer);
    free(s_buffer);
    return;
  }

  fwprintf(xml_file, L"<journey>\n");
  if (journey->activities != NULL) {
    if (journey->first_activity != NULL) {
      ctime_s(s_buffer, MAX_STRING_SIZE, &journey->first_activity->started_at);
      mbstowcs_s(NULL, ws_buffer, strlen(s_buffer) + 1, s_buffer, strlen(s_buffer) - 1);
      fwprintf(xml_file, L"\t<first_activity name=\"%ls\">%ls</first_activity>\n", journey->first_activity->title, ws_buffer);
    }
    if (journey->last_activity != NULL) {
      ctime_s(s_buffer, MAX_STRING_SIZE, &journey->last_activity->started_at);
      mbstowcs_s(NULL, ws_buffer, strlen(s_buffer) + 1, s_buffer, strlen(s_buffer) - 1);
      fwprintf(xml_file, L"\t<last_activity name=\"%ls\">%ls</last_activity>\n", journey->last_activity->title, ws_buffer);
    }
    fwprintf(xml_file, L"\t<total_duration>%ls</total_duration>\n", convertSecondsToTime(journey->duration, ws_buffer));
    fwprintf(xml_file, L"\t<activities>\n");
    struct activity* current_activity = journey->activities;
    struct event*    current_event = NULL;
    while (current_activity) {
      fwprintf(xml_file, L"\t\t<activity>\n");
      fwprintf(xml_file, L"\t\t\t<name>%ls</name>\n", current_activity->title);
      if (current_activity->type == TASK) {
        fwprintf(xml_file, L"\t\t\t<type>TASK</type>\n");
      } else {
        fwprintf(xml_file, L"\t\t\t<type>EVENT</type>\n");
      }
      ctime_s(s_buffer, MAX_STRING_SIZE, &current_activity->started_at);
      mbstowcs_s(NULL, ws_buffer, strlen(s_buffer) + 1, s_buffer, strlen(s_buffer) - 1);
      fwprintf(xml_file, L"\t\t\t<started_at>%ls</started_at>\n", ws_buffer);
      fwprintf(xml_file, L"\t\t\t<total_time>%ls</total_time>\n", convertSecondsToTime(computeActivityDuration(current_activity->events, current_activity->started_at), ws_buffer));
      current_event = current_activity->events;
      fwprintf(xml_file, L"\t\t\t\t<events>\n");
      while (current_event) {
        fwprintf(xml_file, L"\t\t\t\t\t<event>\n");
        if (current_event->type == RESUME) {
          fwprintf(xml_file, L"\t\t\t\t\t\t<type>RESUME</type>\n");
        } else {
          fwprintf(xml_file, L"\t\t\t\t\t\t<type>STOPPED</type>\n");
        }
        ctime_s(s_buffer, MAX_STRING_SIZE, &current_event->at);
        mbstowcs_s(NULL, ws_buffer, strlen(s_buffer) + 1, s_buffer, strlen(s_buffer) - 1);
        fwprintf(xml_file, L"\t\t\t\t\t\t<started_at>%ls</started_at>\n", ws_buffer);
        current_event = current_event->next_event;
        fwprintf(xml_file, L"\t\t\t\t\t</event>\n");
      }
      fwprintf(xml_file, L"\t\t\t\t</events>\n");
      current_activity = current_activity->next_activity;
      fwprintf(xml_file, L"\t\t</activity>\n");
    }
    fwprintf(xml_file, L"\t</activities>\n");
  }
  fwprintf(xml_file, L"</journey>\n");

  free(ws_buffer);
  free(s_buffer);
}

void freeJourney(struct journey* journey) {
  if (!journey) {
    return;
  }

  if (!journey->activities) {
    free(journey);
    return;
  }

  struct activity* head_activity = journey->activities;
  struct activity* next_head_activity = NULL;
  struct event*    head_event = NULL;
  struct event*    next_head_event = NULL;
  while (head_activity) {
    next_head_activity = head_activity->next_activity;

    while (head_event) {
      next_head_event = head_event->next_event;
      free(head_event);
      head_event = next_head_event;
    }
    free(head_activity);
    head_activity = next_head_activity;
  }

  free(journey);
}
