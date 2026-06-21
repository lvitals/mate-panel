fish_resources = gnome.compile_resources('fish-resources',
  'fish.gresource.xml',
  source_dir: '.'
)

fish_sources = [
  'fish.c',
]

fish_deps = [
  glib_dep, gio_dep, gtk_dep, mate_desktop_dep
]

fish_bitmaps_dir = join_paths(datadir, 'mate-panel', 'fish')
install_data(
  'wanda.png',
  'fishanim.png',
  'oldwanda.png',
  'footguy.png',
  'monkey.png',
  install_dir: fish_bitmaps_dir
)

if fish_inprocess
  shared_module('fish-applet',
    fish_sources,
    fish_resources,
    include_directories: include_dirs,
    dependencies: [fish_deps, libmate_panel_applet_dep],
    c_args: ['-DFISH_ICONDIR="@0@"'.format(fish_bitmaps_dir)],
    name_prefix: 'lib',
    name_suffix: 'so',
    install: true,
    install_dir: pkglibdir
  )
else
  fish_exe = executable('fish-applet',
    fish_sources,
    fish_resources,
    include_directories: include_dirs,
    dependencies: [fish_deps, libmate_panel_applet_dep],
    c_args: ['-DFISH_ICONDIR="@0@"'.format(fish_bitmaps_dir)],
    install: true,
    install_dir: libexecdir
  )

  # DBus service file
  fish_service_conf = configuration_data()
  fish_service_conf.set('LOCATION', join_paths(libexecdir, 'fish-applet'))
  configure_file(
    input: 'org.mate.panel.applet.FishAppletFactory.service.in',
    output: 'org.mate.panel.applet.FishAppletFactory.service',
    configuration: fish_service_conf,
    install: true,
    install_dir: join_paths(datadir, 'dbus-1/services')
  )
endif

# Desktop file
fish_desktop_conf = configuration_data()
fish_desktop_conf.set('VERSION', meson.project_version())
if fish_inprocess
  fish_desktop_conf.set('LOCATION', join_paths(pkglibdir, 'libfish-applet.so'))
  fish_desktop_conf.set('IN_PROCESS', 'true')
else
  fish_desktop_conf.set('LOCATION', join_paths(libexecdir, 'fish-applet'))
  fish_desktop_conf.set('IN_PROCESS', 'false')
endif

fish_desktop_in = configure_file(
  input: 'org.mate.panel.FishApplet.mate-panel-applet.desktop.in.in',
  output: 'org.mate.panel.FishApplet.mate-panel-applet.desktop.in',
  configuration: fish_desktop_conf
)

i18n.merge_file(
  input: fish_desktop_in,
  output: 'org.mate.panel.FishApplet.mate-panel-applet',
  type: 'desktop',
  po_dir: '../../po',
  install: true,
  install_dir: appletsdir
)
