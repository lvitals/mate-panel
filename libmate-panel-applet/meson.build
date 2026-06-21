applet_marshal = gnome.genmarshal('mate-panel-applet-marshal',
  sources: 'mate-panel-applet-marshal.list',
  prefix: 'mate_panel_applet_marshal'
)

applet_enums = gnome.mkenums('mate-panel-applet-enums',
  sources: 'mate-panel-applet.h',
  h_template: 'mate-panel-applet-enums.h.template',
  c_template: 'mate-panel-applet-enums.c.template',
  install_header: true,
  install_dir: join_paths(includedir, 'mate-panel-4.0/libmate-panel-applet')
)

libmate_panel_applet_sources = [
  'mate-panel-applet.c',
  'mate-panel-applet-factory.c',
  'mate-panel-applet-gsettings.c',
]

libmate_panel_applet_deps = [glib_dep, gio_dep, gtk_dep]

if have_x11
  libmate_panel_applet_sources += 'panel-plug.c'
  libmate_panel_applet_deps += x_dep
endif

libmate_panel_applet = shared_library('mate-panel-applet-4',
  libmate_panel_applet_sources,
  applet_marshal,
  applet_enums,
  version: '2.0.0',
  soversion: '2',
  include_directories: include_dirs,
  dependencies: libmate_panel_applet_deps,
  install: true
)

libmate_panel_applet_dep = declare_dependency(
  link_with: libmate_panel_applet,
  include_directories: include_directories('.')
)

install_headers(
  'mate-panel-applet.h',
  'mate-panel-applet-gsettings.h',
  subdir: 'mate-panel-4.0/libmate-panel-applet'
)

pkg = import('pkgconfig')
pkg.generate(libmate_panel_applet,
  name: 'libmate-panel-applet-4',
  description: 'MATE Panel Applet Library',
  version: meson.project_version(),
  filebase: 'libmatepanelapplet-4.0',
  subdirs: ['mate-panel-4.0', 'mate-panel-4.0/libmate-panel-applet'],
  requires: ['glib-2.0', 'gio-2.0', 'gtk+-3.0']
)

if get_option('introspection')
  gnome.generate_gir(libmate_panel_applet,
    sources: ['mate-panel-applet.c', 'mate-panel-applet.h', 'mate-panel-applet-gsettings.c', 'mate-panel-applet-gsettings.h', applet_enums],
    namespace: 'MatePanelApplet',
    nsversion: '4.0',
    export_packages: 'libmatepanelapplet-4.0',
    includes: ['GObject-2.0', 'Gtk-3.0', 'Gio-2.0'],
    extra_args: ['--strip-prefix=MatePanel'],
    install: true
  )
endif
