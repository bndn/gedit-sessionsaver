# gedit session saver plugin

This is a plugin for [gedit][1], the official text editor of the GNOME desktop environment. It’s a fork of the original gedit session saver plugin that [have been dropped][2] in [gedit 3.7.1][3]. This repository contains the ready-to-use version of this plugin that I fixed and improved in my [gedit-plugins repository][4].

This plugin is for gedit versions 3 and above. **This plugin is NOT compatible with gedit 2.x**.

The translation of the plugin is not available in this repository, but it is always available in the package _gedit-plugins_ from your distribution. If this package is installed on your system, the translation should works.

[1]: https://projects.gnome.org/gedit/ "gedit text editor"
[2]: https://git.gnome.org/browse/gedit-plugins/commit/?id=7abf46cf9c6e3f12798e813ffeac28e3594167f0 "commit of the drop of the session saver plugin"
[3]: https://git.gnome.org/browse/gedit-plugins/commit/?id=b4b1601b8bc21c43db7b0142ace22040a24b31d2 "commit of the gedit-plugins 3.7.1 release"
[4]: https://github.com/sphax3d/gedit-plugins "sessionsaver-plugin branch in my gedit-plugins repository"

## Installation

1. Download the source code form this repository in [zip format](https://github.com/sphax3d/gedit-sessionsaver/zipball/master), in [tar format](https://github.com/sphax3d/gedit-sessionsaver/tarball/master) or using the `git clone` command:

        $ git clone https://github.com/sphax3d/gedit-sessionsaver.git

2. Link or copy the plugin file and the plugin directory into the gedit plugins directory :

        # link
        $ ln -s gedit-sessionsaver/sessionsaver ~/.local/share/gedit/plugins/
        $ ln -s gedit-sessionsaver/sessionsaver.plugin ~/.local/share/gedit/plugins/
        
        # or copy 
        $ cp gedit-sessionsaver/sessionsaver gedit-sessionsaver/sessionsaver.plugin ~/.local/share/gedit/plugins/

3. Open or restart gedit.
4. Activate the plugin through the **Plugins** tab in the **Edit > Preferences** dialog.

## License

Copyright © 2006 Steve Frécinaux  
GNU GPLv2. See the COPYING file.

Code available on Github at https://github.com/sphax3d/gedit-sessionsaver
