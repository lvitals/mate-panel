na_marshal = gnome.genmarshal('na-marshal',
  sources: 'na-marshal.list',
  prefix: '_na_marshal'
)

libsystem_tray = static_library('system-tray',
  [
    'fixedtip.c',
    'na-tray.c',
    'na-tray-child.c',
    'na-tray-manager.c',
    na_marshal
  ],
  include_directories: [include_dirs, include_directories('..')],
  dependencies: [glib_dep, gio_dep, gtk_dep, x11_dep]
)

libsystem_tray_dep = declare_dependency(
  link_with: libsystem_tray,
  include_directories: include_directories('.')
)
