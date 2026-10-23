"""GTK window: talks only to the Tracker service."""

import re

import gi

gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")
from gi.repository import Adw, Gdk, Gio, GLib, Gtk  # noqa: E402

from .model import State, fmt, parse  # noqa: E402
from .tracker import Tracker  # noqa: E402

TICK_SECONDS = 1
MARGIN = 12
SPACING = 6
WIDTH = 560
HEIGHT = 640
ICON_PLAY = "media-playback-start-symbolic"
ICON_PAUSE = "media-playback-pause-symbolic"
JSON_MIME = "application/json"
QUICK_WIDTH = 640
DURATION_RE = re.compile(r"^\d+:[0-5]\d:[0-5]\d$")  # "01:30:00"
RESPONSE_CANCEL = "cancel"
RESPONSE_SAVE = "save"
RESPONSE_DELETE = "delete"


class TaskForm(Gtk.Box):
    """Title + description entries; Enter adds and starts the task."""

    def __init__(self, tracker: Tracker, on_added=None) -> None:
        super().__init__(spacing=SPACING)
        self.tracker = tracker
        self.on_added = on_added

        self.title_entry = Gtk.Entry(placeholder_text="Tâche", hexpand=True)
        self.desc_entry = Gtk.Entry(placeholder_text="Description",
                                    hexpand=True)
        add = Gtk.Button(icon_name="list-add-symbolic",
                         tooltip_text="Ajouter et démarrer")
        add.add_css_class("suggested-action")

        # Enter in either field adds the task.
        self.title_entry.connect("activate", self._add)
        self.desc_entry.connect("activate", self._add)
        add.connect("clicked", self._add)

        self.append(self.title_entry)
        self.append(self.desc_entry)
        self.append(add)

    def focus(self) -> None:
        self.title_entry.grab_focus()

    def _add(self, _widget) -> None:
        title = self.title_entry.get_text().strip()
        if not title:
            self.focus()
            return

        self.tracker.add(title, self.desc_entry.get_text().strip())
        self.title_entry.set_text("")
        self.desc_entry.set_text("")
        self.focus()

        if self.on_added:
            self.on_added()


class QuickAdd(Adw.Window):
    """Small popup opened from the tray. Escape closes it."""

    def __init__(self, app: Adw.Application, tracker: Tracker) -> None:
        # Explicit size request drops libadwaita's default minimum size,
        # so the window shrinks to one line of entries.
        super().__init__(application=app, title="Nouvelle tâche",
                         width_request=QUICK_WIDTH, height_request=0,
                         resizable=False, hide_on_close=True)
        self.form = TaskForm(tracker, on_added=self.close)
        self.form.set_valign(Gtk.Align.CENTER)

        box = Gtk.Box(margin_top=MARGIN, margin_bottom=MARGIN,
                      margin_start=MARGIN, margin_end=MARGIN)
        box.append(self.form)

        view = Adw.ToolbarView(content=box)
        view.add_top_bar(Adw.HeaderBar())
        self.set_content(view)

        keys = Gtk.EventControllerKey()
        keys.connect("key-pressed", self._key)
        self.add_controller(keys)

    def show(self) -> None:
        self.present()
        self.form.focus()

    def _key(self, _ctrl, keyval, _code, _state) -> bool:
        if keyval != Gdk.KEY_Escape:
            return False

        self.close()
        return True


class Window(Adw.ApplicationWindow):
    def __init__(self, app: Adw.Application, tracker: Tracker,
                 on_history) -> None:
        # Closing only hides: the app lives on in the tray.
        super().__init__(application=app, title="Whid",
                         default_width=WIDTH, default_height=HEIGHT,
                         hide_on_close=True)
        self.tracker = tracker
        self.on_history = on_history
        self.rows = []  # (task, time label, toggle button)
        tracker.listen(self._rebuild)

        self.toasts = Adw.ToastOverlay(child=self._body())
        view = Adw.ToolbarView(content=self.toasts)
        view.add_top_bar(self._header())
        self.set_content(view)

        GLib.timeout_add_seconds(TICK_SECONDS, self._tick)
        self._refresh()

    # --- Layout -------------------------------------------------------------

    def _header(self) -> Adw.HeaderBar:
        load = Gtk.Button(icon_name="document-open-symbolic",
                          tooltip_text="Importer")
        load.connect("clicked", self._ask_load)

        save = Gtk.Button(icon_name="document-save-symbolic",
                          tooltip_text="Exporter")
        save.connect("clicked", self._ask_save)

        history = Gtk.Button(icon_name="x-office-calendar-symbolic",
                             tooltip_text="Rapports")
        history.connect("clicked", lambda _button: self.on_history())

        bar = Adw.HeaderBar()
        bar.pack_start(load)
        bar.pack_start(history)
        bar.pack_end(save)
        return bar

    def _body(self) -> Gtk.Box:
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=MARGIN,
                      margin_top=MARGIN, margin_bottom=MARGIN,
                      margin_start=MARGIN, margin_end=MARGIN)
        box.append(TaskForm(self.tracker))

        self.list = Gtk.ListBox(selection_mode=Gtk.SelectionMode.NONE,
                                valign=Gtk.Align.START)
        self.list.add_css_class("boxed-list")
        self.list.set_placeholder(Gtk.Label(label="Aucune tâche",
                                            margin_top=MARGIN,
                                            margin_bottom=MARGIN))
        box.append(Gtk.ScrolledWindow(child=self.list, vexpand=True,
                                      hscrollbar_policy=Gtk.PolicyType.NEVER))

        self.total = Gtk.Label(xalign=1)
        self.total.set_css_classes(["title-3", "numeric"])
        box.append(self.total)
        return box

    def _row(self, task) -> Adw.ActionRow:
        label = Gtk.Label()
        button = Gtk.Button(valign=Gtk.Align.CENTER)
        button.connect("clicked", self._toggle, task)

        edit = Gtk.Button(icon_name="document-edit-symbolic",
                          valign=Gtk.Align.CENTER, tooltip_text="Modifier")
        edit.add_css_class("flat")
        edit.connect("clicked", self._ask_edit, task)

        delete = Gtk.Button(icon_name="user-trash-symbolic",
                            valign=Gtk.Align.CENTER, tooltip_text="Supprimer")
        delete.add_css_class("flat")
        delete.connect("clicked", self._ask_delete, task)

        row = Adw.ActionRow(title=task.title, subtitle=task.description,
                            use_markup=False)
        row.add_suffix(label)
        row.add_suffix(button)
        row.add_suffix(edit)
        row.add_suffix(delete)
        row.set_activatable_widget(button)

        self.rows.append((task, label, button))
        return row

    # --- State sync ---------------------------------------------------------

    def _rebuild(self) -> None:
        self.list.remove_all()
        self.rows = []
        for task in self.tracker.tasks:
            self.list.append(self._row(task))

        self._refresh()

    def _refresh(self) -> None:
        # Update times and icons; the running task is highlighted.
        for task, label, button in self.rows:
            running = task.state is State.RUNNING
            label.set_text(fmt(task.elapsed()))
            label.set_css_classes(["numeric", "accent"] if running
                                  else ["numeric", "dim-label"])
            button.set_icon_name(ICON_PAUSE if running else ICON_PLAY)
            button.set_css_classes(["circular", "suggested-action"] if running
                                   else ["circular", "flat"])

        self.total.set_text(f"Total  {fmt(self.tracker.total())}")

    def _tick(self) -> bool:
        self._refresh()
        return GLib.SOURCE_CONTINUE

    def _toast(self, text: str) -> None:
        self.toasts.add_toast(Adw.Toast(title=text))

    # --- Handlers -----------------------------------------------------------

    def _toggle(self, _button, task) -> None:
        self.tracker.toggle(task)

    def _ask_edit(self, _button, task) -> None:
        # Fix title, description or time (e.g. task started too late).
        rows = (Adw.EntryRow(title="Tâche", text=task.title),
                Adw.EntryRow(title="Description", text=task.description),
                Adw.EntryRow(title="Temps passé (HH:MM:SS)",
                             text=fmt(task.elapsed())))
        group = Adw.PreferencesGroup()
        for row in rows:
            row.set_activates_default(True)
            group.add(row)

        dialog = Adw.AlertDialog(heading="Modifier la tâche",
                                 extra_child=group)
        dialog.add_response(RESPONSE_CANCEL, "Annuler")
        dialog.add_response(RESPONSE_SAVE, "Enregistrer")
        dialog.set_response_appearance(RESPONSE_SAVE,
                                       Adw.ResponseAppearance.SUGGESTED)
        dialog.set_default_response(RESPONSE_SAVE)
        dialog.set_close_response(RESPONSE_CANCEL)

        for row in rows:
            row.connect("changed", self._validate, dialog, rows)

        dialog.connect("response", self._on_edit, task, rows)
        dialog.present(self)

    def _validate(self, _row, dialog, rows) -> None:
        # Save stays disabled while the title is empty or the time malformed.
        title, _desc, duration = rows
        time_ok = DURATION_RE.match(duration.get_text().strip()) is not None
        duration.set_css_classes([] if time_ok else ["error"])

        valid = time_ok and bool(title.get_text().strip())
        dialog.set_response_enabled(RESPONSE_SAVE, valid)

    def _on_edit(self, _dialog, response, task, rows) -> None:
        if response != RESPONSE_SAVE:
            return

        title, desc, duration = (row.get_text().strip() for row in rows)
        self.tracker.edit(task, title, desc, parse(duration))

    def _ask_delete(self, _button, task) -> None:
        dialog = Adw.AlertDialog(
            heading="Supprimer la tâche ?",
            body=f"« {task.title} » et son temps ({fmt(task.elapsed())}) "
                 "seront perdus.")
        dialog.add_response(RESPONSE_CANCEL, "Annuler")
        dialog.add_response(RESPONSE_DELETE, "Supprimer")
        dialog.set_response_appearance(RESPONSE_DELETE,
                                       Adw.ResponseAppearance.DESTRUCTIVE)
        dialog.set_default_response(RESPONSE_CANCEL)
        dialog.set_close_response(RESPONSE_CANCEL)

        dialog.connect("response", self._on_delete, task)
        dialog.present(self)

    def _on_delete(self, _dialog, response, task) -> None:
        if response != RESPONSE_DELETE:
            return

        self.tracker.remove(task)
        self._toast("Tâche supprimée")

    def _dialog(self) -> Gtk.FileDialog:
        json_filter = Gtk.FileFilter(name="JSON")
        json_filter.add_mime_type(JSON_MIME)
        filters = Gio.ListStore.new(Gtk.FileFilter)
        filters.append(json_filter)
        # Import and export both open in the export folder.
        folder = Gio.File.new_for_path(self.tracker.export_dir())
        return Gtk.FileDialog(filters=filters, default_filter=json_filter,
                              initial_folder=folder)

    def _ask_save(self, _button) -> None:
        dialog = self._dialog()
        dialog.set_initial_name(self.tracker.filename())
        dialog.save(self, None, self._on_save)

    def _on_save(self, dialog, result) -> None:
        try:
            file = dialog.save_finish(result)
        except GLib.Error:
            return  # Cancelled.

        try:
            self.tracker.export(file.get_path())
        except OSError:
            self._toast("Échec de l'export")
            return

        self._toast("Exporté")

    def _ask_load(self, _button) -> None:
        self._dialog().open(self, None, self._on_load)

    def _on_load(self, dialog, result) -> None:
        try:
            file = dialog.open_finish(result)
        except GLib.Error:
            return  # Cancelled.

        try:
            count = self.tracker.load(file.get_path())
        except ValueError:
            self._toast("Fichier invalide")
            return

        self._toast(f"{count} tâche(s) importée(s)")
