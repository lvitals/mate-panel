libmate_panel_applet_private_sources = files(
  'panel-applets-manager-dbus.c',
  'panel-applet-container.c',
  'panel-applet-frame-dbus.c'
)

libmate_panel_applet_private_mini_sources = files(
  'panel-applet-mini.c',
  'panel-applets-manager-dbus.c',
  'panel-applet-container.c'
)

libmate_panel_applet_private = static_library('mate-panel-applet-private',
  libmate_panel_applet_private_sources,
  include_directories: include_dirs,
  dependencies: [glib_dep, gio_dep, gtk_dep, gmodule_dep]
)

libmate_panel_applet_private_mini = static_library('mate-panel-applet-private-mini',
  libmate_panel_applet_private_mini_sources,
  include_directories: include_dirs,
  dependencies: [glib_dep, gio_dep, gtk_dep, gmodule_dep]
)

libmate_panel_applet_private_dep = declare_dependency(
  link_with: libmate_panel_applet_private,
  include_directories: include_directories('.')
)

libmate_panel_applet_private_mini_dep = declare_dependency(
  link_with: libmate_panel_applet_private_mini,
  include_directories: include_directories('.')
)
