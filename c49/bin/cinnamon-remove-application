#!/usr/bin/python2

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

try:
    import sys
    import string
    import os
    import commands
    import threading
    import tempfile
    import gettext

except Exception, detail:
    print detail
    sys.exit(1)

from subprocess import Popen

# i18n
gettext.install("mint-common", "/usr/share/linuxmint/locale")


class RemoveExecuter(threading.Thread):

    def __init__(self, packages):
        threading.Thread.__init__(self)
        self.packages = packages

    def run(self):
        cmd = ["sudo", "/usr/sbin/synaptic", "--hide-main-window",
               "--non-interactive"]
        cmd.append("--progress-str")
        cmd.append("\"" + _("Please wait, this can take some time") + "\"")
        cmd.append("--finish-str")
        cmd.append("\"" + _("Application removed successfully") + "\"")
        f = tempfile.NamedTemporaryFile()
        strbuffer = ""
        for pkg in self.packages:
            strbuffer = strbuffer + "%s\tdeinstall\n" % pkg
        f.write(strbuffer)
        f.flush()
        cmd.append("--set-selections-file")
        cmd.append("%s" % f.name)
        comnd = Popen(cmd)
        returnCode = comnd.wait()
        f.close()
        sys.exit(0)

class MintRemoveWindow:

    def __init__(self, desktopFile):
        self.desktopFile = desktopFile
        (status, output) = commands.getstatusoutput("dpkg -S " + self.desktopFile)
        package = output[:output.find(":")].split(",")[0]
        if status != 0:
            warnDlg = Gtk.MessageDialog(None, 0, Gtk.MessageType.WARNING, Gtk.ButtonsType.YES_NO, _("This menu item is not associated to any package. Do you want to remove it from the menu anyway?"))
            warnDlg.vbox.set_spacing(10)
            response = warnDlg.run()
            if response == Gtk.ResponseType.YES:
                print "removing '%s'" % self.desktopFile
                os.system("rm -f '%s'" % self.desktopFile)
                os.system("rm -f '%s.desktop'" % self.desktopFile)
            warnDlg.destroy()
            sys.exit(0)

        warnDlg = Gtk.MessageDialog(None, 0, Gtk.MessageType.WARNING, Gtk.ButtonsType.OK_CANCEL, _("The following packages will be removed:"))
        warnDlg.vbox.set_spacing(10)

        treeview = Gtk.TreeView()
        column1 = Gtk.TreeViewColumn(_("Packages to be removed"))
        renderer = Gtk.CellRendererText()
        column1.pack_start(renderer, False)
        column1.add_attribute(renderer, "text", 0)
        treeview.append_column(column1)

        packages = []
        model = Gtk.ListStore(str)
        dependenciesString = commands.getoutput("apt-get -s -q remove " + package + " | grep Remv")
        dependencies = string.split(dependenciesString, "\n")
        for dependency in dependencies:
            dependency = dependency.replace("Remv ", "")
            model.append([dependency])
            packages.append(dependency.split()[0])
        treeview.set_model(model)
        treeview.show()

        scrolledwindow = Gtk.ScrolledWindow()
        scrolledwindow.set_shadow_type(Gtk.ShadowType.ETCHED_OUT)
        scrolledwindow.set_size_request(150, 150)
        scrolledwindow.add(treeview)
        scrolledwindow.show()

        warnDlg.get_content_area().add(scrolledwindow)

        response = warnDlg.run()
        if response == Gtk.ResponseType.OK:
            executer = RemoveExecuter(packages)
            executer.start()
        elif response == Gtk.ResponseType.CANCEL:
            sys.exit(0)
        warnDlg.destroy()

if __name__ == "__main__":

    # Exit if the given path does not exist
    if len(sys.argv) < 2 or not os.path.exists(sys.argv[1]):
        sys.exit(1)

    mainwin = MintRemoveWindow(sys.argv[1])
    Gtk.main()
