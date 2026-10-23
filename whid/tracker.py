"""Service layer: enforces a single running task at a time."""

from collections.abc import Callable

from . import storage
from .model import State, Task


class Tracker:
    def __init__(self) -> None:
        self.tasks: list[Task] = []
        self.last: Task | None = None  # Last started task, for resume().
        self._listeners: list[Callable[[], None]] = []

    def listen(self, callback: Callable[[], None]) -> None:
        # Views (window, tray) re-render on any change.
        self._listeners.append(callback)

    def add(self, title: str, description: str) -> Task:
        # A new task becomes the current one.
        task = Task(title, description)
        self.tasks.append(task)
        self.switch(task)
        return task

    def switch(self, task: Task) -> None:
        # Starting a task pauses every other one.
        for other in self.tasks:
            other.pause()

        task.start()
        self.last = task
        self._notify()

    def toggle(self, task: Task) -> None:
        if task.state is State.RUNNING:
            task.pause()
            self._notify()
            return

        self.switch(task)

    def edit(self, task: Task, title: str, description: str,
             seconds: float) -> None:
        task.title = title
        task.description = description
        task.set_time(seconds)
        self._notify()

    def remove(self, task: Task) -> None:
        self.tasks.remove(task)
        if self.last is task:
            self.last = None

        self._notify()

    def current(self) -> Task | None:
        return next((t for t in self.tasks if t.state is State.RUNNING), None)

    def resume(self) -> None:
        # Global play/pause: pause the running task, or resume the last one.
        task = self.current() or self.last
        if task is None:
            return

        self.toggle(task)

    def total(self) -> float:
        return sum(t.elapsed() for t in self.tasks)

    def filename(self) -> str:
        return storage.filename()

    def backup_path(self) -> str:
        return storage.backup_path()

    def export_dir(self) -> str:
        return storage.export_dir()

    def export(self, path: str) -> None:
        storage.save(path, self.tasks)

    def load(self, path: str) -> int:
        # Imported tasks are paused: the user resumes one explicitly.
        self.tasks = storage.load(path)
        self.last = None
        self._notify()
        return len(self.tasks)

    def _notify(self) -> None:
        for callback in self._listeners:
            callback()
