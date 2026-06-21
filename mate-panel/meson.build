panel_marshal = gnome.genmarshal('panel-marshal',
  sources: 'panel-marshal.list',
  prefix: 'panel_marshal'
)

panel_enums = gnome.mkenums_simple('panel-typebuiltins',
  sources: ['panel-enums.h', 'panel-enums-gsettings.h', 'panel-types.h'],
  header_prefix: '#include <glib-object.h>',
  body_prefix: '#include "panel-typebuiltins.h"'
)

panel_resources = gnome.compile_resources('panel-resources',
  'panel.gresource.xml',
  source_dir: '.'
)

panel_test_resources = gnome.compile_resources('panel-test-applets-resources',
  'panel-test-applets.gresource.xml',
  source_dir: '.'
)

panel_sources = files(
  'main.c',
  'panel-widget.c',
  'button-widget.c',
  'panel-session.c',
  'panel.c',
  'applet.c',
  'drawer.c',
  'panel-config-global.c',
  'panel-util.c',
  'panel-properties-dialog.c',
  'panel-run-dialog.c',
  'menu.c',
  'panel-context-menu.c',
  'launcher.c',
  'panel-applet-frame.c',
  'panel-applets-manager.c',
  'panel-shell.c',
  'panel-background.c',
  'panel-stock-icons.c',
  'panel-action-button.c',
  'panel-menu-bar.c',
  'panel-menu-button.c',
  'panel-menu-items.c',
  'panel-separator.c',
  'panel-recent.c',
  'panel-toplevel.c',
  'panel-frame.c',
  'panel-multimonitor.c',
  'panel-a11y.c',
  'panel-bindings.c',
  'panel-layout.c',
  'panel-profile.c',
  'panel-lockdown.c',
  'panel-addto.c',
  'panel-ditem-editor.c',
  'panel-modules.c',
  'panel-applet-info.c',
  'panel-reset.c'
)

mate_panel_deps = [
  glib_dep, gio_dep, gio_unix_dep, gmodule_dep,
  gdk_pixbuf_dep, pango_dep, gtk_dep,
  mate_desktop_dep, mate_menu_dep, dconf_dep, libwnck_dep
]

mate_panel_c_args = ['-DMATEMENU_I_KNOW_THIS_IS_UNSTABLE']

if have_wayland
  panel_sources += files('wayland-backend.c')
  mate_panel_deps += wayland_dep
endif

if have_x11
  panel_sources += files(
    'xstuff.c',
    'panel-xutils.c',
    'panel-force-quit.c',
    'panel-action-protocol.c',
    'panel-struts.c'
  )
  mate_panel_deps += [x11_dep, sm_dep, ice_dep]
endif

if have_randr
  mate_panel_deps += xrandr_dep
endif

executable('mate-panel',
  panel_sources,
  panel_marshal,
  panel_enums,
  panel_resources,
  include_directories: include_dirs,
  dependencies: mate_panel_deps,
  c_args: mate_panel_c_args,
  link_with: [libegg, libmate_panel_applet_private, libpanel_util],
  export_dynamic: true,
  install: true
)

# mate-desktop-item-edit
mate_desktop_item_edit_sources = files(
  'mate-desktop-item-edit.c',
  'panel-ditem-editor.c',
  'panel-util.c'
)

mate_desktop_item_edit_deps = [glib_dep, gio_dep, gtk_dep, mate_desktop_dep, dconf_dep]

if have_x11
  mate_desktop_item_edit_sources += files('xstuff.c')
  mate_desktop_item_edit_deps += x11_dep
endif

executable('mate-desktop-item-edit',
  mate_desktop_item_edit_sources,
  panel_resources,
  panel_marshal,
  include_directories: include_dirs,
  dependencies: mate_desktop_item_edit_deps,
  link_with: libpanel_util,
  export_dynamic: true,
  install: true
)

# mate-panel-test-applets
mate_panel_test_applets_sources = files(
  'panel-modules.c',
  'panel-applet-info.c',
  'panel-applets-manager.c',
  'panel-test-applets.c'
)

executable('mate-panel-test-applets',
  mate_panel_test_applets_sources,
  panel_test_resources,
  panel_marshal,
  include_directories: include_dirs,
  dependencies: [glib_dep, gio_dep, gtk_dep, dconf_dep, mate_desktop_dep],
  link_with: [libmate_panel_applet_private_mini, libpanel_util],
  export_dynamic: true,
  install: true
)

i18n.merge_file(
  input: 'mate-panel.desktop.in',
  output: 'mate-panel.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(datadir, 'applications')
)
