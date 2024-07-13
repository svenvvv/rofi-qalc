/*
 * rofi-qalculate
 * Copyright (C) 2024 svenvvv
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#pragma once

#include <gmodule.h>

struct RofiViewState;

enum RofiBindingsScope {
    GLOBAL,
};

enum RofiAction {
  /** Paste from primary clipboard */
  PASTE_PRIMARY = 1,
  /** Paste from secondary clipboard */
  PASTE_SECONDARY,
  /** Copy to secondary clipboard */
  COPY_SECONDARY,
  /** Clear the entry box. */
  CLEAR_LINE,
  // we don't need any more...
};

extern "C" void rofi_view_reload(void);
extern "C" void rofi_view_trigger_action(RofiViewState * state,
                                         RofiBindingsScope scope, guint action);
extern "C" RofiViewState * rofi_view_get_active();

