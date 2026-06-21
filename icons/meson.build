icon_basenames = [
  'mate-panel-clock',
  'mate-panel-drawer',
  'mate-panel-fish',
  'mate-panel-force-quit',
  'mate-panel-launcher',
  'mate-panel-notification-area',
  'mate-panel-separator',
  'mate-panel',
  'mate-panel-window-list',
  'mate-panel-window-menu',
  'mate-panel-workspace-switcher'
]

sizes = ['16x16', '22x22', '24x24', '32x32', '48x48', '64x64', '96x96', '256x256']
icon_theme_dir = join_paths(datadir, 'icons', 'hicolor')

foreach size : sizes
  icons = []
  foreach name : icon_basenames
    icons += '@0@/@1@.png'.format(size, name)
  endforeach
  install_data(icons, install_dir: join_paths(icon_theme_dir, size, 'apps'))
endforeach

scalable_icons = []
foreach name : icon_basenames
  scalable_icons += 'scalable/@0@.svg'.format(name)
endforeach
install_data(scalable_icons, install_dir: join_paths(icon_theme_dir, 'scalable', 'apps'))

meson.add_install_script('sh', '-c', 'gtk-update-icon-cache -qtf @0@ || true'.format(icon_theme_dir))
