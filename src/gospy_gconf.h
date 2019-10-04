/*
 * Copyright (C) 2004-2007 Emmanuel Saracco <esaracco@users.labs.libre-entreprise.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _GOSPY_GCONF_H
#define _GOSPY_GCONF_H

void gs_set_display_bg_color (const gchar * val);
void gs_set_display_tooltips (const gboolean val);
void gs_set_display_main_icon (const gboolean val);
void gs_set_display_spies_icons (const gboolean val);
gchar *gs_get_display_bg_color (void);
gboolean gs_get_display_tooltips (void);
gboolean gs_get_display_main_icon (void);
gboolean gs_get_display_spies_icons (void);

#endif
