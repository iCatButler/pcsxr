/*
 * Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
 * All Rights Reserved.
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
 * along with this program; if not, see <http://www.gnu.org/licenses>.
 */

#include "cfg.c"

#include <time.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

GtkWidget *MainWindow;
GtkBuilder *xml;


const int NUM_KEYLIST = 3; // emukey + num controllers
const char *widgetname_treeview[3] = {"treeview_e", "treeview1", "treeview2"};
const char *widgetname_change[3] = {"btnchange_e", "btnchange1", "btnchange2"};
const char *widgetname_reset[3] = {"btnreset_e", "btnreset1", "btnreset2"};
const char *widgetname_combodev[3] = {"combodev_e", "combodev1", "combodev2"};

// TODO: this could be removed if the underlying enum order is changed like this has, so then GUI's order will be the same
const int DPad[DKEY_TOTAL] = {
	DKEY_UP,
	DKEY_DOWN,
	DKEY_LEFT,
	DKEY_RIGHT,
	DKEY_CROSS,
	DKEY_CIRCLE,
	DKEY_SQUARE,
	DKEY_TRIANGLE,
	DKEY_L1,
	DKEY_R1,
	DKEY_L2,
	DKEY_R2,
	DKEY_SELECT,
	DKEY_START,
	DKEY_L3,
	DKEY_R3,
	DKEY_ANALOG
};


const char *EmuKeyText[EMU_TOTAL] = {
	N_("Increment state slot"),
	N_("Fast-forwards"),
	N_("Load state"),
	N_("Save state"),
	N_("Screenshot"),
	N_("Escape"),
	N_("Rewind"),
	N_("Alt Speed 1"),
	N_("Alt Speed 2")
};

const char *DPadText[DKEY_TOTAL] = {
	N_("D-Pad Up"),
	N_("D-Pad Down"),
	N_("D-Pad Left"),
	N_("D-Pad Right"),
	N_("Cross"),
	N_("Circle"),
	N_("Square"),
	N_("Triangle"),
	N_("L1"),
	N_("R1"),
	N_("L2"),
	N_("R2"),
	N_("Select"),
	N_("Start"),
	N_("L3"),
	N_("R3"),
	N_("Analog")
};

const char *AnalogText[] = {
	N_("L-Stick Right"),
	N_("L-Stick Left"),
	N_("L-Stick Down"),
	N_("L-Stick Up"),
	N_("R-Stick Right"),
	N_("R-Stick Left"),
	N_("R-Stick Down"),
	N_("R-Stick Up")
};

static int GetSelectedKeyIndex(int padnum) {
	GtkTreeSelection	*selection;
	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkTreePath			*path;
	gboolean			selected;
	int					i;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtk_builder_get_object(xml, widgetname_treeview[padnum+1])));
	selected = gtk_tree_selection_get_selected(selection, &model, &iter);

	if (!selected) {
		return -1;
	}

	path = gtk_tree_model_get_path(model, &iter);
	i = *gtk_tree_path_get_indices(path);
	gtk_tree_path_free(path);

	return i;
}

static void GetKeyDescription(char *buf, int joynum, int key) {
	const char *hatname[16] = {_("Centered"), _("Up"), _("Right"), _("Rightup"),
		_("Down"), "", _("Rightdown"), "", _("Left"), _("Leftup"), "", "",
		_("Leftdown"), "", "", ""};

	KEYDEF* keydef = joynum < 0 ? &g.cfg.E.EmuDef[key].Mapping : &g.cfg.PadDef[joynum].KeyDef[key];

	switch (keydef->JoyEvType) {
		case BUTTON:
			sprintf(buf, _("Joystick: Button %d"), keydef->J.Button);
			break;

		case AXIS:
			sprintf(buf, _("Joystick: Axis %d%c"), abs(keydef->J.Axis) - 1,
				keydef->J.Axis > 0 ? '+' : '-');
			break;

		case HAT:
			sprintf(buf, _("Joystick: Hat %d %s"), (keydef->J.Hat >> 8),
				hatname[keydef->J.Hat & 0x0F]);
			break;

		case NONE:
		default:
			buf[0] = '\0';
			break;
	}

	if (keydef->Key != 0) {
		if (buf[0] != '\0') {
			strcat(buf, " / ");
		}

		strcat(buf, _("Keyboard:"));
		strcat(buf, " ");
		strcat(buf, XKeysymToString(keydef->Key));
	} else if (buf[0] == '\0') {
		strcpy(buf, _("(Not Set)"));
	}
}

static void GetAnalogDescription(char *buf, int joynum, int analognum, int dir) {
	const char *hatname[16] = {_("Centered"), _("Up"), _("Right"), _("Rightup"),
		_("Down"), "", _("Rightdown"), "", _("Left"), _("Leftup"), "", "",
		_("Leftdown"), "", "", ""};

	switch (g.cfg.PadDef[joynum].AnalogDef[analognum][dir].JoyEvType) {
		case BUTTON:
			sprintf(buf, _("Joystick: Button %d"), g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Button);
			break;

		case AXIS:
			sprintf(buf, _("Joystick: Axis %d%c"), abs(g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Axis) - 1,
				g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Axis > 0 ? '+' : '-');
			break;

		case HAT:
			sprintf(buf, _("Joystick: Hat %d %s"), (g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Hat >> 8),
				hatname[g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Hat & 0x0F]);
			break;

		case NONE:
		default:
			buf[0] = '\0';
			break;
	}

	if (g.cfg.PadDef[joynum].AnalogDef[analognum][dir].Key != 0) {
		if (buf[0] != '\0') {
			strcat(buf, " / ");
		}

		strcat(buf, _("Keyboard:"));
		strcat(buf, " ");
		strcat(buf, XKeysymToString(g.cfg.PadDef[joynum].AnalogDef[analognum][dir].Key));
	} else if (buf[0] == '\0') {
		strcpy(buf, _("(Not Set)"));
	}
}

static void UpdateKeyList() {
	GtkWidget *widget;
	GtkListStore *store;
	GtkTreeIter iter;
	int i, j;
	char buf[256];

	for (i = 0; i < NUM_KEYLIST; i++) {
		int total;

		widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_treeview[i]));
		store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

		if ( i == 0 ) {
			total = EMU_TOTAL;
		}
		else {
		switch(g.cfg.PadDef[i-1].Type)
		{
			case PSE_PAD_TYPE_MOUSE:
				total = 0;
				break;
			case PSE_PAD_TYPE_STANDARD:
				total = DKEY_TOTAL - 3;
				break;
			case PSE_PAD_TYPE_ANALOGPAD:
				total = DKEY_TOTAL;
				break;
		}
		}

		for (j = 0; j < total; j++) {
			gtk_list_store_append(store, &iter);
			GetKeyDescription(buf, i-1, i == 0 ? j : DPad[j]); // change order of orig. typedef to up, down, etc
			gtk_list_store_set(store, &iter, 0, i == 0 ? _(EmuKeyText[j]) : _(DPadText[j]), 1, buf, -1);
		}

		if (i > 0 && g.cfg.PadDef[i-1].Type == PSE_PAD_TYPE_ANALOGPAD) {
			for (j = 0; j < 8; j++) {
				gtk_list_store_append(store, &iter);
				GetAnalogDescription(buf, i-1, j / 4, j % 4);
				gtk_list_store_set(store, &iter, 0, _(AnalogText[j]), 1, buf, -1);
			}
		}

		gtk_tree_view_set_model(GTK_TREE_VIEW(widget), GTK_TREE_MODEL(store));
		g_object_unref(G_OBJECT(store));
		gtk_widget_show(widget);
	}
}

static void UpdateKey() {
	int i, index;
	GtkWidget *widget;
	GtkTreeModel *model;
	GtkTreeIter iter;
	char buf[256];

	for (i = 0; i < NUM_KEYLIST; i++) {
		GValue value = { 0, };
		index = GetSelectedKeyIndex(i-1);
		if (index == -1) continue;

		widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_treeview[i]));
		gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)), &model, &iter);

		// this can be removed if DPAD[index] is removed
		if (i == 0) {
			GetKeyDescription(buf, i-1, index);
		}
		else if (index < DKEY_TOTAL) {
			GetKeyDescription(buf, i-1, DPad[index]);
		} else {
			GetAnalogDescription(buf, i-1, (index - DKEY_TOTAL) / 4, (index - DKEY_TOTAL) % 4);
		}

		g_value_init(&value, G_TYPE_STRING);
		g_value_set_string(&value, buf);
		gtk_list_store_set_value(GTK_LIST_STORE(model), &iter, 1, &value);
	}
}

static void OnConfigExit(GtkWidget *widget, gpointer user_data) {
	SavePADConfig();

	gtk_widget_destroy(widget);
	SDL_Quit();
	XCloseDisplay(g.Disp);

	exit(0);
}

static void TreeSelectionChanged(GtkTreeSelection *selection, gpointer user_data) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;

	gboolean selected;
	int i;

	selected = gtk_tree_selection_get_selected(selection, &model, &iter);

	if (selected) {
		path = gtk_tree_model_get_path(model, &iter);
		i = *gtk_tree_path_get_indices(path);
		gtk_tree_path_free(path);

		// If a row was selected, and the row is not blank, we can now enable
		// some of the disabled widgets
	}
	gint padnum = GPOINTER_TO_INT(user_data);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(xml, widgetname_reset[padnum+1])), selected);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(xml, widgetname_change[padnum+1])), selected);
}

static void OnDeviceChanged(GtkWidget *widget, gpointer user_data) {
	int n = GPOINTER_TO_INT(user_data);
	int current = gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) - 1;
	if (n >= 0) {
		g.cfg.PadDef[n].DevNum = current;
	} else {
		g.cfg.E.DevNum = current;
	}
}

static void OnTypeChanged(GtkWidget *widget, gpointer user_data) {
	uint n = GPOINTER_TO_UINT(user_data), current = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    char checkbtn[9];

	int padTypeList[] = {
		PSE_PAD_TYPE_STANDARD,
		PSE_PAD_TYPE_ANALOGPAD,
		PSE_PAD_TYPE_MOUSE
	};

	g.cfg.PadDef[n].Type = padTypeList[current];

    snprintf(checkbtn, sizeof(checkbtn), "checkpv%d", n+1);
    gtk_widget_set_sensitive(GTK_WIDGET(
        gtk_builder_get_object(xml, checkbtn)), (g.cfg.PadDef[n].Type == PSE_PAD_TYPE_ANALOGPAD));

	UpdateKeyList();
}

static void OnThreadedToggled(GtkWidget *widget, gpointer user_data) {
	g.cfg.Threaded = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void OnVisualVibration1Toggled(GtkWidget *widget, gpointer user_data) {
	g.cfg.PadDef[0].VisualVibration = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void OnVisualVibration2Toggled(GtkWidget *widget, gpointer user_data) {
	g.cfg.PadDef[1].VisualVibration = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void OnPhysicalVibration1Toggled(GtkWidget *widget, gpointer user_data) {
    g.cfg.PadDef[0].PhysicalVibration = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void OnPhysicalVibration2Toggled(GtkWidget *widget, gpointer user_data) {
    g.cfg.PadDef[1].PhysicalVibration = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void OnHideCursorToggled(GtkWidget *widget, gpointer user_data) {
	(void)user_data; // unused
	g.cfg.HideCursor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void OnPreventScrSaverToggled(GtkWidget *widget, gpointer user_data) {
	(void)user_data; // unused
	g.cfg.PreventScrSaver = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void ReadDKeyEvent(int padnum, int key) {
	SDL_Joystick *js;
	int i;
	unsigned char buttons[32];
	uint16_t Key;

	KEYDEF* keydef = padnum < 0 ? &g.cfg.E.EmuDef[key].Mapping : &g.cfg.PadDef[padnum].KeyDef[key];
	int8_t devnum = padnum < 0 ? g.cfg.E.DevNum : g.cfg.PadDef[padnum].DevNum;

	if (devnum >= 0) {
        SDL_JoystickEventState(SDL_ENABLE);
		js = SDL_JoystickOpen(devnum);
		SDL_JoystickUpdate();
	} else {
		js = NULL;
        return;
	}

	SDL_JoystickUpdate();
    
    time_t t = time(NULL);
    SDL_Event event;
    while (time(NULL) < t + 10) {
        uint8_t found_key = 0;
        while (SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_JOYAXISMOTION:
                    if (abs(event.jaxis.value) > 16383) {
                        keydef->JoyEvType = AXIS;
                        keydef->J.Axis = (event.jaxis.axis + 1) * (event.jaxis.value > 0 ? 1 : -1);
                        found_key = 1;
                    }
                    break;
                case SDL_JOYHATMOTION:
                    if (event.jhat.value != SDL_HAT_CENTERED) {
                        keydef->JoyEvType = HAT;
                        keydef->J.Hat = ((event.jhat.hat << 8) | event.jhat.value);
                        found_key = 1;
                    }
                    break;
                case SDL_JOYBUTTONDOWN:
                    keydef->JoyEvType = BUTTON;
                    keydef->J.Button = event.jbutton.button;
                    found_key = 1;
                    break;
                default:
                    break;
            }
        }

		XQueryKeymap(g.Disp, buttons);
		for (i = 0; i < 256; ++i) {
			if(buttons[i >> 3] & (1 << (i & 7))) {
				Key = XkbKeycodeToKeysym(g.Disp, i, 0, 0);
				if(Key != XK_Escape) {
					keydef->Key = Key;
				}
                found_key = 1;
			}
		}

        if (found_key) {
            break;
        }
        usleep(5000);
    }

	if (js != NULL) {
		SDL_JoystickClose(js);
	}
}

static void ReadAnalogEvent(int padnum, int analognum, int analogdir) {
	SDL_Joystick *js;
	time_t t;
	GdkEvent *ge;
	int i;
	Sint16 axis, numAxes = 0, InitAxisPos[256], PrevAxisPos[256];
	unsigned char buttons[32];
	uint16_t Key;

	if (g.cfg.PadDef[padnum].DevNum >= 0) {
		js = SDL_JoystickOpen(g.cfg.PadDef[padnum].DevNum);
		SDL_JoystickEventState(SDL_IGNORE);

		SDL_JoystickUpdate();

		numAxes = SDL_JoystickNumAxes(js);
		if (numAxes > 256) numAxes = 256;

		for (i = 0; i < SDL_JoystickNumAxes(js); i++) {
			InitAxisPos[i] = PrevAxisPos[i] = SDL_JoystickGetAxis(js, i);
		}
	} else {
		js = NULL;
	}

	t = time(NULL);

	while (time(NULL) < t + 10) {
		// check joystick events
		if (js != NULL) {
			SDL_JoystickUpdate();

			for (i = 0; i < SDL_JoystickNumButtons(js); i++) {
				if (SDL_JoystickGetButton(js, i)) {
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].JoyEvType = BUTTON;
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Button = i;
					goto end;
				}
			}

			for (i = 0; i < numAxes; i++) {
				axis = SDL_JoystickGetAxis(js, i);
				if (abs(axis) > 16383 && (abs(axis - InitAxisPos[i]) > 4096 || abs(axis - PrevAxisPos[i]) > 4096)) {
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].JoyEvType = AXIS;
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Axis = (i + 1) * (axis > 0 ? 1 : -1);
					goto end;
				}
				PrevAxisPos[i] = axis;
			}

			for (i = 0; i < SDL_JoystickNumHats(js); i++) {
				axis = SDL_JoystickGetHat(js, i);
				if (axis != SDL_HAT_CENTERED) {
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].JoyEvType = HAT;

					if (axis & SDL_HAT_UP) {
						g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Hat = ((i << 8) | SDL_HAT_UP);
					} else if (axis & SDL_HAT_DOWN) {
						g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Hat = ((i << 8) | SDL_HAT_DOWN);
					} else if (axis & SDL_HAT_LEFT) {
						g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Hat = ((i << 8) | SDL_HAT_LEFT);
					} else if (axis & SDL_HAT_RIGHT) {
						g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Hat = ((i << 8) | SDL_HAT_RIGHT);
					}

					goto end;
				}
			}
		}

		// check keyboard events
		XQueryKeymap(g.Disp, buttons);
		for (i = 0; i < 256; ++i) {
			if(buttons[i >> 3] & (1 << (i & 7))) {
				Key = XkbKeycodeToKeysym(g.Disp, i, 0, 0);
				if(Key != XK_Escape) {
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].Key = Key;
				}
				goto end;
			}
		}

		usleep(5000);
	}

end:
	if (js != NULL) {
		SDL_JoystickClose(js);
	}
}

static void OnChangeClicked(GtkWidget* widget, gpointer user_data) {
	int pad = GPOINTER_TO_INT(user_data);
	int index = GetSelectedKeyIndex(pad);

	if (index == -1) {
		return;
	} else if (pad < 0) {
		ReadDKeyEvent(pad, index); // order matches EMUKEY struct
	} else if (index < DKEY_TOTAL) {
		ReadDKeyEvent(pad, DPad[index]);
	} else {
		index -= DKEY_TOTAL;
		ReadAnalogEvent(pad, index / 4, index % 4);
	}

	UpdateKey();
}

static void OnResetClicked(GtkWidget *widget, gpointer user_data) {
	int pad = GPOINTER_TO_INT(user_data);
	int index = GetSelectedKeyIndex(pad);

	if (index == -1) {
		return;
	} else if (pad < 0) {
		g.cfg.E.EmuDef[index].Mapping.Key = 0;
		g.cfg.E.EmuDef[index].Mapping.JoyEvType = NONE;
		g.cfg.E.EmuDef[index].Mapping.J.Button = 0;
	} else if (index < DKEY_TOTAL) {
		g.cfg.PadDef[pad].KeyDef[DPad[index]].Key = 0;
		g.cfg.PadDef[pad].KeyDef[DPad[index]].JoyEvType = NONE;
		g.cfg.PadDef[pad].KeyDef[DPad[index]].J.Button = 0;
	} else {
		index -= DKEY_TOTAL;
		g.cfg.PadDef[pad].AnalogDef[index / 4][index % 4].Key = 0;
		g.cfg.PadDef[pad].AnalogDef[index / 4][index % 4].JoyEvType = NONE;
		g.cfg.PadDef[pad].AnalogDef[index / 4][index % 4].J.Button = 0;
	}

	UpdateKey();
}

static void PopulateDevList() {
	int i, j, n;
	GtkWidget *widget;
	GtkTreeIter iter;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	char buf[256];

	for (i = 0; i < NUM_KEYLIST; i++) {
		widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_combodev[i]));

		renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, FALSE);
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget), renderer, "text", 0);

		store = gtk_list_store_new(1, G_TYPE_STRING);

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, _("None"), -1);

		n = SDL_NumJoysticks();
		for (j = 0; j < n; j++) {
			#if SDL_VERSION_ATLEAST(2, 0, 0)
				SDL_Joystick *joystick = SDL_JoystickOpen(j);
				snprintf(buf, sizeof(buf), "%d: %s", j + 1, SDL_JoystickName(joystick));
			#else
				snprintf(buf, sizeof(buf), "%d: %s", j + 1, SDL_JoystickName(j));
			#endif
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 0, buf, -1);
		}

		gtk_combo_box_set_model(GTK_COMBO_BOX(widget), GTK_TREE_MODEL(store));

		if (i > 0) {
			n = g.cfg.PadDef[i-1].DevNum + 1;
			if (n > SDL_NumJoysticks()) {
				n = 0;
				g.cfg.PadDef[i-1].DevNum = -1;
			}
		}

		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), n);
	}
}

long PADconfigure() {
	GtkWidget *widget;
	GtkTreeSelection *treesel;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
        
	if (SDL_Init(SDL_INIT_JOYSTICK) == -1) {
		fprintf(stderr, "Failed to initialize SDL!\n");
		return -1;
	}

	g.Disp = XOpenDisplay(NULL);
	if (!g.Disp) {
		fprintf(stderr, "XOpenDisplay failed!\n");
		return -1;
	}
        
	LoadPADConfig();

	xml = gtk_builder_new();

	if (!gtk_builder_add_from_resource(xml, "/org/pcsxr/dfinput/dfinput.ui", NULL)) {
		g_warning("We could not load the interface!");
		return -1;
	}
        
	MainWindow = GTK_WIDGET(gtk_builder_get_object(xml, "CfgWnd"));
	gtk_window_set_title(GTK_WINDOW(MainWindow), _("Gamepad/Keyboard Input Configuration"));

	widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_treeview[1])); // pad 1
        
	// column for key
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Key"),
		renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(widget), column);

	// column for button
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Button"),
		renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(widget), column);

	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	gtk_tree_selection_set_mode(treesel, GTK_SELECTION_SINGLE);

	g_signal_connect_data(G_OBJECT(treesel), "changed",
		G_CALLBACK(TreeSelectionChanged), GINT_TO_POINTER(0), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_treeview[2])); // pad 2

	// column for key
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Key"),
		renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(widget), column);

	// column for button
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Button"),
		renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(widget), column);

	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	gtk_tree_selection_set_mode(treesel, GTK_SELECTION_SINGLE);

	g_signal_connect_data(G_OBJECT(treesel), "changed",
		G_CALLBACK(TreeSelectionChanged), GINT_TO_POINTER(1), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, "CfgWnd"));
	g_signal_connect_data(G_OBJECT(widget), "delete_event",
		G_CALLBACK(OnConfigExit), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, "btnclose"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
		G_CALLBACK(OnConfigExit), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, "checkmt"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g.cfg.Threaded);
	g_signal_connect_data(G_OBJECT(widget), "toggled",
		G_CALLBACK(OnThreadedToggled), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, "checkcg"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g.cfg.HideCursor);
	g_signal_connect_data(G_OBJECT(widget), "toggled",
		G_CALLBACK(OnHideCursorToggled), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, "checkps"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g.cfg.PreventScrSaver);
	g_signal_connect_data(G_OBJECT(widget), "toggled",
		G_CALLBACK(OnPreventScrSaverToggled), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_combodev[1]));
	g_signal_connect_data(G_OBJECT(widget), "changed",
		G_CALLBACK(OnDeviceChanged), GINT_TO_POINTER(0), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_combodev[2]));
	g_signal_connect_data(G_OBJECT(widget), "changed",
		G_CALLBACK(OnDeviceChanged), GINT_TO_POINTER(1), NULL, G_CONNECT_AFTER);

	int padTypeList[] = {
		0,
		2, // PSE_PAD_TYPE_MOUSE
		0, // PSE_PAD_TYPE_NEGCON
		0, // PSE_PAD_TYPE_GUN
		0, // PSE_PAD_TYPE_STANDARD
		1, // PSE_PAD_TYPE_ANALOGJOY
		0, // PSE_PAD_TYPE_GUNCON
		1, //PSE_PAD_TYPE_ANALOGPAD
	};

	widget = GTK_WIDGET(gtk_builder_get_object(xml, "combotype1"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget),
		padTypeList[g.cfg.PadDef[0].Type]);
    OnTypeChanged(widget, GUINT_TO_POINTER(0u));
	g_signal_connect_data(G_OBJECT(widget), "changed",
		G_CALLBACK(OnTypeChanged), GUINT_TO_POINTER(0u), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, "combotype2"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget),
		padTypeList[g.cfg.PadDef[1].Type]);
    OnTypeChanged(widget, GUINT_TO_POINTER(1u));
	g_signal_connect_data(G_OBJECT(widget), "changed",
		G_CALLBACK(OnTypeChanged), GUINT_TO_POINTER(1u), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, "checkvv1"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g.cfg.PadDef[0].VisualVibration);
	g_signal_connect_data(G_OBJECT(widget), "toggled",
		G_CALLBACK(OnVisualVibration1Toggled), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, "checkvv2"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g.cfg.PadDef[1].VisualVibration);
	g_signal_connect_data(G_OBJECT(widget), "toggled",
		G_CALLBACK(OnVisualVibration2Toggled), NULL, NULL, G_CONNECT_AFTER);

    widget = GTK_WIDGET(gtk_builder_get_object(xml, "checkpv1"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g.cfg.PadDef[0].PhysicalVibration);
    g_signal_connect_data(G_OBJECT(widget), "toggled",
                          G_CALLBACK(OnPhysicalVibration1Toggled), NULL, NULL, G_CONNECT_AFTER);

    widget = GTK_WIDGET(gtk_builder_get_object(xml, "checkpv2"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g.cfg.PadDef[1].PhysicalVibration);
    g_signal_connect_data(G_OBJECT(widget), "toggled",
                          G_CALLBACK(OnPhysicalVibration2Toggled), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_change[1]));
	gtk_widget_set_sensitive(widget, FALSE);
	g_signal_connect_data(G_OBJECT(widget), "clicked",
		G_CALLBACK(OnChangeClicked), GINT_TO_POINTER(0), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_reset[1]));
	gtk_widget_set_sensitive(widget, FALSE);
	g_signal_connect_data(G_OBJECT(widget), "clicked",
		G_CALLBACK(OnResetClicked), GINT_TO_POINTER(0), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_change[2]));
	gtk_widget_set_sensitive(widget, FALSE);
	g_signal_connect_data(G_OBJECT(widget), "clicked",
		G_CALLBACK(OnChangeClicked), GINT_TO_POINTER(1), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_reset[2]));
	gtk_widget_set_sensitive(widget, FALSE);
	g_signal_connect_data(G_OBJECT(widget), "clicked",
		G_CALLBACK(OnResetClicked), GINT_TO_POINTER(1), NULL, G_CONNECT_AFTER);


	// ************ Emulators keys	**********************
	widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_treeview[0])); // emu

	// column for key
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Key"),
		renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(widget), column);

	// column for button
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Button"),
		renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(widget), column);

	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	gtk_tree_selection_set_mode(treesel, GTK_SELECTION_SINGLE);

	g_signal_connect_data(G_OBJECT(treesel), "changed",
		G_CALLBACK(TreeSelectionChanged), GINT_TO_POINTER(-1), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_change[0]));
	gtk_widget_set_sensitive(widget, FALSE);
	g_signal_connect_data(G_OBJECT(widget), "clicked",
		G_CALLBACK(OnChangeClicked), GINT_TO_POINTER(-1), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_reset[0]));
	gtk_widget_set_sensitive(widget, FALSE);
	g_signal_connect_data(G_OBJECT(widget), "clicked",
		G_CALLBACK(OnResetClicked), GINT_TO_POINTER(-1), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(xml, widgetname_combodev[0]));
	g_signal_connect_data(G_OBJECT(widget), "changed",
		G_CALLBACK(OnDeviceChanged), GINT_TO_POINTER(-1), NULL, G_CONNECT_AFTER);
        
	PopulateDevList();
	UpdateKeyList();
        
	gtk_widget_show(MainWindow);
	gtk_main();
        
	return 0;
}

void PADabout() {
	const char *authors[]= {"Wei Mingzhi <weimingzhi@gmail.com>", "ckain <ckain@iki.fi>", NULL};
	GtkWidget *widget;

	widget = gtk_about_dialog_new();
	gtk_about_dialog_set_logo_icon_name (GTK_ABOUT_DIALOG(widget), "help-about");
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(widget), "Gamepad/Keyboard Input");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(widget), "1.2");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(widget), authors);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(widget), "http://www.codeplex.com/pcsxr/");

	gtk_dialog_run(GTK_DIALOG(widget));
	gtk_widget_destroy(widget);
}

int main(int argc, char *argv[]) {
#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	gtk_init(&argc, &argv);

	if (argc < 2) {
	printf ("Usage: cfgDFInput {about | configure}\n");
		return 0;
	}

	if (strcmp(argv[1], "configure") != 0 &&
		strcmp(argv[1], "about") != 0) {
		printf ("Usage: cfgDFInput {about | configure}\n");
		return 0;
	}

	if(!strcmp(argv[1], "configure"))
		PADconfigure();
	else if(!strcmp(argv[1], "about"))
		PADabout();

	return 0;
}
