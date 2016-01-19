#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

int FlushExpose(Display * display,Window window){
	XEvent xev;
	int i=0;
	while(XCheckTypedWindowEvent (display, window, Expose, &xev))i++;
	return i;
}


Pixel GetColor(Display * display,Window window,char *ColorName){
	XColor Color;
	XWindowAttributes Attributes;

	XGetWindowAttributes(display,window,&Attributes);
	Color.pixel = 0;

	if (!XParseColor (display, Attributes.colormap, ColorName, &Color))
		fprintf(stderr,"can't parse %s\n", ColorName);
	else if(!XAllocColor (display, Attributes.colormap, &Color))
		fprintf(stderr,"can't allocate %s\n", ColorName);       
	return Color.pixel;
}

char* CheckArgs( int argc,
	char** argv,
	char* look_for){
	
	int i, length;
	length= strlen(look_for);
	i= 1;
	while( i < argc ){
		if( strncmp( argv[i], look_for, length ) == 0 ){
			i++;
			if( i < argc ){
				return argv[i];
			}
		}
		i++;
	}
	return (char*) NULL;
}

int CheckArg( int argc,
	char** argv,
	char* look_for){
	
	int i, length;
	length= strlen(look_for);
	i= 1;
	while( i < argc ){
		if( strncmp( argv[i], look_for, length ) == 0 ){
			return 1;
		}
		i++;
	}
	return 0;
}

Cursor MakeCursor(Display * display,
		Window window,
		unsigned int which_cursor){
	Cursor cursor;
	cursor= XCreateFontCursor(display,which_cursor);
	if(cursor!=(Cursor)None){
		XDefineCursor(display,window,cursor);
	}
	return cursor;
}

Window CreateWindow(Display * display,
	Window parent,
	int x, int y,
	unsigned int width,
	unsigned int height,
	unsigned int border,
	unsigned long fore,
	unsigned long back,
	unsigned long event_mask){

	Window window;
	XSetWindowAttributes attributes;
	unsigned long attribute_mask;
	Visual *visual = CopyFromParent;

	attributes.background_pixel= back;
	attributes.border_pixel= fore;
	attributes.event_mask= event_mask;
	attribute_mask= CWBackPixel | CWBorderPixel | CWEventMask;

	window= XCreateWindow( display, parent, x, y, width, height,
		border, CopyFromParent, InputOutput, visual,
		attribute_mask, &attributes );
	return window;
}

void CheckGeometry( Display* display,
	int screen_number,
	int argc, char** argv,
	int* x, int* y,
	unsigned int* width, unsigned int* height){

	char* geometry_string;
	int status;
	int screen_width, screen_height;

	geometry_string= CheckArgs( argc, argv, "-geom" );
	if( geometry_string != (char*) NULL){
		status= XParseGeometry(geometry_string,
			x, y, width, height);
		if( status & XNegative ){
			screen_width= DisplayWidth(display, screen_number);
			*x= screen_width - *width + *x;
		}
		if( status & YNegative ){
			screen_height= DisplayHeight(display, screen_number);
			*y= screen_height - *height + *y;
		}
	}
}

void SetWMHints( Display* display,
	Window window,
	int x, int y,
	unsigned int width, unsigned int height,
	int argc, char** argv,
	char* window_name,
	char* icon_name,
	Window iconwindow,
	char* class_name,
	char* class_type){

	XTextProperty w_name;
	XTextProperty i_name;
	XSizeHints sizehints;
	XWMHints wmhints;
	XClassHint classhints;
	int status;

	status= XStringListToTextProperty( &window_name,
		1, &w_name);
	status= XStringListToTextProperty( &icon_name,
		1, &i_name);
	
	sizehints.x= x;
	sizehints.y= y;
	sizehints.width= width;
	sizehints.height= height;
	sizehints.base_width= width;
	sizehints.base_height= height;
	sizehints.flags= USPosition | USSize | PBaseSize;

	wmhints.initial_state= NormalState;
	wmhints.icon_window= iconwindow;
	wmhints.icon_x= 64;
	wmhints.icon_y= 64;
	wmhints.window_group= window;
	wmhints.input= True;
	wmhints.flags= StateHint | InputHint | IconWindowHint |
		IconPositionHint | WindowGroupHint;

	classhints.res_name= class_name;
	classhints.res_class= class_type;

	XSetWMProperties(display, window,
		&w_name, &i_name,
		argv, argc,
		&sizehints, &wmhints, &classhints);
	XFree(w_name.value);
	XFree(i_name.value);
}

void SetWindowSize( Display* display,
	Window window,
	unsigned int width, unsigned int height,
	unsigned int max_width, unsigned int max_height,
	unsigned int min_width, unsigned int min_height,
	unsigned int base_width, unsigned int base_height,
	long addition_flag){

	XSizeHints sizehints;

	sizehints.width= width;
	sizehints.max_width= max_width;
	sizehints.min_width= min_width;
	sizehints.height= height;
	sizehints.max_height= max_height;
	sizehints.min_height= min_height;
	sizehints.base_width= base_width;
	sizehints.base_height= base_height;
	sizehints.flags=  USSize | addition_flag;

	XSetNormalHints(display, window, &sizehints);
}

void SetFullWMHints( Display* display,
	Window window,
	int x, int y,
	unsigned int width, unsigned int height,
	unsigned int max_width, unsigned int max_height,
	unsigned int min_width, unsigned int min_height,
	unsigned int base_width, unsigned int base_height,
	long addition_flag,
	int argc, char** argv,
	char* window_name,
	char* icon_name,
	Window iconwindow,
	Pixmap iconpixmap,
	char* class_name,
	char* class_type){

	XTextProperty w_name;
	XTextProperty i_name;
	XSizeHints sizehints;
	XWMHints wmhints;
	XClassHint classhints;
	int status;

	status= XStringListToTextProperty( &window_name,
		1, &w_name);
	status= XStringListToTextProperty( &icon_name,
		1, &i_name);
	
	sizehints.x= x;
	sizehints.y= y;
	sizehints.width= width;
	sizehints.max_width= max_width;
	sizehints.min_width= min_width;
	sizehints.height= height;
	sizehints.max_height= max_height;
	sizehints.min_height= min_height;
	sizehints.base_width= base_width;
	sizehints.base_height= base_height;
	sizehints.flags= USSize | addition_flag;
	if(x>0 && y>0) sizehints.flags|= USPosition; 
	wmhints.initial_state= NormalState;
	wmhints.icon_window= iconwindow;
	wmhints.icon_pixmap= iconpixmap;
	wmhints.icon_mask= iconpixmap;
	wmhints.icon_x= 64;
	wmhints.icon_y= 64;
	wmhints.window_group= window;
	wmhints.input= True;
	wmhints.flags= StateHint | InputHint | 
		IconPositionHint | WindowGroupHint;
	if(iconwindow)
		wmhints.flags|=IconWindowHint;
	if(iconpixmap)
		wmhints.flags|=(IconPixmapHint|IconMaskHint);

	classhints.res_name= class_name;
	classhints.res_class= class_type;

	XSetWMProperties(display, window,
		&w_name, &i_name,
		argv, argc,
		&sizehints, &wmhints, &classhints);
	XFree(w_name.value);
	XFree(i_name.value);
}
