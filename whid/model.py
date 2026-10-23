"""Domain model: a task accumulates time while running."""

import time
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum

SECONDS_PER_HOUR = 3600
SECONDS_PER_MINUTE = 60


class State(Enum):
    RUNNING = "running"
    PAUSED = "paused"


class Kind(Enum):
    PAUSE = "pause"
    RESUME = "reprise"


@dataclass
class Event:
    kind: Kind
    at: datetime


@dataclass(eq=False)
class Task:
    title: str
    description: str = ""
    seconds: float = 0.0  # Accumulated before the current run.
    state: State = State.PAUSED
    started: float = 0.0  # Monotonic timestamp of the current run.
    created: datetime | None = None  # Set on first start.
    events: list[Event] = field(default_factory=list)

    def elapsed(self) -> float:
        if self.state is State.PAUSED:
            return self.seconds

        return self.seconds + time.monotonic() - self.started

    def start(self) -> None:
        if self.state is State.RUNNING:
            return

        self.started = time.monotonic()
        self.state = State.RUNNING

        # First start is the creation; later ones are resumes.
        if self.created is None:
            self.created = datetime.now()
            return

        self.events.append(Event(Kind.RESUME, datetime.now()))

    def set_time(self, seconds: float) -> None:
        # Manual correction; a running task goes on from the new value.
        self.seconds = seconds
        self.started = time.monotonic()

    def pause(self) -> None:
        if self.state is State.PAUSED:
            return

        self.seconds = self.elapsed()
        self.state = State.PAUSED
        self.events.append(Event(Kind.PAUSE, datetime.now()))


def fmt(seconds: float) -> str:
    # 3725 -> "01:02:05"
    s = round(seconds)
    hours = s // SECONDS_PER_HOUR
    minutes = s % SECONDS_PER_HOUR // SECONDS_PER_MINUTE
    return f"{hours:02d}:{minutes:02d}:{s % SECONDS_PER_MINUTE:02d}"


def parse(text: str) -> int:
    # "01:02:05" -> 3725
    hours, minutes, seconds = (int(part) for part in text.split(":"))
    return hours * SECONDS_PER_HOUR + minutes * SECONDS_PER_MINUTE + seconds
