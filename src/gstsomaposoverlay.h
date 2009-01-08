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

#ifndef __GST_SOMAPOSOVERLAY_H__
#define __GST_SOMAPOSOVERLAY_H__

#include <gst/gst.h>
#include <gst/base/gstcollectpads.h>

#include <cairo.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_SOMAPOSOVERLAY \
  (gst_soma_pos_overlay_get_type())
#define GST_SOMAPOSOVERLAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SOMAPOSOVERLAY,GstSomaPosOverlay))
#define GST_SOMAPOSOVERLAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SOMAPOSOVERLAY,GstSomaPosOverlayClass))
#define GST_IS_SOMAPOSOVERLAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SOMAPOSOVERLAY))
#define GST_IS_SOMAPOSOVERLAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SOMAPOSOVERLAY))

typedef struct _GstSomaPosOverlay      GstSomaPosOverlay;
typedef struct _GstSomaPosOverlayClass GstSomaPosOverlayClass;

struct _GstSomaPosOverlay
{
  GstElement element;

  GstPad *videosinkpad, *videosrcpad;
  GstPad *binsinkpad, *possinkpad;

  GstCollectPads *collect;
  GstCollectData *video_collect_data;
  GstCollectData *bin_collect_data;
  GstCollectData *pos_collect_data;

  gboolean silent;
  gboolean showvideo;
  gboolean showbinary;
  gboolean showpos;
  gulong diode1col;
  gulong diode2col;

  cairo_surface_t *trail1, *trail2;

  double trail;

  guint64 prev_ts;

  gint width, height;

};

struct _GstSomaPosOverlayClass 
{
  GstElementClass parent_class;
};

GType gst_soma_pos_overlay_get_type (void);

G_END_DECLS

#endif /* __GST_SOMAPOSOVERLAY_H__ */
