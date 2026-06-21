libpanel_util_sources = files(
  'panel-cleanup.c',
  'panel-color.c',
  'panel-error.c',
  'panel-glib.c',
  'panel-gtk.c',
  'panel-icon-chooser.c',
  'panel-keyfile.c',
  'panel-launch.c',
  'panel-list.c',
  'panel-session-manager.c',
  'panel-show.c',
  'panel-xdg.c'
)

libpanel_util = static_library('panel-util',
  libpanel_util_sources,
  include_directories: include_dirs,
  dependencies: [glib_dep, gio_dep, gtk_dep, mate_desktop_dep, dconf_dep]
)

libpanel_util_dep = declare_dependency(
  link_with: libpanel_util,
  include_directories: include_directories('.')
)
