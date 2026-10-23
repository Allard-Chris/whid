"""System tray icon over D-Bus (StatusNotifierItem + DBusMenu).

GTK4 has no tray API. GNOME's AppIndicator extension reads these two
interfaces directly, so we export them ourselves:

  Whid ──RegisterStatusNotifierItem──▶ StatusNotifierWatcher (shell)
   ├─ /StatusNotifierItem : icon, running time label
   └─ /MenuBar            : menu, clicks come back as Event()
"""

import os
from collections.abc import Callable
from enum import IntEnum

from gi.repository import Gio, GLib

from .model import State, fmt
from .mono import Mono
from .tracker import Tracker

WATCHER_NAME = "org.kde.StatusNotifierWatcher"
WATCHER_PATH = "/StatusNotifierWatcher"
ITEM_IFACE = "org.kde.StatusNotifierItem"
ITEM_PATH = "/StatusNotifierItem"
MENU_IFACE = "com.canonical.dbusmenu"
MENU_PATH = "/MenuBar"
DBUSMENU_VERSION = 3
TICK_MS = 400  # Marquee step; the clock is refreshed on the same tick.
TITLE_WIDTH = 12
MARQUEE_GAP = " · "
LABEL_GUIDE = "M" * TITLE_WIDTH + "  00:00:00"  # Widest expected label.
TASK_ID_BASE = 100  # Menu id of task #i is TASK_ID_BASE + i.

XML = """
<node>
  <interface name="org.kde.StatusNotifierItem">
    <property name="Category" type="s" access="read"/>
    <property name="Id" type="s" access="read"/>
    <property name="Title" type="s" access="read"/>
    <property name="Status" type="s" access="read"/>
    <property name="WindowId" type="i" access="read"/>
    <property name="IconName" type="s" access="read"/>
    <property name="ToolTip" type="(sa(iiay)ss)" access="read"/>
    <property name="ItemIsMenu" type="b" access="read"/>
    <property name="Menu" type="o" access="read"/>
    <property name="XAyatanaLabel" type="s" access="read"/>
    <property name="XAyatanaLabelGuide" type="s" access="read"/>
    <method name="Activate"><arg type="i"/><arg type="i"/></method>
    <method name="SecondaryActivate"><arg type="i"/><arg type="i"/></method>
    <method name="ContextMenu"><arg type="i"/><arg type="i"/></method>
    <method name="Scroll"><arg type="i"/><arg type="s"/></method>
    <signal name="NewToolTip"/>
    <signal name="XAyatanaNewLabel"><arg type="s"/><arg type="s"/></signal>
  </interface>
  <interface name="com.canonical.dbusmenu">
    <property name="Version" type="u" access="read"/>
    <property name="TextDirection" type="s" access="read"/>
    <property name="Status" type="s" access="read"/>
    <property name="IconThemePath" type="as" access="read"/>
    <method name="GetLayout">
      <arg type="i" direction="in"/><arg type="i" direction="in"/>
      <arg type="as" direction="in"/>
      <arg type="u" direction="out"/><arg type="(ia{sv}av)" direction="out"/>
    </method>
    <method name="GetGroupProperties">
      <arg type="ai" direction="in"/><arg type="as" direction="in"/>
      <arg type="a(ia{sv})" direction="out"/>
    </method>
    <method name="GetProperty">
      <arg type="i" direction="in"/><arg type="s" direction="in"/>
      <arg type="v" direction="out"/>
    </method>
    <method name="Event">
      <arg type="i" direction="in"/><arg type="s" direction="in"/>
      <arg type="v" direction="in"/><arg type="u" direction="in"/>
    </method>
    <method name="EventGroup">
      <arg type="a(isvu)" direction="in"/><arg type="ai" direction="out"/>
    </method>
    <method name="AboutToShow">
      <arg type="i" direction="in"/><arg type="b" direction="out"/>
    </method>
    <method name="AboutToShowGroup">
      <arg type="ai" direction="in"/>
      <arg type="ai" direction="out"/><arg type="ai" direction="out"/>
    </method>
    <signal name="LayoutUpdated"><arg type="u"/><arg type="i"/></signal>
  </interface>
</node>
"""


class Item(IntEnum):
    ROOT = 0
    ADD = 1
    PLAY = 2
    OPEN = 3
    QUIT = 4
    SEP_TOP = 5
    SEP_BOTTOM = 6


def _text(label: str) -> dict:
    # DBusMenu reads "_" as a mnemonic: double it to keep it literal.
    return {"label": GLib.Variant("s", label.replace("_", "__"))}


SEPARATOR = {"type": GLib.Variant("s", "separator")}


class Tray:
    def __init__(self, tracker: Tracker, icon: str,
                 on_add: Callable[[], None],
                 on_open: Callable[[], None],
                 on_quit: Callable[[], None]) -> None:
        self.tracker = tracker
        self.icon = icon
        self.on_open = on_open
        self.actions = {Item.ADD: on_add, Item.PLAY: tracker.resume,
                        Item.OPEN: on_open, Item.QUIT: on_quit}
        self.revision = 0
        self.label = ""
        self.offset = 0  # Marquee position in the running task title.
        self.mono = Mono()
        self.name = f"org.kde.StatusNotifierItem-{os.getpid()}-1"

        self.bus = Gio.bus_get_sync(Gio.BusType.SESSION)
        node = Gio.DBusNodeInfo.new_for_xml(XML)
        self.bus.register_object(ITEM_PATH, node.lookup_interface(ITEM_IFACE),
                                 self._item_call, self._item_prop, None)
        self.bus.register_object(MENU_PATH, node.lookup_interface(MENU_IFACE),
                                 self._menu_call, self._menu_prop, None)

        Gio.bus_own_name_on_connection(self.bus, self.name,
                                       Gio.BusNameOwnerFlags.NONE,
                                       self._owned, None)

        tracker.listen(self._changed)
        GLib.timeout_add(TICK_MS, self._tick)

    # --- Registration -------------------------------------------------------

    def _owned(self, _bus, _name) -> None:
        # Register now and again whenever the shell (watcher) restarts.
        Gio.bus_watch_name_on_connection(self.bus, WATCHER_NAME,
                                         Gio.BusNameWatcherFlags.NONE,
                                         self._register, None)

    def _register(self, _bus, _name, _owner) -> None:
        self.bus.call(WATCHER_NAME, WATCHER_PATH, WATCHER_NAME,
                      "RegisterStatusNotifierItem",
                      GLib.Variant("(s)", (self.name,)), None,
                      Gio.DBusCallFlags.NONE, -1, None, None)

    # --- StatusNotifierItem -------------------------------------------------

    def _item_prop(self, _bus, _sender, _path, _iface, prop):
        props = {
            "Category": GLib.Variant("s", "ApplicationStatus"),
            "Id": GLib.Variant("s", "whid"),
            "Title": GLib.Variant("s", "Whid"),
            "Status": GLib.Variant("s", "Active"),
            "WindowId": GLib.Variant("i", 0),
            "IconName": GLib.Variant("s", self.icon),
            "ToolTip": GLib.Variant("(sa(iiay)ss)",
                                    (self.icon, [], "Whid", self._status())),
            "ItemIsMenu": GLib.Variant("b", True),
            "Menu": GLib.Variant("o", MENU_PATH),
            "XAyatanaLabel": GLib.Variant("s", self.label),
            "XAyatanaLabelGuide": GLib.Variant("s", LABEL_GUIDE),
        }
        return props.get(prop)

    def _item_call(self, _bus, _sender, _path, _iface, method, _args,
                   invocation) -> None:
        if method == "Activate":
            GLib.idle_add(self.on_open)

        invocation.return_value(None)

    def _status(self) -> str:
        task = self.tracker.current()
        if task is None:
            return "En pause"

        return task.title

    # --- DBusMenu -----------------------------------------------------------

    def _menu_prop(self, _bus, _sender, _path, _iface, prop):
        props = {
            "Version": GLib.Variant("u", DBUSMENU_VERSION),
            "TextDirection": GLib.Variant("s", "ltr"),
            "Status": GLib.Variant("s", "normal"),
            "IconThemePath": GLib.Variant("as", []),
        }
        return props.get(prop)

    def _items(self) -> dict[int, dict]:
        # Flat menu: actions, then one radio entry per task (with time).
        current = self.tracker.current()
        play = _text("Pause" if current else "Reprendre")
        play["enabled"] = GLib.Variant("b", self.tracker.last is not None)

        items = {
            Item.ADD: _text("Nouvelle tâche…"),
            Item.PLAY: play,
            Item.SEP_TOP: SEPARATOR,
        }

        for i, task in enumerate(self.tracker.tasks):
            entry = _text(f"{task.title}   {fmt(task.elapsed())}")
            entry["toggle-type"] = GLib.Variant("s", "radio")
            running = task.state is State.RUNNING
            entry["toggle-state"] = GLib.Variant("i", int(running))
            items[TASK_ID_BASE + i] = entry

        items[Item.SEP_BOTTOM] = SEPARATOR
        items[Item.OPEN] = _text("Ouvrir Whid")
        items[Item.QUIT] = _text("Quitter")
        return items

    def _layout(self):
        children = [GLib.Variant("(ia{sv}av)", (int(i), props, []))
                    for i, props in self._items().items()]
        root = {"children-display": GLib.Variant("s", "submenu")}
        return (int(Item.ROOT), root, children)

    def _menu_call(self, _bus, _sender, _path, _iface, method, args,
                   invocation) -> None:
        if method == "GetLayout":
            reply = GLib.Variant("(u(ia{sv}av))",
                                 (self.revision, self._layout()))
            invocation.return_value(reply)
            return

        if method == "GetGroupProperties":
            ids, _names = args.unpack()
            items = self._items()
            found = [(i, items[i]) for i in ids if i in items]
            invocation.return_value(GLib.Variant("(a(ia{sv}))", (found,)))
            return

        if method == "GetProperty":
            item, name = args.unpack()
            value = self._items().get(item, {}).get(name)
            if value is None:
                invocation.return_dbus_error(f"{MENU_IFACE}.Error",
                                             "unknown property")
                return

            invocation.return_value(GLib.Variant("(v)", (value,)))
            return

        if method == "Event":
            item, event, _data, _time = args.unpack()
            self._event(item, event)
            invocation.return_value(None)
            return

        if method == "EventGroup":
            for item, event, _data, _time in args.unpack()[0]:
                self._event(item, event)

            invocation.return_value(GLib.Variant("(ai)", ([],)))
            return

        if method == "AboutToShow":
            # Times in labels go stale: ask the shell to fetch them again.
            self._relayout()
            invocation.return_value(GLib.Variant("(b)", (True,)))
            return

        if method == "AboutToShowGroup":
            invocation.return_value(GLib.Variant("(aiai)", ([], [])))
            return

        invocation.return_dbus_error(f"{MENU_IFACE}.Error", "unknown method")

    def _event(self, item: int, event: str) -> None:
        if event != "clicked":
            return

        # Run after the D-Bus reply: actions may open windows.
        GLib.idle_add(self._click, item)

    def _click(self, item: int) -> bool:
        index = item - TASK_ID_BASE
        if 0 <= index < len(self.tracker.tasks):
            self.tracker.toggle(self.tracker.tasks[index])
            return GLib.SOURCE_REMOVE

        action = self.actions.get(item)
        if action:
            action()

        return GLib.SOURCE_REMOVE

    # --- Updates ------------------------------------------------------------

    def _emit(self, path: str, iface: str, signal: str, args=None) -> None:
        self.bus.emit_signal(None, path, iface, signal, args)

    def _relayout(self) -> None:
        self.revision += 1
        self._emit(MENU_PATH, MENU_IFACE, "LayoutUpdated",
                   GLib.Variant("(ui)", (self.revision, Item.ROOT)))

    def _changed(self) -> None:
        self._relayout()
        self._emit(ITEM_PATH, ITEM_IFACE, "NewToolTip")
        self.offset = 0  # Restart scrolling on task change.
        self._show()

    def _tick(self) -> bool:
        self.offset += 1
        self._show()
        return GLib.SOURCE_CONTINUE

    def _show(self) -> None:
        # Label next to the icon: "title 00:12:40", empty when paused.
        task = self.tracker.current()
        label = ""
        if task:
            # Fixed-width title: the icon does not move while it scrolls.
            title = self.mono.text(self._marquee(task.title), TITLE_WIDTH)
            label = f"{title} {fmt(task.elapsed())}"

        if label == self.label:
            return

        self.label = label
        self._emit(ITEM_PATH, ITEM_IFACE, "XAyatanaNewLabel",
                   GLib.Variant("(ss)", (label, LABEL_GUIDE)))

    def _marquee(self, title: str) -> str:
        # Long titles loop through a fixed window:
        # "Réunion budget annuel" -> "Réunion budg", "éunion budge", ...
        if len(title) <= TITLE_WIDTH:
            return title

        loop = title + MARQUEE_GAP
        start = self.offset % len(loop)
        return (loop + loop)[start:start + TITLE_WIDTH]
