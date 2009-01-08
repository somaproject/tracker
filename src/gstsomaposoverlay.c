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
 * SECTION:element-somaposoverlay
 *
 * FIXME:Describe somaposoverlay here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! somaposoverlay ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstsomaposoverlay.h"
#include "gstsomasynctracker.h"

#include <cairo.h>


GST_DEBUG_CATEGORY_EXTERN (gst_somatracker_debug);
#define GST_CAT_DEFAULT gst_somatracker_debug

static const GstElementDetails gst_soma_posoverlay_details =
GST_ELEMENT_DETAILS ("Soma position overlay",
    "Mixer/Video/Position",
    "Overlays position data onto video stream",
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
  PROP_SHOWVIDEO,
  PROP_SHOWBINARY,
  PROP_SHOWPOSITION,
  PROP_DIODE1COL,
  PROP_DIODE2COL,
  PROP_TRAIL
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
    GST_STATIC_CAPS ("video/x-raw-rgb, width=640, height=480, bpp=32, depth=24, red_mask=65280, green_mask=16711680, blue_mask=-16777216, endianness=4321")
    );
//video/x-raw-rg, width=640, height=480, bpp=32, depth=24, red_mask=16711680, green_mask=65280, blue_mask=255, endianness=4321")
static GstStaticPadTemplate binsink_factory = GST_STATIC_PAD_TEMPLATE ("bin_sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw-gray")
    );

static GstStaticPadTemplate possink_factory = GST_STATIC_PAD_TEMPLATE ("pos_sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("soma/position")
    );

GST_BOILERPLATE (GstSomaPosOverlay, gst_soma_pos_overlay, GstElement,
    GST_TYPE_ELEMENT);

static void gst_soma_pos_overlay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_soma_pos_overlay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_soma_pos_overlay_set_caps (GstPad * pad, GstCaps * caps);
static GstCaps* gst_soma_pos_overlay_get_caps (GstPad * pad);

static void gst_soma_pos_overlay_finalize (GObject * object);

static GstFlowReturn gst_soma_pos_overlay_collected (GstCollectPads * pads,
						      gpointer data);
static GstStateChangeReturn gst_soma_pos_overlay_change_state (GstElement * element,
    GstStateChange transition);

/* GObject vmethod implementations */


void update_trail( GstSomaPosOverlay *filter,  GstSomaPosition *pos, guint64 ts )
{

  /* if trail==0, do nothing
     if trail<0 (infinite trail), add position to trail
     if trail>0 (timed trail), reduce all positions according to time elapsed and add position to trail */

  double rate;
  guint8 dy=0;
  int stride;
  guint8 *pdata;

  cairo_surface_t *psurf;
  
  if (filter->trail==0)
    return;

  if (pos->diode==0) {
    psurf = filter->trail1;
  } else {
    psurf = filter->trail2;
  }

  stride = cairo_image_surface_get_stride (psurf);
  pdata = cairo_image_surface_get_data(psurf);


  if (filter->trail>0) {
    
    rate = 255.0 / filter->trail;
    dy = (guint8) (rate * (double) (ts - filter->prev_ts) / GST_SECOND); 

    //g_print("rate: %f, dt: %f, dy: %d\n", rate, (double)(ts - filter->prev_ts) / GST_SECOND, dy );



    // loop through all points

    int x,y,idx;

    for (y=0; y<480; y++) {

      idx = (480-y)*stride;

      for (x=0; x<640; x++) {

	if (pdata[idx]>dy) {
	  pdata[idx] -= dy ;
	} else {
	  pdata[idx]=0;
	}

	idx++;

      }
    }

  }

  if (pos->y>0 && pos->x>0) {
    //g_print("set pix: %d, %d\n",pos->x,pos->y);
    pdata[(481-pos->y)*stride+(pos->x-1)] = 255;
  }

  filter->prev_ts = ts;

}

void clean_trail( GstSomaPosOverlay *filter )
{

  cairo_surface_t *psurf;
  cairo_t *cr;

  
  psurf = filter->trail1;
  
  cr = cairo_create (psurf);
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
  cairo_paint(cr);

  psurf = filter->trail2;
  cr = cairo_create (psurf);  
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
  cairo_paint(cr);

}


static void
gst_soma_pos_overlay_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&videosrc_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&videosink_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&binsink_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&possink_factory));

  gst_element_class_set_details (element_class, &gst_soma_posoverlay_details);

}

/* initialize the somaposoverlay's class */
static void
gst_soma_pos_overlay_class_init (GstSomaPosOverlayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->finalize = gst_soma_pos_overlay_finalize;

  gstelement_class->change_state =
    GST_DEBUG_FUNCPTR (gst_soma_pos_overlay_change_state);

  gobject_class->set_property = gst_soma_pos_overlay_set_property;
  gobject_class->get_property = gst_soma_pos_overlay_get_property;

  g_object_class_install_property (gobject_class, PROP_SHOWVIDEO,
      g_param_spec_boolean ("showvideo", "Showvideo", "Show video",
          TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SHOWBINARY,
      g_param_spec_boolean ("showbinary", "Showbinary", "Show binarized video",
          TRUE, G_PARAM_READWRITE)); 

  g_object_class_install_property (gobject_class, PROP_SHOWPOSITION,
      g_param_spec_boolean ("showposition", "Showposition", "Show position",
          TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DIODE1COL,
      g_param_spec_ulong("diode1col", "Diode1col", "Diode 1 color",
			 0, 16777215, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DIODE2COL,
      g_param_spec_ulong("diode2col", "Diode2col", "Diode 2 color",
			 0, 16777215, 0, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TRAIL,
      g_param_spec_double("trail", "Trail", "Trail",
			-1, 10000, 0, G_PARAM_READWRITE));

}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_soma_pos_overlay_init (GstSomaPosOverlay * filter,
    GstSomaPosOverlayClass * gclass)
{
  filter->videosinkpad = gst_pad_new_from_static_template (&videosink_factory, "video_sink");
  gst_pad_set_setcaps_function (filter->videosinkpad,
                                GST_DEBUG_FUNCPTR(gst_soma_pos_overlay_set_caps));
  gst_pad_set_getcaps_function (filter->videosinkpad,
                                GST_DEBUG_FUNCPTR(gst_soma_pos_overlay_get_caps));

  filter->videosrcpad = gst_pad_new_from_static_template (&videosrc_factory, "video_src");
  gst_pad_set_getcaps_function (filter->videosrcpad,
                                GST_DEBUG_FUNCPTR(gst_soma_pos_overlay_get_caps));

  filter->binsinkpad = gst_pad_new_from_static_template (&binsink_factory, "bin_sink");
  gst_pad_set_getcaps_function (filter->binsinkpad,
                                GST_DEBUG_FUNCPTR(gst_soma_pos_overlay_get_caps));

  filter->possinkpad = gst_pad_new_from_static_template (&possink_factory, "pos_sink");
  gst_pad_set_getcaps_function (filter->possinkpad,
                                GST_DEBUG_FUNCPTR(gst_soma_pos_overlay_get_caps));

  gst_element_add_pad (GST_ELEMENT (filter), filter->videosinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->videosrcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->binsinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->possinkpad);

  filter->showvideo = TRUE;
  filter->showbinary = FALSE;
  filter->showpos = FALSE;

  filter->diode1col = 0x000000;
  filter->diode2col = 0x000000;

  filter->trail1 = cairo_image_surface_create(CAIRO_FORMAT_A8 , 640, 480 );
  filter->trail2 = cairo_image_surface_create(CAIRO_FORMAT_A8 , 640, 480 );

  clean_trail( filter );

  filter->trail = 0;
  filter->prev_ts = 0;

  filter->collect = gst_collect_pads_new();
  
  gst_collect_pads_set_function (filter->collect,
      GST_DEBUG_FUNCPTR (gst_soma_pos_overlay_collected), filter);

  filter->video_collect_data = gst_collect_pads_add_pad( filter->collect,
							 filter->videosinkpad, sizeof(GstCollectData) );
  filter->bin_collect_data = gst_collect_pads_add_pad( filter->collect,
							 filter->binsinkpad, sizeof(GstCollectData) );
  filter->pos_collect_data = gst_collect_pads_add_pad( filter->collect,
							 filter->possinkpad, sizeof(GstCollectData) );

}

static void
gst_soma_pos_overlay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSomaPosOverlay *filter = GST_SOMAPOSOVERLAY (object);

  switch (prop_id) {
    case PROP_SHOWVIDEO:
      filter->showvideo = g_value_get_boolean (value);
      break;
    case PROP_SHOWBINARY:
      filter->showbinary = g_value_get_boolean (value);
      break;
    case PROP_SHOWPOSITION:
      filter->showpos = g_value_get_boolean (value);
      break;
    case PROP_DIODE1COL:
      filter->diode1col = g_value_get_ulong (value);
      break;
    case PROP_DIODE2COL:
      filter->diode2col = g_value_get_ulong (value);
      break;
  case PROP_TRAIL:
      filter->trail = g_value_get_double (value);
      clean_trail(filter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_soma_pos_overlay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSomaPosOverlay *filter = GST_SOMAPOSOVERLAY (object);

  switch (prop_id) {
    case PROP_SHOWVIDEO:
      g_value_set_boolean (value, filter->showvideo);
      break;
    case PROP_SHOWBINARY:
      g_value_set_boolean (value, filter->showbinary);
      break;
    case PROP_SHOWPOSITION:
      g_value_set_boolean (value, filter->showpos);
      break;
    case PROP_DIODE1COL:
      g_value_set_ulong (value, filter->diode1col);
      break;
    case PROP_DIODE2COL:
      g_value_set_ulong (value, filter->diode2col);
      break;
  case PROP_TRAIL:
      g_value_set_double (value, filter->trail);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles the link with other elements */
static gboolean
gst_soma_pos_overlay_set_caps (GstPad * pad, GstCaps * caps)
{
  GstSomaPosOverlay *filter;
  gboolean ret=TRUE;
  GstStructure *structure;
  GstCaps *rgbcaps;

  filter = GST_SOMAPOSOVERLAY (gst_pad_get_parent (pad));

  structure = gst_caps_get_structure ( caps, 0);
  gst_structure_get_int (structure, "width", &filter->width);
  gst_structure_get_int (structure, "height", &filter->height);

  g_print("width: %d, height: %d\n", filter->width, filter->height );
  
  //set caps on videosrc pad
  rgbcaps = gst_caps_copy( caps );
  structure = gst_caps_get_structure( rgbcaps,0 );
  gst_structure_set_name (structure, "video/x-raw-rgb");
  gst_structure_remove_field (structure, "format");
  gst_caps_set_simple (rgbcaps, "bpp", G_TYPE_INT, 32, "depth", G_TYPE_INT, 24,
		       "red_mask", G_TYPE_INT, 65280, "green_mask", G_TYPE_INT, 16711680,
		       "blue_mask", G_TYPE_INT, -16777216, "width", G_TYPE_INT, 640, "height", G_TYPE_INT, 480, "endianness", G_TYPE_INT, 4321, NULL );
		       
  ret = gst_pad_set_caps( filter->videosrcpad, rgbcaps );
 
  gst_caps_unref (rgbcaps);

  gst_object_unref (filter);

  return ret;
}


static GstCaps *
gst_soma_pos_overlay_get_caps (GstPad *pad)
{

  GstCaps *caps;

  //we can do what our template says we can do
  caps = gst_caps_copy( gst_pad_get_pad_template_caps (pad) );

  return caps;

}

static void
gst_soma_pos_overlay_finalize (GObject * object)
{
  GstSomaPosOverlay *filter = GST_SOMAPOSOVERLAY (object);

  gst_object_unref (filter->collect);

  if (filter->trail1)
    cairo_surface_destroy( filter->trail1 );

  if (filter->trail2)
    cairo_surface_destroy( filter->trail2 );

  G_OBJECT_CLASS (parent_class)->finalize (object);

}


static GstFlowReturn
gst_soma_pos_overlay_collected (GstCollectPads * pads, gpointer data)
{

  GstSomaPosOverlay *filter;
  GstBuffer *outputbuf=NULL;
  GstBuffer *videobuf, *binbuf, *posbuf;
  GstFlowReturn ret=GST_FLOW_OK;
  cairo_surface_t *psurf, *masksurf;
  cairo_t *cr;
  GstSomaPosition *pos;
  //GstCaps *caps;

  //g_print("Collecting...\n");

  filter = GST_SOMAPOSOVERLAY(data);

  //get video buffer, bin buffer and pos buffer
  videobuf = gst_collect_pads_pop( filter->collect, filter->video_collect_data );
  binbuf = gst_collect_pads_pop( filter->collect, filter->bin_collect_data );
  posbuf = gst_collect_pads_pop( filter->collect, filter->pos_collect_data ); 

  pos = (GstSomaPosition*) GST_BUFFER_DATA(posbuf);

  //create output buffer
  ret = gst_pad_alloc_buffer (filter->videosrcpad, GST_BUFFER_OFFSET_NONE,
           4*640*480, GST_PAD_CAPS (filter->videosrcpad), &outputbuf);

  if (ret!=GST_FLOW_OK)
    goto beach;

  gst_buffer_copy_metadata( outputbuf, videobuf, GST_BUFFER_COPY_TIMESTAMPS );

  //  GST_BUFFER_TIMESTAMP(outputbuf)=GST_BUFFER_TIMESTAMP(videobuf);

  //create surface and cairo context
  psurf = cairo_image_surface_create_for_data( (unsigned char*) GST_BUFFER_DATA(outputbuf), CAIRO_FORMAT_RGB24, 640, 480, 4*640);
  cr = cairo_create(psurf);
  //cairo_identity_matrix(cr);
  //cairo_scale (cr, 0.25, 0.25);

  if (filter->showvideo) {

    //create surface for video
    masksurf = cairo_image_surface_create_for_data( (unsigned char*) GST_BUFFER_DATA(videobuf), CAIRO_FORMAT_A8, 640, 480, 640);
    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_mask_surface(cr, masksurf, 0, 0);
    cairo_surface_destroy(masksurf);
  }

  if (filter->showbinary) {

    //create surface for binarized video
    masksurf = cairo_image_surface_create_for_data( (unsigned char*) GST_BUFFER_DATA(binbuf), CAIRO_FORMAT_A8, 640, 480, 640);

    if (pos->diode==0)
      cairo_set_source_rgba (cr, ((double)((filter->diode1col & 0xFF0000) >> 16)/255), 
			     ((double)((filter->diode1col & 0x00FF00) >> 8)/255) ,
			     ((double)(filter->diode1col & 0x0000FF)/255),
			     0.3);
    else
      cairo_set_source_rgba (cr, (double)((filter->diode2col & 0xFF0000) >> 16)/255, 
			    (double)((filter->diode2col & 0x00FF00) >> 8)/255 ,
			     (double)(filter->diode2col & 0x0000FF)/255,
			     0.3);
    cairo_mask_surface(cr, masksurf, 0, 0);
    cairo_surface_destroy(masksurf);
  }

  if (filter->trail!=0) {

    update_trail(filter, pos, GST_BUFFER_TIMESTAMP(videobuf));
    
    if (pos->diode==0) {
      cairo_set_source_rgba(cr, (double)((filter->diode1col & 0xFF0000) >> 16)/255, 
			   (double)((filter->diode1col & 0x00FF00) >> 8)/255 ,
			    (double)(filter->diode1col & 0x0000FF)/255, 1.0);
      cairo_mask_surface(cr, filter->trail1,0,0);
    } else {
      cairo_set_source_rgba(cr, (double)((filter->diode2col & 0xFF0000) >> 16)/255, 
			   (double)((filter->diode2col & 0x00FF00) >> 8)/255 ,
			   (double)(filter->diode2col & 0x0000FF)/255, 1.0);
      cairo_mask_surface(cr, filter->trail2,0,0);
    }
  }    

  if (filter->showpos && pos->x!=0 && pos->y!=0) {
    cairo_new_sub_path(cr);
    cairo_set_line_width (cr, 2);
    cairo_arc(cr, pos->x, 481-pos->y, 4, 0, 2*M_PI);
    if (pos->diode==0) {
      cairo_set_source_rgb(cr, (double)((filter->diode1col & 0xFF0000) >> 16)/255, 
			   (double)((filter->diode1col & 0x00FF00) >> 8)/255 ,
			   (double)(filter->diode1col & 0x0000FF)/255);
    } else {
      cairo_set_source_rgb(cr, (double)((filter->diode2col & 0xFF0000) >> 16)/255, 
			   (double)((filter->diode2col & 0x00FF00) >> 8)/255 ,
			   (double)(filter->diode2col & 0x0000FF)/255);
    }
    cairo_stroke(cr);
  }


  cairo_surface_destroy(psurf);
  cairo_destroy(cr);
  
  ret = gst_pad_push(filter->videosrcpad, outputbuf);
   

beach:

  gst_buffer_unref( videobuf );
  gst_buffer_unref( binbuf );
  gst_buffer_unref( posbuf );
  

  //create a new output buffer
  //get caps from videosink
  //change caps to rgb
  //set caps of output buffer

  //get input buffers
  
  //create cairo surface for output buffer
  //create cairo context

  //if showvideo, use videobuffer as mask and paint it on surface

  //update internal diode and position cairo surfaces

  //if showbinary, use diode surface as mask and paint it on surface with selected color

  //if showpos, use pos surface as mask and paint it on surface with selected color


  //g_print("Return...%d (%d)\n",ret, GST_FLOW_OK);
  return ret;

}


static GstStateChangeReturn
gst_soma_pos_overlay_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstSomaPosOverlay *filter = GST_SOMAPOSOVERLAY (element);

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
