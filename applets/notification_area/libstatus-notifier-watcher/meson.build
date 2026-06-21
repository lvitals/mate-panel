watcher_codegen = gnome.gdbus_codegen('gf-sn-watcher-v0-gen',
  'org.kde.StatusNotifierWatcher.xml',
  namespace: 'Gf'
)

libstatus_notifier_watcher = static_library('status-notifier-watcher',
  [
    'gf-sn-watcher-v0.c',
    'gf-status-notifier-watcher.c',
    watcher_codegen
  ],
  include_directories: include_dirs,
  dependencies: [glib_dep, gio_dep, gtk_dep]
)

libstatus_notifier_watcher_dep = declare_dependency(
  link_with: libstatus_notifier_watcher,
  include_directories: include_directories('.')
)
