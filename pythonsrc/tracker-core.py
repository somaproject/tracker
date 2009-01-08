#!/usr/bin/env python
 
import pygst
pygst.require('0.10')
import gst

import dbus, dbus.service, dbus.glib, gobject
from somaTrackerPipeline import SomaTrackerPipeline
from somaLightTrackerPipeline import lightTrackerPipeline
 
class TrackerCore(dbus.service.Object):
    def __init__(self, loop,bus_name, object_path="/SomaTracker"):
        dbus.service.Object.__init__(self, bus_name, object_path)
        self.pipeline = lightTrackerPipeline().getTrackerPipeline()
        
        self.loop = loop

#        self.thold = self.pipeline.get_by_name('somaTracker').get_property("threshold")
    

    @dbus.service.method("soma.tracker.TrackerCore")
    def start_tracker(self):
        print "Starting the pipeline"
        if not hasattr(self,'pipeline'):
            self.pipeline = lightTrackerPipeline().getTrackerPipeline()

        self.pipeline.set_state(gst.STATE_PLAYING)
                
    @dbus.service.method("soma.tracker.TrackerCore")
    def stop_tracker(self):
        print "Stoping the pipeline"
        vid = self.pipeline.get_by_name("videoSrc")
        self.pipeline.set_state(gst.STATE_NULL)
        del self.pipeline

    @dbus.service.method("soma.tracker.TrackerCore")
    def kill_tracker_core(self):
        print "Killing mainloop"
        self.loop.quit()
        if hasattr(self, 'pipeline'):
            self.pipeline.set_state(gst.STATE_NULL)

    
    @dbus.service.method("soma.tracker.TrackerCore")
    def get_threshold(self):
        return self.pipeline.get_by_name('somaTracker').get_property("threshold")

    @dbus.service.method("soma.tracker.TrackerCore", in_signature='d', out_signature='b')
    def set_threshold(self, thold):
        if thold>1 or thold<0:
            return False
#        self.pipeline.get_by_name('somaTracker').set_property("threshold", thold)
        self.threshold_changed("tracker-threshold-changed",thold)
        return True

    @dbus.service.signal("soma.tracker.TrackerCore", signature='sd')
    def threshold_changed(self, msg, thold):
        print msg, " to:", thold    

if __name__=="__main__":

    session_bus = dbus.SessionBus()
    bus = dbus.service.BusName("soma.tracker.TrackerCore", bus=session_bus)
    
    loop = gobject.MainLoop()
    tracker = TrackerCore(loop, bus)


    loop.run()


