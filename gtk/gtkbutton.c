/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

/**
 * SECTION:gtkbutton
 * @Short_description: A widget that emits a signal when clicked on
 * @Title: GtkButton
 *
 * The #GtkButton widget is generally used to trigger a callback function that is
 * called when the button is pressed.  The various signals and how to use them
 * are outlined below.
 *
 * The #GtkButton widget can hold any valid child widget.  That is, it can hold
 * almost any other standard #GtkWidget.  The most commonly used child is the
 * #GtkLabel.
 *
 * # CSS nodes
 *
 * GtkButton has a single CSS node with name button. The node will get the
 * style classes .image-button or .text-button, if the content is just an
 * image or label, respectively. It may also receive the .flat style class.
 *
 * Other style classes that are commonly used with GtkButton include
 * .suggested-action and .destructive-action. In special cases, buttons
 * can be made round by adding the .circular style class.
 *
 * Button-like widgets like #GtkToggleButton, #GtkMenuButton, #GtkVolumeButton,
 * #GtkLockButton, #GtkColorButton, #GtkFontButton or #GtkFileChooserButton use
 * style classes such as .toggle, .popup, .scale, .lock, .color, .font, .file
 * to differentiate themselves from a plain GtkButton.
 */

#include "config.h"

#include "gtkbutton.h"
#include "gtkbuttonprivate.h"

#include <string.h>
#include "gtklabel.h"
#include "gtkmain.h"
#include "gtkmarshalers.h"
#include "gtkimage.h"
#include "gtkbox.h"
#include "gtksizerequest.h"
#include "gtktypebuiltins.h"
#include "gtkwidgetprivate.h"
#include "gtkprivate.h"
#include "gtkintl.h"
#include "a11y/gtkbuttonaccessible.h"
#include "gtkapplicationprivate.h"
#include "gtkactionhelper.h"
#include "gtkcsscustomgadgetprivate.h"
#include "gtkcontainerprivate.h"

/* Time out before giving up on getting a key release when animating
 * the close button.
 */
#define ACTIVATE_TIMEOUT 250


enum {
  CLICKED,
  ENTER,
  LEAVE,
  ACTIVATE,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_LABEL,
  PROP_IMAGE,
  PROP_RELIEF,
  PROP_USE_UNDERLINE,
  PROP_IMAGE_POSITION,
  PROP_ALWAYS_SHOW_IMAGE,

  /* actionable properties */
  PROP_ACTION_NAME,
  PROP_ACTION_TARGET,
  LAST_PROP = PROP_ACTION_NAME
};


static void gtk_button_finalize       (GObject            *object);
static void gtk_button_dispose        (GObject            *object);
static void gtk_button_set_property   (GObject            *object,
                                       guint               prop_id,
                                       const GValue       *value,
                                       GParamSpec         *pspec);
static void gtk_button_get_property   (GObject            *object,
                                       guint               prop_id,
                                       GValue             *value,
                                       GParamSpec         *pspec);
static void gtk_button_screen_changed (GtkWidget          *widget,
				       GdkScreen          *previous_screen);
static void gtk_button_realize (GtkWidget * widget);
static void gtk_button_unrealize (GtkWidget * widget);
static void gtk_button_map (GtkWidget * widget);
static void gtk_button_unmap (GtkWidget * widget);
static void gtk_button_size_allocate (GtkWidget * widget,
				      GtkAllocation * allocation);
static gint gtk_button_draw (GtkWidget * widget, cairo_t *cr);
static gint gtk_button_grab_broken (GtkWidget * widget,
				    GdkEventGrabBroken * event);
static gint gtk_button_key_release (GtkWidget * widget, GdkEventKey * event);
static gint gtk_button_enter_notify (GtkWidget * widget,
				     GdkEventCrossing * event);
static gint gtk_button_leave_notify (GtkWidget * widget,
				     GdkEventCrossing * event);
static void gtk_real_button_clicked (GtkButton * button);
static void gtk_real_button_activate  (GtkButton          *button);
static void gtk_button_update_state   (GtkButton          *button);
static void gtk_button_finish_activate (GtkButton         *button,
					gboolean           do_it);

static void gtk_button_constructed (GObject *object);
static void gtk_button_construct_child (GtkButton             *button);
static void gtk_button_state_changed   (GtkWidget             *widget,
					GtkStateType           previous_state);
static void gtk_button_grab_notify     (GtkWidget             *widget,
					gboolean               was_grabbed);
static void gtk_button_do_release      (GtkButton             *button,
                                        gboolean               emit_clicked);

static void gtk_button_actionable_iface_init     (GtkActionableInterface *iface);

static void gtk_button_get_preferred_width             (GtkWidget           *widget,
                                                        gint                *minimum_size,
                                                        gint                *natural_size);
static void gtk_button_get_preferred_height            (GtkWidget           *widget,
                                                        gint                *minimum_size,
                                                        gint                *natural_size);
static void gtk_button_get_preferred_width_for_height  (GtkWidget           *widget,
                                                        gint                 for_size,
                                                        gint                *minimum_size,
                                                        gint                *natural_size);
static void gtk_button_get_preferred_height_for_width  (GtkWidget           *widget,
                                                        gint                 for_size,
                                                        gint                *minimum_size,
                                                        gint                *natural_size);
static void gtk_button_get_preferred_height_and_baseline_for_width (GtkWidget *widget,
								    gint       width,
								    gint      *minimum_size,
								    gint      *natural_size,
								    gint      *minimum_baseline,
								    gint      *natural_baseline);

static void     gtk_button_measure  (GtkCssGadget        *gadget,
                                     GtkOrientation       orientation,
                                     int                  for_size,
                                     int                 *minimum_size,
                                     int                 *natural_size,
                                     int                 *minimum_baseline,
                                     int                 *natural_baseline,
                                     gpointer             data);
static void     gtk_button_allocate (GtkCssGadget        *gadget,
                                     const GtkAllocation *allocation,
                                     int                  baseline,
                                     GtkAllocation       *out_clip,
                                     gpointer             data);
static gboolean gtk_button_render   (GtkCssGadget        *gadget,
                                     cairo_t             *cr,
                                     int                  x,
                                     int                  y,
                                     int                  width,
                                     int                  height,
                                     gpointer             data);

static GParamSpec *props[LAST_PROP] = { NULL, };
static guint button_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (GtkButton, gtk_button, GTK_TYPE_BIN,
                         G_ADD_PRIVATE (GtkButton)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ACTIONABLE, gtk_button_actionable_iface_init))

static void
gtk_button_class_init (GtkButtonClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = (GtkWidgetClass*) klass;
  
  gobject_class->constructed  = gtk_button_constructed;
  gobject_class->dispose      = gtk_button_dispose;
  gobject_class->finalize     = gtk_button_finalize;
  gobject_class->set_property = gtk_button_set_property;
  gobject_class->get_property = gtk_button_get_property;

  widget_class->get_preferred_width = gtk_button_get_preferred_width;
  widget_class->get_preferred_height = gtk_button_get_preferred_height;
  widget_class->get_preferred_width_for_height = gtk_button_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = gtk_button_get_preferred_height_for_width;
  widget_class->get_preferred_height_and_baseline_for_width = gtk_button_get_preferred_height_and_baseline_for_width;
  widget_class->screen_changed = gtk_button_screen_changed;
  widget_class->realize = gtk_button_realize;
  widget_class->unrealize = gtk_button_unrealize;
  widget_class->map = gtk_button_map;
  widget_class->unmap = gtk_button_unmap;
  widget_class->size_allocate = gtk_button_size_allocate;
  widget_class->draw = gtk_button_draw;
  widget_class->grab_broken_event = gtk_button_grab_broken;
  widget_class->key_release_event = gtk_button_key_release;
  widget_class->enter_notify_event = gtk_button_enter_notify;
  widget_class->leave_notify_event = gtk_button_leave_notify;
  widget_class->state_changed = gtk_button_state_changed;
  widget_class->grab_notify = gtk_button_grab_notify;

  klass->clicked = NULL;
  klass->activate = gtk_real_button_activate;

  props[PROP_LABEL] =
    g_param_spec_string ("label",
                         P_("Label"),
                         P_("Text of the label widget inside the button, if the button contains a label widget"),
                         NULL,
                         GTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_USE_UNDERLINE] =
    g_param_spec_boolean ("use-underline",
                          P_("Use underline"),
                          P_("If set, an underline in the text indicates the next character should be used for the mnemonic accelerator key"),
                          FALSE,
                          GTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_RELIEF] =
    g_param_spec_enum ("relief",
                       P_("Border relief"),
                       P_("The border relief style"),
                       GTK_TYPE_RELIEF_STYLE,
                       GTK_RELIEF_NORMAL,
                       GTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkButton:image:
   *
   * The child widget to appear next to the button text.
   *
   * Since: 2.6
   */
  props[PROP_IMAGE] =
    g_param_spec_object ("image",
                         P_("Image widget"),
                         P_("Child widget to appear next to the button text"),
                         GTK_TYPE_WIDGET,
                         GTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkButton:image-position:
   *
   * The position of the image relative to the text inside the button.
   *
   * Since: 2.10
   */
  props[PROP_IMAGE_POSITION] =
    g_param_spec_enum ("image-position",
                       P_("Image position"),
                       P_("The position of the image relative to the text"),
                       GTK_TYPE_POSITION_TYPE,
                       GTK_POS_LEFT,
                       GTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkButton:always-show-image:
   *
   * If %TRUE, the button will ignore the #GtkSettings:gtk-button-images
   * setting and always show the image, if available.
   *
   * Use this property if the button would be useless or hard to use
   * without the image.
   *
   * Since: 3.6
   */
  props[PROP_ALWAYS_SHOW_IMAGE] =
     g_param_spec_boolean ("always-show-image",
                           P_("Always show image"),
                           P_("Whether the image will always be shown"),
                           FALSE,
                           GTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_PROP, props);

  g_object_class_override_property (gobject_class, PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (gobject_class, PROP_ACTION_TARGET, "action-target");

  /**
   * GtkButton::clicked:
   * @button: the object that received the signal
   *
   * Emitted when the button has been activated (pressed and released).
   */
  button_signals[CLICKED] =
    g_signal_new (I_("clicked"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GtkButtonClass, clicked),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * GtkButton::activate:
   * @widget: the object which received the signal.
   *
   * The ::activate signal on GtkButton is an action signal and
   * emitting it causes the button to animate press then release.
   * Applications should never connect to this signal, but use the
   * #GtkButton::clicked signal.
   */
  button_signals[ACTIVATE] =
    g_signal_new (I_("activate"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GtkButtonClass, activate),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
  widget_class->activate_signal = button_signals[ACTIVATE];

  gtk_widget_class_set_accessible_type (widget_class, GTK_TYPE_BUTTON_ACCESSIBLE);
  gtk_widget_class_set_css_name (widget_class, "button");
}

static void
multipress_pressed_cb (GtkGestureMultiPress *gesture,
                       guint                 n_press,
                       gdouble               x,
                       gdouble               y,
                       GtkWidget            *widget)
{
  GtkButton *button = GTK_BUTTON (widget);
  GtkButtonPrivate *priv = button->priv;

  if (gtk_widget_get_focus_on_click (widget) && !gtk_widget_has_focus (widget))
    gtk_widget_grab_focus (widget);

  priv->in_button = TRUE;

  if (!priv->activate_timeout)
    {
      priv->button_down = TRUE;
      gtk_button_update_state (button);
    }
  gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

static gboolean
touch_release_in_button (GtkButton *button)
{
  GtkButtonPrivate *priv;
  gint width, height;
  GdkEvent *event;
  gdouble x, y;

  priv = button->priv;
  event = gtk_get_current_event ();

  if (!event)
    return FALSE;

  if (event->type != GDK_TOUCH_END ||
      event->touch.window != priv->event_window)
    {
      gdk_event_free (event);
      return FALSE;
    }

  gdk_event_get_coords (event, &x, &y);
  width = gdk_window_get_width (priv->event_window);
  height = gdk_window_get_height (priv->event_window);

  gdk_event_free (event);

  if (x >= 0 && x <= width &&
      y >= 0 && y <= height)
    return TRUE;

  return FALSE;
}

static void
multipress_released_cb (GtkGestureMultiPress *gesture,
                        guint                 n_press,
                        gdouble               x,
                        gdouble               y,
                        GtkWidget            *widget)
{
  GtkButton *button = GTK_BUTTON (widget);
  GtkButtonPrivate *priv = button->priv;
  GdkEventSequence *sequence;

  gtk_button_do_release (button,
                         gtk_widget_is_sensitive (GTK_WIDGET (button)) &&
                         (button->priv->in_button ||
                          touch_release_in_button (button)));

  sequence = gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));

  if (sequence)
    {
      priv->in_button = FALSE;
      gtk_button_update_state (button);
    }
}

static void
multipress_gesture_update_cb (GtkGesture       *gesture,
                              GdkEventSequence *sequence,
                              GtkButton        *button)
{
  GtkButtonPrivate *priv = button->priv;
  GtkAllocation allocation;
  gboolean in_button;
  gdouble x, y;

  if (sequence != gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture)))
    return;

  gtk_widget_get_allocation (GTK_WIDGET (button), &allocation);
  gtk_gesture_get_point (gesture, sequence, &x, &y);

  in_button = (x >= 0 && y >= 0 && x < allocation.width && y < allocation.height);

  if (priv->in_button != in_button)
    {
      priv->in_button = in_button;
      gtk_button_update_state (button);
    }
}

static void
multipress_gesture_cancel_cb (GtkGesture       *gesture,
                              GdkEventSequence *sequence,
                              GtkButton        *button)
{
  gtk_button_do_release (button, FALSE);
}

static void
gtk_button_init (GtkButton *button)
{
  GtkButtonPrivate *priv;

  button->priv = gtk_button_get_instance_private (button);
  priv = button->priv;

  gtk_widget_set_can_focus (GTK_WIDGET (button), TRUE);
  gtk_widget_set_receives_default (GTK_WIDGET (button), TRUE);
  gtk_widget_set_has_window (GTK_WIDGET (button), FALSE);

  priv->label_text = NULL;

  priv->constructed = FALSE;
  priv->in_button = FALSE;
  priv->button_down = FALSE;
  priv->use_underline = FALSE;

  priv->image_position = GTK_POS_LEFT;
  priv->use_action_appearance = TRUE;

  priv->gesture = gtk_gesture_multi_press_new (GTK_WIDGET (button));
  gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (priv->gesture), FALSE);
  gtk_gesture_single_set_exclusive (GTK_GESTURE_SINGLE (priv->gesture), TRUE);
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (priv->gesture), GDK_BUTTON_PRIMARY);
  g_signal_connect (priv->gesture, "pressed", G_CALLBACK (multipress_pressed_cb), button);
  g_signal_connect (priv->gesture, "released", G_CALLBACK (multipress_released_cb), button);
  g_signal_connect (priv->gesture, "update", G_CALLBACK (multipress_gesture_update_cb), button);
  g_signal_connect (priv->gesture, "cancel", G_CALLBACK (multipress_gesture_cancel_cb), button);
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (priv->gesture), GTK_PHASE_BUBBLE);

  priv->gadget = gtk_css_custom_gadget_new_for_node (gtk_widget_get_css_node (GTK_WIDGET (button)),
                                                     GTK_WIDGET (button),
                                                     gtk_button_measure,
                                                     gtk_button_allocate,
                                                     gtk_button_render,
                                                     NULL,
                                                     NULL);

}

static void
gtk_button_finalize (GObject *object)
{
  GtkButton *button = GTK_BUTTON (object);
  GtkButtonPrivate *priv = button->priv;

  g_clear_pointer (&priv->label_text, g_free);
  g_clear_object (&priv->gesture);
  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (gtk_button_parent_class)->finalize (object);
}

static void
gtk_button_constructed (GObject *object)
{
  GtkButton *button = GTK_BUTTON (object);
  GtkButtonPrivate *priv = button->priv;

  G_OBJECT_CLASS (gtk_button_parent_class)->constructed (object);

  priv->constructed = TRUE;

  if (priv->label_text != NULL || priv->image != NULL)
    gtk_button_construct_child (button);
}

static void
gtk_button_dispose (GObject *object)
{
  GtkButton *button = GTK_BUTTON (object);
  GtkButtonPrivate *priv = button->priv;

  g_clear_object (&priv->action_helper);

  G_OBJECT_CLASS (gtk_button_parent_class)->dispose (object);
}

static void
gtk_button_set_action_name (GtkActionable *actionable,
                            const gchar   *action_name)
{
  GtkButton *button = GTK_BUTTON (actionable);

  if (!button->priv->action_helper)
    button->priv->action_helper = gtk_action_helper_new (actionable);

  g_signal_handlers_disconnect_by_func (button, gtk_real_button_clicked, NULL);
  if (action_name)
    g_signal_connect_after (button, "clicked", G_CALLBACK (gtk_real_button_clicked), NULL);

  gtk_action_helper_set_action_name (button->priv->action_helper, action_name);
}

static void
gtk_button_set_action_target_value (GtkActionable *actionable,
                                    GVariant      *action_target)
{
  GtkButton *button = GTK_BUTTON (actionable);

  if (!button->priv->action_helper)
    button->priv->action_helper = gtk_action_helper_new (actionable);

  gtk_action_helper_set_action_target_value (button->priv->action_helper, action_target);
}

static void
gtk_button_set_property (GObject         *object,
                         guint            prop_id,
                         const GValue    *value,
                         GParamSpec      *pspec)
{
  GtkButton *button = GTK_BUTTON (object);

  switch (prop_id)
    {
    case PROP_LABEL:
      gtk_button_set_label (button, g_value_get_string (value));
      break;
    case PROP_IMAGE:
      gtk_button_set_image (button, (GtkWidget *) g_value_get_object (value));
      break;
    case PROP_ALWAYS_SHOW_IMAGE:
      gtk_button_set_always_show_image (button, g_value_get_boolean (value));
      break;
    case PROP_RELIEF:
      gtk_button_set_relief (button, g_value_get_enum (value));
      break;
    case PROP_USE_UNDERLINE:
      gtk_button_set_use_underline (button, g_value_get_boolean (value));
      break;
    case PROP_IMAGE_POSITION:
      gtk_button_set_image_position (button, g_value_get_enum (value));
      break;
    case PROP_ACTION_NAME:
      gtk_button_set_action_name (GTK_ACTIONABLE (button), g_value_get_string (value));
      break;
    case PROP_ACTION_TARGET:
      gtk_button_set_action_target_value (GTK_ACTIONABLE (button), g_value_get_variant (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_button_get_property (GObject         *object,
                         guint            prop_id,
                         GValue          *value,
                         GParamSpec      *pspec)
{
  GtkButton *button = GTK_BUTTON (object);
  GtkButtonPrivate *priv = button->priv;

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, priv->label_text);
      break;
    case PROP_IMAGE:
      g_value_set_object (value, (GObject *)priv->image);
      break;
    case PROP_ALWAYS_SHOW_IMAGE:
      g_value_set_boolean (value, gtk_button_get_always_show_image (button));
      break;
    case PROP_RELIEF:
      g_value_set_enum (value, gtk_button_get_relief (button));
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, priv->use_underline);
      break;
    case PROP_IMAGE_POSITION:
      g_value_set_enum (value, priv->image_position);
      break;
    case PROP_ACTION_NAME:
      g_value_set_string (value, gtk_action_helper_get_action_name (priv->action_helper));
      break;
    case PROP_ACTION_TARGET:
      g_value_set_variant (value, gtk_action_helper_get_action_target_value (priv->action_helper));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static const gchar *
gtk_button_get_action_name (GtkActionable *actionable)
{
  GtkButton *button = GTK_BUTTON (actionable);

  return gtk_action_helper_get_action_name (button->priv->action_helper);
}

static GVariant *
gtk_button_get_action_target_value (GtkActionable *actionable)
{
  GtkButton *button = GTK_BUTTON (actionable);

  return gtk_action_helper_get_action_target_value (button->priv->action_helper);
}

static void
gtk_button_actionable_iface_init (GtkActionableInterface *iface)
{
  iface->get_action_name = gtk_button_get_action_name;
  iface->set_action_name = gtk_button_set_action_name;
  iface->get_action_target_value = gtk_button_get_action_target_value;
  iface->set_action_target_value = gtk_button_set_action_target_value;
}

/**
 * gtk_button_new:
 *
 * Creates a new #GtkButton widget. To add a child widget to the button,
 * use gtk_container_add().
 *
 * Returns: The newly created #GtkButton widget.
 */
GtkWidget*
gtk_button_new (void)
{
  return g_object_new (GTK_TYPE_BUTTON, NULL);
}

static void
gtk_button_construct_child (GtkButton *button)
{
  GtkButtonPrivate *priv = button->priv;
  GtkStyleContext *context;
  GtkWidget *child;
  GtkWidget *label;
  GtkWidget *box;
  GtkWidget *image = NULL;

  context = gtk_widget_get_style_context (GTK_WIDGET (button));
  gtk_style_context_remove_class (context, "image-button");
  gtk_style_context_remove_class (context, "text-button");

  if (!priv->constructed)
    return;

  if (!priv->label_text && !priv->image)
    return;

  if (priv->image)
    {
      GtkWidget *parent;

      image = g_object_ref (priv->image);

      parent = gtk_widget_get_parent (image);
      if (parent)
	gtk_container_remove (GTK_CONTAINER (parent), image);
    }

  priv->image = NULL;

  child = gtk_bin_get_child (GTK_BIN (button));
  if (child)
    gtk_container_remove (GTK_CONTAINER (button), child);

  if (image)
    {
      priv->image = image;
      g_object_set (priv->image,
		    "visible", TRUE,
		    "no-show-all", TRUE,
		    NULL);

      if (priv->image_position == GTK_POS_LEFT ||
	  priv->image_position == GTK_POS_RIGHT)
	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      else
	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

      gtk_widget_set_valign (image, GTK_ALIGN_BASELINE);
      gtk_widget_set_valign (box, GTK_ALIGN_BASELINE);

      if (priv->image_position == GTK_POS_LEFT ||
	  priv->image_position == GTK_POS_TOP)
        gtk_box_pack_start (GTK_BOX (box), priv->image, FALSE, FALSE);
      else
        gtk_box_pack_end (GTK_BOX (box), priv->image, FALSE, FALSE);

      if (priv->label_text)
	{
          if (priv->use_underline)
            {
	      label = gtk_label_new_with_mnemonic (priv->label_text);
	      gtk_label_set_mnemonic_widget (GTK_LABEL (label),
                                             GTK_WIDGET (button));
            }
          else
            label = gtk_label_new (priv->label_text);

	  gtk_widget_set_valign (label, GTK_ALIGN_BASELINE);

	  if (priv->image_position == GTK_POS_RIGHT ||
	      priv->image_position == GTK_POS_BOTTOM)
            gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE);
	  else
            gtk_box_pack_end (GTK_BOX (box), label, FALSE, FALSE);
	}
      else
        {
          gtk_style_context_add_class (context, "image-button");
        }

      gtk_container_add (GTK_CONTAINER (button), box);
      gtk_widget_show_all (box);

      g_object_unref (image);

      return;
    }

  if (priv->use_underline)
    {
      label = gtk_label_new_with_mnemonic (priv->label_text);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
    }
  else
    label = gtk_label_new (priv->label_text);

  gtk_widget_set_valign (label, GTK_ALIGN_BASELINE);

  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (button), label);

  gtk_style_context_add_class (context, "text-button");
}


/**
 * gtk_button_new_with_label:
 * @label: The text you want the #GtkLabel to hold.
 *
 * Creates a #GtkButton widget with a #GtkLabel child containing the given
 * text.
 *
 * Returns: The newly created #GtkButton widget.
 */
GtkWidget*
gtk_button_new_with_label (const gchar *label)
{
  return g_object_new (GTK_TYPE_BUTTON, "label", label, NULL);
}

/**
 * gtk_button_new_from_icon_name:
 * @icon_name: an icon name
 * @size: (type int): an icon size (#GtkIconSize)
 *
 * Creates a new button containing an icon from the current icon theme.
 *
 * If the icon name isn’t known, a “broken image” icon will be
 * displayed instead. If the current icon theme is changed, the icon
 * will be updated appropriately.
 *
 * This function is a convenience wrapper around gtk_button_new() and
 * gtk_button_set_image().
 *
 * Returns: a new #GtkButton displaying the themed icon
 *
 * Since: 3.10
 */
GtkWidget*
gtk_button_new_from_icon_name (const gchar *icon_name,
			       GtkIconSize  size)
{
  GtkWidget *button;
  GtkWidget *image;

  image = gtk_image_new_from_icon_name (icon_name, size);
  button =  g_object_new (GTK_TYPE_BUTTON,
			  "image", image,
			  NULL);

  return button;
}

/**
 * gtk_button_new_with_mnemonic:
 * @label: The text of the button, with an underscore in front of the
 *         mnemonic character
 *
 * Creates a new #GtkButton containing a label.
 * If characters in @label are preceded by an underscore, they are underlined.
 * If you need a literal underscore character in a label, use “__” (two
 * underscores). The first underlined character represents a keyboard
 * accelerator called a mnemonic.
 * Pressing Alt and that key activates the button.
 *
 * Returns: a new #GtkButton
 */
GtkWidget*
gtk_button_new_with_mnemonic (const gchar *label)
{
  return g_object_new (GTK_TYPE_BUTTON, "label", label, "use-underline", TRUE,  NULL);
}

/**
 * gtk_button_clicked:
 * @button: The #GtkButton you want to send the signal to.
 *
 * Emits a #GtkButton::clicked signal to the given #GtkButton.
 */
void
gtk_button_clicked (GtkButton *button)
{
  g_return_if_fail (GTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[CLICKED], 0);
}

/**
 * gtk_button_set_relief:
 * @button: The #GtkButton you want to set relief styles of
 * @relief: The GtkReliefStyle as described above
 *
 * Sets the relief style of the edges of the given #GtkButton widget.
 * Two styles exist, %GTK_RELIEF_NORMAL and %GTK_RELIEF_NONE.
 * The default style is, as one can guess, %GTK_RELIEF_NORMAL.
 * The deprecated value %GTK_RELIEF_HALF behaves the same as
 * %GTK_RELIEF_NORMAL.
 */
void
gtk_button_set_relief (GtkButton      *button,
		       GtkReliefStyle  relief)
{
  GtkStyleContext *context;
  GtkReliefStyle old_relief;

  g_return_if_fail (GTK_IS_BUTTON (button));

  old_relief = gtk_button_get_relief (button);
  if (old_relief != relief)
    {
      context = gtk_widget_get_style_context (GTK_WIDGET (button));
      if (relief == GTK_RELIEF_NONE)
        gtk_style_context_add_class (context, GTK_STYLE_CLASS_FLAT);
      else
        gtk_style_context_remove_class (context, GTK_STYLE_CLASS_FLAT);

      g_object_notify_by_pspec (G_OBJECT (button), props[PROP_RELIEF]);
    }
}

/**
 * gtk_button_get_relief:
 * @button: The #GtkButton you want the #GtkReliefStyle from.
 *
 * Returns the current relief style of the given #GtkButton.
 *
 * Returns: The current #GtkReliefStyle
 */
GtkReliefStyle
gtk_button_get_relief (GtkButton *button)
{
  GtkStyleContext *context;

  g_return_val_if_fail (GTK_IS_BUTTON (button), GTK_RELIEF_NORMAL);

  context = gtk_widget_get_style_context (GTK_WIDGET (button));
  if (gtk_style_context_has_class (context, GTK_STYLE_CLASS_FLAT))
    return GTK_RELIEF_NONE;
  else
    return GTK_RELIEF_NORMAL;
}

static void
gtk_button_realize (GtkWidget *widget)
{
  GtkButton *button = GTK_BUTTON (widget);
  GtkButtonPrivate *priv = button->priv;
  GtkAllocation allocation;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  gtk_widget_get_allocation (widget, &allocation);

  gtk_widget_set_realized (widget, TRUE);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
                            GDK_BUTTON_RELEASE_MASK |
                            GDK_TOUCH_MASK |
                            GDK_ENTER_NOTIFY_MASK |
                            GDK_LEAVE_NOTIFY_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  window = gtk_widget_get_parent_window (widget);
  gtk_widget_set_window (widget, window);
  g_object_ref (window);

  priv->event_window = gdk_window_new (window,
                                       &attributes, attributes_mask);
  gtk_widget_register_window (widget, priv->event_window);
}

static void
gtk_button_unrealize (GtkWidget *widget)
{
  GtkButton *button = GTK_BUTTON (widget);
  GtkButtonPrivate *priv = button->priv;

  if (priv->activate_timeout)
    gtk_button_finish_activate (button, FALSE);

  if (priv->event_window)
    {
      gtk_widget_unregister_window (widget, priv->event_window);
      gdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  GTK_WIDGET_CLASS (gtk_button_parent_class)->unrealize (widget);
}

static void
gtk_button_map (GtkWidget *widget)
{
  GtkButton *button = GTK_BUTTON (widget);
  GtkButtonPrivate *priv = button->priv;

  GTK_WIDGET_CLASS (gtk_button_parent_class)->map (widget);

  if (priv->event_window)
    gdk_window_show (priv->event_window);
}

static void
gtk_button_unmap (GtkWidget *widget)
{
  GtkButton *button = GTK_BUTTON (widget);
  GtkButtonPrivate *priv = button->priv;

  if (priv->event_window)
    {
      gdk_window_hide (priv->event_window);
      priv->in_button = FALSE;
    }

  GTK_WIDGET_CLASS (gtk_button_parent_class)->unmap (widget);
}

static void
gtk_button_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
  GtkButton *button = GTK_BUTTON (widget);
  GtkButtonPrivate *priv = button->priv;
  GtkAllocation clip;

  gtk_widget_set_allocation (widget, allocation);
  gtk_css_gadget_allocate (priv->gadget,
                           allocation,
                           gtk_widget_get_allocated_baseline (widget),
                           &clip);

  gtk_widget_set_clip (widget, &clip);
}

static void
gtk_button_allocate (GtkCssGadget        *gadget,
                     const GtkAllocation *allocation,
                     int                  baseline,
                     GtkAllocation       *out_clip,
                     gpointer             unused)
{
  GtkWidget *widget;
  GtkWidget *child;

  widget = gtk_css_gadget_get_owner (gadget);

  child = gtk_bin_get_child (GTK_BIN (widget));
  if (child && gtk_widget_get_visible (child))
    gtk_widget_size_allocate_with_baseline (child, (GtkAllocation *)allocation, baseline);

  if (gtk_widget_get_realized (widget))
    {
      GtkAllocation border_allocation;
      gtk_css_gadget_get_border_allocation (gadget, &border_allocation, NULL);
      gdk_window_move_resize (GTK_BUTTON (widget)->priv->event_window,
                              border_allocation.x,
                              border_allocation.y,
                              border_allocation.width,
                              border_allocation.height);
    }

  gtk_container_get_children_clip (GTK_CONTAINER (widget), out_clip);
}

static gboolean
gtk_button_draw (GtkWidget *widget,
		 cairo_t   *cr)
{
  gtk_css_gadget_draw (GTK_BUTTON (widget)->priv->gadget, cr);

  return FALSE;
}

static gboolean
gtk_button_render (GtkCssGadget *gadget,
                   cairo_t      *cr,
                   int           x,
                   int           y,
                   int           width,
                   int           height,
                   gpointer      data)
{
  GtkWidget *widget;

  widget = gtk_css_gadget_get_owner (gadget);

  GTK_WIDGET_CLASS (gtk_button_parent_class)->draw (widget, cr);

  return gtk_widget_has_visible_focus (widget);
}

static void
gtk_button_do_release (GtkButton *button,
                       gboolean   emit_clicked)
{
  GtkButtonPrivate *priv = button->priv;

  if (priv->button_down)
    {
      priv->button_down = FALSE;

      if (priv->activate_timeout)
	return;

      if (emit_clicked)
        gtk_button_clicked (button);

      gtk_button_update_state (button);
    }
}

static gboolean
gtk_button_grab_broken (GtkWidget          *widget,
			GdkEventGrabBroken *event)
{
  GtkButton *button = GTK_BUTTON (widget);
  
  gtk_button_do_release (button, FALSE);

  return TRUE;
}

static gboolean
gtk_button_key_release (GtkWidget   *widget,
			GdkEventKey *event)
{
  GtkButton *button = GTK_BUTTON (widget);
  GtkButtonPrivate *priv = button->priv;

  if (priv->activate_timeout)
    {
      gtk_button_finish_activate (button, TRUE);
      return TRUE;
    }
  else if (GTK_WIDGET_CLASS (gtk_button_parent_class)->key_release_event)
    return GTK_WIDGET_CLASS (gtk_button_parent_class)->key_release_event (widget, event);
  else
    return FALSE;
}

static gboolean
gtk_button_enter_notify (GtkWidget        *widget,
			 GdkEventCrossing *event)
{
  GtkButton *button = GTK_BUTTON (widget);
  GtkButtonPrivate *priv = button->priv;

  if ((event->window == button->priv->event_window) &&
      (event->detail != GDK_NOTIFY_INFERIOR))
    {
      priv->in_button = TRUE;
      gtk_button_update_state (button);
    }

  return FALSE;
}

static gboolean
gtk_button_leave_notify (GtkWidget        *widget,
			 GdkEventCrossing *event)
{
  GtkButton *button = GTK_BUTTON (widget);
  GtkButtonPrivate *priv = button->priv;

  if ((event->window == button->priv->event_window) &&
      (event->detail != GDK_NOTIFY_INFERIOR))
    {
      priv->in_button = FALSE;
      gtk_button_update_state (button);
    }

  return FALSE;
}

static void
gtk_real_button_clicked (GtkButton *button)
{
  GtkButtonPrivate *priv = button->priv;

  if (priv->action_helper)
    gtk_action_helper_activate (priv->action_helper);
}

static gboolean
button_activate_timeout (gpointer data)
{
  gtk_button_finish_activate (data, TRUE);

  return FALSE;
}

static void
gtk_real_button_activate (GtkButton *button)
{
  GtkWidget *widget = GTK_WIDGET (button);
  GtkButtonPrivate *priv = button->priv;
  GdkDevice *device;
  guint32 time;

  device = gtk_get_current_event_device ();

  if (device && gdk_device_get_source (device) != GDK_SOURCE_KEYBOARD)
    device = gdk_device_get_associated_device (device);

  if (gtk_widget_get_realized (widget) && !priv->activate_timeout)
    {
      time = gtk_get_current_event_time ();

      /* bgo#626336 - Only grab if we have a device (from an event), not if we
       * were activated programmatically when no event is available.
       */
      if (device && gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
	{
          if (gdk_seat_grab (gdk_device_get_seat (device), priv->event_window,
                             GDK_SEAT_CAPABILITY_KEYBOARD, TRUE,
                             NULL, NULL, NULL, NULL) == GDK_GRAB_SUCCESS)
            {
              gtk_device_grab_add (widget, device, TRUE);
              priv->grab_keyboard = device;
              priv->grab_time = time;
	    }
	}

      priv->activate_timeout = gdk_threads_add_timeout (ACTIVATE_TIMEOUT,
						button_activate_timeout,
						button);
      g_source_set_name_by_id (priv->activate_timeout, "[gtk+] button_activate_timeout");
      priv->button_down = TRUE;
      gtk_button_update_state (button);
    }
}

static void
gtk_button_finish_activate (GtkButton *button,
			    gboolean   do_it)
{
  GtkWidget *widget = GTK_WIDGET (button);
  GtkButtonPrivate *priv = button->priv;

  g_source_remove (priv->activate_timeout);
  priv->activate_timeout = 0;

  if (priv->grab_keyboard)
    {
      gdk_seat_ungrab (gdk_device_get_seat (priv->grab_keyboard));
      gtk_device_grab_remove (widget, priv->grab_keyboard);
      priv->grab_keyboard = NULL;
    }

  priv->button_down = FALSE;

  gtk_button_update_state (button);

  if (do_it)
    gtk_button_clicked (button);
}


static void
gtk_button_measure (GtkCssGadget   *gadget,
		    GtkOrientation  orientation,
                    int             for_size,
		    int            *minimum,
		    int            *natural,
		    int            *minimum_baseline,
		    int            *natural_baseline,
                    gpointer        data)
{
  GtkWidget *widget;
  GtkWidget *child;

  widget = gtk_css_gadget_get_owner (gadget);
  child = gtk_bin_get_child (GTK_BIN (widget));

  if (child && gtk_widget_get_visible (child))
    {
       _gtk_widget_get_preferred_size_for_size (child,
                                                orientation,
                                                for_size,
                                                minimum, natural,
                                                minimum_baseline, natural_baseline);
    }
  else
    {
      *minimum = 0;
      *natural = 0;
      if (minimum_baseline)
        *minimum_baseline = 0;
      if (natural_baseline)
        *natural_baseline = 0;
    }
}

static void
gtk_button_get_preferred_width (GtkWidget *widget,
                                gint      *minimum_size,
                                gint      *natural_size)
{
  gtk_css_gadget_get_preferred_size (GTK_BUTTON (widget)->priv->gadget,
                                     GTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
gtk_button_get_preferred_height (GtkWidget *widget,
                                 gint      *minimum_size,
                                 gint      *natural_size)
{
  gtk_css_gadget_get_preferred_size (GTK_BUTTON (widget)->priv->gadget,
                                     GTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
gtk_button_get_preferred_width_for_height (GtkWidget *widget,
                                           gint       for_size,
                                           gint      *minimum_size,
                                           gint      *natural_size)
{
  gtk_css_gadget_get_preferred_size (GTK_BUTTON (widget)->priv->gadget,
                                     GTK_ORIENTATION_HORIZONTAL,
                                     for_size,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
gtk_button_get_preferred_height_for_width (GtkWidget *widget,
                                           gint       for_size,
                                           gint      *minimum_size,
                                           gint      *natural_size)
{
  gtk_css_gadget_get_preferred_size (GTK_BUTTON (widget)->priv->gadget,
                                     GTK_ORIENTATION_VERTICAL,
                                     for_size,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
gtk_button_get_preferred_height_and_baseline_for_width (GtkWidget *widget,
							gint       for_size,
							gint      *minimum_size,
							gint      *natural_size,
							gint      *minimum_baseline,
							gint      *natural_baseline)
{
  gtk_css_gadget_get_preferred_size (GTK_BUTTON (widget)->priv->gadget,
                                     GTK_ORIENTATION_VERTICAL,
                                     for_size,
                                     minimum_size, natural_size,
                                     minimum_baseline, natural_baseline);
}

/**
 * gtk_button_set_label:
 * @button: a #GtkButton
 * @label: a string
 *
 * Sets the text of the label of the button to @str.
 *
 * This will also clear any previously set labels.
 */
void
gtk_button_set_label (GtkButton   *button,
		      const gchar *label)
{
  GtkButtonPrivate *priv;
  gchar *new_label;

  g_return_if_fail (GTK_IS_BUTTON (button));

  priv = button->priv;

  new_label = g_strdup (label);
  g_free (priv->label_text);
  priv->label_text = new_label;

  gtk_button_construct_child (button);
  
  g_object_notify_by_pspec (G_OBJECT (button), props[PROP_LABEL]);
}

/**
 * gtk_button_get_label:
 * @button: a #GtkButton
 *
 * Fetches the text from the label of the button, as set by
 * gtk_button_set_label(). If the label text has not 
 * been set the return value will be %NULL. This will be the 
 * case if you create an empty button with gtk_button_new() to 
 * use as a container.
 *
 * Returns: The text of the label widget. This string is owned
 * by the widget and must not be modified or freed.
 */
const gchar *
gtk_button_get_label (GtkButton *button)
{
  g_return_val_if_fail (GTK_IS_BUTTON (button), NULL);

  return button->priv->label_text;
}

/**
 * gtk_button_set_use_underline:
 * @button: a #GtkButton
 * @use_underline: %TRUE if underlines in the text indicate mnemonics
 *
 * If true, an underline in the text of the button label indicates
 * the next character should be used for the mnemonic accelerator key.
 */
void
gtk_button_set_use_underline (GtkButton *button,
			      gboolean   use_underline)
{
  GtkButtonPrivate *priv;

  g_return_if_fail (GTK_IS_BUTTON (button));

  priv = button->priv;

  use_underline = use_underline != FALSE;

  if (use_underline != priv->use_underline)
    {
      priv->use_underline = use_underline;

      gtk_button_construct_child (button);
      
      g_object_notify_by_pspec (G_OBJECT (button), props[PROP_USE_UNDERLINE]);
    }
}

/**
 * gtk_button_get_use_underline:
 * @button: a #GtkButton
 *
 * Returns whether an embedded underline in the button label indicates a
 * mnemonic. See gtk_button_set_use_underline ().
 *
 * Returns: %TRUE if an embedded underline in the button label
 *               indicates the mnemonic accelerator keys.
 */
gboolean
gtk_button_get_use_underline (GtkButton *button)
{
  g_return_val_if_fail (GTK_IS_BUTTON (button), FALSE);

  return button->priv->use_underline;
}

static void
gtk_button_update_state (GtkButton *button)
{
  GtkButtonPrivate *priv = button->priv;
  GtkStateFlags new_state;
  gboolean depressed;

  if (priv->activate_timeout)
    depressed = TRUE;
  else
    depressed = priv->in_button && priv->button_down;

  new_state = gtk_widget_get_state_flags (GTK_WIDGET (button)) &
    ~(GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_ACTIVE);

  if (priv->in_button)
    new_state |= GTK_STATE_FLAG_PRELIGHT;

  if (depressed)
    new_state |= GTK_STATE_FLAG_ACTIVE;

  gtk_widget_set_state_flags (GTK_WIDGET (button), new_state, TRUE);
}

static void 
show_image_change_notify (GtkButton *button)
{
  GtkButtonPrivate *priv = button->priv;

  if (priv->image) 
    {
      gtk_widget_show (priv->image);
    }
}

static void
gtk_button_screen_changed (GtkWidget *widget,
			   GdkScreen *previous_screen)
{
  GtkButton *button;
  GtkButtonPrivate *priv;

  if (!gtk_widget_has_screen (widget))
    return;

  button = GTK_BUTTON (widget);
  priv = button->priv;

  /* If the button is being pressed while the screen changes the
    release might never occur, so we reset the state. */
  if (priv->button_down)
    {
      priv->button_down = FALSE;
      gtk_button_update_state (button);
    }

  show_image_change_notify (button);
}

static void
gtk_button_state_changed (GtkWidget    *widget,
                          GtkStateType  previous_state)
{
  GtkButton *button = GTK_BUTTON (widget);

  if (!gtk_widget_is_sensitive (widget))
    gtk_button_do_release (button, FALSE);
}

static void
gtk_button_grab_notify (GtkWidget *widget,
			gboolean   was_grabbed)
{
  GtkButton *button = GTK_BUTTON (widget);
  GtkButtonPrivate *priv = button->priv;

  if (priv->activate_timeout &&
      priv->grab_keyboard &&
      gtk_widget_device_is_shadowed (widget, priv->grab_keyboard))
    gtk_button_finish_activate (button, FALSE);

  if (!was_grabbed)
    gtk_button_do_release (button, FALSE);
}

/**
 * gtk_button_set_image:
 * @button: a #GtkButton
 * @image: a widget to set as the image for the button
 *
 * Set the image of @button to the given widget. The image will be
 * displayed if the label text is %NULL or if
 * #GtkButton:always-show-image is %TRUE. You don’t have to call
 * gtk_widget_show() on @image yourself.
 *
 * Since: 2.6
 */
void
gtk_button_set_image (GtkButton *button,
		      GtkWidget *image)
{
  GtkButtonPrivate *priv;
  GtkWidget *parent;

  g_return_if_fail (GTK_IS_BUTTON (button));
  g_return_if_fail (image == NULL || GTK_IS_WIDGET (image));

  priv = button->priv;

  if (priv->image)
    {
      parent = gtk_widget_get_parent (priv->image);
      if (parent)
        gtk_container_remove (GTK_CONTAINER (parent), priv->image);
    }

  priv->image = image;

  gtk_button_construct_child (button);

  g_object_notify_by_pspec (G_OBJECT (button), props[PROP_IMAGE]);
}

/**
 * gtk_button_get_image:
 * @button: a #GtkButton
 *
 * Gets the widget that is currenty set as the image of @button.
 *
 * Returns: (nullable) (transfer none): a #GtkWidget or %NULL in case
 *     there is no image
 *
 * Since: 2.6
 */
GtkWidget *
gtk_button_get_image (GtkButton *button)
{
  g_return_val_if_fail (GTK_IS_BUTTON (button), NULL);
  
  return button->priv->image;
}

/**
 * gtk_button_set_image_position:
 * @button: a #GtkButton
 * @position: the position
 *
 * Sets the position of the image relative to the text 
 * inside the button.
 *
 * Since: 2.10
 */ 
void
gtk_button_set_image_position (GtkButton       *button,
			       GtkPositionType  position)
{
  GtkButtonPrivate *priv;

  g_return_if_fail (GTK_IS_BUTTON (button));
  g_return_if_fail (position >= GTK_POS_LEFT && position <= GTK_POS_BOTTOM);
  
  priv = button->priv;

  if (priv->image_position != position)
    {
      priv->image_position = position;

      gtk_button_construct_child (button);

      g_object_notify_by_pspec (G_OBJECT (button), props[PROP_IMAGE_POSITION]);
    }
}

/**
 * gtk_button_get_image_position:
 * @button: a #GtkButton
 *
 * Gets the position of the image relative to the text 
 * inside the button.
 *
 * Returns: the position
 *
 * Since: 2.10
 */
GtkPositionType
gtk_button_get_image_position (GtkButton *button)
{
  g_return_val_if_fail (GTK_IS_BUTTON (button), GTK_POS_LEFT);
  
  return button->priv->image_position;
}

/**
 * gtk_button_set_always_show_image:
 * @button: a #GtkButton
 * @always_show: %TRUE if the menuitem should always show the image
 *
 * If %TRUE, the button will ignore the #GtkSettings:gtk-button-images
 * setting and always show the image, if available.
 *
 * Use this property if the button  would be useless or hard to use
 * without the image.
 *
 * Since: 3.6
 */
void
gtk_button_set_always_show_image (GtkButton *button,
                                  gboolean    always_show)
{
  GtkButtonPrivate *priv;

  g_return_if_fail (GTK_IS_BUTTON (button));

  priv = button->priv;

  if (priv->always_show_image != always_show)
    {
      priv->always_show_image = always_show;

      if (priv->image)
        {
          gtk_widget_show (priv->image);
        }

      g_object_notify_by_pspec (G_OBJECT (button), props[PROP_ALWAYS_SHOW_IMAGE]);
    }
}

/**
 * gtk_button_get_always_show_image:
 * @button: a #GtkButton
 *
 * Returns whether the button will ignore the #GtkSettings:gtk-button-images
 * setting and always show the image, if available.
 *
 * Returns: %TRUE if the button will always show the image
 *
 * Since: 3.6
 */
gboolean
gtk_button_get_always_show_image (GtkButton *button)
{
  g_return_val_if_fail (GTK_IS_BUTTON (button), FALSE);

  return button->priv->always_show_image;
}

/**
 * gtk_button_get_event_window:
 * @button: a #GtkButton
 *
 * Returns the button’s event window if it is realized, %NULL otherwise.
 * This function should be rarely needed.
 *
 * Returns: (transfer none): @button’s event window.
 *
 * Since: 2.22
 */
GdkWindow*
gtk_button_get_event_window (GtkButton *button)
{
  g_return_val_if_fail (GTK_IS_BUTTON (button), NULL);

  return button->priv->event_window;
}
