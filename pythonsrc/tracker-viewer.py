#p!/usr/bin/env python
 
import pygst, pygtk
pygst.require('0.10')
pygtk.require('2.0')
import gst, gtk, gtk.glade, dbus, dbus.glib, gobject
from somaTrackerViewerPipeline import SomaTrackerViewerPipeline
 
class TrackerViewer():
    def __init__(self):
        
        self.pipe = SomaTrackerViewerPipeline().getTrackerPipeline()

        self.bus = self.pipe.get_bus()
        self.bus.add_signal_watch()
        self.bus.enable_sync_message_emission()
        self.bus.connect("message", self.on_message)
        self.bus.connect("sync-message::element", self.on_sync_message)

        self.gladeFile = 'trackerViewer.glade'
        windowName = "mainWindow"
        self.wTree = gtk.glade.XML(self.gladeFile, windowName)

        self.window = self.wTree.get_widget(windowName)
        
        self.window.show()

        if (self.window):
            self.window.connect("destroy", self.quit)

        dic = { 
                "on_thold_scale_value_changed"  : self.thold_change,
                "on_thold_lock_btn_toggled"     : self.thold_lock_toggle,
                "on_playpause_btn_toggled"      : self.play_pause,
                "on_reset_trail_btn_clicked"    : self.reset_trail,
                "on_pos_overlay_toggled"        : self.overlay_toggled,
                "on_enable_trail_toggled"       : self.trail_toggled,
              }

        self.wTree.signal_autoconnect(dic)
        
        dBus = dbus.SessionBus()
        self.tracker = dBus.get_object('soma.tracker.TrackerCore', "/SomaTracker")

    
    def on_message(self, bus, message):
#        print message
         return

    def on_sync_message(self, bus, message):
        if message.structure is None:
            return
        message_name = message.structure.get_name()
        if message_name == "prepare-xwindow-id":
            print "\tAsking for xwindow-id"
            drawArea = self.wTree.get_widget("draw_area")
            imagesink = message.src
            imagesink.set_property("force-aspect-ratio", True)
            imagesink.set_xwindow_id(drawArea.window.xid)


## --- GTK METHODS --- ##
    def quit(self, *args):
        print "Quitting, shutting down tracker-core"
        self.tracker.kill_tracker_core()
        gtk.main_quit(*args)        

    def thold_change(self, widget):
        thold = widget.get_value()
        print "New threshold selected:", thold

    def thold_lock_toggle(self, widget):
        print "Threshold Lock:", widget.get_active()
        self.wTree.get_widget('thold_scale').set_sensitive(not(widget.get_active()))

    def play_pause(self, widget):
        active = widget.get_active()
        print "Playing: ", active
        if active:
            widget.set_label("gtk-media-pause")
            print "\tStarting VIEWER pipeline"
            self.pipe.set_state(gst.STATE_PLAYING)
            self.tracker.start_tracker()
            
        else:
            widget.set_label("gtk-media-play")
            self.tracker.stop_tracker()
            self.pipe.set_state(gst.STATE_NULL)


    def reset_trail(self, widget):
        print "Reset Trail overlay trail"

    def overlay_toggled(self, widget):
        active = widget.get_active()
        print "Enable Overlay: ", active
        self.wTree.get_widget('enable_trail').set_sensitive(active)
        if not(active):
            self.wTree.get_widget('reset_trail_btn').set_sensitive(active)
        else:
            self.wTree.get_widget('reset_trail_btn').set_sensitive(self.wTree.get_widget("enable_trail").get_active())
            
    def trail_toggled(self, widget):
        active = widget.get_active()
        print "Enable Trails: ", active
        self.wTree.get_widget('reset_trail_btn').set_sensitive(active)
        
        




if __name__=="__main__":
    viewer = TrackerViewer()    
    gtk.main()    
