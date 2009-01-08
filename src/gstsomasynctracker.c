/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2008 Fabian Kloosterman <<user@hostname.org>>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-somasynctracker
 *
 * FIXME:Describe somasynctracker here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! somasynctracker ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstsomasynctracker.h"
#include "gstsomacameraevent.h"

GST_DEBUG_CATEGORY_EXTERN (gst_somatracker_debug);
#define GST_CAT_DEFAULT gst_somatracker_debug

static const GstElementDetails gst_soma_sync_tracker_details =
GST_ELEMENT_DETAILS ("Soma sync tracker",
    "Analyzer/Mixer/Video",
    "Synchronizes diode events and video stream, computes diode coordinates",
    "Fabian Kloosterman <fkloos@mit.edu>");


/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_THRESHOLD
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate videosink_factory = GST_STATIC_PAD_TEMPLATE ("video_sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw-gray")
    );

static GstStaticPadTemplate videosrc_factory = GST_STATIC_PAD_TEMPLATE ("video_src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw-gray")
    );

static GstStaticPadTemplate videobinsrc_factory = GST_STATIC_PAD_TEMPLATE ("bw_src",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("video/x-raw-gray")
    );

static GstStaticPadTemplate diodesink_factory = GST_STATIC_PAD_TEMPLATE ("diode_sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("soma/diode")
    );

static GstStaticPadTemplate positionsrc_factory = GST_STATIC_PAD_TEMPLATE ("pos_src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("soma/position")
    );

GST_BOILERPLATE (GstSomaSyncTracker, gst_soma_sync_tracker, GstElement,
    GST_TYPE_ELEMENT);

static void gst_soma_sync_tracker_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_soma_sync_tracker_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_soma_sync_tracker_set_caps (GstPad * pad, GstCaps * caps);
static GstCaps* gst_soma_sync_tracker_get_caps (GstPad * pad);

static void gst_soma_sync_tracker_finalize (GObject * object);
static GstStateChangeReturn gst_soma_sync_tracker_change_state (GstElement * element,
    GstStateChange transition);

static GstPad *gst_soma_sync_tracker_request_new_pad (GstElement * element,
						      GstPadTemplate * temp, const gchar * unused);

static GstFlowReturn gst_soma_sync_tracker_collected (GstCollectPads * pads,
						      gpointer data);

/* GObject vmethod implementations */

static void
gst_soma_sync_tracker_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&videosrc_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&videosink_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&videobinsrc_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&diodesink_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&positionsrc_factory));

  gst_element_class_set_details (element_class, &gst_soma_sync_tracker_details);


}

/* initialize the somasynctracker's class */
static void
gst_soma_sync_tracker_class_init (GstSomaSyncTrackerClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->finalize = gst_soma_sync_tracker_finalize;

  gstelement_class->change_state =
    GST_DEBUG_FUNCPTR (gst_soma_sync_tracker_change_state);

  gobject_class->set_property = gst_soma_sync_tracker_set_property;
  gobject_class->get_property = gst_soma_sync_tracker_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_THRESHOLD,
      g_param_spec_double ("threshold", "Threshold", "Threshold for binarize",
          MIN_THRESHOLD, MAX_THRESHOLD, DEFAULT_THRESHOLD, G_PARAM_READWRITE));

  gstelement_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_soma_sync_tracker_request_new_pad);


}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_soma_sync_tracker_init (GstSomaSyncTracker * filter,
    GstSomaSyncTrackerClass * gclass)
{

  //video sink pad
  filter->videosinkpad = gst_pad_new_from_static_template (&videosink_factory, "video_sink");
  gst_pad_set_setcaps_function (filter->videosinkpad,
                                GST_DEBUG_FUNCPTR(gst_soma_sync_tracker_set_caps));
  gst_pad_set_getcaps_function (filter->videosinkpad,
                                GST_DEBUG_FUNCPTR(gst_soma_sync_tracker_get_caps));

  //video src pad
  filter->videosrcpad = gst_pad_new_from_static_template (&videosrc_factory, "video_src");
  gst_pad_set_getcaps_function (filter->videosrcpad,
                                GST_DEBUG_FUNCPTR(gst_soma_sync_tracker_get_caps));

  //video bin src pad will be created upon request

  //diode sink pad
  filter->diodesinkpad = gst_pad_new_from_static_template (&diodesink_factory, "diode_sink");
  gst_pad_set_getcaps_function (filter->diodesinkpad,
                                GST_DEBUG_FUNCPTR(gst_soma_sync_tracker_get_caps));

  //position src pad
  filter->positionsrcpad = gst_pad_new_from_static_template (&positionsrc_factory, "pos_src");
  gst_pad_set_getcaps_function (filter->positionsrcpad,
                                GST_DEBUG_FUNCPTR(gst_soma_sync_tracker_get_caps));

  gst_element_add_pad (GST_ELEMENT (filter), filter->videosinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->videosrcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->diodesinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->positionsrcpad);

  filter->silent = FALSE;
  filter->threshold = DEFAULT_THRESHOLD;

  filter->collect = gst_collect_pads_new ();

  gst_collect_pads_set_function (filter->collect,
      GST_DEBUG_FUNCPTR (gst_soma_sync_tracker_collected), filter);

  filter->video_collect_data = gst_collect_pads_add_pad (filter->collect,
      filter->videosinkpad, sizeof (GstCollectData));
  filter->diode_collect_data = gst_collect_pads_add_pad (filter->collect,
        filter->diodesinkpad, sizeof (GstCollectData));

}

static void
gst_soma_sync_tracker_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSomaSyncTracker *filter = GST_SOMASYNCTRACKER (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    case PROP_THRESHOLD:
      filter->threshold = g_value_get_double(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_soma_sync_tracker_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSomaSyncTracker *filter = GST_SOMASYNCTRACKER (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    case PROP_THRESHOLD:
      g_value_set_double(value, filter->threshold);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles the link with other elements */
static gboolean
gst_soma_sync_tracker_set_caps (GstPad * pad, GstCaps * caps)
{
  GstSomaSyncTracker *filter;
  GstPad *otherpad1, *otherpad2;
  gboolean ret=TRUE;
  GstStructure *structure=NULL;

  filter = GST_SOMASYNCTRACKER (gst_pad_get_parent (pad));
  if (pad == filter->videosinkpad) {
    otherpad1 = filter->videosrcpad;
    otherpad2 = filter->videobinsrcpad;
  } else if (pad == filter->videosrcpad) {
    otherpad1 = filter->videosinkpad;
    otherpad2 = filter->videobinsrcpad;
  } else {
    otherpad1 = filter->videosrcpad;
    otherpad2 = filter->videosinkpad;
  }

  if (otherpad1)
    ret = ret && gst_pad_set_caps(otherpad1, caps);

  if (otherpad2)
    ret = ret && gst_pad_set_caps(otherpad2, caps);

  structure = gst_caps_get_structure ( caps, 0);
  gst_structure_get_int (structure, "width", &filter->width);
  gst_structure_get_int (structure, "height", &filter->height);

  GST_DEBUG_OBJECT(filter, "caps set: width=%d, height=%d",filter->width,filter->height);
  gst_object_unref (filter);


  return ret;
}

static GstCaps *
gst_soma_sync_tracker_get_caps (GstPad *pad)
{

  GstCaps *caps;

  //we can do what our template says we can do
  caps = gst_caps_copy( gst_pad_get_pad_template_caps (pad) );

  return caps;

}


static void
gst_soma_sync_tracker_finalize (GObject * object)
{
  GstSomaSyncTracker *filter = GST_SOMASYNCTRACKER (object);

  gst_object_unref (filter->collect);

  G_OBJECT_CLASS (parent_class)->finalize (object);

}


static GstPad *
gst_soma_sync_tracker_request_new_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * unused)
{

  GstPad *srcpad;
  GstSomaSyncTracker *filter = GST_SOMASYNCTRACKER (element);
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (element);

  g_return_val_if_fail (templ == gst_element_class_get_pad_template(klass,"bw_src"), NULL);

  GST_DEBUG_OBJECT( filter, "creating bw_src pad...");

  srcpad = gst_pad_new_from_template (templ, "bw_src");

  //  gst_pad_set_setcaps_function (srcpad,
  //    GST_DEBUG_FUNCPTR (gst_pad_proxy_setcaps));
  gst_pad_set_getcaps_function (srcpad,
      GST_DEBUG_FUNCPTR (gst_soma_sync_tracker_get_caps));
  gst_element_add_pad (GST_ELEMENT (filter), srcpad);

  filter->videobinsrcpad = srcpad;

  return srcpad;
}



static GstFlowReturn
gst_soma_sync_tracker_collected (GstCollectPads * pads, gpointer data)
{

  GstSomaSyncTracker *filter;
  GstBuffer *videobuffer, *diodebuffer, *binarybuffer;
  GstSomaDiode* diode;
  guint8 *img;
  gint sz;
  double threshold;
  gint x, y;
  guint64 sumx=0, sumy=0;
  GstBuffer *posbuffer = NULL;
  GstSomaPosition *pos=NULL;
  gint idx;
  guint32 count=0;

  filter = GST_SOMASYNCTRACKER (data);

  GST_DEBUG_OBJECT( filter, "collecting video and diode buffers...");

  //get buffers
  videobuffer = gst_collect_pads_pop( filter->collect, filter->video_collect_data );
  diodebuffer = gst_collect_pads_pop( filter->collect, filter->diode_collect_data );
  
  diode = (GstSomaDiode*) GST_BUFFER_DATA( diodebuffer );

  //set timestamp of video buffer
  GST_BUFFER_TIMESTAMP(videobuffer) = diode->timestamp*20000;
  
  //copy video buffer and make it writable
  binarybuffer = gst_buffer_copy( videobuffer );
  binarybuffer = gst_buffer_make_writable( binarybuffer );
 
  img = (guint8*) GST_BUFFER_DATA(binarybuffer);
  sz = GST_BUFFER_SIZE(binarybuffer);
  
  threshold = (double) (filter->threshold * 256.0);

  /* compute mean (x,y) of pixels above threshold, where
     1<=x<=width and 1<=y<=height, with x increasing from left to right
     and y increasing from bottom to top
     note that if no pixel is above threshold, (x,y)==(0,0) */
     
  for(x=1;x<=filter->width;x++) {

    idx = x-1;
    
    for (y=filter->height;y>=1;y--) {
      
      img[idx] = img[idx]>=threshold?255:0;
	
      if (img[idx]>0) {
	sumx += x;
	sumy += y;
	count++;
      }

      idx += filter->width;
      
      
    }
    
  }

  if (count>0) {
    sumx = sumx / count;
    sumy = sumy / count;
  }

  GST_DEBUG_OBJECT( filter, "computed position: %lld, %lld (n=%ld)", sumx, sumy, count);

  //if position src pad is linked, construct the buffer and push it out
  if (gst_pad_is_linked(filter->positionsrcpad)) {

    if ( gst_pad_alloc_buffer (filter->positionsrcpad, GST_BUFFER_OFFSET_NONE,
			       sizeof(GstSomaPosition), GST_PAD_CAPS (filter->positionsrcpad), &posbuffer) == GST_FLOW_OK ) {
    
      pos = (GstSomaPosition*) GST_BUFFER_DATA( posbuffer );
      pos->timestamp = diode->timestamp;
      pos->diode = diode->diode;
      pos->x = (guint16) sumx;
      pos->y = (guint16) sumy;

      //gst_buffer_copy_metadata( posbuffer, videobuffer, GST_BUFFER_COPY_TIMESTAMPS);
      GST_BUFFER_TIMESTAMP(posbuffer)=GST_BUFFER_TIMESTAMP(videobuffer);

      GST_DEBUG_OBJECT( filter, "pushing position buffer: timestamp %lld, duration: %lld", GST_BUFFER_TIMESTAMP(posbuffer), GST_BUFFER_DURATION(posbuffer));

      gst_pad_push (filter->positionsrcpad, posbuffer );
   

    }
  }

  //if videobin src pad exists and is linked, push out binarybuffer
  //otherwise unref it
  if (filter->videobinsrcpad && gst_pad_is_linked(filter->videobinsrcpad)) {

    gst_pad_push (filter->videobinsrcpad, binarybuffer );

    GST_DEBUG_OBJECT( filter, "pushing out binary video buffer...");

  } else {

    gst_buffer_unref( binarybuffer );

  }


  //if video src pad is linked, push out videobuffer
  if (gst_pad_is_linked(filter->videosrcpad)) {

    gst_pad_push(filter->videosrcpad, videobuffer);

    GST_DEBUG_OBJECT( filter, "pushing out video buffer...");

  } else {

    gst_buffer_unref( videobuffer );

  }

  //unref diode buffer
  gst_buffer_unref( diodebuffer );

  return GST_FLOW_OK;


}


static GstStateChangeReturn
gst_soma_sync_tracker_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstSomaSyncTracker *filter = GST_SOMASYNCTRACKER (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_collect_pads_start (filter->collect);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      /* need to unblock the collectpads before calling the
       * parent change_state so that streaming can finish */
      gst_collect_pads_stop (filter->collect);
      break;
    default:
      break;
  }

  ret = parent_class->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    default:
      break;
  }

  return ret;
}

