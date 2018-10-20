mediaserver2_lib = library('mediaserver2',
        files('media-server2-server-table.c',
              'media-server2-server.c',
              'media-server2-client.c',
              'media-server2-observer.c'),
        dependencies : [
            dependency('gio-2.0'),
            dependency('glib-2.0'),
            dependency('gobject-2.0'),
            dependency('dbus-glib-1')
        ]
)

mediaserver2 = declare_dependency(include_directories : include_directories('.'), link_with : mediaserver2_lib)
