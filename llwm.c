#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct Client Client;
struct Client {
  char name[256];
  int x, y, w, h;
  int prevx, prevy, prevw, prevh;
  int isfullscreen;
  Client *next;
  Window win;
};

typedef struct Workspace Workspace;
struct Workspace {
  char name[256];
  Client *clients;
  Workspace *next;
};

Workspace workspaces;
Workspace *ws = &workspaces;
char *cur_ws;
Client rootclient;
Client *rc = &rootclient;
Client barclient;
Client *bc = &barclient;
Display * dpy;
int BORDER_W;
XFontStruct *fontinfo;
GC gc;
XGCValues grv;
char *cur_tm;

void root_startup();
void root_resize(int w, int h);
void bar_startup();
void map_bar();
void draw_bar_text();
void set_time();
void set_grabs();
int xerror(Display *dpy, XErrorEvent *ee);
void spawn(char * const args[]);

Workspace* create_workspace(char *str);
void destroy_workspace(char *str);
Workspace* get_workspace(char *str);
void change_workspace(char *str);
void move_client_workspace(char *str, Client *c);

void create_client(Window w, int x, int y, int width, int height);
void destroy_client(Window w);
void remove_client(Workspace *s, Client *c);
Client* get_client(Window w);
Workspace* get_client_workspace(Window w);
void add_client_to_ws(Client *c, Workspace *s);
void fs_client(Client *c);

int main(void) {
  XWindowAttributes attr;
  XButtonEvent start;
  XEvent ev;
  
  if(!(dpy = XOpenDisplay(0x0))) return 1;

  root_startup();
  bar_startup();
  map_bar();
  set_grabs();
  set_time();

  start.subwindow = None;
  for (;;) {
    XNextEvent(dpy, &ev);
    if (ev.type == Expose) {
      draw_bar_text();
    }
    
    if(ev.type == KeyPress && ev.xkey.subwindow != None && ev.xkey.subwindow != bc->win) {
      if (XKeysymToKeycode(dpy, XStringToKeysym("F")) == ev.xkey.keycode) {
	Client *c = get_client(ev.xkey.subwindow);
	fs_client(c);
      } else if (XKeysymToKeycode(dpy, XStringToKeysym("F1")) == ev.xkey.keycode) {
	XRaiseWindow(dpy, ev.xkey.subwindow);
      } else if (XKeysymToKeycode(dpy, XStringToKeysym("Q")) == ev.xkey.keycode) {
	XDestroyWindow(dpy, ev.xkey.subwindow);
      } else if (XKeysymToKeycode(dpy, XStringToKeysym("1")) <= ev.xkey.keycode &&
		 XKeysymToKeycode(dpy, XStringToKeysym("0")) >= ev.xkey.keycode) {
	if (ev.xkey.state == (ShiftMask|Mod4Mask)) {
	  move_client_workspace(XKeysymToString(XkbKeycodeToKeysym(dpy, ev.xkey.keycode, 0, 0)), get_client(ev.xkey.subwindow));
	} else {
	  change_workspace(XKeysymToString(XkbKeycodeToKeysym(dpy, ev.xkey.keycode, 0, 0)));
	  draw_bar_text();
	}
      } else if (XKeysymToKeycode(dpy, XStringToKeysym("Return")) == ev.xkey.keycode) {
	char * const args[] = { "x-terminal-emulator", NULL };
	spawn(args);
      } else if (XKeysymToKeycode(dpy, XStringToKeysym("D")) == ev.xkey.keycode) {
	char * const args[] = { "dmenu_run", NULL };
	spawn(args);
      }
    } else if (ev.type == ButtonPress && ev.xbutton.subwindow != None) {
      XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
      start = ev.xbutton;
    } else if (ev.type == MotionNotify && start.subwindow != None) {
      if (start.subwindow != bc->win) {
	Client *c = get_client(start.subwindow);
	if (!c->isfullscreen) {
	  XRaiseWindow(dpy, start.subwindow);
	  int xdiff = ev.xbutton.x_root - start.x_root;
	  int ydiff = ev.xbutton.y_root - start.y_root;
	  XMoveResizeWindow(dpy, start.subwindow,
			    attr.x + (start.button==1 ? xdiff : 0),
			    attr.y + (start.button==1 ? ydiff : 0),
			    MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
			    MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
	}
      }
    } else if (ev.type == ButtonRelease) {
      start.subwindow = None;
    } else if (ev.type == KeyPress) {
      if (XKeysymToKeycode(dpy, XStringToKeysym("Return")) == ev.xkey.keycode) {
	char * const args[] = { "x-terminal-emulator", NULL };
	spawn(args);	
      } else if (XKeysymToKeycode(dpy, XStringToKeysym("D")) == ev.xkey.keycode) {
	char * const args[] = { "dmenu_run", NULL };
	spawn(args);
      } else if (XKeysymToKeycode(dpy, XStringToKeysym("1")) <= ev.xkey.keycode &&
		 XKeysymToKeycode(dpy, XStringToKeysym("0")) >= ev.xkey.keycode) {
	if (ev.xkey.state == Mod4Mask) {
	  change_workspace(XKeysymToString(XkbKeycodeToKeysym(dpy, ev.xkey.keycode, 0, 0)));
	  draw_bar_text();
	}
      }
    } else if (ev.type == DestroyNotify) {
      destroy_client(ev.xdestroywindow.window);
    } else if (ev.type == ConfigureNotify) {
      if (ev.xconfigure.window == rc->win) {
	XClearWindow(dpy, rc->win);
	root_resize(ev.xconfigure.width, ev.xconfigure.height);
	XDestroyWindow(dpy, bc->win);
	bar_startup();
	map_bar();
      }
    } else if (ev.type == MapRequest) {
      if (ev.xmaprequest.window != rc->win && ev.xmaprequest.window != bc->win) {
	XWindowAttributes xw;
	XGetWindowAttributes(dpy, ev.xmaprequest.window, &xw);
	if (!get_client(ev.xmaprequest.window)) {
	  create_client(ev.xmaprequest.window, xw.x, xw.y, xw.width, xw.height);
	  XMapWindow(dpy, ev.xmaprequest.window);
	}
      }
    } else if (ev.type == ConfigureRequest) {
      XWindowChanges wc;
      wc.x = ev.xconfigurerequest.x;
      wc.y = ev.xconfigurerequest.y;
      wc.width = ev.xconfigurerequest.width;
      wc.height = ev.xconfigurerequest.height;
      wc.border_width = ev.xconfigurerequest.border_width;
      wc.sibling = ev.xconfigurerequest.above;
      wc.stack_mode = ev.xconfigurerequest.detail;

      XConfigureWindow(dpy, ev.xconfigurerequest.window, ev.xconfigurerequest.value_mask, &wc);
    }
  }
}

void root_startup() {
  rc->x = 1;
  rc->y = 1;
  rc->w = ScreenOfDisplay(dpy, 0)->width;
  rc->h = ScreenOfDisplay(dpy, 0)->height;
  rc->next = NULL;
  rc->win = DefaultRootWindow(dpy);

  XSetErrorHandler(xerror);

  XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask|SubstructureNotifyMask|StructureNotifyMask);
  
  create_workspace("1");
}

void root_resize(int w, int h) {
  rc->w = w;
  rc->h = h;
}

void bar_startup() {
  bc->x = 0;
  bc->y = 0;
  bc->w = rc->w - 2;
  bc->h = 10;
  BORDER_W = (rc->w - bc->w) / 2;
  bc->next = NULL;
  
  if (bc->w == 0) bc->w = 1;
  
  bc->win = XCreateSimpleWindow(dpy, RootWindow(dpy, 0), 0, 0, bc->w, bc->h, BORDER_W, WhitePixel(dpy, 0), BlackPixel(dpy, 0));

  XSelectInput(dpy, bc->win, ExposureMask|KeyPressMask);
  fontinfo = XLoadQueryFont(dpy, "6x10");
  
  grv.foreground = WhitePixel(dpy, 0);
  grv.font = fontinfo->fid;
  gc = XCreateGC(dpy, bc->win, GCFont+GCForeground, &grv);
}

void map_bar() {
  XMapWindow(dpy, bc->win);
}

void draw_bar_text() {
  set_time();
  XClearWindow(dpy, bc->win);
  XDrawString(dpy, bc->win, gc, 2, bc->h - 2, cur_ws, 1);
  XDrawString(dpy, bc->win, gc, bc->w - 95 - strlen(cur_tm), bc->h - 2, cur_tm, strlen(cur_tm));
}

void set_time() {
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char tmp[255];
  sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  cur_tm = tmp;
}

int xerror(Display *dpy, XErrorEvent *ee) {
  if (ee->error_code == BadWindow) {
    return 0;
  } else {
    exit(EXIT_FAILURE);
  }
}

void spawn(char * const args[]) {
  if (fork() == 0) {
    setsid();
    execvp(args[0], args);
  }
}

Workspace* create_workspace(char *str) {
  Workspace *s;

  if (cur_ws == NULL) {
    strcpy(ws->name, str);
    ws->clients = NULL;
    ws->next = NULL;
    cur_ws = ws->name;
    return ws;
  }

  if (strcmp(ws->name, str) == 0) return ws;
  
  for (s = ws; s->next; s = s->next) if (strcmp(s->name, str) == 0) return s;

  Workspace *ns = (struct Workspace*)malloc(sizeof(struct Workspace));

  strcpy(ns->name, str);
  ns->clients = NULL;
  ns->next = NULL;
  s->next = ns;
  
  return ns;
}

void destroy_workspace(char *str) {
  Workspace *s;
  Workspace *prev = NULL;

  if (strcmp(str, cur_ws) == 0) return;
  
  for (s = ws; s; s = s->next) {
    if (strcmp(s->name, str) != 0) prev = s;
    else break;
  }

  if (s->clients) return;

  if (!prev) {
    ws = s->next;
  } else {
    prev->next = s->next;
  }

  free(s);
}

Workspace* get_workspace(char *str) {
  Workspace *s;
  
  for (s = ws; s; s = s->next) if (strcmp(s->name, str) == 0) return s;

  return NULL;
}

void change_workspace(char *str) {
  Workspace *s;
  Client *tmp;

  if (strcmp(cur_ws, str) == 0) return;

  for (tmp = get_workspace(cur_ws)->clients; tmp; tmp = tmp->next)
    XUnmapWindow(dpy, tmp->win);


  s = create_workspace(str);

  cur_ws = s->name;

  for (tmp = s->clients; tmp; tmp = tmp->next) {
    if (tmp->isfullscreen)
      XMoveResizeWindow(dpy, tmp->win, 0, bc->h + 2, rc->w, rc->h - bc->h);

    XMapWindow(dpy, tmp->win);
  }
}

void move_client_workspace(char *str, Client *c) {
  Workspace *s = get_workspace(cur_ws);
  Workspace *tarws = create_workspace(str);

  if (!s || s == tarws) return;

  remove_client(s, c);
  add_client_to_ws(c, tarws);
}

void create_client(Window w, int x, int y, int width, int height) {
  Client *nc = (struct Client*)malloc(sizeof(struct Client));
  nc->x = x;
  nc->y = y;
  nc->w = width;
  nc->h = height;
  nc->isfullscreen = 0;
  nc->next = NULL;
  nc->win = w;

  add_client_to_ws(nc, get_workspace(cur_ws));
}

void destroy_client(Window w) {
  Workspace *tmpws = ws;
  Client *tc = NULL;

  tmpws = get_client_workspace(w);

  if (!tmpws) return;

  tc = tmpws->clients;

  if (!tc) return;

  remove_client(tmpws, tc);

  free(tc);

  if (strcmp(tmpws->name, cur_ws) != 0) destroy_workspace(tmpws->name);
}

void remove_client(Workspace *s, Client *c) {
  Client *prev = NULL;
  Client *tc = NULL;
  
  for (tc = s->clients; tc; tc = tc->next)
    if (tc->win != c->win) prev = tc;
    else break;
 

  if (!prev) {
    if (tc) {
      s->clients = tc->next;
    } else {
      s->clients = NULL;
    }
  } else {
    prev->next = tc->next;
  }
}

Client* get_client(Window w) {
  Workspace *tmpws = ws;
  Client *tc = NULL;

  for (tmpws = ws; tmpws; tmpws = tmpws->next)
    for (tc = tmpws->clients; tc; tc = tc->next)
      if (tc->win == w) return tc;

  return NULL;
}

Workspace* get_client_workspace(Window w) {
  Workspace *tmpws = ws;
  Client *tc = NULL;

  while (tmpws != NULL) {
    if (tmpws->clients != NULL) {
      tc = tmpws->clients;

      while (tc != NULL) {
	if (tc->win == w) return tmpws;
	tc = tc->next;
      }
    }
    tmpws = tmpws->next;
  }

  return NULL;
}

void add_client_to_ws(Client *c, Workspace *s) {
  Client *nc;

  if (strcmp(s->name, cur_ws) != 0) XUnmapWindow(dpy, c->win);

  if (!s->clients) {
    s->clients = c;
    return;
  }

  for (nc = s->clients; nc->next; nc = nc->next);
  nc->next = c;
}

//not a real fullscreen, just makes it easy to resize clients to max size
void fs_client(Client *c) {
  XWindowAttributes attr;
  if (!c) return;
  
  if (!c->isfullscreen) {
    XGetWindowAttributes(dpy, c->win, &attr);
    c->prevx = attr.x;
    c->prevy = attr.y;
    c->prevw = attr.width;
    c->prevh = attr.height;
    XRaiseWindow(dpy, c->win);
    XMoveResizeWindow(dpy, c->win, 0, bc->h + 2, rc->w, rc->h - bc->h);
    c->isfullscreen = 1;
  } else {
    XMoveResizeWindow(dpy, c->win, c->prevx, c->prevy, c->prevw, c->prevh);
    c->isfullscreen = 0;
  }  
}

void set_grabs() {
  for (char ch = '0'; ch <= '9'; ch++) {
    const char* tmp = &ch;
    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym(tmp)), Mod4Mask, rc->win, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym(tmp)), ShiftMask|Mod4Mask, rc->win, True, GrabModeAsync, GrabModeAsync);
  }  
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod4Mask, rc->win, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F")), Mod4Mask, rc->win, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("D")), Mod4Mask, rc->win, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("Q")), ShiftMask|Mod4Mask, rc->win, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("Return")), Mod4Mask, rc->win, True, GrabModeAsync, GrabModeAsync);
  XGrabButton(dpy, Button1, Mod4Mask, rc->win, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton(dpy, Button3, Mod4Mask, rc->win, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
}
