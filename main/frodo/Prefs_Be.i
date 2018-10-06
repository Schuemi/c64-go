/*
 *  Prefs_Be.i - Global preferences, Be specific stuff
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 */

#include <InterfaceKit.h>
#include <StorageKit.h>
#include <Path.h>


// Special colors
const rgb_color light_color = {255, 255, 255, 0};
const rgb_color fill_color = {216, 216, 216, 0};
const rgb_color dark_color = {184, 184, 184, 0};


// Window thread messages
const uint32 MSG_OK = 'okok';
const uint32 MSG_CANCEL = 'cncl';
const uint32 MSG_SPRITES_ON = 'spon';
const uint32 MSG_SPRITE_COLLISIONS = 'scol';
const uint32 MSG_JOYSTICK_1_ON = 'joy1';
const uint32 MSG_JOYSTICK_2_ON = 'joy2';
const uint32 MSG_JOYSTICK_SWAP = 'jswp';
const uint32 MSG_LIMIT_SPEED = 'lmit';
const uint32 MSG_FAST_RESET = 'frst';
const uint32 MSG_CIA_IRQ_HACK = 'cirq';
const uint32 MSG_SID_FILTERS = 'filt';
const uint32 MSG_DOUBLE_SCAN = 'dbls';
const uint32 MSG_MAP_SLASH = 'mpsl';
const uint32 MSG_EMUL_1541_PROC = '15pr';
const uint32 MSG_DRVTYPE_8 = 'drt8';
const uint32 MSG_DRVTYPE_9 = 'drt9';
const uint32 MSG_DRVTYPE_10 = 'drt:';
const uint32 MSG_DRVTYPE_11 = 'drt;';
const uint32 MSG_GETDRIVE_8 = 'gtd8';
const uint32 MSG_GETDRIVE_9 = 'gtd9';
const uint32 MSG_GETDRIVE_10 = 'gtd:';
const uint32 MSG_GETDRIVE_11 = 'gtd;';
const uint32 MSG_DRIVE_PANEL_RETURNED = 'gdr8';
const uint32 MSG_SID_TYPE = 'sidt';
const uint32 MSG_REU_SIZE = 'reus';
const uint32 MSG_DISPLAY_TYPE = 'dspt';

const uint32 MSG_OPEN = 'open';
const uint32 MSG_SAVE = 'save';
const uint32 MSG_SAVE_AS = 'svas';
const uint32 MSG_REVERT = 'rvrt';
const uint32 MSG_OPEN_PANEL_RETURNED = 'oprt';
const uint32 MSG_SAVE_PANEL_RETURNED = 'svrt';


/*
 *  Preferences window class
 */

class NumberControl;
class PathControl;

class PrefsWindow : public BWindow {
public:
	PrefsWindow(Prefs *p, bool start, char *path);
	virtual void MessageReceived(BMessage *msg);
	virtual bool QuitRequested(void);
	virtual bool FilterKeyDown(uint32 *aChar, BView **target);

private:
	BCheckBox *make_checkbox(BRect frame, char *label, uint32 what, BView *parent);
	NumberControl *make_number_entry(BRect frame, char *label_text, BView *parent);
	BPopUpMenu *make_drvtype_popup(BRect frame, uint32 what, BView *parent);
	PathControl *make_path_entry(BRect frame, char *label, BView *parent);
	BPopUpMenu *make_sidtype_popup(BRect frame, char *label_text, uint32 what, BView *parent);
	BPopUpMenu *make_reusize_popup(BRect frame, char *label_text, uint32 what, BView *parent);
	BPopUpMenu *make_disptype_popup(BRect frame, char *label_text, uint32 what, BView *parent);
	void set_values(void);
	void get_values(void);
	void ghost_controls(void);

	Prefs *prefs;

	BMessenger this_messenger;
	BFilePanel *open_panel;		// For opening prefs
	BFilePanel *save_panel;		// For saving prefs
	BFilePanel *file_panel;		// For D64/T64 drives
	BFilePanel *dir_panel;		// For directory drives
	int panel_drive_num;		// Drive number (0..3) of the file panel

	BButton *g_ok;
	BButton *g_cancel;
	NumberControl *g_normal_cycles;
	NumberControl *g_bad_line_cycles;
	NumberControl *g_cia_cycles;
	NumberControl *g_floppy_cycles;
	NumberControl *g_skip_frames;
	BPopUpMenu *g_drive_type[4];
	PathControl *g_drive_path[4];
	BPopUpMenu *g_sid_type;
	BPopUpMenu *g_reu_size;
	BPopUpMenu *g_display_type;
	BCheckBox *g_sprites_on;
	BCheckBox *g_sprite_collisions;
	BCheckBox *g_joystick_1_on;
	BCheckBox *g_joystick_2_on;
	BCheckBox *g_joystick_swap;
	BCheckBox *g_limit_speed;
	BCheckBox *g_fast_reset;
	BCheckBox *g_cia_irq_hack;
	BCheckBox *g_sid_filters;
	BCheckBox *g_double_scan;
	BCheckBox *g_map_slash;
	BCheckBox *g_emul_1541_proc;

	char *prefs_path;
	bool startup;
};


/*
 *  Start preferences editor (asynchronously)
 *  startup = false: Send MSG_PREFS_DONE to application on close
 *  startup = true : Send MSG_STARTUP to application on close if not canceled,
 *                   B_QUIT_REQUESTED otherwise
 *  prefs_name points to the file name of the preferences (which may be changed)
 */

bool Prefs::ShowEditor(bool startup, char *prefs_name)
{
	PrefsWindow *win = new PrefsWindow(this, startup, prefs_name);
	win->Show();
	return true;
}


/*
 *  Number-only BTextControl
 */

// Class definition
class NumberControl : public BTextControl {
public:
	NumberControl(BRect frame, float divider, const char *name, const char *label, const char *text, BMessage *message);
	void SetValue(long value);
	long Value(void);
};

// Constructor: Allow only digits
NumberControl::NumberControl(BRect frame, float divider, const char *name, const char *label, const char *text, BMessage *message)
 : BTextControl(frame, name, label, text, message, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE)
{
	SetDivider(divider);
	for (int c=0; c<256; c++)
		if (!isdigit(c) && c != B_BACKSPACE && c != B_LEFT_ARROW && c != B_RIGHT_ARROW) 
			((BTextView *)ChildAt(0))->DisallowChar(c);
}

// Set integer value
void NumberControl::SetValue(long value)
{
	char str[32];
	sprintf(str, "%ld", value);
	SetText(str);
}

// Get integer value
long NumberControl::Value(void)
{
	return atol(Text());
}


/*
 *  Path-entry BTextControl
 */

// Class definition
class PathControl : public BTextControl {
public:
	PathControl(BRect frame, float divider, const char *name, const char *label, const char *text, BMessage *message);
	virtual void MessageReceived(BMessage *msg);
};

// Constructor: Disable some keys
PathControl::PathControl(BRect frame, float divider, const char *name, const char *label, const char *text, BMessage *message)
 : BTextControl(frame, name, label, text, message, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE)
{
	SetDivider(divider);
	for (int c=0; c<' '; c++)
		if (c != B_BACKSPACE && c != B_LEFT_ARROW && c != B_RIGHT_ARROW) 
			((BTextView *)ChildAt(0))->DisallowChar(c);
}

// Message received: Look out for dropped refs
void PathControl::MessageReceived(BMessage *msg)
{
	if (msg->what == B_SIMPLE_DATA) {
		entry_ref the_ref;
		BEntry the_entry;

		// First look for refs
		if (msg->FindRef("refs", &the_ref) == B_NO_ERROR) {
			if (the_entry.SetTo(&the_ref) == B_NO_ERROR) {
				BPath the_path;
				the_entry.GetPath(&the_path);
				SetText(the_path.Path());
			}
		} else
			BTextControl::MessageReceived(msg);

		MakeFocus();
	} else
		BTextControl::MessageReceived(msg);
}


/*
 *  Open preferences window
 */

PrefsWindow::PrefsWindow(Prefs *p, bool start, char *path) : BWindow(BRect(0, 0, 400, 349), "Frodo Preferences", B_TITLED_WINDOW, B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_ZOOMABLE), this_messenger(this)
{
	int i;
	prefs = p;
	startup = start;
	prefs_path = path;

	// Move window to right position
	Lock();
	MoveTo(80, 80);

	// Set up menus
	BMenuBar *bar = new BMenuBar(Bounds(), "");
	BMenu *menu = new BMenu("Preferences");
	BMenuItem *item;
	menu->AddItem(item = new BMenuItem("About Frodo" B_UTF8_ELLIPSIS, new BMessage(B_ABOUT_REQUESTED)));
	item->SetTarget(be_app);
	menu->AddItem(new BSeparatorItem);
	menu->AddItem(new BMenuItem("Open" B_UTF8_ELLIPSIS, new BMessage(MSG_OPEN), 'O'));
	menu->AddItem(new BMenuItem("Save", new BMessage(MSG_SAVE), 'S'));
	menu->AddItem(new BMenuItem("Save As" B_UTF8_ELLIPSIS, new BMessage(MSG_SAVE_AS), 'A'));
	menu->AddItem(new BMenuItem("Revert", new BMessage(MSG_REVERT)));
	menu->AddItem(new BSeparatorItem);
	menu->AddItem(item = new BMenuItem("Quit Frodo", new BMessage(B_QUIT_REQUESTED), 'Q'));
	item->SetTarget(be_app);
	bar->AddItem(menu);
	AddChild(bar);
	SetKeyMenuBar(bar);
	int mbar_height = bar->Frame().bottom + 1;

	// Resize window to fit menu bar
	ResizeBy(0, mbar_height);

	// Light gray background
	BRect b = Bounds();
	BView *top = new BView(BRect(0, mbar_height, b.right, b.bottom), "top", B_FOLLOW_NONE, B_WILL_DRAW);
	AddChild(top);
	top->SetViewColor(fill_color);

	// Checkboxes
	g_sprites_on = make_checkbox(BRect(10, 10, 160, 21), "Sprite display", MSG_SPRITES_ON, top);
	g_sprite_collisions = make_checkbox(BRect(10, 25, 160, 36), "Sprite collisions", MSG_SPRITE_COLLISIONS, top);
	g_joystick_1_on = make_checkbox(BRect(10, 40, 160, 51), "Joystick on port 1", MSG_JOYSTICK_1_ON, top);
	g_joystick_2_on = make_checkbox(BRect(10, 55, 160, 66), "Joystick on port 2", MSG_JOYSTICK_2_ON, top);
	g_joystick_swap = make_checkbox(BRect(10, 70, 160, 81), "Swap joysticks", MSG_JOYSTICK_SWAP, top);
	g_limit_speed = make_checkbox(BRect(10, 85, 160, 96), "Limit speed", MSG_LIMIT_SPEED, top);
	g_fast_reset = make_checkbox(BRect(10, 100, 160, 111), "Fast reset", MSG_FAST_RESET, top);
	g_cia_irq_hack = make_checkbox(BRect(10, 115, 160, 126), "Clear CIA ICR on write", MSG_CIA_IRQ_HACK, top);
	g_sid_filters = make_checkbox(BRect(10, 130, 160, 141), "SID filters", MSG_SID_FILTERS, top);
	g_double_scan = make_checkbox(BRect(10, 145, 160, 156), "Doublescan lines", MSG_DOUBLE_SCAN, top);

	// Number entry fields
	g_normal_cycles = make_number_entry(BRect(160, 10, 390, 26), "Cycles per line (CPU)", top);
	g_bad_line_cycles = make_number_entry(BRect(160, 30, 390, 46), "Cycles per Bad Line (CPU)", top);
	g_cia_cycles = make_number_entry(BRect(160, 50, 390, 66), "Cycles per line (CIA)", top);
	g_floppy_cycles = make_number_entry(BRect(160, 70, 390, 86), "Cycles per line (1541)", top);
	g_skip_frames = make_number_entry(BRect(160, 90, 390, 106), "Draw every n-th frame", top);

	// Popup fields
	g_display_type = make_disptype_popup(BRect(160, 110, 390, 126), "Display type", MSG_DISPLAY_TYPE, top);
	g_sid_type = make_sidtype_popup(BRect(160, 130, 390, 146), "SID emulation type", MSG_SID_TYPE, top);
	g_reu_size = make_reusize_popup(BRect(160, 150, 390, 166), "REU size", MSG_REU_SIZE, top);

	// Prepare on/off pictures for file panel buttons
	BView *view = new BView(BRect(0, 0, 19, 15), "", B_FOLLOW_NONE, 0);
	AddChild(view);
	view->SetViewColor(fill_color);

	view->BeginPicture(new BPicture);
	view->SetHighColor(fill_color);
	view->FillRect(BRect(0, 0, 19, 15));
	view->SetHighColor(light_color);
	view->StrokeRect(BRect(0, 0, 18, 0));
	view->StrokeRect(BRect(0, 0, 0, 14));
	view->SetHighColor(dark_color);
	view->StrokeRect(BRect(0, 15, 19, 15));
	view->StrokeRect(BRect(19, 0, 19, 15));
	view->SetFont(be_plain_font);
	view->SetHighColor(0, 0, 0);
	view->SetLowColor(fill_color);
	view->MovePenTo(7, 11);
	view->DrawString("B");
	BPicture *on = view->EndPicture();

	view->BeginPicture(new BPicture);
	view->SetHighColor(dark_color);
	view->FillRect(BRect(0, 0, 19, 15));
	view->SetHighColor(128, 128, 128);
	view->StrokeRect(BRect(0, 0, 18, 0));
	view->StrokeRect(BRect(0, 0, 0, 14));
	view->SetHighColor(light_color);
	view->StrokeRect(BRect(0, 15, 19, 15));
	view->StrokeRect(BRect(19, 0, 19, 15));
	view->SetFont(be_plain_font);
	view->SetHighColor(0, 0, 0);
	view->SetLowColor(dark_color);
	view->MovePenTo(7, 11);
	view->DrawString("B");
	BPicture *off = view->EndPicture();

	RemoveChild(view);
	delete view;

	// Drive settings
	BBox *drvbox = new BBox(BRect(10, 173, 390, 304));
	top->AddChild(drvbox);
	drvbox->SetViewColor(fill_color);
	drvbox->SetLowColor(fill_color);
	drvbox->SetLabel("Drives");

	for (i=0; i<4; i++) {
		char str[4];
		sprintf(str, "%d", i+8);
		g_drive_path[i] = make_path_entry(BRect(10, 14+i*20, 299, 30+i*20), str, drvbox);
		drvbox->AddChild(new BPictureButton(BRect(304, 16+i*20, 323, 31+i*20), "", new BPicture(*on), new BPicture(*off), new BMessage(MSG_GETDRIVE_8 + i)));
		g_drive_type[i] = make_drvtype_popup(BRect(329, 14+i*20, 373, 30+i*20), MSG_DRVTYPE_8 + i, drvbox);
	}

	g_map_slash = make_checkbox(BRect(10, 94, 300, 110), "Map '/'<->'\\' in filenames", MSG_MAP_SLASH, drvbox);
	g_emul_1541_proc = make_checkbox(BRect(10, 109, 300, 125), "Enable 1541 processor emulation", MSG_EMUL_1541_PROC, drvbox);

	// "OK" button
	top->AddChild(g_ok = new BButton(BRect(20, 315, 90, 340), "", startup ? "Start" : "OK", new BMessage(MSG_OK)));
	SetDefaultButton(g_ok);

	// "Cancel" button
	top->AddChild(g_cancel = new BButton(BRect(b.right-90, 315, b.right-20, 340), "", startup ? "Quit" : "Cancel", new BMessage(MSG_CANCEL)));

	// Set the values of all controls to reflect the preferences
	set_values();
	g_normal_cycles->MakeFocus();

	// Create file panels
	open_panel = new BFilePanel(B_OPEN_PANEL, &this_messenger, NULL, 0, false, new BMessage(MSG_OPEN_PANEL_RETURNED));
	open_panel->Window()->SetTitle("Frodo: Open preferences");
	save_panel = new BFilePanel(B_SAVE_PANEL, &this_messenger, NULL, 0, false, new BMessage(MSG_SAVE_PANEL_RETURNED));
	save_panel->Window()->SetTitle("Frodo: Save preferences");
	file_panel = new BFilePanel(B_OPEN_PANEL, &this_messenger, NULL, 0, false, new BMessage(MSG_DRIVE_PANEL_RETURNED));
	file_panel->SetPanelDirectory(&AppDirectory);
	dir_panel = new BFilePanel(B_OPEN_PANEL, &this_messenger, NULL, B_DIRECTORY_NODE, false, new BMessage(MSG_DRIVE_PANEL_RETURNED));
	dir_panel->SetPanelDirectory(&AppDirectory);
	dir_panel->Window()->SetTitle("Frodo: Select directory");
	dir_panel->SetButtonLabel(B_DEFAULT_BUTTON, "Select");

	Unlock();
}


/*
 *  Create checkbox
 */

BCheckBox *PrefsWindow::make_checkbox(BRect frame, char *label, uint32 what, BView *parent)
{
	BCheckBox *checkbox = new BCheckBox(frame, "", label, new BMessage(what));
	parent->AddChild(checkbox);
	return checkbox;
}


/*
 *  Create number entry field
 */

NumberControl *PrefsWindow::make_number_entry(BRect frame, char *label_text, BView *parent)
{
	NumberControl *num = new NumberControl(frame, frame.right-frame.left-55, "", label_text, NULL, NULL);
	parent->AddChild(num);

	num->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_RIGHT);
	num->SetFont(be_plain_font);
	num->ChildAt(0)->SetFont(be_plain_font);

	return num;
}


/*
 *  Create drive type popup menu
 */

BPopUpMenu *PrefsWindow::make_drvtype_popup(BRect frame, uint32 what, BView *parent)
{
	BPopUpMenu *popup = new BPopUpMenu("drive_type popup", true, true);
	popup->AddItem(new BMenuItem("Dir", new BMessage(what)));
	popup->AddItem(new BMenuItem("D64", new BMessage(what)));
	popup->AddItem(new BMenuItem("T64", new BMessage(what)));
	popup->SetTargetForItems(this);
	BMenuField *menu_field = new BMenuField(frame, "drive_type", NULL, popup);
	menu_field->SetDivider(0);
	parent->AddChild(menu_field);
	return popup;
}


/*
 *  Create path entry field
 */

PathControl *PrefsWindow::make_path_entry(BRect frame, char *label, BView *parent)
{
	PathControl *path = new PathControl(frame, 16, "", label, NULL, NULL);
	parent->AddChild(path);

	path->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	path->SetFont(be_plain_font);
	path->ChildAt(0)->SetFont(be_plain_font);
	((BTextView *)(path->ChildAt(0)))->SetMaxBytes(255);

	return path;
}


/*
 *  Create display type popup
 */

BPopUpMenu *PrefsWindow::make_disptype_popup(BRect frame, char *label_text, uint32 what, BView *parent)
{
	BPopUpMenu *popup = new BPopUpMenu("display_type popup", true, true);
	popup->AddItem(new BMenuItem("Window", new BMessage(what)));
	popup->AddItem(new BMenuItem("Screen", new BMessage(what)));
	popup->SetTargetForItems(this);
	BMenuField *menu_field = new BMenuField(frame, "display_type", label_text, popup);
	menu_field->SetDivider(frame.Width()-55);
	menu_field->SetAlignment(B_ALIGN_RIGHT);
	parent->AddChild(menu_field);
	return popup;
}


/*
 *  Create SID type popup
 */

BPopUpMenu *PrefsWindow::make_sidtype_popup(BRect frame, char *label_text, uint32 what, BView *parent)
{
	BPopUpMenu *popup = new BPopUpMenu("sid_type popup", true, true);
	popup->AddItem(new BMenuItem("None", new BMessage(what)));
	popup->AddItem(new BMenuItem("Digital", new BMessage(what)));
	popup->SetTargetForItems(this);
	BMenuField *menu_field = new BMenuField(frame, "sid_type", label_text, popup);
	menu_field->SetDivider(frame.Width()-55);
	menu_field->SetAlignment(B_ALIGN_RIGHT);
	parent->AddChild(menu_field);
	return popup;
}


/*
 *  Create REU size popup
 */

BPopUpMenu *PrefsWindow::make_reusize_popup(BRect frame, char *label_text, uint32 what, BView *parent)
{
	BPopUpMenu *popup = new BPopUpMenu("reu_size popup", true, true);
	popup->AddItem(new BMenuItem("None", new BMessage(what)));
	popup->AddItem(new BMenuItem("128K", new BMessage(what)));
	popup->AddItem(new BMenuItem("256K", new BMessage(what)));
	popup->AddItem(new BMenuItem("512K", new BMessage(what)));
	popup->SetTargetForItems(this);
	BMenuField *menu_field = new BMenuField(frame, "reu_size", label_text, popup);
	menu_field->SetDivider(frame.Width()-55);
	menu_field->SetAlignment(B_ALIGN_RIGHT);
	parent->AddChild(menu_field);
	return popup;
}


/*
 *  Set the values of the controls
 */

void PrefsWindow::set_values(void)
{
	prefs->Check();

	g_normal_cycles->SetValue(prefs->NormalCycles);
	g_bad_line_cycles->SetValue(prefs->BadLineCycles);
	g_cia_cycles->SetValue(prefs->CIACycles);
	g_floppy_cycles->SetValue(prefs->FloppyCycles);
	g_skip_frames->SetValue(prefs->SkipFrames);

	for (int i=0; i<4; i++) {
		g_drive_type[i]->ItemAt(prefs->DriveType[i])->SetMarked(true);
		g_drive_path[i]->SetText(prefs->DrivePath[i]);
	}

	g_sid_type->ItemAt(prefs->SIDType)->SetMarked(true);
	g_reu_size->ItemAt(prefs->REUSize)->SetMarked(true);
	g_display_type->ItemAt(prefs->DisplayType)->SetMarked(true);

	g_sprites_on->SetValue(prefs->SpritesOn ? B_CONTROL_ON : B_CONTROL_OFF);
	g_sprite_collisions->SetValue(prefs->SpriteCollisions ? B_CONTROL_ON : B_CONTROL_OFF);
	g_joystick_1_on->SetValue(prefs->Joystick1On ? B_CONTROL_ON : B_CONTROL_OFF);
	g_joystick_2_on->SetValue(prefs->Joystick2On ? B_CONTROL_ON : B_CONTROL_OFF);
	g_joystick_swap->SetValue(prefs->JoystickSwap ? B_CONTROL_ON : B_CONTROL_OFF);
	g_limit_speed->SetValue(prefs->LimitSpeed ? B_CONTROL_ON : B_CONTROL_OFF);
	g_fast_reset->SetValue(prefs->FastReset ? B_CONTROL_ON : B_CONTROL_OFF);
	g_cia_irq_hack->SetValue(prefs->CIAIRQHack ? B_CONTROL_ON : B_CONTROL_OFF);
	g_sid_filters->SetValue(prefs->SIDFilters ? B_CONTROL_ON : B_CONTROL_OFF);
	g_double_scan->SetValue(prefs->DoubleScan ? B_CONTROL_ON : B_CONTROL_OFF);

	g_map_slash->SetValue(prefs->MapSlash ? B_CONTROL_ON : B_CONTROL_OFF);
	g_emul_1541_proc->SetValue(prefs->Emul1541Proc ? B_CONTROL_ON : B_CONTROL_OFF);

	ghost_controls();
}


/*
 *  Get the values of the controls
 */

void PrefsWindow::get_values(void)
{
	prefs->NormalCycles = g_normal_cycles->Value();
	prefs->BadLineCycles = g_bad_line_cycles->Value();
	prefs->CIACycles = g_cia_cycles->Value();
	prefs->FloppyCycles = g_floppy_cycles->Value();
	prefs->SkipFrames = g_skip_frames->Value();

	for (int i=0; i<4; i++)
		strcpy(prefs->DrivePath[i], g_drive_path[i]->Text());

	prefs->Check();
}


/*
 *  Enable/disable certain controls
 */

void PrefsWindow::ghost_controls(void)
{
	g_normal_cycles->SetEnabled(!IsFrodoSC);
	g_bad_line_cycles->SetEnabled(!IsFrodoSC);
	g_cia_cycles->SetEnabled(!IsFrodoSC);
	g_floppy_cycles->SetEnabled(!IsFrodoSC);
	g_cia_irq_hack->SetEnabled(!IsFrodoSC);
	g_double_scan->SetEnabled(prefs->DisplayType == DISPTYPE_SCREEN);
}


/*
 *  Message from controls/menus received
 */

void PrefsWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_OK:
			get_values();
			if (startup)
				be_app->PostMessage(MSG_STARTUP);
			else {
				BMessage msg(MSG_PREFS_DONE);
				msg.AddBool("canceled", false);
				msg.AddPointer("prefs", prefs);
				be_app->PostMessage(&msg);
			}
			PostMessage(B_QUIT_REQUESTED);
			break;

		case MSG_CANCEL:
			if (startup)
				be_app->PostMessage(B_QUIT_REQUESTED);
			else {
				BMessage msg(MSG_PREFS_DONE);
				msg.AddBool("canceled", true);
				msg.AddPointer("prefs", prefs);
				be_app->PostMessage(&msg);
			}
			PostMessage(B_QUIT_REQUESTED);
			break;

		case MSG_SPRITES_ON:
			prefs->SpritesOn = !prefs->SpritesOn;
			break;

		case MSG_SPRITE_COLLISIONS:
			prefs->SpriteCollisions = !prefs->SpriteCollisions;
			break;

		case MSG_JOYSTICK_1_ON:
			prefs->Joystick1On = !prefs->Joystick1On;
			break;

		case MSG_JOYSTICK_2_ON:
			prefs->Joystick2On = !prefs->Joystick2On;
			break;

		case MSG_JOYSTICK_SWAP:
			prefs->JoystickSwap = !prefs->JoystickSwap;
			break;

		case MSG_LIMIT_SPEED:
			prefs->LimitSpeed = !prefs->LimitSpeed;
			break;

		case MSG_FAST_RESET:
			prefs->FastReset = !prefs->FastReset;
			break;

		case MSG_CIA_IRQ_HACK:
			prefs->CIAIRQHack = !prefs->CIAIRQHack;
			break;

		case MSG_SID_FILTERS:
			prefs->SIDFilters = !prefs->SIDFilters;
			break;

		case MSG_DOUBLE_SCAN:
			prefs->DoubleScan = !prefs->DoubleScan;
			break;

		case MSG_SID_TYPE:
			prefs->SIDType = msg->FindInt32("index");
			break;

		case MSG_REU_SIZE:
			prefs->REUSize = msg->FindInt32("index");
			break;

		case MSG_DISPLAY_TYPE:
			prefs->DisplayType = msg->FindInt32("index");
			g_double_scan->SetEnabled(prefs->DisplayType == DISPTYPE_SCREEN);
			break;

		case MSG_MAP_SLASH:
			prefs->MapSlash = !prefs->MapSlash;
			break;

		case MSG_EMUL_1541_PROC:
			prefs->Emul1541Proc = !prefs->Emul1541Proc;
			break;

		case MSG_DRVTYPE_8:
		case MSG_DRVTYPE_9:
		case MSG_DRVTYPE_10:
		case MSG_DRVTYPE_11:
			prefs->DriveType[msg->what & 3] = msg->FindInt32("index");
			break;

		case MSG_GETDRIVE_8:
		case MSG_GETDRIVE_9:
		case MSG_GETDRIVE_10:
		case MSG_GETDRIVE_11:
			panel_drive_num = msg->what & 3;
			file_panel->Hide();
			dir_panel->Hide();
			if (prefs->DriveType[panel_drive_num] == DRVTYPE_D64) {
				file_panel->Window()->SetTitle("Frodo: Select disk image file");
				file_panel->Show();
			} else if (prefs->DriveType[panel_drive_num] == DRVTYPE_T64) {
				file_panel->Window()->SetTitle("Frodo: Select archive file");
				file_panel->Show();
			} else
				dir_panel->Show();
			break;

		case MSG_DRIVE_PANEL_RETURNED:	{	// Drive path file panel returned
			entry_ref the_ref;
			BEntry the_entry;
			if (msg->FindRef("refs", &the_ref) == B_NO_ERROR)
				if (the_entry.SetTo(&the_ref) == B_NO_ERROR) {
					BPath the_path;
					the_entry.GetPath(&the_path);
					strncpy(prefs->DrivePath[panel_drive_num], the_path.Path(), 255);
					prefs->DrivePath[panel_drive_num][255] = 0;
					set_values();
				}
			break;
		}

		case MSG_OPEN:
			open_panel->Show();
			break;

		case MSG_OPEN_PANEL_RETURNED: {	// Open file panel returned
			get_values();	// Useful if Load() is unsuccessful

			entry_ref the_ref;
			BEntry the_entry;
			if (msg->FindRef("refs", &the_ref) == B_NO_ERROR)
				if (the_entry.SetTo(&the_ref) == B_NO_ERROR)
					if (the_entry.IsFile()) {
						BPath the_path;
						the_entry.GetPath(&the_path);
						strncpy(prefs_path, the_path.Path(), 1023);
						prefs_path[1023] = 0;
						prefs->Load(prefs_path);
						set_values();
					}
		}

		case MSG_SAVE:
			get_values();
			prefs->Save(prefs_path);
			break;

		case MSG_SAVE_AS:
			save_panel->Show();
			break;

		case MSG_SAVE_PANEL_RETURNED: {	// Save file panel returned
			entry_ref the_ref;
			BEntry the_entry;
			if (msg->FindRef("directory", &the_ref) == B_NO_ERROR)
				if (the_entry.SetTo(&the_ref) == B_NO_ERROR) {
					BPath the_path;
					the_entry.GetPath(&the_path);
					strncpy(prefs_path, the_path.Path(), 1023);
					strncat(prefs_path, "/", 1023);
					strncat(prefs_path, msg->FindString("name"), 1023);
					prefs_path[1023] = 0;
					get_values();
					if (!prefs->Save(prefs_path))
						ShowRequester("Couldn't save preferences.", "Too bad");
				}
			break;
		}

		case MSG_REVERT:
			get_values();	// Useful if Load() is unsuccessful
			prefs->Load(prefs_path);
			set_values();
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}


/*
 *  Intercept ESC key (works as clicking the Cancel button)
 */

bool PrefsWindow::FilterKeyDown(uint32 *aChar, BView **target)
{
	if (*aChar == B_ESCAPE) {
		// Flash Cancel button
		g_cancel->SetValue(B_CONTROL_ON);
		snooze(100000.0);
		PostMessage(MSG_CANCEL);
	}
	return true;
}


/*
 *  Quit requested
 */

bool PrefsWindow::QuitRequested(void)
{
	delete open_panel;
	delete save_panel;
	delete file_panel;
	delete dir_panel;
	return true;
}
