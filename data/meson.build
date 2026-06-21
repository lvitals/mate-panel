layout_dir = join_paths(datadir, 'mate-panel', 'layouts')

subdir('theme')

install_data(
  'default.layout',
  'fedora.layout',
  'linuxmint.layout',
  'mageia.layout',
  'opensuse.layout',
  'ubuntu.layout',
  install_dir: layout_dir
)

# GSettings schemas are generated together so the build tree can compile a
# local gschemas.compiled for uninstalled runs.
schemas = [
  {
    'input': 'org.mate.panel.gschema.xml.in',
    'output': 'org.mate.panel.gschema.xml',
  },
  {
    'input': 'org.mate.panel.object.gschema.xml.in',
    'output': 'org.mate.panel.object.gschema.xml',
  },
  {
    'input': 'org.mate.panel.toplevel.gschema.xml.in',
    'output': 'org.mate.panel.toplevel.gschema.xml',
  },
  {
    'input': 'org.mate.panel.menubar.gschema.xml.in',
    'output': 'org.mate.panel.menubar.gschema.xml',
  },
  {
    'input': '../applets/clock/org.mate.panel.applet.clock.gschema.xml.in',
    'output': 'org.mate.panel.applet.clock.gschema.xml',
  },
  {
    'input': '../applets/fish/org.mate.panel.applet.fish.gschema.xml.in',
    'output': 'org.mate.panel.applet.fish.gschema.xml',
  },
  {
    'input': '../applets/notification_area/org.mate.panel.applet.notification-area.gschema.xml.in',
    'output': 'org.mate.panel.applet.notification-area.gschema.xml',
  },
  {
    'input': '../applets/wncklet/org.mate.panel.applet.window-list.gschema.xml.in',
    'output': 'org.mate.panel.applet.window-list.gschema.xml',
  },
  {
    'input': '../applets/wncklet/org.mate.panel.applet.window-list-previews.gschema.xml.in',
    'output': 'org.mate.panel.applet.window-list-previews.gschema.xml',
  },
  {
    'input': '../applets/wncklet/org.mate.panel.applet.workspace-switcher.gschema.xml.in',
    'output': 'org.mate.panel.applet.workspace-switcher.gschema.xml',
  },
]

schema_conf = configuration_data()
schema_conf.set('GETTEXT_PACKAGE', meson.project_name())

enums_xml = custom_target('org.mate.panel.enums.xml',
  input: '../mate-panel/panel-enums-gsettings.h',
  output: 'org.mate.panel.enums.xml',
  command: [
    glib_mkenums,
    '--comments', '<!-- @comment@ -->',
    '--fhead', '<schemalist>\n',
    '--vhead', '  <enum id=\'org.mate.panel.@EnumName@\'>\n',
    '--vprod', '    <value nick=\'@valuenick@\' value=\'@valuenum@\'/>\n',
    '--vtail', '  </enum>\n',
    '--ftail', '</schemalist>\n',
    '@INPUT@'
  ],
  capture: true,
  install: true,
  install_dir: join_paths(datadir, 'glib-2.0/schemas')
)

schema_outputs = [enums_xml]
foreach s : schemas
  schema_outputs += configure_file(
    input: s['input'],
    output: s['output'],
    configuration: schema_conf,
    install: true,
    install_dir: join_paths(datadir, 'glib-2.0/schemas')
  )
endforeach

custom_target('gschemas.compiled',
  input: schema_outputs,
  output: 'gschemas.compiled',
  command: [find_program('glib-compile-schemas'), meson.current_build_dir()],
  build_by_default: true
)
