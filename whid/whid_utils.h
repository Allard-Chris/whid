#pragma once
#ifndef _HEADER_WHID_UTILS_H
#define _HEADER_WHID_UTILS_H

#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

// definitions
#define MAX_SIZE 255
#define MAX_STRING_SIZE 255
#define MAX_STRING_BUFFER_SIZE MAX_STRING_SIZE * sizeof(char)
#define MAX_WSTRING_BUFFER_SIZE MAX_STRING_SIZE * sizeof(wchar_t)
#define EVENT 0
#define TASK 1
#define STOPPED 0
#define RESUME 1
typedef char               i8;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

// Structs part
typedef struct event {
  u8            type;
  time_t        at;
  wchar_t*      reason;
  struct event* next_event;
} event;

typedef struct activity {
  wchar_t*         title;
  time_t           started_at;
  event*           events;
  u8               id;
  bool             runnable;
  struct activity* next_activity;
} activity;

typedef struct journey {
  struct activity* first_activity;
  struct activity* last_activity;
  u32              duration;
  u8               nb_task;
  struct activity* activities;
} journey;

// Functions part
struct journey* createJourney(void);
void            createActivity(wchar_t* title, bool runnable, u8 id, struct activity** head_activity, struct activity** last_activity);
void            createEvent(u8 type, wchar_t* reason, struct event** head_event);
wchar_t*        convertSecondsToTime(u32 total_seconds, wchar_t* buffer);
u32             computeActivityDuration(struct event* head_event, time_t beginning);
u32             computeJourneyDuration(struct activity* head_activities);
wchar_t*        ctimeToWstring(wchar_t* ws_buffer, time_t* start);
void            exportToFile(FILE* xml_file, struct journey* journey);
void            freeJourney(struct journey* journey);
i8              getUserChoice(void);
void            printActivity(struct activity* activity);
void            printEvent(struct event* event);
void            printJourney(struct journey* journey);
void            removeActivity(u8 id, struct activity** head_activity);
void            removeCR(wchar_t* ws_buffer);
wchar_t*        setTitle(wchar_t* pre_title, bool mandatory);
wchar_t*        strncpyTruncate(wchar_t* dest, size_t sz_dest, const wchar_t* src);
void            waitUserInput(void);
#endif  // _HEADER_WHID_UTILS_H
