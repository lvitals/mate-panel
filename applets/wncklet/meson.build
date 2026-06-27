wncklet_resources = gnome.compile_resources('wncklet-resources',
  'wncklet.gresource.xml',
  source_dir: '.'
)

wncklet_sources = files(
  'wncklet.c',
  'window-menu.c',
  'window-list.c',
  'showdesktop.c',
  'workspace-switcher.c',
)

wncklet_deps = [
  glib_dep, gio_dep, gtk_dep, libwnck_dep, mate_desktop_dep
]

if have_x11
  wncklet_deps += x_dep
endif

if have_wayland
  wayland_scanner = find_program('wayland-scanner')
  wayland_protos = [
    'marco-workspace-pager-v1',
    'ext-workspace-v1',
    'wlr-foreign-toplevel-management-unstable-v1'
  ]
  
  wncklet_wayland_sources = []
  foreach proto : wayland_protos
    xml = files('wayland-protocol/@0@.xml'.format(proto))
    
    c_file = custom_target('@0@-code.c'.format(proto),
      input: xml,
      output: '@0@-code.c'.format(proto),
      command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@']
    )
    
    h_file = custom_target('@0@-client.h'.format(proto),
      input: xml,
      output: '@0@-client.h'.format(proto),
      command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@']
    )
    
    wncklet_wayland_sources += [c_file, h_file]
  endforeach
  
  wncklet_sources += files('wayland-backend.c')
  wncklet_sources += wncklet_wayland_sources
  wncklet_deps += wayland_dep
endif

if wncklet_inprocess
  shared_module('wnck-applet',
    wncklet_sources,
    wncklet_resources,
    include_directories: include_dirs,
    dependencies: [wncklet_deps, libmate_panel_applet_dep],
    name_prefix: 'lib',
    name_suffix: 'so',
    install: true,
    install_dir: pkglibdir
  )
else
  wncklet_exe = executable('wnck-applet',
    wncklet_sources,
    wncklet_resources,
    include_directories: include_dirs,
    dependencies: [wncklet_deps, libmate_panel_applet_dep],
    install: true,
    install_dir: libexecdir
  )

  # DBus service file
  wncklet_service_conf = configuration_data()
  location = join_paths(libexecdir, 'wnck-applet')
  if have_wayland
    location = 'env GDK_BACKEND=x11 ' + location
  endif
  wncklet_service_conf.set('LOCATION', location)
  configure_file(
    input: 'org.mate.panel.applet.WnckletFactory.service.in',
    output: 'org.mate.panel.applet.WnckletFactory.service',
    configuration: wncklet_service_conf,
    install: true,
    install_dir: join_paths(datadir, 'dbus-1/services')
  )
endif

# Desktop file
wncklet_desktop_conf = configuration_data()
wncklet_desktop_conf.set('VERSION', meson.project_version())
if wncklet_inprocess
  wncklet_desktop_conf.set('LOCATION', join_paths(pkglibdir, 'libwnck-applet.so'))
  wncklet_desktop_conf.set('IN_PROCESS', 'true')
else
  wncklet_desktop_conf.set('LOCATION', join_paths(libexecdir, 'wnck-applet'))
  wncklet_desktop_conf.set('IN_PROCESS', 'false')
endif

wncklet_desktop_in = configure_file(
  input: 'org.mate.panel.Wncklet.mate-panel-applet.desktop.in.in',
  output: 'org.mate.panel.Wncklet.mate-panel-applet.desktop.in',
  configuration: wncklet_desktop_conf
)

i18n.merge_file(
  input: wncklet_desktop_in,
  output: 'org.mate.panel.Wncklet.mate-panel-applet',
  type: 'desktop',
  po_dir: '../../po',
  install: true,
  install_dir: appletsdir
)
