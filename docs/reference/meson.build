glib_docpath = glib.get_pkgconfig_variable('prefix') / 'share' / 'gtk-doc' / 'html'
gtk_docpath = gtk.get_pkgconfig_variable('prefix') / 'share' / 'gtk-doc' / 'html'

docpath = abs_datadir / 'gtk-doc' / 'html'

version_xml = configure_file(input: 'version.xml.in', output: 'version.xml',
    configuration: {'PACKAGE_VERSION': version})

build_selector_table = configure_file(input: 'build-selector-table.pl.in',
    output: 'build-selector-table.pl', configuration: {'PERL': perl.path()})
gi_dispa = files('../../libchimara/gi_dispa.c')
selectors_xml = custom_target('selectors.xml', input: gi_dispa,
    output: 'selectors.xml', command: [perl, build_selector_table, '@INPUT@'],
    capture: true, build_by_default: true)

expand_content_files = [
    'glk-api-conventions.sgml',
    'glk-display-style.sgml',
    'glk-line-input.sgml',
    'glk-main-function.sgml',
    'glk-normalization.sgml',
    'glk-other-events.sgml',
    'glk-output.sgml',
    'glk-window-arrangement.sgml',
]

gnome.gtkdoc('chimara',
    main_sgml: 'chimara-docs.sgml',
    src_dir: 'libchimara',
    dependencies: [gobject, libchimara_dep],
    gobject_typesfile: 'chimara.types',
    ignore_headers: [
        'abort.h',
        'charset.h',
        'chimara-glk-private.h',
        'chimara-marshallers.h',
        'event.h',
        'fileref.h',
        'graphics.h',
        'hyperlink.h',
        'init.h',
        'input.h',
        'magic.h',
        'mouse.h',
        'pager.h',
        'resource.h',
        'schannel.h',
        'stream.h',
        'strio.h',
        'style.h',
        'timer.h',
        'ui-message.h',
        'window.h',
    ],
    scan_args: ['--ignore-decorators=GLK_ATTRIBUTE_NORETURN'],
    mkdb_args: [
        '--sgml-mode',
        '--output-format=xml',
        '--expand-content-files=@0@'.format(' '.join(expand_content_files)),
    ],
    fixxref_args: [
        '--html-dir=@0@'.format(docpath),
        '--extra-dir=@0@'.format(glib_docpath / 'glib'),
        '--extra-dir=@0@'.format(glib_docpath / 'gobject'),
        '--extra-dir=@0@'.format(glib_docpath / 'gio'),
        '--extra-dir=@0@'.format(gtk_docpath / 'gtk'),
    ],
    html_assets: [
        'images/fig1.png',
        'images/fig2.png',
        'images/fig3a.png',
        'images/fig3b.png',
        'images/fig3c.png',
        'images/fig4a.png',
        'images/fig4b.png',
        'images/fig4c.png',
        'images/fig5-7a.png',
        'images/fig6.png',
        'images/fig7b.png',
        'images/fig7c.png',
        'images/fig7d.png',
        'images/fig7e.png',
        'images/fig8a.png',
        'images/fig8b.png',
        'images/fig8c.png',
        'images/fig9.png',
        'images/fig10.png',
        'images/fig11.png',
    ],
    content_files: [
        version_xml,
        selectors_xml,
        'blorb-library.sgml',
        'blorb.sgml',
        'dispatch-selectors.sgml',
        'dispatch.sgml',
        'glk-api-conventions.sgml',
        'glk-character-encoding.sgml',
        'glk-display-style.sgml',
        'glk-front-matter.sgml',
        'glk-introduction.sgml',
        'glk-line-input.sgml',
        'glk-main-function.sgml',
        'glk-normalization.sgml',
        'glk-other-events.sgml',
        'glk-output.sgml',
        'glk-porting.sgml',
        'glk-sound-resources.sgml',
        'glk-window-arrangement.sgml',
        'glk-windows.sgml',
    ],
    install: true)
