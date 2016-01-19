#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/xpm.h>
#include <X11/Xproto.h>
#include <WMaker.h>
#include <WINGs.h>
#include "WINGsP.h"
#include <wraster.h>
#include <signal.h>
#include "xlibgeneral.h"

void
wAbort()
{
	            exit(1);
}

#define MODEFILE "/.wmtimebomb/modefile"

#define BS 20
#define BSx2 40
#define BSx3 60
#define BSx4 80
#define MAXRECORD 30
#define MAXWIDTH 50
#define MAXHEIGHT 30
#define MAXMODE 10
#define MAXPATH 200

#define SW_RESTART 0
#define SW_RESETTIME 1
#define SW_CHEAT 2
#define SW_SETBOMB 3
#define SW_SETTIME 4
#define SW_HISTORY 5
#define SW_GETNAME 6
#define SW_CHECKNAME 7
#define SW_SETSIZE 8

#define ANAME "anonymous"

typedef struct _TimeBomb {
	int x;
	int y;
	int bs;
	int clock;
	int * arena;
	int * mine;
} TimeBomb;

typedef struct _TimeBombHistoryList {
	WMWindow *win;
	WMList *historyList;
	WMButton *resetButton,*doneButton,*removeButton;
	WMLabel *label;
	char * name;
} TimeBombHistoryList;


typedef struct _XpmIcon {
	Pixmap pixmap;
	Pixmap mask;
	XpmAttributes attributes;
} XpmIcon;

char *ProgName;
Display* display;
Window window,iconwin;
Window bombwin;
WMAppContext *app;
WMScreen *scr;
WMInputPanel *panel[5];

TimeBombHistoryList *historylist;
int nrecord;

WMMenu *menu;
WMMenu *menu_pref;
XTextProperty xtp;
char stringbuffer[3][50];
char **stringhistory;
char * string[3];

GC gc;
GC igc;
TimeBomb tb;
TimeBomb *tbtable[MAXMODE];
TimeBomb tbdefault;
XpmIcon cbutton,nbutton,pbutton,qbutton,sbutton;
XpmIcon rbutton;
XpmIcon rbuttona;
XpmIcon countbar;
XpmIcon rbuttonb;
XpmIcon id;
XpmIcon timebombicon;
XpmIcon clock;
XpmIcon title;
XpmIcon about;
int abouttick;
XpmIcon *rb;
XpmIcon *rrb;

int status;
int zz;
int yy;
int hit;
int clockn;
int clearcount;
int iconswitch[10];
int windowswitch[2];

char *username;
char *homedir;
char *history_table[MAXRECORD+1];
static int history_tcompare(char **a,char **b);
static int history_tcomparestr(char *a,char *b);
static void history_tload();
static void history_treset();
static void history_treinit();
static void history_add();
static void history_save();

static void print_help();
static void call_setbs(void *foo, int item, Time time);
static void call_setclock(void *foo, int item, Time time);
static void getname();
static void call_setsize(void *foo, int item, Time time);
static void call_mode(TimeBomb *data, int item, Time time);

static void call_restart(void *foo, int item, Time time);
static void call_history(void *foo, int item, Time time);
static void call_quit(void *foo, int item, Time time);

void reinit(int x,int y,int bs,int clock);
int count_bomb(int x,int y);
int count_clear();
void init_pixmap();
void reset_bomb(int x,int y);
void wdbreset_bomb(int x,int y);
void dbreset_bomb(int x,int y);
void show_hint();
void redraw_iconwin();
void redraw_counter();
void redraw_bomb();
void redraw_arena();
void redraw_clock();
void timebomb_init();
void quit_bomb();
void restart();
void firsthit(int x,int y);

int main(int argc, char** argv){
	FILE *historyfile;
	Window tempwin;
	Atom delete_win, miniaturize_win;
	Atom prots[6];
	char defaultstring[200];
	int i;
	XGCValues gcv;
	long gcm;

	FILE *modefile;
	char **modelist;
	int maxmode;

	int screen_number;
	int x,y;
	unsigned int width, height;
	char* displayname= NULL;
	XEvent ev;

	printf("Maliwan's Time Bomb version 0.2.0.\n");
	printf("1998 by Eleventen Lab. Thailand\n");
	srandom(time(NULL));

	for(i=0;i<10;iconswitch[i++]=0);
	abouttick=0;
	status=0;
	displayname= CheckArgs( argc, argv, "-disp" );

	if(CheckArg(argc, argv, "-s")) windowswitch[0]=1; else windowswitch[0]=0;
	if(CheckArg(argc, argv, "-c")) windowswitch[1]=1; else windowswitch[1]=0;
	if(CheckArg(argc, argv, "-h")){print_help();exit(0);}
	if(CheckArg(argc, argv, "-?")){print_help();exit(0);}

	sprintf(stringbuffer[0],"xxxxxxxxxxxxxxxxxxxxxxxxxx");
	string[0]=stringbuffer[0];
	string[1]=NULL;
	XStringListToTextProperty(string,1,&xtp);
	xtp.value=stringbuffer[0];

	timebomb_init();
	display= XOpenDisplay( displayname );
	if( display == (Display*) NULL){
		printf("Error: can't open display [%s]\n",
			XDisplayName( displayname ));
	}

	rb=&rbutton;
	rrb=rb;
	x= 150;
	y= 64;
	/****************************/
	homedir=malloc(strlen(getenv("HOME"))+strlen(MODEFILE));
	sprintf(homedir,"%s%s",getenv("HOME"),MODEFILE);
	modefile=fopen(homedir, "r");
	if(!modefile){
		printf("Can not find .wmtimebomb/modefile in your home directory.\nMake sure that you have read Readme file.\n");
		exit(1);
	}
	free(homedir);
	i=0;
	tbdefault.x=0;
	while(i<MAXMODE && fgets(defaultstring,200,modefile)){
		sscanf(defaultstring,"%dx%dx%dx%d",&tb.x,&tb.y,&tb.bs,&tb.clock);
		if(tb.clock>999||tb.clock<0)tb.clock=999;
		if(tb.x>MAXWIDTH)tb.x=MAXWIDTH;
		if(tb.x<3)tb.x=3;
		if(tb.y>MAXHEIGHT||tb.y<=0)tb.y=MAXHEIGHT;
		if(tb.bs>=tb.x*tb.y)tb.bs=tb.x*tb.y-2;
		if(tb.bs<0)tb.bs=0;
		tbtable[i]=malloc(sizeof(TimeBomb));
		tbtable[i]->x=tb.x;
		tbtable[i]->y=tb.y;
		tbtable[i]->bs=tb.bs;
		tbtable[i]->clock=tb.clock;
		if(!tbdefault.x){
			tbdefault.x=tb.x;
			tbdefault.y=tb.y;
			tbdefault.bs=tb.bs;
			tbdefault.clock=tb.clock;
		}
		i++;
	}
	maxmode=i;
	fclose(modefile);
	
	width= tbdefault.x*BS;
	height= tbdefault.y*BS;
	screen_number= DefaultScreen( display );
	CheckGeometry( display, screen_number, argc, argv,
		&x, &y, &width, &height );
	
	window=CreateWindow(display,
		RootWindow(display, screen_number),
		x, y, width, height+40, 0,
		WhitePixel(display, screen_number),
		BlackPixel(display, screen_number),
		ExposureMask|ButtonPressMask|StructureNotifyMask);
	iconwin=CreateWindow(display,
		RootWindow(display, screen_number),
		400, 400, 60, 40, 0,
		BlackPixel(display, screen_number),
		WhitePixel(display, screen_number),
		ExposureMask|ButtonPressMask|ButtonReleaseMask|StructureNotifyMask);

	delete_win=XInternAtom(display,"WM_DELETE_WINDOW", False);
	prots[0]=delete_win;
	prots[1]=miniaturize_win;
	XSetWMProtocols(display,window,prots,2);

	bombwin=CreateWindow(display,
		window,
		0, 40, width, height, 0,
		12345,
		54321,
		ExposureMask| ButtonPressMask| StructureNotifyMask);
	if( window == (Window) None){
		printf("Error: can't open window.\n");
		XCloseDisplay(display);
		exit(1);
	}

	gcm=GCForeground|GCBackground|GCGraphicsExposures;
	gcv.foreground = 0;
	gcv.background = 12345;
	gcv.graphics_exposures=True;
	gc=XCreateGC(display, window, gcm, &gcv);
	igc=XCreateGC(display, window, gcm, &gcv);

	init_pixmap();

	MakeCursor(display,bombwin,68);
	MakeCursor(display,window,62);

	SetFullWMHints(display, window,
		-1, -1, width, height,
		width, height+40,
		width, height+40,
		width, height+40,
		PBaseSize|PMinSize|PMaxSize,
		argc, argv,
		"Time Bomb 0.2.0",
		"Time Bomb",
		(Window)iconwin,
		(Pixmap)timebombicon.pixmap,
		"timebomb","Timebomb");

	
	app=WMAppCreateWithMain(display, DefaultScreen(display), window);
	WMInitializeApplication("Timebomb", &argc, argv);
	scr = WMCreateSimpleApplicationScreen(display);
	menu=WMMenuCreate(app, "Time Bomb");
	WMMenuAddItem(menu, "Restart", (WMMenuAction)call_restart, NULL,NULL,NULL);
	menu_pref=WMMenuCreate(app, "Mode");
	WMMenuAddSubmenu(menu, "Mode",menu_pref);
	i=0;
	while(i<maxmode){
		sprintf(defaultstring,"%dx%d, mines x%d %d seconds",tbtable[i]->x,tbtable[i]->y,tbtable[i]->bs,tbtable[i]->clock);
		WMMenuAddItem(menu_pref, defaultstring, (WMMenuAction)call_mode,tbtable[i],NULL,NULL);
		i++;
	}
	WMMenuAddItem(menu_pref, "Set size", (WMMenuAction)call_setsize, NULL,NULL,NULL);
	WMMenuAddItem(menu_pref, "Set time", (WMMenuAction)call_setclock, NULL,NULL,NULL);
	WMMenuAddItem(menu_pref, "Set bomb", (WMMenuAction)call_setbs, NULL,NULL,NULL);
	WMMenuAddItem(menu, "History", (WMMenuAction)call_history, NULL,NULL,NULL);
	WMMenuAddItem(menu, "Exit", (WMMenuAction)call_quit, NULL,NULL,NULL);
	WMAppSetMainMenu(app,menu);
	WMRealizeMenus(app);


	XMapRaised(display, window);
	XMapRaised(display, bombwin);
	reinit(tbdefault.x,tbdefault.y,tbdefault.bs,tbdefault.clock);
	XFlush(display);
	history_tload();
	history_treset();

	while(XPending(display)>0)
		XNextEvent(display,&ev);
	redraw_arena();

	signal(SIGALRM,redraw_clock);
	for(;;){
		char * pointman;
		yy=1;
		WMNextEvent(display,&ev);
		WMHandleEvent(&ev);
		WMProcessEvent(app,&ev);
		yy=0;
		if(iconswitch[SW_GETNAME]){
			if(panel[2]->done){
				iconswitch[SW_GETNAME]=0;
				if(panel[2]->result == WAPRDefault){
					pointman= WMGetTextFieldText(panel[2]->text);
					while(*pointman==' ')pointman++;
					if(!*pointman) pointman= NULL;
				}
				else
					pointman= NULL;
				if(pointman)history_add(pointman,clockn);
				WMDestroyInputPanel(panel[2]);
			}
		}
		if(iconswitch[SW_SETBOMB] && !iconswitch[SW_GETNAME]){
			if(panel[0]->done){
				iconswitch[SW_SETBOMB]=0;
				if(panel[0]->result == WAPRDefault)
					pointman= WMGetTextFieldText(panel[0]->text);
				else
					pointman= NULL;
				if(pointman){
					tb.bs=atoi(pointman);
					if(tb.bs>=tb.x*tb.y)tb.bs=tb.x*tb.y-2;
					if(tb.bs<0)tb.bs=0;
					reinit(tb.x,tb.y,tb.bs,tb.clock);
				}
				WMDestroyInputPanel(panel[0]);
			}
		}
		if(iconswitch[SW_SETSIZE] && !iconswitch[SW_GETNAME]){
			if(panel[3]->done){
				iconswitch[SW_SETSIZE]=0;
				if(panel[3]->result == WAPRDefault)
					pointman= WMGetTextFieldText(panel[3]->text);
				else
					pointman= NULL;
				if(pointman){
					tb.x=atoi(pointman);
					if(tb.x>MAXWIDTH)tb.x=MAXWIDTH;
					if(tb.x<3)tb.x=3;
					while(*pointman){
						if(*pointman==' ')break;
						if(*pointman=='x')break;
						pointman++;
					}
					pointman++;
					tb.y=atoi(pointman);
					if(tb.y>MAXHEIGHT||tb.y<=0)tb.y=MAXHEIGHT;
				}
				WMDestroyInputPanel(panel[3]);
				reinit(tb.x,tb.y,tb.bs,tb.clock);
			}
		}
		if(iconswitch[SW_SETTIME] && !iconswitch[SW_GETNAME]){
			if(panel[1]->done){
				iconswitch[SW_SETTIME]=0;
				if(panel[1]->result == WAPRDefault)
					pointman= WMGetTextFieldText(panel[1]->text);
				else
					pointman= NULL;
				if(pointman){
					tb.clock=atoi(pointman);
					if(tb.clock>999||tb.clock<0)tb.clock=999;
					reinit(tb.x,tb.y,tb.bs,tb.clock);
				}
				WMDestroyInputPanel(panel[1]);
			}
		}
		switch(ev.type){
			case NoExpose:
				break;
			case ButtonRelease:
				if(!iconswitch[SW_GETNAME]){
				if(ev.xbutton.window==iconwin){
					x=ev.xbutton.x/BS;
					y=ev.xbutton.y/BS;
					if(!(x||y) && iconswitch[SW_RESTART]){
						XCopyArea(display,sbutton.pixmap,iconwin,igc,0,0,BS,BS,0,0);
						reinit(tb.x,tb.y,tb.bs,tb.clock);
					}
					if(x==1&&y==0 && iconswitch[SW_RESETTIME]){
						XCopyArea(display,cbutton.pixmap,iconwin,igc,0,0,BS,BS,BS,0);
						clockn=tb.clock;
					}
					if(x==2&&y==0 && iconswitch[SW_CHEAT]){
						XCopyArea(display,id.pixmap,iconwin,igc,0,0,BS,BS,BSx2,0);
						show_hint();
						redraw_arena();
					}
					if(y==1){

					}
				}
				iconswitch[SW_RESTART]=0;
				iconswitch[SW_RESETTIME]=0;
				iconswitch[SW_CHEAT]=0;
				redraw_iconwin();
				}
				break;
			case ButtonPress:
				if(!iconswitch[SW_GETNAME]){
				if(ev.xbutton.window==window){
					XFillRectangle(display,window,gc,(tb.x-4)*BS,0,BSx4,BSx2);
					XCopyArea(display,title.pixmap,window,gc,0,0,256,40,0,0);
					if(ev.xbutton.button==1)
						windowswitch[0]=!windowswitch[0];
					else if(ev.xbutton.button==3)
						windowswitch[1]=!windowswitch[1];
					redraw_counter();
				}
				else if(ev.xbutton.window==tempwin){
					XDestroyWindow(display,tempwin);
					abouttick=0;
				}
				else if(ev.xbutton.window==iconwin){
					x=ev.xbutton.x/BS;
					y=ev.xbutton.y/BS;
					if(!(x||y)){
						iconswitch[SW_RESTART]=1;
						XCopyArea(display,pbutton.pixmap,iconwin,igc,0,0,BS,BS,0,0);
					}
					if(x==1&&y==0){
						iconswitch[SW_RESETTIME]=1;
						XCopyArea(display,pbutton.pixmap,iconwin,igc,0,0,BS,BS,BS,0);
					}
					if(x==2&&y==0){
						iconswitch[SW_CHEAT]=1;
						XCopyArea(display,pbutton.pixmap,iconwin,igc,0,0,BS,BS,BSx2,0);
					}
				}
				else if(ev.xbutton.window==bombwin){
					x=ev.xbutton.x/BS;
					y=ev.xbutton.y/BS;
					if(ev.xbutton.button==1 && !tb.arena[y*tb.x+x]){
						if(hit){
							firsthit(x,y);
							alarm(1);
						}
						if(tb.mine[y*tb.x+x]){
							status=1;
						}
						else{
							reset_bomb(x,y);
						}
						redraw_arena();
						redraw_bomb();
						redraw_counter();
					}
					if(ev.xbutton.button==2){
						if(!status){
							wdbreset_bomb(x,y);
						}
					}
					if(ev.xbutton.button==3){
						if(status==1){
							reinit(tb.x,tb.y,tb.bs,tb.clock);
						}
						else if(status==2){
							if(tb.arena[y*tb.x+x]==5 && !abouttick){
								tempwin=CreateWindow(display,
									RootWindow(display, screen_number),
									x, y, 256, 256, 0,
									WhitePixel(display, screen_number),
									BlackPixel(display, screen_number),
									ButtonPressMask|ExposureMask|StructureNotifyMask);
									XMapRaised(display, tempwin);
									XCopyArea(display,about.pixmap,
											tempwin,igc,0,0,256,256,0,0);
								abouttick=1;
							}
						}
						else if(!status){
							switch(tb.arena[y*tb.x+x]){
							case 0:
								tb.arena[y*tb.x+x]=3;
								break;
							case 2:
								tb.arena[y*tb.x+x]=0;
								break;
							case 3:
								tb.arena[y*tb.x+x]=2;
								break;
							}
							redraw_arena();
						}
					}
				}
				}
				break;
			case Expose:
				if(ev.xexpose.window==tempwin){
					XCopyArea(display,about.pixmap,tempwin,igc,0,0,256,256,0,0);
				}
				if(ev.xexpose.window==window){
					XCopyArea(display,title.pixmap,window,gc,0,0,256,40,0,0);
					redraw_counter();
				}
				if(ev.xexpose.window==bombwin){
					redraw_arena();
					redraw_bomb();
				}
				if(ev.xexpose.window==iconwin){
					redraw_iconwin();
				}
				break;
			case ClientMessage:
				if(ev.xclient.data.l[0]==delete_win && ev.xclient.window==window){
					XFreeGC(display,gc);
					XDestroyWindow(display,bombwin);
					XDestroyWindow(display,iconwin);
					XDestroyWindow(display,window);
					XCloseDisplay(display);
					exit(0);
				}
				break;
		}

		if(status==2 && !iconswitch[SW_CHECKNAME]){
			int elem;
			char filename[MAXPATH];
			char winstring[100];
			char buffer[100];
			iconswitch[SW_CHECKNAME]=1;
			sprintf(filename,"/.wmtimebomb/%dx%dx%dx%d",tb.x,tb.y,tb.bs,tb.clock);
			homedir=malloc(strlen(getenv("HOME"))+strlen(filename));
			sprintf(homedir,"%s%s",getenv("HOME"),filename);
			historyfile=fopen(homedir, "a+");
			free(homedir);
			rewind(historyfile);
			elem=0;
			while(elem<MAXRECORD && fgets(buffer,200,historyfile)){
				history_table[elem]=strdup(buffer);
				elem++;
			}
			nrecord=elem;
			fclose(historyfile);
			sprintf(winstring,"a %d",clockn);
			if(elem<MAXRECORD)
				getname();
			else if(elem && history_tcomparestr(history_table[elem-1],winstring)>0) getname();

			history_treset();
		}
	}
	XDestroyWindow(display, window);
	XCloseDisplay(display);
	return 0;
}

/*************************************/
static void print_help(){
	printf("\nUsage: wmtimebomb -c -h -s\n\n");
}

static void call_setbs(void *foo, int item, Time time){
	if(!iconswitch[SW_GETNAME])
	if(!iconswitch[SW_SETBOMB]){
		iconswitch[SW_SETBOMB]=1;
		sprintf(stringbuffer[2],"%d",tb.bs);
		panel[0] = WMCreateInputPanel(scr,NULL, "Set bombs",
				"how many bomb you want?",
				stringbuffer[2], "OK", "Cancel");
		WMMapWidget(panel[0]->win);
	}
}

static void call_setclock(void *foo, int item, Time time){
	if(!iconswitch[SW_GETNAME])
	if(!iconswitch[SW_SETTIME]){
		iconswitch[SW_SETTIME]=1;
		sprintf(stringbuffer[2],"%d",tb.clock);
		panel[1] = WMCreateInputPanel(scr, NULL,"Set time",
				"how long you need?",
				stringbuffer[2], "OK", "Cancel");
		WMMapWidget(panel[1]->win);
	}
}

static void getname(){
	if(!iconswitch[SW_GETNAME]){
		iconswitch[SW_GETNAME]=1;
		sprintf(stringbuffer[2],"anonymous");
		panel[2] = WMCreateInputPanel(scr,NULL, "Enter your name",
				"What's your name?",
				stringbuffer[2], "OK", "Cancel");
		WMMapWidget(panel[2]->win);
	}
}

static void call_setsize(void *foo, int item, Time time){
	if(!iconswitch[SW_GETNAME])
	if(!iconswitch[SW_SETSIZE]){
		iconswitch[SW_SETSIZE]=1;
		sprintf(stringbuffer[2],"%dx%d",tb.x,tb.y);
		panel[3] = WMCreateInputPanel(scr,NULL, "Enter size",
				"WIDTHxHEIGHT",
				stringbuffer[2], "OK", "Cancel");
		WMMapWidget(panel[3]->win);
	}
}

static void call_mode(TimeBomb *data, int item, Time time){
	if(!iconswitch[SW_GETNAME])
		reinit(data->x,data->y,data->bs,data->clock);
}

static void call_restart(void *foo, int item, Time time){
	if(!iconswitch[SW_GETNAME]){
		reinit(tb.x,tb.y,tb.bs,tb.clock);
		redraw_arena();
	}
}

static int history_tcompare(char **a,char **b){
	int ia,ib;
	char *aa, *bb;
	aa=*a;
	bb=*b;
	while(*aa)aa++;
	while(*(aa-1)!=' ')aa--;
	while(*bb)bb++;
	while(*(bb-1)!=' ')bb--;
	ia=atoi(aa);
	ib=atoi(bb);
	if(ia < ib){
		return 1;
	}
	else if(ia == ib){
		return 0;
	}
	else return -1;
}

static int history_tcomparestr(char *a,char *b){
	int ia,ib;
	char *aa, *bb;
	aa=a;
	bb=b;
	while(*aa)aa++;
	while(*(aa-1)!=' ')aa--;
	while(*bb)bb++;
	while(*(bb-1)!=' ')bb--;
	ia=atoi(aa);
	ib=atoi(bb);
	if(ia < ib){
		return 1;
	}
	else if(ia == ib){
		return 0;
	}
	else return -1;
}

static void history_tload(){
	int elem;
	FILE *historyfile;
	char filename[MAXPATH];
	char buffer[200];
	sprintf(filename,"/.wmtimebomb/%dx%dx%dx%d",tb.x,tb.y,tb.bs,tb.clock);
	homedir=malloc(strlen(getenv("HOME"))+strlen(filename));
	sprintf(homedir,"%s%s",getenv("HOME"),filename);
	historyfile=fopen(homedir, "a+");
	free(homedir);
	rewind(historyfile);
	bzero(history_table,sizeof(char *)*(MAXRECORD+1));
	elem=0;
	while(elem<MAXRECORD && fgets(buffer,200,historyfile)){
		history_table[elem]=strdup(buffer);
		elem++;
	}
	nrecord=elem;

	qsort(history_table,nrecord,sizeof(char *),(void *)history_tcompare);
	fclose(historyfile);
}

static void history_add(char *name,int time){
	int elem,nlist;
	FILE *historyfile;
	char filename[MAXPATH];
	char buffer[200];
	sprintf(filename,"/.wmtimebomb/%dx%dx%dx%d",tb.x,tb.y,tb.bs,tb.clock);
	homedir=malloc(strlen(getenv("HOME"))+strlen(filename));
	sprintf(homedir,"%s%s",getenv("HOME"),filename);
	historyfile=fopen(homedir, "r");
	rewind(historyfile);
	bzero(history_table,sizeof(char *)*(MAXRECORD+1));
	elem=0;
	while(elem<MAXRECORD && fgets(buffer,200,historyfile)){
		history_table[elem]=strdup(buffer);
		elem++;
	}
	fclose(historyfile);
	sprintf(buffer,"%s %d\n",name,time);
	history_table[elem]=strdup(buffer);
	nrecord=elem+1;
	if(iconswitch[SW_HISTORY]){
		nlist=WMGetListNumberOfRows(historylist->historyList);
		if(nlist==MAXRECORD) WMRemoveListItem(historylist->historyList,MAXRECORD-1);
		for(elem=0;elem<nlist;elem++){
			if(history_tcomparestr(buffer,
				WMGetListItem(historylist->historyList,elem)->text)<0) break;
		}
		WMInsertListItem(historylist->historyList,elem,buffer);
		if(elem>=0){
			WMSelectListItem(historylist->historyList,elem);
			WMSetButtonEnabled(historylist->removeButton,1);
			WMSetButtonEnabled(historylist->resetButton,1);
		}
		else{
			WMSetButtonEnabled(historylist->removeButton,0);
			WMSetButtonEnabled(historylist->resetButton,0);
		}
	}

	qsort(history_table,nrecord,sizeof(char *),(void *)history_tcompare);
	historyfile=fopen(homedir, "w");
	free(homedir);
	for(elem=0;elem<nrecord;elem++){
		fputs(history_table[elem],historyfile);
	}
	fclose(historyfile);
}

static void history_renew(WMList *tlist){
	int elem;
	int nlist;
	FILE *historyfile;
	char filename[MAXPATH];
	char buffer[200];
	nlist=WMGetListNumberOfRows(tlist);
	bzero(history_table,sizeof(char *)*(MAXRECORD+1));
	for(elem=0;elem<nlist;elem++){
		history_table[elem]=strdup(WMGetListItem(tlist,elem)->text);
	}
	nrecord=elem;

	sprintf(filename,"/.wmtimebomb/%dx%dx%dx%d",tb.x,tb.y,tb.bs,tb.clock);
	homedir=malloc(strlen(getenv("HOME"))+strlen(filename));
	sprintf(homedir,"%s%s",getenv("HOME"),filename);
	historyfile=fopen(homedir, "w");
	free(homedir);
	for(elem=0;elem<nrecord;elem++){
		fputs(history_table[elem],historyfile);
	}
	fclose(historyfile);
	for(elem=0;elem<nrecord;elem++){
		free(history_table[elem]);
	}
}


static void history_treset(){
	int elem;
	for(elem=0;elem<nrecord;elem++){
		free(history_table[elem]);
	}
}

void _history_reset(WMWidget *self, void *data){
	TimeBombHistoryList *tbptr;
	tbptr=data;
	WMClearList(tbptr->historyList);
	WMSetButtonEnabled(tbptr->removeButton,0);
	WMSetButtonEnabled(tbptr->resetButton,0);
	history_renew(tbptr->historyList);
	return;
}

static void history_treinit(){
	int nlist;
	int elem;
	char buffer[200];
	if(iconswitch[SW_HISTORY]){
		sprintf(buffer,"width:%d height%d time:%d bomb:%d",tb.x,tb.y,tb.clock,tb.bs);
		WMSetLabelText(historylist->label,buffer);
		WMClearList(historylist->historyList);
		history_tload();
		for(elem=0;elem<nrecord;elem++){
			WMAddListItem(historylist->historyList,history_table[elem]);
		}
		history_treset();
	}
}

void _history_listaction(WMList *self, void * data){
	TimeBombHistoryList *tbptr;
	WMListItem *items;
	int elem;
	tbptr=data;
	elem=WMGetListSelectedItemRow(self);
	if(strlen((char*)WMGetListItem(self,elem))){
		WMSetButtonSelected(tbptr->removeButton,0);
	}
	else WMSetButtonSelected(tbptr->removeButton,1);
	return;
}

static void _history_remove(WMWidget *self, void *data){
	WMListItem *items;
	TimeBombHistoryList *tbptr;
	int elem;
	tbptr=data;
	elem=WMGetListSelectedItemRow(tbptr->historyList);
	if(elem>=0 && elem <WMGetListNumberOfRows(tbptr->historyList)){
		WMRemoveListItem(tbptr->historyList,elem);
	}
	items=WMGetListItem(tbptr->historyList,0);
	elem=WMGetListNumberOfRows(tbptr->historyList)>elem?elem:WMGetListNumberOfRows(tbptr->historyList)-1;
	if(elem>=0){
		WMSelectListItem(tbptr->historyList,elem);
	}
	else{
		WMSetButtonEnabled(tbptr->removeButton,0);
		WMSetButtonEnabled(tbptr->resetButton,0);
	}
	history_renew(tbptr->historyList);
}

void _doneAction(WMWidget *self, void *data){
	int elem;
	TimeBombHistoryList *tbptr;
	tbptr=(TimeBombHistoryList *)data;
	WMUnmapWidget(tbptr->win);
	WMDestroyWidget(tbptr->win);
	free(data);
	iconswitch[SW_HISTORY]=0;
	return;
}

void _ccAction(WMWidget *self, void *data){
	int elem;
	TimeBombHistoryList *tbptr;
	tbptr=(TimeBombHistoryList *)data;
	WMUnmapWidget(tbptr->win);
	WMDestroyWidget(tbptr->win);
	free(data);
	iconswitch[SW_HISTORY]=0;

	return;
}

static void call_history(void *foo, int item, Time time){
	int elem;
	WMListItem *items;
	char buffer[200];
	if(!iconswitch[SW_HISTORY]){
		iconswitch[SW_HISTORY]=1;
		historylist=(TimeBombHistoryList *)malloc(sizeof(TimeBombHistoryList));
		memset(historylist, 0, (sizeof(TimeBombHistoryList)));
		historylist->win=WMCreateWindow(scr,"History");
		WMSetWindowTitle(historylist->win,"History");
		WMResizeWidget(historylist->win,316,244);
		WMSetWindowCloseAction(historylist->win, _ccAction, historylist);

		historylist->historyList=WMCreateList(historylist->win);
		historylist->name="girl7";
		history_tload();
		for(elem=0;elem<nrecord;elem++){
			WMAddListItem(historylist->historyList,history_table[elem]);
		}
		history_treset();
		WMSetListAction(historylist->historyList,(void *)_history_listaction, historylist);
		WMResizeWidget(historylist->historyList,300,168);
		WMMoveWidget(historylist->historyList, 8, 8);

		historylist->resetButton=WMCreateCommandButton(historylist->win);
		WMResizeWidget(historylist->resetButton,100,24);
		WMMoveWidget(historylist->resetButton, 208, 184);
		WMSetButtonText(historylist->resetButton, "Reset history");
		WMSetButtonAction(historylist->resetButton, (WMAction*)_history_reset,historylist);

		historylist->removeButton=WMCreateCommandButton(historylist->win);
		WMResizeWidget(historylist->removeButton,100,24);
		WMMoveWidget(historylist->removeButton, 108, 184);
		WMSetButtonText(historylist->removeButton, "Remove record");
		WMSetButtonAction(historylist->removeButton, (WMAction*)_history_remove,historylist);

		historylist->doneButton=WMCreateCommandButton(historylist->win);
		WMResizeWidget(historylist->doneButton,100,24);
		WMMoveWidget(historylist->doneButton, 8, 184);
		WMSetButtonText(historylist->doneButton, "Done");
		WMSetButtonAction(historylist->doneButton, (WMAction*)_doneAction,historylist);

		historylist->label=WMCreateLabel(historylist->win);
		WMResizeWidget(historylist->label,300,20);
		WMMoveWidget(historylist->label,8, 216);
		WMSetLabelRelief(historylist->label, WRSunken);
		WMSetLabelTextAlignment(historylist->label,WALeft);

		WMRealizeWidget(historylist->win);
		WMMapSubwidgets(historylist->win);
		WMMapWidget(historylist->win);
		sprintf(buffer,"width:%d height%d time:%d bomb:%d",tb.x,tb.y,tb.clock,tb.bs);
		WMSetLabelText(historylist->label,buffer);
		elem=WMGetListNumberOfRows(historylist->historyList);
		if(elem)WMSelectListItem(historylist->historyList,0);
		else{
			WMSetButtonEnabled(historylist->removeButton,0);
			WMSetButtonEnabled(historylist->resetButton,0);
		}
	}
}

static void call_quit(void *foo, int item, Time time){
	quit_bomb();
}

void reinit(int x,int y,int bs,int clock){
	tb.bs=bs;
	tb.x=x;
	tb.y=y;
	tb.clock=clock;
	history_treinit();
	XResizeWindow(display,window,x*BS,y*BS+40);
	XResizeWindow(display,bombwin,x*BS,y*BS);
	restart();
	redraw_arena();
	redraw_counter();
}

int count_mark(int x,int y){
	int c;
	int mi,ni,mf,nf,m,n;
	c=0;
	mi=x-1;ni=y-1;
	if(mi<0)mi=0;if(ni<0)ni=0;
	mf=x+1;nf=y+1;
	if(mf>=tb.x)mf=tb.x-1;if(nf>=tb.y)nf=tb.y-1;
	for(n=ni ; n<=nf; n++){
	for(m=mi ; m<=mf; m++){
		if(tb.arena[n*tb.x+m]==3)c++;
	}
	}
	return c;
}

int count_bomb(int x,int y){
	int c;
	int mi,ni,mf,nf,m,n;
	c=0;
	mi=x-1;ni=y-1;
	if(mi<0)mi=0;if(ni<0)ni=0;
	mf=x+1;nf=y+1;
	if(mf>=tb.x)mf=tb.x-1;if(nf>=tb.y)nf=tb.y-1;
	for(n=ni ; n<=nf; n++){
	for(m=mi ; m<=mf; m++){
		if(tb.mine[n*tb.x+m])c++;
	}
	}
	return c;
}

int count_clear(){
	int c;
	int x,y;
	c=0;
	for(y=0 ; y<tb.y; y++){
	for(x=0 ; x<tb.x; x++){
		if(tb.arena[y*tb.x+x]==1)c++;
	}
	}
	return c;
}

void init_pixmap(){

#include "XPM/about.xpm"
#include "XPM/bomb.xpm"
#include "XPM/clock.xpm"
#include "XPM/countbar.xpm"
#include "XPM/cbutton.xpm"
#include "XPM/id.xpm"
#include "XPM/title.xpm"
#include "XPM/button.xpm"
#include "XPM/pbutton.xpm"
#include "XPM/qbutton.xpm"
#include "XPM/rbutton.xpm"
#include "XPM/rbuttona.xpm"
#include "XPM/rbuttonb.xpm"
#include "XPM/sbutton.xpm"

	nbutton.attributes.width=BS;
	nbutton.attributes.height=BS;
	nbutton.attributes.valuemask=(XpmSize);
	pbutton.attributes.width=BS;
	pbutton.attributes.height=BS;
	pbutton.attributes.valuemask=(XpmSize);
	qbutton.attributes.width=BS;
	qbutton.attributes.height=BS;
	qbutton.attributes.valuemask=(XpmSize);
	sbutton.attributes.width=BS;
	sbutton.attributes.height=BS;
	sbutton.attributes.valuemask=(XpmSize);
	rbutton.attributes.width=BS;
	rbutton.attributes.height=BS;
	rbutton.attributes.valuemask=(XpmSize);
	cbutton.attributes.width=BS;
	cbutton.attributes.height=BS;
	cbutton.attributes.valuemask=(XpmSize);
	id.attributes.width=BS;
	id.attributes.height=BS;
	id.attributes.valuemask=(XpmSize);
	title.attributes.width=256;
	title.attributes.height=48;
	title.attributes.valuemask=(XpmSize);
	about.attributes.width=256;
	about.attributes.height=256;
	about.attributes.valuemask=(XpmSize);
	clock.attributes.width=200;
	clock.attributes.height=BS;
	clock.attributes.valuemask=XpmSize;
	countbar.attributes.width=160;
	countbar.attributes.height=BS;
	countbar.attributes.valuemask=XpmSize;

	XpmCreatePixmapFromData(display,window, about_xpm,
			&about.pixmap, &about.mask, &about.attributes);
	XpmCreatePixmapFromData(display,window, title_xpm,
			&title.pixmap, &title.mask, &title.attributes);
	XpmCreatePixmapFromData(display,window, id_xpm,
			&id.pixmap, &id.mask, &id.attributes);
	XpmCreatePixmapFromData(display,window, button_xpm,
			&nbutton.pixmap, &nbutton.mask, &nbutton.attributes);
	XpmCreatePixmapFromData(display,window, pbutton_xpm,
			&pbutton.pixmap, &pbutton.mask, &pbutton.attributes);
	XpmCreatePixmapFromData(display,window, qbutton_xpm,
			&qbutton.pixmap, &qbutton.mask, &qbutton.attributes);
	XpmCreatePixmapFromData(display,window, sbutton_xpm,
			&sbutton.pixmap, &sbutton.mask, &sbutton.attributes);
	XpmCreatePixmapFromData(display,window, cbutton_xpm,
			&cbutton.pixmap, &cbutton.mask, &cbutton.attributes);
	XpmCreatePixmapFromData(display,window, bomb_xpm,
			&timebombicon.pixmap, &timebombicon.mask, &timebombicon.attributes);
	XpmCreatePixmapFromData(display,window, rbutton_xpm,
			&rbutton.pixmap, &rbutton.mask, &rbutton.attributes);
	XpmCreatePixmapFromData(display,window, rbuttona_xpm,
			&rbuttona.pixmap, &rbuttona.mask, &rbuttona.attributes);
	XpmCreatePixmapFromData(display,window, rbuttonb_xpm,
			&rbuttonb.pixmap, &rbuttonb.mask, &rbuttonb.attributes);
	XpmCreatePixmapFromData(display,window, clock_xpm,
			&clock.pixmap, &clock.mask, &clock.attributes);
	XpmCreatePixmapFromData(display,window, countbar_xpm,
			&countbar.pixmap, &countbar.mask, &countbar.attributes);

}

void dbreset_bomb(int x,int y){
	int a,b;
	int mi,ni,mf,nf,m,n;

	if(status) return;
	if(!tb.arena[y*tb.x+x]) return;
	a=count_bomb(x,y);
	b=count_mark(x,y);
	if(a!=b) return;
	mi=x-1;ni=y-1;
	if(mi<0)mi=0;if(ni<0)ni=0;
	mf=x+1;nf=y+1;
	if(mf>=tb.x)mf=tb.x-1;if(nf>=tb.y)nf=tb.y-1;
	for(n=ni ; n<=nf; n++)
	for(m=mi ; m<=mf; m++){
		if(tb.arena[n*tb.x+m]!=3){
			if(tb.mine[n*tb.x+m]){
				status=1;
				redraw_bomb();
			}
		}
	}
	if(!status)
	for(n=ni ; n<=nf; n++)
	for(m=mi ; m<=mf; m++){
		if(tb.arena[n*tb.x+m]!=3){
			reset_bomb(m,n);
		}
	}
	
	redraw_arena();
	redraw_counter();
}

void wdbreset_bomb(int x,int y){
	int a,b;
	int mi,ni,mf,nf,m,n;

	if(status) return;
	mi=x-1;ni=y-1;
	if(mi<0)mi=0;if(ni<0)ni=0;
	mf=x+1;nf=y+1;
	if(mf>=tb.x)mf=tb.x-1;if(nf>=tb.y)nf=tb.y-1;
	for(n=ni ; n<=nf; n++)
	for(m=mi ; m<=mf; m++){
			dbreset_bomb(m,n);
	}
}

void reset_bomb(int x,int y){
	int c;
	int mi,ni,mf,nf,m,n;

	if(status) return;
	if(!tb.arena[y*tb.x+x]){
		tb.arena[y*tb.x+x]=1;
		clearcount++;
	}
	if(clearcount==tb.x*tb.y-tb.bs){
	if(count_clear()==tb.x*tb.y-tb.bs){ 		/* kun niew+ */
		status=2;
		return;
	}
	}
	if(count_bomb(x,y))return;
	c=0;
	mi=x-1;ni=y-1;
	if(mi<0)mi=0;if(ni<0)ni=0;
	mf=x+1;nf=y+1;
	if(mf>=tb.x)mf=tb.x-1;if(nf>=tb.y)nf=tb.y-1;
	for(n=ni ; n<=nf; n++){
	for(m=mi ; m<=mf; m++){
		if(!tb.arena[n*tb.x+m])reset_bomb(m,n);
	}
	}
}

void show_hint(){
	int x,y;
	for(y=0;y<tb.y;y++){
	for(x=0;x<tb.x;x++){
		if(tb.mine[y*tb.x+x] && tb.arena[y*tb.x+x]==0)
			tb.arena[y*tb.x+x]=3;
	}
	}
}

void redraw_iconwin(){
	XCopyArea(display,clock.pixmap,iconwin,igc,clockn/100*BS,0,BS,BS,0,BS);
	XCopyArea(display,clock.pixmap,iconwin,igc,(clockn%100)/10*BS,0,BS,BS,BS,BS);
	XCopyArea(display,clock.pixmap,iconwin,igc,clockn%10*BS,0,BS,BS,BSx2,BS);
	if(windowswitch[0]){
		XCopyArea(display,title.pixmap,window,gc,(tb.x-4)*BS,BS,BSx3,BS,(tb.x-4)*BS,BS);
		if(clockn/1000){
			XCopyArea(display,clock.pixmap,window,igc,clockn/1000*BS,0,BS,BS,(tb.x-4)*BS,BS);
			XCopyArea(display,clock.pixmap,window,igc,(clockn%1000)/100*BS,0,BS,BS,(tb.x-3)*BS,BS);
			XCopyArea(display,clock.pixmap,window,igc,(clockn%100)/10*BS,0,BS,BS,(tb.x-2)*BS,BS);
			XCopyArea(display,clock.pixmap,window,igc,clockn%10*BS,0,BS,BS,(tb.x-1)*BS,BS);
		}
		else if((clockn%1000)/100){
			XCopyArea(display,clock.pixmap,window,igc,(clockn%1000)/100*BS,0,BS,BS,(tb.x-3)*BS,BS);
			XCopyArea(display,clock.pixmap,window,igc,(clockn%100)/10*BS,0,BS,BS,(tb.x-2)*BS,BS);
			XCopyArea(display,clock.pixmap,window,igc,clockn%10*BS,0,BS,BS,(tb.x-1)*BS,BS);
		}
		else if((clockn%100)/10){
			XCopyArea(display,clock.pixmap,window,igc,(clockn%100)/10*BS,0,BS,BS,(tb.x-2)*BS,BS);
			XCopyArea(display,clock.pixmap,window,igc,clockn%10*BS,0,BS,BS,(tb.x-1)*BS,BS);
		}
		else XCopyArea(display,clock.pixmap,window,igc,clockn%10*BS,0,BS,BS,(tb.x-1)*BS,BS);
	}
	if(!(iconswitch[SW_RESTART]||iconswitch[SW_RESETTIME]||iconswitch[SW_CHEAT])){
	XCopyArea(display,sbutton.pixmap,iconwin,igc,0,0,BS,BS,0,0);
	XCopyArea(display,cbutton.pixmap,iconwin,igc,0,0,BS,BS,BS,0);
	XCopyArea(display,id.pixmap,iconwin,igc,0,0,BS,BS,BSx2,0);
	}
}

void redraw_counter(){
	int bombcount;
	bombcount=tb.x*tb.y-clearcount-tb.bs;
	if(windowswitch[1]){
		XCopyArea(display,title.pixmap,window,gc,(tb.x-4)*BS,0,BSx3,BS,(tb.x-4)*BS,0);
		if(bombcount/1000){
			XCopyArea(display,clock.pixmap,window,igc,bombcount/1000*BS,0,BS,BS,(tb.x-4)*BS,0);
			XCopyArea(display,clock.pixmap,window,igc,(bombcount%1000)/100*BS,0,BS,BS,(tb.x-3)*BS,0);
			XCopyArea(display,clock.pixmap,window,igc,(bombcount%100)/10*BS,0,BS,BS,(tb.x-2)*BS,0);
			XCopyArea(display,clock.pixmap,window,igc,bombcount%10*BS,0,BS,BS,(tb.x-1)*BS,0);
		}
		else if((bombcount%1000)/100){
			XCopyArea(display,clock.pixmap,window,igc,(bombcount%1000)/100*BS,0,BS,BS,(tb.x-3)*BS,0);
			XCopyArea(display,clock.pixmap,window,igc,(bombcount%100)/10*BS,0,BS,BS,(tb.x-2)*BS,0);
			XCopyArea(display,clock.pixmap,window,igc,bombcount%10*BS,0,BS,BS,(tb.x-1)*BS,0);
		}
		else if((bombcount%100)/10){
			XCopyArea(display,clock.pixmap,window,igc,(bombcount%100)/10*BS,0,BS,BS,(tb.x-2)*BS,0);
			XCopyArea(display,clock.pixmap,window,igc,bombcount%10*BS,0,BS,BS,(tb.x-1)*BS,0);
		}
		else XCopyArea(display,clock.pixmap,window,igc,bombcount%10*BS,0,BS,BS,(tb.x-1)*BS,0);
	}
	redraw_iconwin();
}

void redraw_bomb(){
	int x,y;
	rrb=rb;
	if(status==1)
	for(y=0;y<tb.y;y++){
	for(x=0;x<tb.x;x++){
		if(tb.mine[y*tb.x+x]){
			tb.arena[y*tb.x+x]=4;
			if(rrb==&rbuttona) rrb=&rbutton;
			else if(rrb==&rbutton) rrb=&rbuttonb;
			else if(rrb==&rbuttonb) rrb=&rbuttona;
			XCopyArea(display,rrb->pixmap,bombwin,gc,0,0,BS,BS,x*BS,y*BS);
		}
	}
	}
}

void redraw_arena(){
	int c;
	int x,y;
	for(y=0;y<tb.y;y++){
	for(x=0;x<tb.x;x++){
		if(status==2 && tb.mine[y*tb.x+x] && clockn){
			tb.arena[y*tb.x+x]=5;
		}
		if(status==1 && tb.mine[y*tb.x+x]){
			tb.arena[y*tb.x+x]=4;
		}
		switch(tb.arena[y*tb.x+x]){
		case 0:
			XCopyArea(display,nbutton.pixmap,bombwin,gc,0,0,BS,BS,x*BS,y*BS);
			break;
		case 1:
			c=count_bomb(x,y);
			if(c){
				XCopyArea(display,countbar.pixmap,bombwin,gc,(8-c)*BS,0,BS,BS,x*BS,y*BS);
			}
			else XCopyArea(display,pbutton.pixmap,bombwin,gc,0,0,BS,BS,x*BS,y*BS);
			break;
		case 2:
			XCopyArea(display,qbutton.pixmap,bombwin,gc,0,0,BS,BS,x*BS,y*BS);
			break;
		case 3:
			XCopyArea(display,sbutton.pixmap,bombwin,gc,0,0,BS,BS,x*BS,y*BS);
			break;
		case 5:
			XCopyArea(display,id.pixmap,bombwin,gc,0,0,BS,BS,x*BS,y*BS);
			break;
		}
	}
	}
}

void redraw_clock(){

	if(clockn>0 && !hit && !status)
		clockn--;
	if(clockn<=0){
		status=1;
	}
	if(!XPending(display) && yy){
		if(clockn<tb.clock){
			switch(status){
			case 0:
			stringbuffer[1][0]=0;
			break;
			case 1:
			sprintf(stringbuffer[1]," Game Over");
			break;
			case 2:
			sprintf(stringbuffer[1]," Congratulations");
			break;
			}
			sprintf(stringbuffer[0],"%d%s",clockn,stringbuffer[1]);
			string[0]=stringbuffer[0];
		}
		else {
			sprintf(stringbuffer[0],"Time Bomb");
		}
		XSetWMName(display,window,&xtp);
		redraw_iconwin();
	switch(zz){
		case 0:
		zz=1;
		rb=&rbuttonb;
		break;
		case 1:
		zz=2;
		rb=&rbuttona;
		break;
		case 2:
		zz=0;
		rb=&rbutton;
		break;
	}
	redraw_bomb();
	XFlush(display);
	}

	signal(SIGALRM,redraw_clock);
	alarm(1);
}

void timebomb_init(){
	tb.arena=(int *)malloc(1500*sizeof(int));
	tb.mine=(int *)malloc(1500*sizeof(int));
}

void quit_bomb(){
	printf("Good bye.\n");
	exit(1);
}

void restart(){
	int count,b,c;
	status=0;
	c=tb.x*tb.y*sizeof(int);
	count=100;
	b=0;
	bzero(tb.arena,c);
	bzero(tb.mine,c);
	hit=1;
	clockn=tb.clock;
	clearcount=0;
	iconswitch[SW_CHECKNAME]=0;
}

void firsthit(int x,int y){
	int count;
	int b,c;
	count=tb.bs;
	hit=0;
	status=0;
	c=tb.x*tb.y;
	b=0;
	while(count){
		b=abs(random())%c;
		if(tb.mine[b]==0 && (b != y*tb.x+x)){
			tb.mine[b]=1;
			count--;
		}
	}
}

