#!/usr/bin/env python

import pygst
pygst.require("0.10")
import gst, gobject

# class VideoTextOverlayPipeline
# class VideoPos2OggPipeline
# class Camera2SomaPipeline
# class LightTrackerPipeline
# class OldSomaTrackerPipeline

class TrackerCorePipeline(object):
    def __init__(self):
        
        pipe = gst.element_factory_make("pipeline", 'trackerPipeline')

        camera2Soma = Camera2SomaPipeline().getPipeline()
        videoTextOverlay = VideoTextOverlayPipeline().getPipeline()
        videoPos2Ogg = VideoPos2OggPipeline().getPipeline()
        videoOverlay = gst.element_factory_make('somavideooverlay', "somaOverlay")
        
        pipe.add(camera2Soma)
        pipe.add(videoTextOverlay)
        pipe.add(videoPos2Ogg)
        pipe.add(videoOverlaySwitch)
        pipe.add(colorSpace)
        
        posQ = gst.element_factory_make('queue', "posQ")                
        videoQ1 = gst.element_factory_make('queue', "videoQ1")                
        videoQ2 = gst.element_factory_make('queue', "videoQ2")

        pipe.add(posQ)
        pipe.add(videoQ1)
        pipe.add(videoQ2)

        posT = gst.element_factory_make('tee', "posT")
        videoT = gst.element_factory_make('tee', "vidoeT")
    
        pipe.add(posT)
        pipe.add(videoT)
    
        qA = gst.element_factory_make('queue', "qA")
        qB = gst.element_factory_make('queue', "qB")
        qC = gst.element_factory_make('queue', "qC")
        qD = gst.element_factory_make('queue', "qD")
            
        pipe.add(qA)
        pipe.add(qB)
        pipe.add(qC)
        pipe.add(qD)

        qX = gst.element_factory_make('queue', "qX")
        qY = gst.element_factory_make('queue', "qY")

        pipe.add(qX)
        pipe.add(qY)
        
        udpsink = gst.element_factory_make('udpsink')
        tcpclientsink = gst.element_factory_make('tcpclientsink')
        imageSink = gst.element_factory_make('xvimagesink')
        
        pipe.add(udpsink)
        pipe.add(tcpclientsink)
        pipe.add(imagesink)
        
        camera2Soma.link_pads('video_src', videoQ1, "sink")
        camera2Soma.link_pads('bw_src', videoQ2, "sink")
        camera2Soma.link_pads('pos_src', posQ, "sink")

        posQ.link(posT)
        posT.link(qA)
        posT.link(qB)
        posT.link(qC)
        posT.link(qD)

        videoQ1.link(videoT)
        videoQ2.link_pads('src', videoOverlay, "video_sink1")
        videoT.link(qX)
        videoT.link(qY)

        qA.link(udpsink)
        qB.link_pads("src", videoPos2Ogg, 'pos_sink')
        qC.link_pads("src", videoTextOverlay, 'pos_sink')
        qD.link_pads("src", videoOverlay, 'pos_sink')

        qX.link_pads("src", videoPos2Ogg, 'video_sink')
        qY.link_pads("src", videoOverlay, 'video_sink2')

        videoOverlay.link_pads('video_src', videoTextOverlay, "video_sink")
        videoTextOverlay.link(imageSink)

        self.pipline=pipe
        

    def getPipeline():
        return self.pipeline
        #link the Queues from the vidto T to Videopos2ogg and videooverlayswith
        #link video overaly switch to videotextoverlay
        #link videotext overlay to xvimagesink
        
        

class VideoTextOverlayPipeline(object):
    def __init__(self):
        pipe = gst.element_factory_make("pipeline", 'videotextoverlayPipeline')

        pos2text = gst.element_factory_make('somapos2text', "pos2text")
        color = gst.element_factory_make('ffmpegcolorspace', "color")
        q = gst.element_factory_make('queue', "q")
        clockO = gst.element_factory_make('clockoverlay', "clockO")
        clockO.set_property('halignment', "right")
        timeO = gst.element_factory_make('timeoverlay', "timeO")
        textO = gst.element_factory_make('textoverlay', "textO")

        pipe.add(pos2text)
        pipe.add(color)
        pipe.add(q)
        pipe.add(clockO)
        pipe.add(timeO)
        pipe.add(textO)

        gst.element_link_many(color, q, clockO, timeO, textO)
        gst.element_link_many(pos2text, textO)

        pos_sink = gst.GhostPad('pos_sink', pos2text.get_pad("sink"))
        video_sink = gst.GhostPad('video_sink', color.get_pad("sink"))

        video_src = gst.GhostPad('video_src', textO.get_pad("src"))
       
        pipe.add_pad(pos_sink)
        pipe.add_pad(video_sink)
        pipe.add_pad(video_src)
        self.pipeline = pipe

    def getPipeline(self):
        return self.pipeline


#Class up to date as of 08.12.02 according to docs
class VideoPos2OggPipeline(object):
    def __init__(self):
        pipe = gst.element_factory_make('pipeline', "vidpos2oggPipeline")
        
        pos2text = gst.element_factory_make('somapos2text', "pos2text")
        textFilt = gst.element_factory_make('capsfilter')
        textFilt.set_property('caps', gst.caps_from_string("text/x-cmml, encoded=False"))
        theora = gst.element_factory_make('theoraenc', "theora")
        oggmux = gst.element_factory_make('oggmux', "oggmux")
        
        pipe.add(pos2text)
        pipe.add(textFilt)
        pipe.add(theora)
        pipe.add(oggmux)
        
        gst.element_link_many(theora, oggmux)
        gst.element_link_many(pos2text, textFilt, oggmux) #<----- WHY IS THIS LINK FAILING!
    
        pos_sink = gst.GhostPad("pos_sink", pos2text.get_pad('sink'))
        vid_sink = gst.GhostPad('video_sink', theora.get_pad("sink"))

        ogg_src = gst.GhostPad("ogg_src", oggmux.get_pad('src'))

        pipe.add_pad(pos_sink)
        pipe.add_pad(vid_sink)
        pipe.add_pad(ogg_src)

        self.pipeline = pipe

    def getPipeline(self):
        return self.pipeline
        
#Class up to date as off 08.12.02 built according to pipeline in the docs
class Camera2SomaPipeline(object):
    def __init__(self):

        pipe = gst.element_factory_make('pipeline', "cam2soma_pipeline")
        
        cam = gst.element_factory_make('dc1394src', "camera")
        cfiltVid = gst.element_factory_make('capsfilter', "camera_capsfilter")
        cfiltVid.set_property('caps', gst.caps_from_string("video/x-raw-gray, width=640, height=480, framerate=100/1"))
        qVid = gst.element_factory_make('queue', "q_video")
        qVid.set_property("leaky", 2)
        qVid.set_property("max-size-buffers", 1)

        color = gst.element_factory_make('ffmpegcolorspace', "color")

        pipe.add(cam)
        pipe.add(cfiltVid)
        pipe.add(qVid)
        pipe.add(color)    

        gst.element_link_many(cam, cfiltVid, qVid, color)

        events = gst.element_factory_make('somaeventsource', "ses")
        cfiltEv = gst.element_factory_make('capsfilter', "events_capsfilter")
        cfiltEv.set_property('caps', gst.caps_from_string("soma/event, src=74"))
        camEv = gst.element_factory_make('somacameraevent', 'sce')
        qEv = gst.element_factory_make('queue', "q_events")
        qEv.set_property("leaky", 2)
        qEv.set_property("max-size-buffers", 1)

        pipe.add(events)
        pipe.add(cfiltEv)
        pipe.add(camEv)
        pipe.add(qEv)        

        gst.element_link_many(events, cfiltEv, camEv, qEv)

        sync = gst.element_factory_make('somasynctracker')

        pipe.add(sync)

        qEv.link_pads('src', sync, "diode_sink")
        color.link_pads('src', sync, "video_sink")

        posPad = gst.GhostPad("pos_src", sync.get_pad('pos_src'))
        bwPad = gst.GhostPad("bw_src", sync.get_pad('bw_src'))
        videoPad = gst.GhostPad("video_src", sync.get_pad('video_src'))

        pipe.add_pad(posPad)
        pipe.add_pad(bwPad)
        pipe.add_pad(videoPad)
        
        self.pipeline = pipe 

    def getPipeline(self):
        return self.pipeline

class LightTrackerPipeline(object):
    def __init__(self):

        self.pipe = gst.element_factory_make("pipeline", 'somaPipeline')

        videoSrc = gst.element_factory_make("dc1394src", 'videoSrc')

        capsfilt = gst.element_factory_make("capsfilter")
        caps = gst.caps_from_string("video/x-raw-gray,  width=640, height=480, framerate=100/1")
        capsfilt.set_property('caps', caps)

        color = gst.element_factory_make("ffmpegcolorspace")

#        capsfilt2 = gst.element_factory_make('capsfilter')
#        caps = gst.caps_from_string('video/x-raw-yuv')
#        capsfilt2.set_property('caps', caps)

#        filter = gst.element_factory_make("hqdn3d", 'filt')

        theora = gst.element_factory_make("theoraenc")

        ogg = gst.element_factory_make("oggmux")

        tcp = gst.element_factory_make("tcpclientsink")
        tcp.set_property('sync', False)
        tcp.set_property("host", 'localhost')
        tcp.set_property("port", 7000)

        self.pipe.add(videoSrc)
        self.pipe.add(capsfilt)
        self.pipe.add(color)
#        self.pipe.add(capsfilt2)
 #       self.pipe.add(filter)
        self.pipe.add(theora)
        self.pipe.add(ogg)
        self.pipe.add(tcp)

  #      gst.element_link_many(videoSrc, capsfilt, color, capsfilt2, filter, theora, ogg, tcp)
        gst.element_link_many(videoSrc, capsfilt, color, theora, ogg, tcp)

    def getTrackerPipeline(self):
        return self.pipe



class OldSomaTrackerPipeline(object):
    def __init__(self):

        self.pipe = gst.element_factory_make("pipeline", 'somaPipeline')
        eventSrc = gst.element_factory_make("somaeventsource", 'somaES')
        self.pipe.add(eventSrc)
	
        eventFilt = gst.element_factory_make("capsfilter", 'eventFilt')
        eventCaps = gst.caps_from_string("soma/event,src=74")
        eventFilt.set_property("caps", eventCaps)
        self.pipe.add(eventFilt)

        camEvent = gst.element_factory_make("somacameraevent", 'somaCE')
        self.pipe.add(camEvent)	

        eventQ = gst.element_factory_make("queue", 'eventQ')
        eventQ.set_property("leaky", 2)
        eventQ.set_property("max-size-buffers", 1)
        self.pipe.add(eventQ)        

        #link all previously created (event related) elements to each other
        gst.element_link_many(eventSrc, eventFilt, camEvent, eventQ)
        
        cam = gst.element_factory_make("dc1394src", 'videoSrc')
        self.pipe.add(cam)

        camFilt = gst.element_factory_make("capsfilter", 'camFilt')
        camCaps = gst.caps_from_string("video/x-raw-gray, width=640, height=480, framerate=100/1")
        camFilt.set_property('caps', camCaps)
        self.pipe.add(camFilt)

        camQ = gst.element_factory_make("queue", 'cameraQ')
        camQ.set_property("leaky", 2)
        camQ.set_property("max-size-buffers", 1)
        self.pipe.add(camQ)

        cam2Color = gst.element_factory_make("ffmpegcolorspace", 'camera2color')
        self.pipe.add(cam2Color)        

        #link all previously created (video related) elements to each other
        gst.element_link_many(cam, camFilt, camQ, cam2Color)

        #create main SomaSyncTracker Element --------!
        somaTracker = gst.element_factory_make("somasynctracker", 'somaTracker')
        self.pipe.add(somaTracker)

        #link event and video streams to somaTracker
        eventQ.link_pads('src', somaTracker, "diode_sink")
        cam2Color.link_pads('src', somaTracker, "video_sink")
        

        videoColor = gst.element_factory_make('ffmpegcolorspace', "somaColor")
        self.pipe.add(videoColor)
        videoQ = gst.element_factory_make('queue', "videoQ")
        self.pipe.add(videoQ)
#        highFreqFilt = gst.element_factory_make('hqdn3d', "highFreqFilt")
#        self.pipe.add(highFreqFilt)
        clockOver = gst.element_factory_make('clockoverlay', "clockOver")
        clockOver.set_property('halignment', "right")
        self.pipe.add(clockOver)
        timeOver = gst.element_factory_make('timeoverlay', "timeOver")
        self.pipe.add(timeOver)
        #connect all these elements
#        gst.element_link_many(videoColor, videoQ, highFreqFilt, clockOver, timeOver)
        gst.element_link_many(videoColor, videoQ, clockOver, timeOver)   #use to remove hqdn3d filt
       #connect this to somaTracker
        somaTracker.link_pads('video_src', videoColor, "sink")
                    
        posQ_0 = gst.element_factory_make('queue', "posQ_0")
        self.pipe.add(posQ_0)
        
        t = gst.element_factory_make("tee", 't')
        self.pipe.add(t)

        posQ_1 = gst.element_factory_make('queue', "posQ_1")
        self.pipe.add(posQ_1)
        pos2text = gst.element_factory_make('somapos2text', "pos2text")
        self.pipe.add(pos2text)
         #connect all these elements
        gst.element_link_many(posQ_0, t, posQ_1, pos2text)
        #connect this stream to somaTracker
        somaTracker.link_pads('pos_src', posQ_0, "sink")        


        textOver = gst.element_factory_make ('textoverlay', "textOver")
        self.pipe.add(textOver)
        #link two (text, video) into the text overlay
        gst.element_link_many(pos2text, textOver)
        gst.element_link_many(timeOver, textOver)
#        posQ_1.link_pads('src', textOver, "text_sink")
#        time.link_pads('src', textOver, "video_sink")
        imageSink = gst.element_factory_make('xvimagesink')
        self.pipe.add(imageSink)
        imageSink.set_property("sync", False)
        gst.element_link_many(textOver,imageSink)
        
        
    def getTrackerPipeline(self):
        return self.pipe

if __name__=="__main__":

    print "Starting"
    tracker = TrackerCorePipeline()
    pipeline = tracker.getPipeline()
    pipeline.set_state(gst.STATE_PLAYING)

    loop = gobject.MainLoop()
    
    try:
        loop.run()
    except:
        loop.quit()
    
    pipeline.set_state(gst.STATE_NULL)

