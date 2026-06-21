sn_dbus_menu_gen = gnome.gdbus_codegen('sn-dbus-menu-gen',
  'com.canonical.dbusmenu.xml',
  namespace: 'Sn'
)

sn_host_gen = gnome.gdbus_codegen('sn-host-v0-gen',
  'org.kde.StatusNotifierHost.xml',
  namespace: 'Sn'
)

sn_item_gen = gnome.gdbus_codegen('sn-item-v0-gen',
  'org.kde.StatusNotifierItem.xml',
  namespace: 'Sn'
)

sn_watcher_gen = gnome.gdbus_codegen('sn-watcher-v0-gen',
  'org.kde.StatusNotifierWatcher.xml',
  namespace: 'Sn'
)

libstatus_notifier = static_library('status-notifier',
  [
    'sn-dbus-menu.c',
    'sn-dbus-menu-item.c',
    'sn-flat-button.c',
    'sn-host-v0.c',
    'sn-item.c',
    'sn-item-v0.c',
    sn_dbus_menu_gen,
    sn_host_gen,
    sn_item_gen,
    sn_watcher_gen
  ],
  include_directories: [include_dirs, include_directories('..')],
  dependencies: [glib_dep, gio_dep, gtk_dep, mate_desktop_dep, libm]
)

libstatus_notifier_dep = declare_dependency(
  link_with: libstatus_notifier,
  include_directories: include_directories('.')
)
