subdir('libstatus-notifier-watcher')
subdir('status-notifier')

libtray_sources = files(
  'na-grid.c',
  'na-host.c',
  'na-item.c',
)

libtray_deps = [
  glib_dep, gio_dep, gtk_dep,
  libstatus_notifier_watcher_dep,
  libstatus_notifier_dep
]

if have_x11
  subdir('system-tray')
  libtray_deps += libsystem_tray_dep
endif

libtray = static_library('tray',
  libtray_sources,
  include_directories: include_dirs,
  dependencies: libtray_deps
)

na_resources = gnome.compile_resources('na-resources',
  'na.gresource.xml',
  source_dir: '.'
)

notification_area_sources = files(
  'main.c',
)

notification_area_deps = [
  glib_dep, gio_dep, gtk_dep, mate_desktop_dep
]

if notification_area_inprocess
  shared_module('notification-area-applet',
    notification_area_sources,
    na_resources,
    include_directories: include_dirs,
    dependencies: [notification_area_deps, libmate_panel_applet_dep],
    link_with: libtray,
    c_args: [
      '-DG_LOG_DOMAIN="notification-area-applet"',
      '-DPROVIDE_WATCHER_SERVICE=1'
    ],
    name_prefix: 'lib',
    name_suffix: 'so',
    install: true,
    install_dir: pkglibdir
  )
else
  notification_area_exe = executable('notification-area-applet',
    notification_area_sources,
    na_resources,
    include_directories: include_dirs,
    dependencies: [notification_area_deps, libmate_panel_applet_dep],
    link_with: libtray,
    c_args: [
      '-DG_LOG_DOMAIN="notification-area-applet"',
      '-DPROVIDE_WATCHER_SERVICE=1'
    ],
    install: true,
    install_dir: libexecdir
  )

  # DBus service file
  notification_area_service_conf = configuration_data()
  location = join_paths(libexecdir, 'notification-area-applet')
  if have_wayland
    location = 'env GDK_BACKEND=x11 ' + location
  endif
  notification_area_service_conf.set('LOCATION', location)
  configure_file(
    input: 'org.mate.panel.applet.NotificationAreaAppletFactory.service.in',
    output: 'org.mate.panel.applet.NotificationAreaAppletFactory.service',
    configuration: notification_area_service_conf,
    install: true,
    install_dir: join_paths(datadir, 'dbus-1/services')
  )
endif

# Desktop file
notification_area_desktop_conf = configuration_data()
notification_area_desktop_conf.set('VERSION', meson.project_version())
if notification_area_inprocess
  notification_area_desktop_conf.set('LOCATION', join_paths(pkglibdir, 'libnotification-area-applet.so'))
  notification_area_desktop_conf.set('IN_PROCESS', 'true')
else
  notification_area_desktop_conf.set('LOCATION', join_paths(libexecdir, 'notification-area-applet'))
  notification_area_desktop_conf.set('IN_PROCESS', 'false')
endif

notification_area_desktop_in = configure_file(
  input: 'org.mate.panel.NotificationAreaApplet.mate-panel-applet.desktop.in.in',
  output: 'org.mate.panel.NotificationAreaApplet.mate-panel-applet.desktop.in',
  configuration: notification_area_desktop_conf
)

i18n.merge_file(
  input: notification_area_desktop_in,
  output: 'org.mate.panel.NotificationAreaApplet.mate-panel-applet',
  type: 'desktop',
  po_dir: '../../po',
  install: true,
  install_dir: appletsdir
)
