project('mate-panel', 'c',
  version: '1.29.0',
  meson_version: '>= 0.59.0',
  default_options: [
    'warning_level=1',
    'c_std=gnu99',
  ]
)

gnome = import('gnome')
i18n = import('i18n')

cc = meson.get_compiler('c')

# Configuration directories
prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))
includedir = join_paths(prefix, get_option('includedir'))
libdir = join_paths(prefix, get_option('libdir'))
libexecdir = join_paths(prefix, get_option('libexecdir'))
pkglibdir = join_paths(libdir, 'mate-panel')
modulesdir = join_paths(libdir, 'mate-panel', 'modules')
appletsdir = join_paths(datadir, 'mate-panel', 'applets')

# Global compiler flags
add_project_arguments([
  '-DHAVE_CONFIG_H',
  '-I' + meson.current_build_dir(),
  '-DDATADIR="@0@"'.format(datadir),
  '-DMATELOCALEDIR="@0@"'.format(join_paths(datadir, 'locale')),
  '-DPANELDATADIR="@0@"'.format(join_paths(datadir, 'mate-panel')),
  '-DPANEL_MODULES_DIR="@0@"'.format(modulesdir),
  '-DMATE_PANEL_APPLETS_DIR="@0@"'.format(appletsdir)
], language: 'c')

# Standard dependencies
glib_req = '>= 2.50.0'
gio_req = '>= 2.50.0'
gdk_pixbuf_req = '>= 2.26.0'
pango_req = '>= 1.15.4'
gtk_req = '>= 3.24.0'
libmate_desktop_req = '>= 1.28.2'
libmate_menu_req = '>= 1.21.0'
dconf_req = '>= 0.13.4'
libwnck_req = '>= 43.0'
mateweather_req = '>= 1.17.0'

glib_dep = dependency('glib-2.0', version: glib_req)
gio_dep = dependency('gio-2.0', version: gio_req)
gio_unix_dep = dependency('gio-unix-2.0', version: gio_req)
gmodule_dep = dependency('gmodule-2.0')
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: gdk_pixbuf_req)
pango_dep = dependency('pango', version: pango_req)
gtk_dep = dependency('gtk+-3.0', version: gtk_req)
mate_desktop_dep = dependency('mate-desktop-2.0', version: libmate_desktop_req)
mate_menu_dep = dependency('libmate-menu', version: libmate_menu_req)
dconf_dep = dependency('dconf', version: dconf_req)
libwnck_dep = dependency('libwnck-3.0', version: libwnck_req)
mateweather_dep = dependency('mateweather', version: mateweather_req)

# X11 and Wayland support
x11_opt = get_option('x11')
have_x11 = false
x11_dep = []
x_dep = []
if not x11_opt.disabled()
  x11_dep = dependency('x11', required: x11_opt.enabled())
  if x11_dep.found()
    have_x11 = true
    x_dep = [x11_dep]
  endif
endif

wayland_opt = get_option('wayland')
have_wayland = false
wayland_dep = []
if not wayland_opt.disabled()
  gtk_layer_shell_dep = dependency('gtk-layer-shell-0', required: wayland_opt.enabled())
  wayland_client_dep = dependency('wayland-client', required: wayland_opt.enabled())
  if gtk_layer_shell_dep.found() and wayland_client_dep.found()
    have_wayland = true
    wayland_dep = [gtk_layer_shell_dep, wayland_client_dep]
  endif
endif

if not have_x11 and not have_wayland
  error('At least one backend (X11 or Wayland) must be enabled.')
endif

# XRandr support
xrandr_opt = get_option('xrandr')
have_randr = false
xrandr_dep = []
if have_x11 and not xrandr_opt.disabled()
  xrandr_dep = dependency('xrandr', version: '>= 1.3.0', required: xrandr_opt.enabled())
  if xrandr_dep.found()
    have_randr = true
  endif
endif

# Evolution Data Server support
eds_opt = get_option('eds')
have_eds = false
eds_dep = []
if not eds_opt.disabled()
  libecal_dep = dependency('libecal-2.0', version: '>= 3.33.2', required: eds_opt.enabled())
  libedataserver_dep = dependency('libedataserver-1.2', version: '>= 3.5.3', required: eds_opt.enabled())
  if libecal_dep.found() and libedataserver_dep.found()
    have_eds = true
    eds_dep = [libecal_dep, libedataserver_dep]
  endif
endif

# Session Management (X11 only)
sm_dep = dependency('sm', required: have_x11)
ice_dep = dependency('ice', required: have_x11)

# Math library
libm = cc.find_library('m', required: false)

# Headers and functions
conf = configuration_data()
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('PACKAGE_NAME', meson.project_name())
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('PACKAGE_BUGREPORT', 'https://github.com/mate-desktop/mate-panel/issues')
conf.set_quoted('PACKAGE_URL', 'https://mate-desktop.org')
conf.set_quoted('PACKAGE_STRING', '@0@ @1@'.format(meson.project_name(), meson.project_version()))
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())

if cc.has_header('langinfo.h')
  conf.set('HAVE_LANGINFO_H', 1)
endif
if cc.has_function('nl_langinfo')
  conf.set('HAVE_NL_LANGINFO', 1)
endif
if cc.has_member('struct dirent', 'd_type', prefix: '#include <dirent.h>')
  conf.set('HAVE_STRUCT_DIRENT_D_TYPE', 1)
endif

if have_x11
  conf.set('HAVE_X11', 1)
endif
if have_wayland
  conf.set('HAVE_WAYLAND', 1)
endif
if have_randr
  conf.set('HAVE_RANDR', 1)
endif
if have_eds
  conf.set('HAVE_EDS', 1)
endif

# Applets in-process logic
clock_inprocess = true
fish_inprocess = true
notification_area_inprocess = true
wncklet_inprocess = true

if clock_inprocess
  conf.set('CLOCK_INPROCESS', 1)
endif
if fish_inprocess
  conf.set('FISH_INPROCESS', 1)
endif
if notification_area_inprocess
  conf.set('NOTIFICATION_AREA_INPROCESS', 1)
endif
if wncklet_inprocess
  conf.set('WNCKLET_INPROCESS', 1)
endif

# Configuration directories (defined at top)

conf.set_quoted('DATADIR', datadir)
conf.set_quoted('MATELOCALEDIR', join_paths(datadir, 'locale'))
conf.set_quoted('PANELDATADIR', join_paths(datadir, 'mate-panel'))
conf.set_quoted('PANEL_MODULES_DIR', modulesdir)
conf.set_quoted('MATE_PANEL_APPLETS_DIR', appletsdir)

configure_file(
  output: 'config.h',
  configuration: conf
)

include_dirs = [
  include_directories('.'),
  include_directories('mate-panel'),
  include_directories('mate-panel/libpanel-util'),
  include_directories('mate-panel/libmate-panel-applet-private'),
  include_directories('libmate-panel-applet'),
  include_directories('mate-panel/mate-submodules'),
]

# Generate helper variables for GSettings schemas and gettext
glib_compile_resources = find_program('glib-compile-resources')
gdbus_codegen = find_program('gdbus-codegen')
glib_genmarshal = find_program('glib-genmarshal')
glib_mkenums = find_program('glib-mkenums')

subdir('po')
libegg_deps = [
  glib_dep,
  gtk_dep,
  ice_dep,
  sm_dep,
]
subdir('mate-panel/mate-submodules/libegg')
libegg = egg
subdir('mate-panel/libpanel-util')
subdir('mate-panel/libmate-panel-applet-private')
subdir('libmate-panel-applet')
subdir('mate-panel')
subdir('applets')
subdir('data')
subdir('icons')
subdir('help')
subdir('man')
subdir('doc')

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true
)
