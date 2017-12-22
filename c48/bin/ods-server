#!/usr/bin/python

import dbus
import dbus.decorators
import dbus.glib
import gobject
from optparse import OptionParser
import sys
from signal import *
import time

class ServerSession:
    
    def __init__(self, session_object_path):
        self.prefix = '>>'+session_object_path+'<< '
        self.bus = dbus.SessionBus()
        
        # get org.openobex.ServerSession object
        session_obj = self.bus.get_object('org.openobex', session_object_path)
        self.session = dbus.Interface(session_obj, 'org.openobex.ServerSession')
        
        # connect signals
        self.session.connect_to_signal('Disconnected', self.disconnected_cb)
        self.session.connect_to_signal('Cancelled', self.cancelled_cb)
        self.session.connect_to_signal('TransferStarted', self.transfer_started_cb)
        self.session.connect_to_signal('TransferProgress', self.transfer_progress_cb)
        self.session.connect_to_signal('TransferCompleted', self.transfer_completed_cb)
        self.session.connect_to_signal('ErrorOccurred', self.error_occurred_cb)
        self.session.connect_to_signal('RemoteDisplayRequested', self.remote_display_requested_cb)
    
    # emitted when session is disconnected
    def disconnected_cb(self):
        print self.prefix,
        print 'Disconnected'
    
    # emitted when transfer is cancelled
    def cancelled_cb(self):
        print self.prefix,
        print 'Transfer cancelled'

    # emitted when transfer begins
    def transfer_started_cb(self, filename, local_path, total_bytes):
        print self.prefix,
        print 'Transfer started (%s, %s, %d)' % (filename, local_path, total_bytes)
        self.total_bytes = total_bytes
        
        info = self.session.GetTransferInfo()
        print self.prefix, 'All transfer info:'
        for name,value in info.iteritems():
            print self.prefix, '--', name, '=', value
        
        if options.ask_to_accept:
            print "Accept file? Type 'a' to accept, 'r' to reject:"
            command = raw_input('>>> ')
            if command.strip() == 'a':
                print 'Accepting'
                self.session.Accept()
            else:
                print 'Rejecting'
                self.session.Reject()   
    
    # emitted constantly during transfer
    def transfer_progress_cb(self, bytes_transferred):
        print self.prefix,
        if self.total_bytes > 0:
            print 'Progress: %d %%' % int(float(bytes_transferred)/self.total_bytes*100)
        else:
            print 'Progress'
    
    # emitted when transfer is completed
    def transfer_completed_cb(self):
        print self.prefix,
        print 'Transfer completed'
    
    # emitted when error occurs (for instance link error)
    def error_occurred_cb(self, error_name, error_message):
        print self.prefix,
        print 'Error occurred: %s: %s' % (error_name, error_message)
        
    # emitted when BIP RemoteDisplay feature is requested
    def remote_display_requested_cb(self, img_filename):
   	    print self.prefix,
   	    print 'RemoteDisplay requested for image: ', img_filename


class Tester:
    
    def __init__(self, server_type, root_path, options):
        self.server_type = server_type
        self.root_path = root_path
        self.options = options
        
        # get bus
        if options.system_bus:
            self.bus = dbus.SystemBus()
        else:
            self.bus = dbus.SessionBus()
        
        # get org.openobex.Manager object
        manager_obj = self.bus.get_object('org.openobex', '/org/openobex')
        self.manager = dbus.Interface(manager_obj, 'org.openobex.Manager')
        
        # call Create{Bluetooth|Tty}Server with specified server type 
        # (opp - Object Push Profile,
        #  ftp - File Transfer Profile,
        #  bip - Basic Imaging Profile)
        # returns Server object path
        if options.tty_dev:
            server_path = self.manager.CreateTtyServer(options.tty_dev,
                                                       self.server_type)
        else:
            server_path = self.manager.CreateBluetoothServer(options.local_device,
                                             self.server_type, options.pairing)
        print 'Server object: ', server_path
        # get org.openobex.Server object
        server_obj = self.bus.get_object('org.openobex', server_path)
        self.server = dbus.Interface(server_obj, 'org.openobex.Server')

        # connect signals
        self.server.connect_to_signal('Started', self.started_cb)
        self.server.connect_to_signal('Stopped', self.stopped_cb)
        self.server.connect_to_signal('Closed', self.closed_cb)
        self.server.connect_to_signal('ErrorOccurred', self.error_occurred_cb)
        self.server.connect_to_signal('SessionCreated', self.session_created_cb)
        self.server.connect_to_signal('SessionRemoved', self.session_removed_cb)
        
        # require remote device to send thumbnails for BIP PutImage sessions
        self.server.SetOption('RequireImagingThumbnails', options.thumbnails)
        # start server with specified options
        self.server.Start(self.root_path, not options.readonly, not options.ask_to_accept)
        
        self.main_loop = gobject.MainLoop()
        self.main_loop.run()
    
    # emitted when Server is started
    def started_cb(self):
        print 'Started'
    
    # emitted when Server is stopped
    def stopped_cb(self):
        print 'Stopped'
    
    # emitted when Server is closed
    def closed_cb(self):
        print 'Closed'
    
    def error_occurred_cb(self, error_name, error_message):
        print 'Error occurred: %s: %s' % (error_name, error_message)
    
    # emitted when client connects to server (server session is established)    
    def session_created_cb(self, session_object_path):
        print 'Session created: %s' % session_object_path
	session_info = self.server.GetServerSessionInfo(session_object_path)
	print 'Session Bluetooth address: %s' % session_info['BluetoothAddress']
        session = ServerSession(session_object_path)
    
    # emitted when client disconnects
    def session_removed_cb(self, session_object_path):
        print 'Session removed: %s' % session_object_path

if __name__ == '__main__':
    gobject.threads_init()
    dbus.glib.init_threads()

    usage = 'Usage: '+sys.argv[0]+' [options] profile path'
    parser = OptionParser(usage)
    parser.add_option('-l', '--local', dest='local_device',
                      default='00:00:00:00:00:00',
                      help='ADDRESS of Bluetooth adapter to listen on. Default is 00:00:00:00:00:00',
                      metavar='ADDRESS')
    parser.add_option('-p', '--pairing', dest='pairing',
                      action='store_true', default=False,
                      help='Require remote devices to be paired before allowing them to connect. Disabled by default')
    parser.add_option('-y', '--tty', dest='tty_dev',
                      help='Listen on specified TTY device instead of Bluetooth. If TTY device is used, all Bluetooth options are ignored.',
                      metavar='TTY_DEV')
    parser.add_option('-s', '--system-bus', dest='system_bus',
                      action='store_true', default=False,
                      help='Search for obex-data-server on System bus instead of Session bus (use when ods is running in D-Bus System bus)')
    parser.add_option('-r', '--readonly', dest='readonly',
                      action='store_true', default=False,
                      help='Disallow any write operations. Allowed by default')
    parser.add_option('-t', '--thumbnails', dest='thumbnails',
                      action='store_true', default=False,
                      help='Require remote device to send thumbnails when using Imaging (BIP) server')
    parser.add_option('-a', '--ask-to-accept', dest='ask_to_accept',
                      action='store_true', default=False,
                      help='Prompt user to accept or reject every file. By default all files are accepted')
                      
    options, args = parser.parse_args()
    
    if len(args) != 2:
        print usage
        print
        print 'error: Wrong number of arguments'
        exit()
    
    tester = Tester(args[0], args[1], options)
    
