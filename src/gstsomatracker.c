#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gstsomasynctracker.h"
#include "gstsomapos2text.h"
#include "gstsomacameraevent.h"
#include "gstsomacmml2text.h"
#include "gstsomaposoverlay.h"

GST_DEBUG_CATEGORY (gst_somatracker_debug);
#define GST_CAT_DEFAULT gst_somatracker_debug

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
plugin_init (GstPlugin * plugin)
{


  if (!gst_element_register (plugin, "somasynctracker", GST_RANK_NONE,
			     GST_TYPE_SOMASYNCTRACKER) ||
      !gst_element_register (plugin, "somapos2text", GST_RANK_NONE,
			     GST_TYPE_SOMAPOS2TEXT) ||
      !gst_element_register (plugin, "somacameraevent", GST_RANK_NONE,
			     GST_TYPE_SOMACAMERAEVENT) ||
      !gst_element_register (plugin, "somacmml2text", GST_RANK_NONE,
			     GST_TYPE_SOMACMML2TEXT)  ||
      !gst_element_register (plugin, "somaposoverlay", GST_RANK_NONE,
			     GST_TYPE_SOMAPOSOVERLAY) )
    {
      return FALSE;
    }


  /* debug category for fltering log messages */
  GST_DEBUG_CATEGORY_INIT (gst_somatracker_debug, "somatracker",
			   0, "Soma tracker elements");

  return TRUE;
}

/* gstreamer looks for this structure to register somasynctrackers */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "somatracker",
    "Soma video capture and position tracking",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
