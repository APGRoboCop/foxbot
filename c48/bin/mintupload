#!/usr/bin/python2

import sys
import os
import time
import traceback

try:
    import gi
    gi.require_version("Gtk", "3.0")
    from gi.repository import Gtk, Gdk
except:
    print "You do not have all the dependencies!"
    sys.exit(1)

from mintupload_core import *

Gdk.threads_init()
__version__ = VERSION
# i18n
gettext.install("mintupload", "/usr/share/linuxmint/locale")

# Location of the UI file
UI_FILE = "/usr/share/linuxmint/mintupload/mintupload.ui"


def notify(message, timeout=3000):
    os.system("notify-send \"" + _("Upload Manager") + "\" \"" + message + "\" -t " + str(timeout))


class GtkUploader(MintUploader):

    '''Wrapper for the gtk management of MintUploader'''

    def __init__(self, service, files):
        MintUploader.__init__(self, service, files)
        Gdk.threads_enter()
        try:
            self.builder = Gtk.Builder()
            self.builder.add_from_file(UI_FILE)

            if len(filenames) > 1:
                title = _("%(percentage)s of %(number)d files - Uploading to %(service)s") % {'percentage': '0%', 'number': len(filenames), 'service': "\"" + service['name'] + "\""}
            else:
                title = _("%(percentage)s of 1 file - Uploading to %(service)s") % {'percentage': '0%', 'service': "\"" + service['name'] + "\""}

            self.builder.get_object("main_window").set_title(title)
            self.builder.get_object("main_window").set_icon_name(ICON)

            self.progressbar = self.builder.get_object("progressbar")

            self.builder.get_object("main_window").connect("destroy", self.close_window)
            self.builder.get_object("main_window").connect("delete_event", self.close_window)
            self.builder.get_object("cancel_button").connect("clicked", self.cancel)
        finally:
            Gdk.threads_leave()

    def run(self):

        Gdk.threads_enter()
        try:
            self.progressbar.show()
        finally:
            Gdk.threads_leave()

        # Calculate total size
        self.num_files_left = len(self.files)
        self.total_size = 0
        self.size_so_far = 0
        self.size_of_finished_files_so_far = 0
        self.percentage = 0
        self.cancel_required = False
        self.start_time = time.time()

        try:
            for f in self.files:
                self.total_size += os.path.getsize(f)

            for f in self.files:
                if self.cancel_required:
                    notify(_("The upload to '%(service)s' was cancelled") % {'service': service['name']})
                    Gtk.main_quit()
                    sys.exit(0)
                else:
                    # Upload 1 file
                    filename = os.path.split(f)[1]
                    self.upload(f)

                self.num_files_left -= 1
                self.size_of_finished_files_so_far += os.path.getsize(f)

        except Exception, e:
            notify((_("Upload to '%s' failed: ") % service['name']) + str(e))
            traceback.print_exc()
            Gtk.main_quit()
            sys.exit(0)

        if len(self.files) > 1:
            notify(_("Successfully uploaded %(number)d files to '%(service)s'") % {'number': len(self.files), 'service': service['name']})
        else:
            notify(_("Successfully uploaded 1 file to '%(service)s'") % {'service': service['name']})
            
        Gtk.main_quit()
        sys.exit(0)

    def close_window(self, widget=None, event=None):
        if self.cancel_required:
            self.builder.get_object("main_window").hide()
        else:
            self.builder_cancel = self.builder.get_object("close_dialog")
            self.builder_cancel.get_object("close_dialog").set_icon_name(ICON)
            self.builder_cancel.get_object("label_cancel").set_text(_("Do you want to cancel this upload?"))
            self.builder_cancel.get_object("cancel_button").set_label(_("Cancel"))
            self.builder_cancel.get_object("continue_button").set_label(_("Run in the background"))
            self.builder_cancel.get_object("cancel_button").connect("clicked", self.hide_window, True)
            self.builder_cancel.get_object("continue_button").connect("clicked", self.hide_window, False)
            self.builder_cancel.get_object("close_dialog").set_title(_("Cancel upload?"))
            self.builder_cancel.get_object("close_dialog").show()
        return True

    def hide_window(self, widget, cancel):
        self.builder_cancel.get_object("close_dialog").hide()
        if cancel:
            self.cancel(widget)
        self.builder.get_object("main_window").hide()

    def cancel(self, widget):
        self.cancel_required = True
        self.builder.get_object("cancel_button").set_sensitive(False)

    def progress(self, message, color=None):
        pass

    def pct(self, so_far, total=None):
        percentage = str(int(self.percentage * 100)) + "%"
        if self.num_files_left > 1:
            message = _("Uploading %(number)d files to %(service)s") % {'number': self.num_files_left, 'service': "\"" + service['name'] + "\""}
            title = _("%(percentage)s of %(number)d files - Uploading to %(service)s") % {'percentage': percentage, 'number': self.num_files_left, 'service': "\"" + service['name'] + "\""}
        else:
            message = _("Uploading 1 file to %(service)s") % {'service': "\"" + service['name'] + "\""}
            title = _("%(percentage)s of 1 file - Uploading to %(service)s") % {'percentage': percentage, 'service': "\"" + service['name'] + "\""}

        self.percentage = float(self.size_so_far) / float(self.total_size)
        Gdk.threads_enter()
        try:
            self.progressbar.set_fraction(self.percentage)
            self.progressbar.set_text(str(int(self.percentage * 100)) + "%")
            self.builder.get_object("upload_label").set_text(message)
            self.builder.get_object("main_window").set_title(title)
        finally:
            Gdk.threads_leave()
        pass

    def common_callback(self):
        self.pct(self.size_so_far, self.total_size)
        self.calculate_time()

        if self.speed > 0 and self.time_remaining > 0:
            message = _("%(size_so_far)s of %(total_size)s - %(time_remaining)s left (%(speed)s/sec)") % {'size_so_far': self.size_to_string(self.size_so_far, 1), 'total_size': self.size_to_string(self.total_size, 1), 'time_remaining': self.time_to_string(self.time_remaining), 'speed': self.size_to_string(self.speed, 1)}
        else:
            message = _("%(size_so_far)s of %(total_size)s") % {'size_so_far': self.size_to_string(self.size_so_far, 1), 'total_size': self.size_to_string(self.total_size, 1)}

        Gdk.threads_enter()
        try:
            self.builder.get_object("label_details").set_text(message)
        finally:
            Gdk.threads_leave()
        return

    def my_ftp_callback(self, buffer):
        self.size_so_far += len(buffer) - 1
        self.common_callback()
        return

    def my_sftp_callback(self, so_far, total=None):
        self.size_so_far = self.size_of_finished_files_so_far + so_far
        self.common_callback()
        return

    def success(self):
        pass

    def size_to_string(self, size, decimals):
        size = float(size)
        kilo = float(1024)
        mega = float(1024 * 1024)
        giga = float(1024 * 1024 * 1024)
        strSize = str(size) + _("B")
        if size >= kilo:
            strSize = str(round(size / kilo, decimals)) + _("KB")
        if size >= mega:
            strSize = str(round(size / mega, decimals)) + _("MB")
        if size >= giga:
            strSize = str(round(size / giga, decimals)) + _("GB")
        return strSize

    def time_to_string(self, time):
        hours, remainder = divmod(time, 3600)
        minutes, seconds = divmod(remainder, 60)
        if time > 7200:
            str = _("%(hours)d hours, %(minutes)d minutes") % {'hours': hours, 'minutes': minutes}
        elif time > 3600:
            str = _("1 hour, %d minutes") % minutes
        elif time > 120:
            str = _("%(minutes)d minutes, %(seconds)d seconds") % {'minutes': minutes, 'seconds': seconds}
        elif time > 60:
            str = _("1 minute, %d seconds") % seconds
        else:
            str = _("%d seconds") % seconds
        return str

    def calculate_time(self):
        self.size_remaining = self.total_size - self.size_so_far
        time_spent = time.time() - self.start_time
        if time_spent > 0:
            self.speed = float(self.size_so_far) / float(time_spent)
        else:
            self.speed = 0
        if self.speed > 0:
            self.time_remaining = float(self.size_remaining) / float(self.speed)
        else:
            self.time_remaining = 0

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print """Usage: mintupload service file [more files]"""
        exit(0)

    service_name = sys.argv[1]
    service = None
    known_services = read_services()
    for known_service in known_services:
        if known_service['name'] == service_name:
            service = known_service

    if service is None:
        print "Unknown service: " + service_name
        os.system("notify-send \"" + _("Unknown service: %s") % service_name + "\"")
    else:
        filenames = sys.argv[2:]

        uploader = GtkUploader(service, filenames)
        uploader.start()
        Gtk.main()
