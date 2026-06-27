libsystem_timezone = static_library('system-timezone',
  ['system-timezone.c', 'system-timezone.h'],
  include_directories: include_dirs,
  dependencies: [glib_dep, gio_dep]
)

clock_marshal = gnome.genmarshal('clock-marshallers',
  sources: 'clock-marshallers.list',
  prefix: '_clock_marshal'
)

clock_enums = gnome.mkenums_simple('clock-typebuiltins',
  sources: ['clock-utils.h'],
  header_prefix: '#include <glib-object.h>',
  body_prefix: '#include "clock-typebuiltins.h"'
)

clock_resources = gnome.compile_resources('clock-resources',
  'clock.gresource.xml',
  source_dir: '.'
)

clock_sources = [
  'calendar-window.c',
  'clock.c',
  'clock-face.c',
  'clock-location.c',
  'clock-location-tile.c',
  'clock-map.c',
  'clock-sunpos.c',
  'clock-utils.c',
  'set-timezone.c',
]

if have_eds
  clock_sources += [
    'calendar-client.c',
    'calendar-sources.c',
  ]
endif

clock_deps = [
  glib_dep, gio_dep, gtk_dep, mateweather_dep, mate_desktop_dep, libm
]

if have_eds
  clock_deps += eds_dep
endif

if have_wayland
  clock_deps += wayland_dep
endif

if clock_inprocess
  shared_module('clock-applet',
    clock_sources,
    clock_marshal,
    clock_enums,
    clock_resources,
    include_directories: include_dirs,
    dependencies: [clock_deps, libmate_panel_applet_dep],
    link_with: libsystem_timezone,
    c_args: ['-DMATEWEATHER_I_KNOW_THIS_IS_UNSTABLE'],
    name_prefix: 'lib',
    name_suffix: 'so',
    install: true,
    install_dir: pkglibdir
  )
else
  clock_exe = executable('clock-applet',
    clock_sources,
    clock_marshal,
    clock_enums,
    clock_resources,
    include_directories: include_dirs,
    dependencies: [clock_deps, libmate_panel_applet_dep],
    link_with: libsystem_timezone,
    c_args: ['-DMATEWEATHER_I_KNOW_THIS_IS_UNSTABLE'],
    install: true,
    install_dir: libexecdir
  )

  # DBus service file
  clock_service_conf = configuration_data()
  location = join_paths(libexecdir, 'clock-applet')
  if have_wayland
    location = 'env GDK_BACKEND=x11 ' + location
  endif
  clock_service_conf.set('LOCATION', location)
  configure_file(
    input: 'org.mate.panel.applet.ClockAppletFactory.service.in',
    output: 'org.mate.panel.applet.ClockAppletFactory.service',
    configuration: clock_service_conf,
    install: true,
    install_dir: join_paths(datadir, 'dbus-1/services')
  )
endif

# Desktop file
clock_desktop_conf = configuration_data()
clock_desktop_conf.set('VERSION', meson.project_version())
if clock_inprocess
  clock_desktop_conf.set('LOCATION', join_paths(pkglibdir, 'libclock-applet.so'))
  clock_desktop_conf.set('IN_PROCESS', 'true')
else
  clock_desktop_conf.set('LOCATION', join_paths(libexecdir, 'clock-applet'))
  clock_desktop_conf.set('IN_PROCESS', 'false')
endif

clock_desktop_in = configure_file(
  input: 'org.mate.panel.ClockApplet.mate-panel-applet.desktop.in.in',
  output: 'org.mate.panel.ClockApplet.mate-panel-applet.desktop.in',
  configuration: clock_desktop_conf
)

i18n.merge_file(
  input: clock_desktop_in,
  output: 'org.mate.panel.ClockApplet.mate-panel-applet',
  type: 'desktop',
  po_dir: '../../po',
  install: true,
  install_dir: appletsdir
)

subdir('pixmaps')
