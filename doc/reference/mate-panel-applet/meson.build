if get_option('gtk-doc')
  gnome.gtkdoc('mate-panel-applet',
    main_xml: 'mate-panel-applet-docs.sgml',
    src_dir: join_paths(meson.source_root(), 'libmate-panel-applet'),
    dependencies: [libmate_panel_applet_dep, gtk_dep, gio_dep],
    scan_args: [
      '--ignore-headers=MATE_Panel.h mate-panel-applet-enums.h mate-panel-applet-marshal.h mate-panel-applet-private.h mate-panel-applet-shell.h',
    ],
    mkdb_args: [
      '--output-format=xml',
    ],
    install: true
  )
endif
