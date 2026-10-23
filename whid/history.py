"""Report history: pick days on a calendar, list their tasks.

  ┌ Rapports ────────────────┐       ┌ ‹ Détail ──────── Copier ┐
  │   ‹   octobre 2026   ›   │       │ jeudi 22 octobre 2026    │
  │      L  M  M  J  V  S  D │  ───▶ │   Mail     01:30:00 1,50 │
  │ S43 19 20 21 [22] 23 ... │       │   Réunion  00:45:00 0,75 │
  │ Effacer  [Afficher (1)]  │       │ Total      02:15:00 2,25 │
  └──────────────────────────┘       └──────────────────────────┘

Days without a report are greyed out. A week label selects its days.
"""

import calendar
from datetime import date

import gi

gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")
from gi.repository import Adw, Gdk, Gtk  # noqa: E402

from .model import DAYS, MONTHS, Task, decimal, fmt  # noqa: E402
from .reports import Reports  # noqa: E402

WIDTH = 440
HEIGHT = 580
MARGIN = 12
SPACING = 6
MONTHS_PER_YEAR = 12
DAY_HEADERS = ("L", "M", "M", "J", "V", "S", "D")
PICKER_TAG = "picker"


def _duration(seconds: float) -> str:
    # "01:30:00 · 1,50 h"
    return f"{fmt(seconds)} · {decimal(seconds)} h"


def _day_name(day: date) -> str:
    # "jeudi 22 octobre 2026"
    return f"{DAYS[day.weekday()]} {day.day} {MONTHS[day.month - 1]} {day.year}"


def _week(day: date) -> int:
    return day.isocalendar().week


class History(Adw.Window):
    def __init__(self, app: Adw.Application, reports: Reports) -> None:
        super().__init__(application=app, title="Rapports",
                         default_width=WIDTH, default_height=HEIGHT,
                         hide_on_close=True)
        self.reports = reports
        self.days: set[date] = set()      # Days having a report.
        self.selected: set[date] = set()
        self.month = date.today().replace(day=1)

        self.nav = Adw.NavigationView()
        self.nav.add(self._picker())
        self.set_content(self.nav)

    def show(self) -> None:
        # Rescan on each opening; keep the selection of still valid days.
        self.days = self.reports.days()
        self.selected &= self.days
        self.nav.pop_to_tag(PICKER_TAG)
        self._render()
        self.present()

    # --- Calendar page ------------------------------------------------------

    def _picker(self) -> Adw.NavigationPage:
        prev = Gtk.Button(icon_name="go-previous-symbolic",
                          tooltip_text="Mois précédent")
        prev.connect("clicked", self._move, -1)
        after = Gtk.Button(icon_name="go-next-symbolic",
                           tooltip_text="Mois suivant")
        after.connect("clicked", self._move, 1)

        self.month_label = Gtk.Label(hexpand=True)
        self.month_label.add_css_class("title-4")

        bar = Gtk.Box(spacing=SPACING)
        bar.append(prev)
        bar.append(self.month_label)
        bar.append(after)

        self.grid = Adw.Bin()

        self.clear = Gtk.Button(label="Effacer")
        self.clear.connect("clicked", self._clear)
        self.open = Gtk.Button(hexpand=True)
        self.open.add_css_class("suggested-action")
        self.open.connect("clicked", self._open)

        actions = Gtk.Box(spacing=SPACING)
        actions.append(self.clear)
        actions.append(self.open)

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=MARGIN,
                      margin_top=MARGIN, margin_bottom=MARGIN,
                      margin_start=MARGIN, margin_end=MARGIN)
        box.append(bar)
        box.append(self.grid)
        box.append(actions)

        view = Adw.ToolbarView(content=box)
        view.add_top_bar(Adw.HeaderBar())
        return Adw.NavigationPage(title="Rapports", tag=PICKER_TAG,
                                  child=view)

    def _render(self) -> None:
        # Rebuild the month grid: week column, then Monday to Sunday.
        month = f"{MONTHS[self.month.month - 1]} {self.month.year}"
        self.month_label.set_text(month.capitalize())

        grid = Gtk.Grid(row_spacing=SPACING, column_spacing=SPACING,
                        column_homogeneous=True, halign=Gtk.Align.CENTER)
        for col, name in enumerate(DAY_HEADERS, start=1):
            header = Gtk.Label(label=name)
            header.add_css_class("dim-label")
            grid.attach(header, col, 0, 1, 1)

        weeks = calendar.Calendar().monthdatescalendar(self.month.year,
                                                        self.month.month)
        for row, week in enumerate(weeks, start=1):
            grid.attach(self._week_button(week), 0, row, 1, 1)
            for col, day in enumerate(week, start=1):
                grid.attach(self._day_button(day), col, row, 1, 1)

        self.grid.set_child(grid)
        self._update()

    def _day_button(self, day: date) -> Gtk.Widget:
        # Days of adjacent months stay blank.
        if day.month != self.month.month:
            return Gtk.Label()

        button = Gtk.ToggleButton(label=str(day.day),
                                  active=day in self.selected,
                                  sensitive=day in self.days)
        if day in self.selected:
            button.add_css_class("suggested-action")

        if day not in self.days:
            button.add_css_class("flat")

        if day == date.today():
            button.add_css_class("heading")

        button.connect("toggled", self._pick, day)
        return button

    def _week_button(self, week: list[date]) -> Gtk.Button:
        available = [day for day in week if day in self.days]
        button = Gtk.Button(label=f"S{_week(week[0])}",
                            sensitive=bool(available),
                            tooltip_text="Sélectionner la semaine")
        button.set_css_classes(["flat", "dim-label", "caption"])
        button.connect("clicked", self._pick_week, available)
        return button

    def _update(self) -> None:
        count = len(self.selected)
        plural = "s" if count > 1 else ""
        self.open.set_label(f"Afficher ({count} jour{plural})")
        self.open.set_sensitive(count > 0)
        self.clear.set_sensitive(count > 0)

    def _move(self, _button, step: int) -> None:
        index = self.month.year * MONTHS_PER_YEAR + self.month.month - 1 + step
        self.month = date(index // MONTHS_PER_YEAR,
                          index % MONTHS_PER_YEAR + 1, 1)
        self._render()

    def _pick(self, button: Gtk.ToggleButton, day: date) -> None:
        if button.get_active():
            self.selected.add(day)
            button.add_css_class("suggested-action")
        else:
            self.selected.discard(day)
            button.remove_css_class("suggested-action")

        self._update()

    def _pick_week(self, _button, days: list[date]) -> None:
        # Whole week already selected -> unselect it; otherwise select it.
        week = set(days)
        if week <= self.selected:
            self.selected -= week
        else:
            self.selected |= week

        self._render()

    def _clear(self, _button) -> None:
        self.selected.clear()
        self._render()

    # --- Detail page --------------------------------------------------------

    def _open(self, _button) -> None:
        report = []  # (day, tasks or None when unreadable)
        for day in sorted(self.selected):
            try:
                report.append((day, self.reports.tasks(day)))
            except ValueError:
                report.append((day, None))

        self.nav.push(self._detail(report))

    def _detail(self, report) -> Adw.NavigationPage:
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=MARGIN,
                      margin_top=MARGIN, margin_bottom=MARGIN,
                      margin_start=MARGIN, margin_end=MARGIN)
        for day, tasks in report:
            box.append(self._group(day, tasks))

        # Grand total only makes sense over several days.
        if len(report) > 1:
            total = Gtk.Label(xalign=1,
                              label=f"Total  {_duration(_sum(report))}")
            total.set_css_classes(["title-4", "numeric"])
            box.append(total)

        toasts = Adw.ToastOverlay(child=Gtk.ScrolledWindow(
            child=box, vexpand=True, hscrollbar_policy=Gtk.PolicyType.NEVER))

        copy = Gtk.Button(icon_name="edit-copy-symbolic",
                          tooltip_text="Copier le texte")
        copy.connect("clicked", self._copy, report, toasts)
        header = Adw.HeaderBar()
        header.pack_end(copy)

        view = Adw.ToolbarView(content=toasts)
        view.add_top_bar(header)
        return Adw.NavigationPage(title="Détail", child=view)

    def _group(self, day: date, tasks: list[Task] | None) -> Gtk.Widget:
        group = Adw.PreferencesGroup(title=_day_name(day).capitalize())
        if tasks is None:
            group.set_description("Rapport illisible")
            return group

        seconds = sum(t.elapsed() for t in tasks)
        group.set_description(f"Semaine {_week(day)} · total "
                              f"{_duration(seconds)}")

        for task in tasks:
            row = Adw.ActionRow(title=task.title, subtitle=task.description,
                                use_markup=False)
            time = Gtk.Label(label=_duration(task.elapsed()))
            time.add_css_class("numeric")
            row.add_suffix(time)
            group.add(row)

        if not tasks:
            group.add(Adw.ActionRow(title="Aucune tâche"))

        return group

    def _copy(self, _button, report, toasts: Adw.ToastOverlay) -> None:
        Gdk.Display.get_default().get_clipboard().set(_text(report))
        toasts.add_toast(Adw.Toast(title="Copié"))


def _sum(report) -> float:
    return sum(t.elapsed() for _day, tasks in report for t in tasks or [])


def _text(report) -> str:
    # Plain text for pasting into another tool:
    #   Jeudi 22 octobre 2026 (semaine 43) : 02:15:00 · 2,25 h
    #     - Mail (tri) : 01:30:00 · 1,50 h
    lines = []
    for day, tasks in report:
        head = f"{_day_name(day).capitalize()} (semaine {_week(day)})"
        if tasks is None:
            lines += [f"{head} : rapport illisible", ""]
            continue

        seconds = sum(t.elapsed() for t in tasks)
        lines.append(f"{head} : {_duration(seconds)}")
        for task in tasks:
            name = task.title
            if task.description:
                name += f" ({task.description})"

            lines.append(f"  - {name} : {_duration(task.elapsed())}")

        lines.append("")

    if len(report) > 1:
        lines.append(f"Total : {_duration(_sum(report))}")

    return "\n".join(lines).rstrip() + "\n"
