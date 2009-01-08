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
 * SECTION:element-somacameraevent
 *
 * FIXME:Describe somacameraevent here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! somacameraevent ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <time.h>
#include <sys/time.h>

#include <somanetevent/netevent.h>

#include "gstsomacameraevent.h"


GST_DEBUG_CATEGORY_EXTERN (gst_somatracker_debug);
#define GST_CAT_DEFAULT gst_somatracker_debug

static const GstElementDetails gst_soma_camera_event_details =
GST_ELEMENT_DETAILS ("Soma camera event analyzer",
    "Analyzer/SomaEvents",
    "Extracts diode and camera events from soma event stream",
    "Fabian Kloosterman <fkloos@mit.edu>");

#define CAMERA 1
#define DIODE0 2
#define DIODE1 4


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
  PROP_EVTCMD
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("soma/event")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("soma/diode")
    );

GST_BOILERPLATE (GstSomaCameraEvent, gst_soma_camera_event, GstElement,
    GST_TYPE_ELEMENT);

static void gst_soma_camera_event_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_soma_camera_event_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

//static gboolean gst_soma_camera_event_set_caps (GstPad * pad, GstCaps * caps);
static GstCaps* gst_soma_camera_event_get_caps (GstPad * pad);
static GstFlowReturn gst_soma_camera_event_chain (GstPad * pad, GstBuffer * buf);

/* GObject vmethod implementations */

static void
gst_soma_camera_event_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));

  gst_element_class_set_details (element_class, &gst_soma_camera_event_details);

}

/* initialize the somacameraevent's class */
static void
gst_soma_camera_event_class_init (GstSomaCameraEventClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_soma_camera_event_set_property;
  gobject_class->get_property = gst_soma_camera_event_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_EVTCMD,
      g_param_spec_int ("evtcmd", "EventCommand", "Event command code",
			  0, 255, 0, G_PARAM_READWRITE));

}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_soma_camera_event_init (GstSomaCameraEvent * filter,
    GstSomaCameraEventClass * gclass)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  //gst_pad_set_setcaps_function (filter->sinkpad,
  //                              GST_DEBUG_FUNCPTR(gst_soma_camera_event_set_caps));
  gst_pad_set_getcaps_function (filter->sinkpad,
                                GST_DEBUG_FUNCPTR(gst_soma_camera_event_get_caps));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_soma_camera_event_chain));

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  gst_pad_set_getcaps_function (filter->srcpad,
                                GST_DEBUG_FUNCPTR(gst_soma_camera_event_get_caps));

  //gst_pad_use_fixed_caps( filter->srcpad );
  //gst_pad_set_caps (filter->srcpad, gst_caps_new_simple ("soma/diode",NULL) );

  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = FALSE;
  filter->evtcmd = 0x30;
  filter->prevstate = 0;
  filter->prevdiode = -1;
  filter->currentdiode = -1;
  filter->got_camera = FALSE;
  filter->got_diode = FALSE;

}

static void
gst_soma_camera_event_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSomaCameraEvent *filter = GST_SOMACAMERAEVENT (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    case PROP_EVTCMD:
      filter->evtcmd = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_soma_camera_event_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSomaCameraEvent *filter = GST_SOMACAMERAEVENT (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    case PROP_EVTCMD:
      g_value_set_int (value, filter->evtcmd);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}



static GstCaps *
gst_soma_camera_event_get_caps (GstPad *pad)
{

  GstCaps *caps;

  //we can do what our template says we can do
  caps = gst_caps_copy( gst_pad_get_pad_template_caps (pad) );

  return caps;

}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_soma_camera_event_chain (GstPad * pad, GstBuffer * buf)
{
  GstSomaCameraEvent *filter;
  GstFlowReturn ret=GST_FLOW_OK;
  GstBuffer *outbuf = NULL;
  GstSomaDiode *diode;
  struct event_t *events;
  int nevents=0, i;
  guint64 timestamp=0;
  guint16 delta_on, delta_off;
  //struct timeval now;
  //int ts;

  filter = GST_SOMACAMERAEVENT (GST_OBJECT_PARENT (pad));

  events = (struct event_t*) GST_BUFFER_DATA(buf);
  nevents = GST_BUFFER_SIZE(buf) / sizeof(struct event_t);

  GST_DEBUG_OBJECT(filter, "Received %d soma events", nevents);

  for (i=0; i<nevents; i++) {

    if (events[i].cmd==filter->evtcmd) {

      delta_on = (~filter->prevstate) & events[i].data[1];
      delta_off = filter->prevstate & (~events[i].data[1]);

      GST_DEBUG_OBJECT(filter, "digital out event: %d (was: %d)", events[i].data[1]);

      filter->prevstate = events[i].data[1];

      if (delta_on & CAMERA) {

	if (filter->got_camera) {
	  //error?
	  GST_WARNING("received a camera on event, but we haven't pushed out the previous buffer yet\n");
	}

	//retrieve timestamp
	timestamp = events[i].data[2];
	timestamp = (timestamp << 16) | events[i].data[3];
	timestamp = (timestamp << 16) | events[i].data[4];

	//now = time(NULL);

	//ts = gettimeofday(&now,NULL);

	//guint64 x=now.tv_sec;
	//x=x*1000000;
	//x=x+now.tv_usec;

	//g_print("timestamp: %lld, wall clock: %lld\n", timestamp, x - filter->prevwallclock);

	//filter->prevwallclock = x;

	//save timestamp
	filter->currenttimestamp = timestamp;

	//
	filter->got_camera = TRUE;

      }

      if (delta_on & DIODE0) {

	if (filter->got_diode) {
	  GST_WARNING("received a diode0 on event, but we haven't pushed out the previous buffer yet\n");
	}

	if (filter->prevdiode==0) {
	  GST_WARNING("received two diode0 on events in a row");
	}

	filter->currentdiode = 0;
	filter->got_diode = TRUE;

      }


      if (delta_on & DIODE1) {

	if (filter->got_diode) {
	  GST_WARNING("received a diode1 on event, but we haven't pushed out the previous buffer yet\n");
	}

	if (filter->prevdiode==1) {
	  GST_WARNING("received two diode0 on events in a row");
	}

	filter->currentdiode = 1;
	filter->got_diode = TRUE;

      }
	
    }

    if (filter->got_camera && filter->got_diode) {
      
      //create output buffer
      ret = gst_pad_alloc_buffer (filter->srcpad, GST_BUFFER_OFFSET_NONE,
				  sizeof(GstSomaDiode), GST_PAD_CAPS (filter->srcpad), &outbuf);
      
      
      diode = (GstSomaDiode*) GST_BUFFER_DATA( outbuf );
      diode->timestamp = filter->currenttimestamp;
      diode->diode = filter->currentdiode;
      
      filter->got_camera = FALSE;
      filter->got_diode = FALSE;
      
      filter->prevdiode = diode->diode;
      
      gst_buffer_copy_metadata( outbuf, buf, GST_BUFFER_COPY_TIMESTAMPS );

      //push out buffer
      GST_DEBUG_OBJECT(filter, "pushing diode buffer: timestamp=%llu, diode=%d", diode->timestamp, diode->diode);
      ret = gst_pad_push( filter->srcpad, outbuf );
      
      
    }
    
  }

  return ret;

}


