#include "whid_struct.h"

// Functions for structs.

void AddActivityInChainedList(struct Activity** fp_listActivities, struct Activity* p_newActivity) {
  struct Activity* p_currentActivity = NULL;

  if (fp_listActivities == NULL) {
    return;
  }

  if (*fp_listActivities == NULL) {
    *fp_listActivities = p_newActivity;
  } else {
    p_currentActivity = *fp_listActivities;

    while (p_currentActivity->p_nextActivity != NULL) {
      p_currentActivity = p_currentActivity->p_nextActivity;
    }

    p_currentActivity->p_nextActivity = p_newActivity;
  }
}

void AddEventInChainedList(struct Event** fp_listEvents, struct Event* p_newEvent) {
  struct Event* p_currentEvent = NULL;

  if (*fp_listEvents == NULL) {
    *fp_listEvents = p_newEvent;
  } else {
    p_currentEvent = *fp_listEvents;

    while (p_currentEvent->p_nextEvent != NULL) {
      p_currentEvent = p_currentEvent->p_nextEvent;
    }

    p_currentEvent->p_nextEvent = p_newEvent;
  }
}

u32 ComputeActivityDuration(struct Event* p_listEvents, time_t beginning) {
  u32           duration = 0;
  time_t        startedAt = beginning;
  struct Event* p_currentEvent = p_listEvents;

  if (p_currentEvent == NULL) {
    return duration;
  }

  while (p_currentEvent != NULL) {
    if (p_currentEvent->type == EVENT_TYPE_RESUME) {
      startedAt = p_currentEvent->at;
    } else {
      duration += (u32)difftime(p_currentEvent->at, startedAt);
    }
    p_currentEvent = p_currentEvent->p_nextEvent;
  }

  return duration;
}

u32 ComputeJourneyDuration(struct Activity* p_listActivities) {
  u32              duration = 0;
  struct Activity* p_currentActivity = p_listActivities;

  if (p_currentActivity == NULL) {
    return duration;
  }

  while (p_currentActivity != NULL) {
    duration += ComputeActivityDuration(p_currentActivity->p_listEvents, p_currentActivity->startedAt);
    p_currentActivity = p_currentActivity->p_nextActivity;
  }

  return duration;
}

u32 ComputeIdleTimeDuration(struct Journey* journey) {
  u32 breaksDuration = 0;
  u32 idleDuration = 0;
  u32 journeyDuration = ComputeJourneyDuration(journey->p_listActivities);
  if (journey->p_breakTime != NULL) {
    breaksDuration = ComputeActivityDuration(journey->p_breakTime->p_listEvents, journey->p_breakTime->startedAt);
  }

  if (journey->checkOut != (time_t)-1) {
    idleDuration = (u32)difftime(journey->checkOut, journey->checkIn);
  } else {
    idleDuration = (u32)difftime(time(NULL), journey->checkIn);
  }

  return (idleDuration - journeyDuration - breaksDuration);
}

struct Journey* CreateJourney(void) {
  struct Journey* p_newJourney = (struct Journey*)malloc(sizeof(struct Journey));

  if (p_newJourney == NULL) {
    return NULL;
  }

  p_newJourney->location = LOCATION_OFFICE;
  p_newJourney->duration = 0;
  p_newJourney->currentStatus = STATE_STOPPED;
  p_newJourney->checkIn = (time_t)-1;
  p_newJourney->checkOut = (time_t)-1;
  p_newJourney->p_listActivities = NULL;
  p_newJourney->p_lastActivity = NULL;
  p_newJourney->p_breakTime = NULL;

  return p_newJourney;
}

struct Activity* CreateActivity(wchar_t* title, bool isBreak, time_t startedAt, struct Activity** fp_listActivities) {
  struct Activity* p_newActivity = (struct Activity*)malloc(sizeof(struct Activity));

  if (p_newActivity == NULL) {
    return NULL;
  }

  if (isBreak == true) {
    p_newActivity->id = BREAK_TIME_ID;
  } else {
    p_newActivity->id = GetNbActivities(fp_listActivities) + 1;
  }
  p_newActivity->p_title = title;

  if (startedAt == 0) {
    p_newActivity->startedAt = time(NULL);
  } else {
    p_newActivity->startedAt = startedAt;
  }

  p_newActivity->p_listEvents = NULL;
  p_newActivity->p_nextActivity = NULL;

  AddActivityInChainedList(fp_listActivities, p_newActivity);
  return p_newActivity;
}

struct Event* CreateEvent(u8 type, wchar_t* reason, time_t atTime, struct Event** fp_listEvents) {
  struct Event* p_newEvent = (struct Event*)malloc(sizeof(struct Event));

  if (p_newEvent == NULL) {
    return NULL;
  }

  p_newEvent->type = type;

  if (atTime == 0) {
    p_newEvent->at = time(NULL);
  } else {
    p_newEvent->at = atTime;
  }

  p_newEvent->p_reason = reason;
  p_newEvent->p_nextEvent = NULL;
  AddEventInChainedList(fp_listEvents, p_newEvent);

  return p_newEvent;
}

struct Activity* FindActivity(u8 id, struct Activity** fp_listActivities) {
  struct Activity* p_currentActivity = *fp_listActivities;

  while (p_currentActivity != NULL) {
    if (p_currentActivity->id == id) {
      break;
    }
    p_currentActivity = p_currentActivity->p_nextActivity;
  }
  return p_currentActivity;
}

void FreeJourney(struct Journey* p_journey) {
  struct Activity* p_currentActivity = NULL;
  struct Activity* p_nextActivity = NULL;
  struct Event*    p_currentEvent = NULL;
  struct Event*    p_nextEvent = NULL;

  if (p_journey == NULL)
    return;

  p_currentActivity = p_journey->p_listActivities;

  while (p_currentActivity != NULL) {
    p_nextActivity = p_currentActivity->p_nextActivity;
    p_currentEvent = p_currentActivity->p_listEvents;

    while (p_currentEvent != NULL) {
      p_nextEvent = p_currentEvent->p_nextEvent;
      if (p_currentEvent->p_reason != NULL) {
        free(p_currentEvent->p_reason);
      }

      free(p_currentEvent);
      p_currentEvent = p_nextEvent;
    }

    if (p_currentActivity->p_title != NULL) {
      free(p_currentActivity->p_title);
    }

    free(p_currentActivity);
    p_currentActivity = p_nextActivity;
  }

  if (p_journey->p_breakTime != NULL) {
    p_currentEvent = p_journey->p_breakTime->p_listEvents;
    while (p_currentEvent != NULL) {
      p_nextEvent = p_currentEvent->p_nextEvent;
      if (p_currentEvent->p_reason != NULL) {
        free(p_currentEvent->p_reason);
      }

      free(p_currentEvent);
      p_currentEvent = p_nextEvent;
    }
    free(p_journey->p_breakTime);
  }

  free(p_journey);
}

u8 GetNbActivities(struct Activity** fp_listActivities) {
  u8               nbActivities = 0;
  struct Activity* p_currentActivity = *fp_listActivities;

  while (p_currentActivity != NULL) {
    nbActivities += 1;
    p_currentActivity = p_currentActivity->p_nextActivity;
  }

  return nbActivities;
}

bool IsActivityExists(wchar_t* title, struct Activity** fp_listActivities) {
  struct Activity* p_currentActivity = *fp_listActivities;
  bool             result = false;
  i32              cmpValue = 0;

  while (p_currentActivity != NULL) {
    cmpValue = wcscmp(title, p_currentActivity->p_title);
    if (cmpValue == 0) {
      result = true;
      break;
    }
    p_currentActivity = p_currentActivity->p_nextActivity;
  }

  return result;
}

void RemoveActivity(u8 id, struct Activity** fp_listActivity) {
  struct Activity* p_currentActivity = *fp_listActivity;
  struct Activity* p_previousActivity = NULL;
  struct Activity* p_nextActivity = NULL;
  struct Event*    p_currentEvent = NULL;
  struct Event*    p_nextEvent = NULL;
  u8               newId = 0;

  while (p_currentActivity != NULL) {
    p_nextActivity = p_currentActivity->p_nextActivity;

    if (p_currentActivity->id == id) {
      if (p_previousActivity == NULL) {
        *fp_listActivity = p_nextActivity;
      } else {
        p_previousActivity->p_nextActivity = p_nextActivity;
      }
      p_nextEvent = p_currentActivity->p_listEvents;

      while (p_nextEvent != NULL) {
        p_currentEvent = p_nextEvent;
        if (p_currentEvent->p_reason) {
          free(p_currentEvent->p_reason);
        }
        p_nextEvent = p_currentEvent->p_nextEvent;
        free(p_currentEvent);
      }
      free(p_currentActivity->p_title);
      free(p_currentActivity);
      break;
    }
    p_previousActivity = p_currentActivity;
    p_currentActivity = p_currentActivity->p_nextActivity;
  }

  // Reasign ID for all activities.
  p_currentActivity = *fp_listActivity;
  while (p_currentActivity != NULL) {
    if (p_currentActivity->id != BREAK_TIME_ID) {
      p_currentActivity->id = newId;
      newId += 1;
    }
    p_currentActivity = p_currentActivity->p_nextActivity;
  }
}