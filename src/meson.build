executable(
    'grilo-ms2',
    files('grilo-mediaserver2.c'),
    dependencies : [
        mediaserver2,
        dependency('dbus-glib-1'),
        dependency('grilo-0.3', version : '>= 0.3')
    ],
    c_args : [
        '-DPREFIX=@0@'.format(get_option('prefix')),
        '-DSYSCONFDIR="@0@"'.format(get_option('sysconfdir'))
    ],
    install : true
)

executable(
    'test-client',
    files('test-client.c'),
    dependencies : [
        mediaserver2,
        dependency('gobject-2.0')
    ],
    c_args : [
        '-DPREFIX=@0@'.format(get_option('prefix')),
    ]
)
