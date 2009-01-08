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
 * SECTION:element-somapos2text
 *
 * FIXME:Describe somapos2text here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! somapos2text ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "gstsomapos2text.h"
#include "gstsomasynctracker.h"
#include "utils.h"

GST_DEBUG_CATEGORY_EXTERN (gst_somatracker_debug);
#define GST_CAT_DEFAULT gst_somatracker_debug

static const GstElementDetails gst_soma_pos2text_details =
GST_ELEMENT_DETAILS ("Soma position to text",
    "Filter/Text/Position",
    "Converts diode position data to text",
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
    GST_STATIC_CAPS ("soma/position")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("text/x-pango-markup; text/plain; text/x-cmml, encoded=false")
    );

GST_BOILERPLATE (GstSomaPos2Text, gst_soma_pos2text, GstElement,
    GST_TYPE_ELEMENT);

static void gst_soma_pos2text_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_soma_pos2text_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_soma_pos2text_src_set_caps (GstPad * pad, GstCaps * caps);
static GstCaps * gst_soma_pos2text_get_caps (GstPad * pads);
static GstFlowReturn gst_soma_pos2text_chain (GstPad * pad, GstBuffer * buf);

static GstStateChangeReturn
gst_soma_pos2text_change_state (GstElement * element, GstStateChange transition); 


/* GObject vmethod implementations */

static void
gst_soma_pos2text_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));

  gst_element_class_set_details (element_class, &gst_soma_pos2text_details);

}

/* initialize the somapos2text's class */
static void
gst_soma_pos2text_class_init (GstSomaPos2TextClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_soma_pos2text_set_property;
  gobject_class->get_property = gst_soma_pos2text_get_property;

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_soma_pos2text_change_state);

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FMT,
				   g_param_spec_string ("format", "Format", "Text formatting string",
							"", G_PARAM_READWRITE ) );
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_soma_pos2text_init (GstSomaPos2Text * filter,
    GstSomaPos2TextClass * gclass)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_getcaps_function (filter->sinkpad,
                                GST_DEBUG_FUNCPTR(gst_soma_pos2text_get_caps));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_soma_pos2text_chain));

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  gst_pad_set_setcaps_function (filter->srcpad,
                                GST_DEBUG_FUNCPTR(gst_soma_pos2text_src_set_caps));
  gst_pad_set_getcaps_function (filter->srcpad,
                                GST_DEBUG_FUNCPTR(gst_soma_pos2text_get_caps));

  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = FALSE;
  filter->text_format = SOMATEXTINVALID;
  filter->init = FALSE;

}

static void
gst_soma_pos2text_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSomaPos2Text *filter = GST_SOMAPOS2TEXT (object);

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
gst_soma_pos2text_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSomaPos2Text *filter = GST_SOMAPOS2TEXT (object);

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

/* GstElement vmethod implementations */

static gboolean
gst_soma_pos2text_src_set_caps(GstPad * pad, GstCaps * caps )
{

  GstSomaPos2Text *filter;
  GstStructure *structure;
  gboolean ret=TRUE;

  filter = GST_SOMAPOS2TEXT (GST_OBJECT_PARENT (pad));

  if (!GST_IS_CAPS(caps))
    return FALSE;

  structure = gst_caps_get_structure (caps, 0);

  if (gst_structure_has_name (structure, "text/x-cmml")) {
    filter->text_format = SOMATEXTCMML;
  } else if (gst_structure_has_name (structure, "text/plain")) {
    filter->text_format = SOMATEXTPLAIN;
  } else if (gst_structure_has_name (structure, "text/x-pango-markup")) {
    filter->text_format = SOMATEXTPANGO;
  } else {
    filter->text_format = SOMATEXTINVALID;
    ret = FALSE;
  }

  GST_DEBUG_OBJECT(filter, "text format: %d", filter->text_format);

  return ret;
}

static GstCaps*
gst_soma_pos2text_get_caps (GstPad *pad)
{

  GstCaps *caps;

  //we can do what our template says we can do
  caps = gst_caps_copy( gst_pad_get_pad_template_caps (pad) );

  return caps;

}

static gboolean
gst_soma_pos2text_src_negotiate(GstSomaPos2Text *filter)
{

  const GstCaps *templ;
  GstCaps *othercaps, *intersect, *target;

  templ = gst_pad_get_pad_template_caps(filter->srcpad);

  GST_DEBUG_OBJECT(filter, "Performing src pad caps negotiation...");

  /*see what the peer can do*/
  othercaps = gst_pad_peer_get_caps(filter->srcpad);
  if (othercaps) {
    intersect = gst_caps_intersect(othercaps, templ);
    gst_caps_unref(othercaps);

    if (gst_caps_is_empty(intersect))
      goto no_format;

    /* select the first caps */
    target = gst_caps_copy_nth(intersect,0);

    gst_caps_unref(intersect);
  } else {
    target = gst_caps_ref((GstCaps*) templ);
  }

  gst_pad_set_caps(filter->srcpad, target);
  gst_caps_unref(target);

  return TRUE;

 no_format:
  {
    gst_caps_unref(intersect);
    return FALSE;
  }

}

static GstFlowReturn
get_buffer(GstSomaPos2Text *filter, GstBuffer **outbuf)
{
  GstFlowReturn ret;

  if (GST_PAD_CAPS(filter->srcpad)==NULL) {
    if (!gst_soma_pos2text_src_negotiate(filter))
      return GST_FLOW_NOT_NEGOTIATED;
  }

  GST_DEBUG_OBJECT(filter, "Allocating output buffer with caps %" GST_PTR_FORMAT, GST_PAD_CAPS(filter->srcpad));

  ret = gst_pad_alloc_buffer_and_set_caps(filter->srcpad,
					  GST_BUFFER_OFFSET_NONE,
					  0,
					  GST_PAD_CAPS(filter->srcpad), 
					  outbuf);
  if (ret!=GST_FLOW_OK)
    return ret;

  return GST_FLOW_OK;

}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_soma_pos2text_chain (GstPad * pad, GstBuffer * buf)
{
  GstSomaPos2Text *filter;
  GstSomaPosition* pos;
  GstFlowReturn ret=GST_FLOW_OK;
  GstBuffer *outbuf = NULL;
  char txt[512];
  guint hh,mm,ss,ms;
  struct timeval now;
  char *p1;
  char tmpstr[32];
  int k;
  char inittxt[32];


  const char *tokens[] = {"[ts]", "[diode]", "[x]", "[y]", "[hh]", "[mm]", "[ss]", "[ms]"};

  pos = (GstSomaPosition*) GST_BUFFER_DATA(buf);
  
  filter = GST_SOMAPOS2TEXT (GST_OBJECT_PARENT (pad));

  //convert timestamp to hh:mm:ss.ms
  hh = (guint) (pos->timestamp / (50000 * 60 * 60));
  mm = (guint) ((pos->timestamp / (50000 * 60)) % 60);
  ss = (guint) ((pos->timestamp / 50000) % 60);
  ms = (guint) ((pos->timestamp % 50000) / 50 );

  /* Let's get a buffer. Caps are negotiated if needed */
  ret = get_buffer(filter, &outbuf);
  if (ret != GST_FLOW_OK)
    goto beach;

  // handle plain/pango text and cmml separately
  switch (filter->text_format) {
  case SOMATEXTPLAIN:
  case SOMATEXTPANGO:

    //use default text if format text is empty
    if (filter->fmt==NULL) {
      sprintf( txt, "timestamp: %llu, diode: %d, x: %d, y: %d\n", (long long unsigned int) pos->timestamp, pos->diode, pos->x, pos->y );
    } else {
      //replace special tokens
      
      char **values = (char**) g_malloc(sizeof(char*)*8);

      sprintf( tmpstr, "%llu", (long long unsigned int) pos->timestamp );
      values[0] = g_strdup(tmpstr);
      sprintf( tmpstr, "%d", pos->diode );	
      values[1] = g_strdup(tmpstr);
      sprintf( tmpstr, "%d", pos->x );
      values[2] = g_strdup(tmpstr);
      sprintf( tmpstr, "%d", pos->y );
      values[3] = g_strdup(tmpstr);
      sprintf( tmpstr, "%02d", hh );
      values[4] = g_strdup(tmpstr);
      sprintf( tmpstr, "%02d", mm );
      values[5] = g_strdup(tmpstr);
      sprintf( tmpstr, "%02d", ss );
      values[6] = g_strdup(tmpstr);
      sprintf( tmpstr, "%03d", ms );
      values[7] = g_strdup(tmpstr);

      p1 = replace_tokens( filter->fmt, tokens, values, 8);

      sprintf( txt, "%s", p1);

      g_free(p1);

      for (k=0;k<8;k++)
	{
	  g_free(values[k]);
	}
      g_free(values);

    }

    break;
  case SOMATEXTCMML:

    //wall time
    gettimeofday(&now,NULL);
    
    if (filter->init) {

      sprintf( inittxt, "<cmml><stream basetime=\"%02u:%02u:%02u.%03u\"/>\n\
<head><title>Testing soma tracker</title></head>\n", 0,0,0,0);

      filter->init=FALSE;
    }
    
    sprintf( txt, "%s\
<clip start=\"npt:%02u:%02u:%02u.%03u\">\n\
<meta name=\"walltime\" content=\"%02llu.%06lu\"/>\
<meta name=\"timestamp\" content=\"%llu\"/>\
<meta name=\"diode\" content=\"%d\"/>\
<meta name=\"x\" content=\"%d\"/>\
<meta name=\"y\" content=\"%d\"/>\n\
</clip>\n",
	     inittxt,
	     hh,mm,ss,ms,
	     (long long unsigned int) now.tv_sec, (long unsigned int) now.tv_usec,
	     (long long unsigned int) pos->timestamp, pos->diode, pos->x, pos->y );


    break;
  default:
    sprintf( txt, "no text");
    break;
  }

  //resize buffer data
  if (GST_BUFFER_MALLOCDATA(outbuf)!=NULL) {
    g_free(GST_BUFFER_MALLOCDATA(outbuf));
    GST_BUFFER_MALLOCDATA(outbuf)=NULL;
  }

  GST_BUFFER_SIZE(outbuf) = strlen(txt);
  GST_BUFFER_MALLOCDATA(outbuf) = g_malloc(strlen(txt));
  GST_BUFFER_DATA(outbuf) = GST_BUFFER_MALLOCDATA(outbuf);

  memcpy( (char*) GST_BUFFER_DATA( outbuf ), txt, strlen(txt) );
  
  gst_buffer_copy_metadata( outbuf, buf, GST_BUFFER_COPY_TIMESTAMPS );  


  GST_DEBUG_OBJECT( filter, "pushing out text buffer...");

  

 beach:
  gst_buffer_unref(buf);
  //gst_object_unref(filter);
  //g_free(pos);
  return gst_pad_push (filter->srcpad, outbuf);

}


static GstStateChangeReturn
gst_soma_pos2text_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstSomaPos2Text * src = GST_SOMAPOS2TEXT (element);
  
  switch (transition) {
  case GST_STATE_CHANGE_NULL_TO_READY: 
    GST_LOG_OBJECT (src, "State change null to ready"); 
    break; 
    
  case GST_STATE_CHANGE_READY_TO_PAUSED:
    GST_LOG_OBJECT (src, "State ready to paused"); 
    break; 
     
  case GST_STATE_CHANGE_PAUSED_TO_PLAYING: 
    GST_LOG_OBJECT (src, "State change paused to playing");
    src->init=TRUE;
    break; 
    
  default:
    break; 
  } 
  
  if (ret == GST_STATE_CHANGE_FAILURE) 
    return ret; 
  
   // now the other direction
   ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition); 

   switch (transition) { 
   case GST_STATE_CHANGE_PLAYING_TO_PAUSED: 
     GST_LOG_OBJECT (src, "State change playing to paused"); 
     break; 

   case GST_STATE_CHANGE_PAUSED_TO_READY: 
     GST_LOG_OBJECT (src, "State change paused to ready"); 
     break;
     case GST_STATE_CHANGE_READY_TO_NULL:
       GST_LOG_OBJECT (src, "State change ready to null"); 
       break; 
     default: 
       break; 
   } 

  return ret;
}






