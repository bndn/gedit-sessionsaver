/*
 * gedit-bookmarks-message-goto-previous.h
 * This file is part of gedit
 *
 * Copyright (C) 2011 - Paolo Borelli
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef __GEDIT_BOOKMARKS_MESSAGE_GOTO_PREVIOUS_H__
#define __GEDIT_BOOKMARKS_MESSAGE_GOTO_PREVIOUS_H__

#include <gedit/gedit-message.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_BOOKMARKS_MESSAGE_GOTO_PREVIOUS            (gedit_bookmarks_message_goto_previous_get_type ())
#define GEDIT_BOOKMARKS_MESSAGE_GOTO_PREVIOUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                               GEDIT_TYPE_BOOKMARKS_MESSAGE_GOTO_PREVIOUS,\
                                                               GeditBookmarksMessageGotoPrevious))
#define GEDIT_BOOKMARKS_MESSAGE_GOTO_PREVIOUS_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                                               GEDIT_TYPE_BOOKMARKS_MESSAGE_GOTO_PREVIOUS,\
                                                               GeditBookmarksMessageGotoPrevious const))
#define GEDIT_BOOKMARKS_MESSAGE_GOTO_PREVIOUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                                               GEDIT_TYPE_BOOKMARKS_MESSAGE_GOTO_PREVIOUS,\
                                                               GeditBookmarksMessageGotoPreviousClass))
#define GEDIT_IS_BOOKMARKS_MESSAGE_GOTO_PREVIOUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                                               GEDIT_TYPE_BOOKMARKS_MESSAGE_GOTO_PREVIOUS))
#define GEDIT_IS_BOOKMARKS_MESSAGE_GOTO_PREVIOUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                                               GEDIT_TYPE_BOOKMARKS_MESSAGE_GOTO_PREVIOUS))
#define GEDIT_BOOKMARKS_MESSAGE_GOTO_PREVIOUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                                               GEDIT_TYPE_BOOKMARKS_MESSAGE_GOTO_PREVIOUS,\
                                                               GeditBookmarksMessageGotoPreviousClass))

typedef struct _GeditBookmarksMessageGotoPrevious        GeditBookmarksMessageGotoPrevious;
typedef struct _GeditBookmarksMessageGotoPreviousClass   GeditBookmarksMessageGotoPreviousClass;
typedef struct _GeditBookmarksMessageGotoPreviousPrivate GeditBookmarksMessageGotoPreviousPrivate;

struct _GeditBookmarksMessageGotoPrevious
{
	GeditMessage parent;

	GeditBookmarksMessageGotoPreviousPrivate *priv;
};

struct _GeditBookmarksMessageGotoPreviousClass
{
	GeditMessageClass parent_class;
};

GType gedit_bookmarks_message_goto_previous_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_BOOKMARKS_MESSAGE_GOTO_PREVIOUS_H__ */
