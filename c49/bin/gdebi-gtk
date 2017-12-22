#! /usr/bin/python3
#
# Copyright (c) 2005-2009 Canonical Ltd
#
# AUTHOR:
# Michael Vogt <mvo@ubuntu.com>
#
# This file is part of GDebi
#
# GDebi is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published
# by the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# GDebi is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GDebi; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

# silly py3 compat, py3.3 should make this unneeded
try:
    unicode
except NameError:
    unicode = lambda *args: args[0]

import sys

from optparse import OptionParser
from GDebi.GDebiGtk import GDebiGtk

from gettext import gettext as _
import gettext

from gi.repository import Gtk


if __name__ == "__main__":
    data="/usr/share/gdebi"

    localesApp="gdebi"
    localesDir="/usr/share/locale"
    gettext.bindtextdomain(localesApp, localesDir)
    gettext.textdomain(localesApp)

    parser = OptionParser()
    parser.add_option("-n", "--non-interactive",
                      action="store_true", dest="non_interactive",
                      default=False,
                      help=unicode(_("Run non-interactive (dangerous!)"),"UTF-8"))
    parser.add_option("--auto-close", "",
                      action="store_true", default=False,
                      help=unicode(_("Auto close when the install is finished"),"UTF-8"))
    parser.add_option("--datadir", "", default="",
                      help=unicode(_("Use alternative datadir"),"UTF_8"))
    parser.add_option("-r", "--remove", default="False",
                      action="store_true", dest="remove",
                      help=unicode(_("Remove package"),"UTF-8"))
    (options, args) = parser.parse_args()

    if options.datadir:
        data = options.datadir

    try:
        Gtk.init_check(sys.argv)
    except RuntimeError as e:
        sys.stderr.write("Can not start %s: %s. Exiting\n" % (sys.argv[0], e))
        sys.exit(1)

    afile = ""
    if len(args) >= 1:
        afile = args[0]

    try:
        app = GDebiGtk(datadir=data,options=options,file=afile)
    except SystemError as e:
        err_header = _("Software index is broken")
        err_body = _("This is a major failure of your software "
                    "management system. Please check for broken packages "
                    "with synaptic, check the file permissions and "
                    "correctness of the file '/etc/apt/sources.list' and "
                    "reload the software information with: "
                    "'sudo apt-get update' and 'sudo apt-get install -f'."
                    )
        dia = Gtk.MessageDialog(None, 0, Gtk.MessageType.ERROR,
                                Gtk.ButtonsType.OK, "")
        dia.set_markup("<b><big>%s</big></b>" % err_header)
        dia.format_secondary_text(err_body)
        dia.run()
        sys.exit(1)

    if options.non_interactive == True:
        if options.remove == True:
            app.on_button_remove_clicked(None)
        else:
            app.on_button_install_clicked(None)
        if options.auto_close == True:
            sys.exit()
    app.run()
