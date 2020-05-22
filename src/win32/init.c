/*
 *  Initialisation routines.
 *
 *  Platform: Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/08/14  Added the WinMain ifdefs (yuck).
 *  Version: 3.10  2001/12/01  Added LibMain and DllEntryPoint.
 *  Version: 3.17  2002/01/08  Added support for timers.
 *  Version: 3.19  2002/01/26  Changed resource check.
 *  Version: 3.25  2002/07/07  Added file dialog persistence.
 *  Version: 3.34  2002/12/18  Modified DLL/LIB pre-processing.
 *  Version: 3.35  2002/12/23  Added portable App initialisation.
 *  Version: 3.43  2003/04/25  Moved window class registration to win.c.
 *  Version: 3.45  2003/05/12  DLLs: set_app_instance, set_win32_instance.
 *  Version: 3.48  2003/06/05  Better support for non-graphical Apps.
 *  Version: 3.50  2003/12/30  Native and portable paths work in app_exec.
 *  Version: 3.53  2004/05/08  Now using app_to_portable_path.
 *  Version: 3.57  2005/08/16  Initialised app->socket_fd to -1 for Win32.
 *  Version: 3.60  2007/06/06  No need for global instance variable.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

#if 0	//!!
/*
 *  Global write-once variable:
 */

static HINSTANCE app_this_instance = 0;

/*
 *  Routines:
 */

void app_set_win32_instance(HINSTANCE instance)
{
	app_this_instance = instance;
}
#endif

void app_set_app_instance(App *app, HINSTANCE instance)
{
	HDC screen;

	/* Initialise string table and similar portable things. */
	app_app_initialise(app);

	/* Win32 implementations don't use sockets to communicate events. */
	app->socket_fd = -1;

	/* Is the GUI available? */
	if (instance == 0)
	{
		app->gui_available = 0;
		return;
	}

	app->gui_available = 1;
	app_extra(app)->this_instance = instance;

	/* Initialise the screen dimensions */
	app->screen_area.x = 0;
	app->screen_area.y = 0;
	app->screen_area.width  = GetSystemMetrics(SM_CXSCREEN);
	app->screen_area.height = GetSystemMetrics(SM_CYSCREEN);

	screen = CreateIC("DISPLAY", NULL, NULL, NULL);
	app->screen_mm.x = 0;
	app->screen_mm.y = 0;
	app->screen_mm.width  = GetDeviceCaps(screen, HORZSIZE);
	app->screen_mm.height = GetDeviceCaps(screen, VERTSIZE);
	DeleteDC(screen);
}

App *app_new_app(int argc, char *argv[])
{
	App *app;
	char *path;

	/* Create the App structure */
	app = app_zero_alloc(sizeof(struct App));
	if (app == NULL)
		return NULL;
	app->extra = app_zero_alloc(sizeof(struct AppExtra));
	if (app->extra == NULL) {
		app_free(app);
		return NULL;
	}

	/* Copy the program name from argv[0] */
	if (argv && argv[0]) {
		path = app_to_portable_path(argv[0]);
		if ((path[0] != '/') && (path[1] != ':')) {
			app->program_name = app_form_file_path(
					app_current_folder(), path);
		}
		else {
			app->program_name = app_form_file_path("", path);
		}
		app_free(path);
		app->has_resources = app_file_has_resources(app->program_name);
	}
	else {
		app->program_name = app_copy_string("");
		app->has_resources = 0; /* no resources, since no name */
	}

	/* Deactivate X-Windows style mouse-based copy/paste */
	app->use_X_copy_paste = 0;

	app_set_app_instance(app, GetModuleHandle(NULL));	//!!

#ifdef USE_FREETYPE
	FT_Init_FreeType(&app->ft_library);
#endif

	return app;
}

void app_del_app(App *app)
{
#ifdef USE_FREETYPE
	FT_Done_FreeType(app->ft_library);
#endif

	app_app_deinitialise(app);
	app_free(app_extra(app));
	app_free(app);
}

char * app_to_native_path(const char *filepath);

int app_exec(App *app, const char *cmd)
{
	char *command;
	int result;

	command = app_to_native_path(cmd);
	if (WinExec(command, SW_SHOW) < 32)
		result = 0;
	else
		result = 1;
	app_free(command);
	return result;
}

void app_beep(App *app)
{
	MessageBeep(-1);
}

#ifdef COMPILE_AS_A_DLL

/*
 *  Win16 DLL entry point:
 */
int PASCAL LibMain(HINSTANCE this_instance, WORD data_seg,
			WORD heap_size, LPSTR cmd_param)
{
	/* app_this_instance = this_instance; */	//!!
	return 1;
}

/*
 *  Win32 DLL entry point:
 */
BOOL WINAPI DllMain(HINSTANCE this_instance, DWORD reason,
			LPVOID reserved)
{
	/*if (reason == DLL_PROCESS_ATTACH)
		app_this_instance = this_instance; */	//!!
	return 1;
}

#else /* not a DLL */

/*
 *  Windows library/GUI program entry point:
 *  There's a little magic here to make both Borland and MS
 *  compilers work in the same way.
 *  Depending on your compiler, you may need to comment out
 *  some of the declarations or function calls below this line.
 */

extern int main(int, char**);
extern int     _argc;
extern char ** _argv;
extern int     __argc;
extern char ** __argv;

int PASCAL WinMain (HINSTANCE this_instance, HINSTANCE prev_instance,
		    LPSTR cmd_param, int cmd_show)
{
	/* app_this_instance = this_instance; */	//!!

	#ifdef __BORLANDC__
		main(_argc, _argv);
	#else
		main(__argc, __argv);
	#endif

	return 0;
}

#endif
