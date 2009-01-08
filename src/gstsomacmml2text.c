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
 * SECTION:element-somacmml2text
 *
 * FIXME:Describe somacmml2text here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! somacmml2text ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <string.h>

#include "gstsomacmml2text.h"

GST_DEBUG_CATEGORY_EXTERN (gst_somatracker_debug);
#define GST_CAT_DEFAULT gst_somatracker_debug

static const GstElementDetails gst_soma_cmml2text_details =
GST_ELEMENT_DETAILS ("Soma cmml to text",
    "Filter/Text",
    "Converts an non-encoded cmml stream to text",
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
  PROP_FMT
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("text/x-cmml, encoded=false")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("text/plain")
    );

GST_BOILERPLATE (GstSomaCmml2Text, gst_soma_cmml2text, GstElement,
    GST_TYPE_ELEMENT);

static void gst_soma_cmml2text_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_soma_cmml2text_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

//static gboolean gst_soma_cmml2text_set_caps (GstPad * pad, GstCaps * caps);
static GstCaps* gst_soma_cmml2text_get_caps (GstPad * pad);
static GstFlowReturn gst_soma_cmml2text_chain (GstPad * pad, GstBuffer * buf);

/* GObject vmethod implementations */

static void
gst_soma_cmml2text_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));

  gst_element_class_set_details (element_class, &gst_soma_cmml2text_details);

}

/* initialize the somacmml2text's class */
static void
gst_soma_cmml2text_class_init (GstSomaCmml2TextClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_soma_cmml2text_set_property;
  gobject_class->get_property = gst_soma_cmml2text_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FMT,
      g_param_spec_string ("format", "Format", "Text formatting string",
          "", G_PARAM_READWRITE));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_soma_cmml2text_init (GstSomaCmml2Text * filter,
    GstSomaCmml2TextClass * gclass)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  //gst_pad_set_setcaps_function (filter->sinkpad,
  //                              GST_DEBUG_FUNCPTR(gst_soma_cmml2text_set_caps));
  gst_pad_set_getcaps_function (filter->sinkpad,
                                GST_DEBUG_FUNCPTR(gst_soma_cmml2text_get_caps));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_soma_cmml2text_chain));

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  gst_pad_set_getcaps_function (filter->srcpad,
                                GST_DEBUG_FUNCPTR(gst_soma_cmml2text_get_caps));

  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);
  filter->silent = FALSE;

}

static void
gst_soma_cmml2text_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSomaCmml2Text *filter = GST_SOMACMML2TEXT (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    case PROP_FMT:
      if (filter->fmt!=NULL)
	g_free(filter->fmt);

      filter->fmt = g_strdup(g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_soma_cmml2text_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSomaCmml2Text *filter = GST_SOMACMML2TEXT (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    case PROP_FMT:
      g_value_set_string(value, filter->fmt);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static GstCaps *
gst_soma_cmml2text_get_caps (GstPad *pad)
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
gst_soma_cmml2text_chain (GstPad * pad, GstBuffer * buf)
{
  GstSomaCmml2Text *filter;
  char *pstr, *pstr2;
  char timestr[20];
  guint64 oldtimestamp;
  guint64 timestamp;
  guint hh,mm,ss,ms;
  char txt[256];
  GstBuffer *txtbuffer;
  char *ptxt;

  filter = GST_SOMACMML2TEXT (GST_OBJECT_PARENT (pad));

  //change caps from text/cmml (encoded=false) to text/plain
  gst_buffer_set_caps( buf, GST_PAD_CAPS( filter->srcpad ) );

  //if we don't have a format string, just push the cmml clip
  if (filter->fmt==NULL)
    return gst_pad_push(filter->srcpad, buf);

  //FIX ME:
  //otherwise parse the metadata in the clip
  //replace tokens in format string with metadata values
  //create a new buffer
  //set timestamp of new buffer
  //unref incoming buffer
  //push out new buffer

  pstr = strstr( (char*) GST_BUFFER_DATA(buf), "timestamp" );

  pstr+=20;
  pstr2 = strstr( pstr, "\"" );
  memcpy( timestr, pstr, pstr2-pstr );
  timestamp = (guint64) atoll(timestr);
  oldtimestamp = GST_BUFFER_TIMESTAMP(buf);
  GST_BUFFER_TIMESTAMP(buf) = timestamp*20000;
  
  hh = (guint) (timestamp / (50000 * 60 * 60));
  mm = (guint) ((timestamp / (50000 * 60)) % 60);
  ss = (guint) ((timestamp / 50000) % 60);
  ms = (guint) ((timestamp % 50000) / 50 );
  
  g_print("cmml buffer timestamp: %llu (old:%llu) %s\n", GST_BUFFER_TIMESTAMP(buf), oldtimestamp, timestr);
  
  sprintf(txt, "%02d:%02d:%02d.%03d", hh,mm,ss,ms);
  
  gst_pad_alloc_buffer(filter->srcpad, GST_BUFFER_OFFSET_NONE, strlen(txt), GST_PAD_CAPS(filter->srcpad), &txtbuffer);
  ptxt = (char*) GST_BUFFER_DATA(txtbuffer);
  memcpy( ptxt, txt, strlen(txt) );
  
  gst_buffer_copy_metadata( txtbuffer, buf, GST_BUFFER_COPY_TIMESTAMPS );  
  GST_BUFFER_TIMESTAMP(txtbuffer) = timestamp*20000;

  gst_buffer_unref(buf);

  /* just push out the incoming buffer without touching it */
  return gst_pad_push (filter->srcpad, txtbuffer);
}


