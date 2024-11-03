#pragma once
#ifndef _HEADER_WHID_STRUCT_H
#define _HEADER_WHID_STRUCT_H

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>

// Default variables type.
typedef signed char   i8;
typedef unsigned char u8;
typedef unsigned int  u32;
typedef signed int    i32;

// Constants definition.
#define LOCATION_OFFICE 1
#define LOCATION_HOME 2
#define EVENT_TYPE_STOPPED 0
#define EVENT_TYPE_RESUME 1
#define STATE_STOPPED 0
#define STATE_RUNNING 1
#define STATE_BREAK 2
// A day can have only 253 activities (u8 for activity ID).
// A keep this three ID for special activities.
#define NULL_ID 255
#define BREAK_TIME_ID 254

// Constants definition.
#define MAX_SIZE 255
#define MAX_STRING_SIZE 255
#define MAX_STRING_BUFFER_SIZE MAX_STRING_SIZE * sizeof(wchar_t)

// Structs.
typedef struct Event {
  u8            type;
  time_t        at;
  wchar_t*      p_reason;
  struct Event* p_nextEvent;
} T_EVENT;

typedef struct Activity {
  u8               id;
  wchar_t*         p_title;
  time_t           startedAt;
  struct Event*    p_listEvents;
  struct Activity* p_nextActivity;
} T_ACTIVITY;

typedef struct Journey {
  u8               location;
  u32              duration;
  u8               currentStatus;
  time_t           checkIn;
  time_t           checkOut;
  struct Activity* p_listActivities;
  struct Activity* p_lastActivity;
  struct Activity* p_breakTime;
} T_JOURNEY;

// Functions for structs.
void             AddActivityInChainedList(struct Activity** fp_listActivities, struct Activity* p_newActivity);
void             AddEventInChainedList(struct Event** fp_listEvents, struct Event* p_newEvent);
u32              ComputeActivityDuration(struct Event* p_listEvents, time_t beginning);
u32              ComputeJourneyDuration(struct Activity* p_listActivities);
u32              ComputeIdleTimeDuration(struct Journey* p_journey);
struct Journey*  CreateJourney(void);
struct Activity* CreateActivity(wchar_t* p_title, bool isBreak, time_t startedAt, struct Activity** fp_listActivities);
struct Event*    CreateEvent(u8 type, wchar_t* p_reason, time_t atTime, struct Event** fp_listEvents);
struct Activity* FindActivity(u8 id, struct Activity** fp_listActivities);
void             FreeJourney(struct Journey* p_journey);
u8               GetNbActivities(struct Activity** fp_listActivities);
bool             IsActivityExists(wchar_t* p_title, struct Activity** fp_listActivities);
void             RemoveActivity(u8 id, struct Activity** fp_listActivities);

#endif  // _HEADER_WHID_STRUCT_H