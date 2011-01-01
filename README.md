MythSvtPlay
===========

MythSvtPlay is a plugin for the Linux DVR software MythTV
(http://www.mythtv.org) that can be used to watch the streams
published by the swedish national television SVT on SVT Play
(http://svtplay.se).

Compatibility
-------------

MythSvtPlay is available for MythTV versions 0.22, 0.23 and 0.24. 
Any new development will only take place on the '0.24-compatible' branch. 

The Linux distribution used during development is Ubuntu 10.10 and
Mythbuntu 10.10. Other distributions might have other names for the
different packages as well as alternate installation paths. Again,
please report any issues you might have.

Compiling and installing
------------------------

__Dependencies:__

* libmyth-dev
* libqt4-dev
* libqt4 (4.5)
* libqt4-xml
* libqt4-xmlpatterns
* libqt4-network

__Other programs:__

* mplayer
* (optional) flvstreamer   (supports rtmp streams)
* (optional) rtmpdump      (for rtmps and rtmpe streams)

The plugin uses mplayer to play the streams available on SVT
Play. Unfortunately, mplayer doesn't natively support streams in the
rtmp, rtmpe and rtmps formats. If the plugin cannot find either
flvstreamer or rtmpdump in the users path, it will fall back to using
the wmv streams. The wmv streams are however much slower and prone to
buffering and installing at least flvstreamer is highly
recommended.

__Compiling:__

For convenience, the plugin is packaged in the same build structure as
the official plugins.  Building is therefore as easy as navigating to
the mythplugins folder and executing the following commands:

    $ ./configure
    $ qmake mythplugins.pro
    $ make

__Installing:__

When it has finished compiling, it is necessary to install the
binaries and the theme files in the correct place. This is done by
executing the following:

    $ (sudo) make install

Now all that remains is to alter the mythtv menu files to allow you to
start the plugin. If you're using the default menu theme (not to be
confused with regular mythtv themes) the files are usually available
in /usr/share/mythtv/themes/defaultmenu/. Change the appropriate menu
file (I suggest library.xml or mainmenu.xml) by adding the following:

    <button>
        <type>MENU_SVTPLAY</type>
        <text>Watch SVT Play</text>
        <text lang="SV">Titta på SVT Play</text>
        <description>Watch programs and clips published at svtplay.se</description>
        <action>PLUGIN mythsvtplay</action>
        <depends>mythsvtplay</depends>
    </button>

Usage
-----

__Refreshing the program cache:__

The first time the plugin loads, it will download basic information on
all programs available on SVT Play. This information is cached and
used whenever the plugin is started later. If you wish to refresh this
cache (for example when SVT introduces new shows) simply press MENU
(m) on the main menu. A dialog will popup and ask you to confirm the
refresh.

__Marking favorites:__

It is possible to mark programs as favorites in the main program
menu. Simply go to a program of your choice and press RECORD (r) and
it will be added to the favorites menu item. Press RECORD (r) again to
unmark it. 

Unfortunately this marking is not reflected on the program menu items
yet. This issue is purely visual however, the functionality is there
and I will fix it when I rework the UI.

Known Issues / Bugs
-------------------

* When using the streams downloaded by flvstreamer or rtmpdump,
  mplayer won't prevent you from seeking beyond what's currently
  downloaded. This will result in mplayer quitting early, before the
  stream has been played through completely.

* The favorite marking issue mentioned above.

Todo
----

* Redo the main menu ui
* Allow searching
