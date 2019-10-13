// sowm
// An itsy bitsy floating window manager
// with roots in catwm.

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/extensions/shape.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define WIN    (c=list;c;c=c->next)
#define FOC(W) XSetInputFocus(dis, W, RevertToParent, CurrentTime);

typedef union {
    const char** com;
    const int i;
} Arg;

struct key {
    unsigned int mod;
    KeySym keysym;
    void (*function)(const Arg arg);
    const Arg arg;
};

typedef struct client client;
struct client{
    client *next, *prev;
    Window win;
    XWindowAttributes a;
    int f;
};

typedef struct ws ws;
struct ws{client *list;};

static void button_press(XEvent *e);
static void button_release();
static void configure_request(XEvent *e);
static void key_grab();
static void key_press(XEvent *e);
static void map_request(XEvent *e);
static void notify_destroy(XEvent *e);
static void notify_enter(XEvent *e);
static void notify_motion(XEvent *e);
static void run(const Arg arg);
static void win_add(Window w);
static void win_center(Window w);
static void win_center_current();
static void win_del(Window w);
static void win_fs(Window w);
static void win_fs_current();
static void win_kill();
static void win_next();
static void win_round_corners(Window w);
static void win_to_ws(const Arg arg);
static void ws_go(const Arg arg);
static void ws_save(int i);
static void ws_sel(int i);

static client *list = { 0 };
static ws     ws_list[10];
static int    desk = 1, sh, sw, s, j;

static Display           *dis;
static Window            root, cur;
static XButtonEvent      start;
static XWindowAttributes attr;

#include "config.h"

static void (*events[LASTEvent])(XEvent *e) = {
    [ButtonPress]      = button_press,
    [ButtonRelease]    = button_release,
    [ConfigureRequest] = configure_request,
    [KeyPress]         = key_press,
    [MapRequest]       = map_request,
    [DestroyNotify]    = notify_destroy,
    [EnterNotify]      = notify_enter,
    [MotionNotify]     = notify_motion
};

void notify_destroy(XEvent *e) {
    win_del(e->xdestroywindow.window);

    if (list) FOC(list->win);
}

void notify_enter(XEvent *e) {
    if (e->xcrossing.window != root) FOC(e->xcrossing.window)
}

void notify_motion(XEvent *e) {
    client *c;

    if (start.subwindow != None) {
        int xd = e->xbutton.x_root - start.x_root;
        int yd = e->xbutton.y_root - start.y_root;

        while(XCheckTypedEvent(dis, MotionNotify, e));

        XMoveResizeWindow(dis, start.subwindow,
            attr.x + (start.button==1 ? xd : 0),
            attr.y + (start.button==1 ? yd : 0),
            attr.width  + (start.button==3 ? xd : 0),
            attr.height + (start.button==3 ? yd : 0));

        win_round_corners(start.subwindow);
    }

    for WIN if (c->win == start.subwindow) c->f = 0;
}

void key_grab() {
    KeyCode code;

    for (int i=0; i < sizeof(keys)/sizeof(*keys); ++i)
        if ((code = XKeysymToKeycode(dis, keys[i].keysym)))
            XGrabKey(dis, code, keys[i].mod, root,
                     True, GrabModeAsync, GrabModeAsync);
}

void key_press(XEvent *e) {
    KeySym keysym = XKeycodeToKeysym(dis, e->xkey.keycode, 0);

    for (int i=0; i < sizeof(keys)/sizeof(*keys); ++i)
        if (keys[i].keysym == keysym && keys[i].mod == e->xkey.state)
            keys[i].function(keys[i].arg);
}

void button_press(XEvent *e) {
    if (e->xbutton.subwindow == None) return;

    XGetWindowAttributes(dis, e->xbutton.subwindow, &attr);
    XRaiseWindow(dis, e->xbutton.subwindow);
    start = e->xbutton;
}

void button_release() {
    start.subwindow = None;
}

Window win_current() {
    XGetInputFocus(dis, &cur, &j);
    return cur;
}

void win_add(Window w) {
    client *c, *t;

    if (!(c = (client *)calloc(1,sizeof(client))))
        exit(1);

    if (!list) {
        c->next = 0;
        c->prev = 0;
        c->win  = w;
        list    = c;

    } else {
        for (t=list;t->next;t=t->next);

        c->next = 0;
        c->prev = t;
        c->win  = w;
        t->next = c;
    }

    ws_save(desk);
}

void win_del(Window w) {
    client *c;

    for WIN if (c->win == w) {
        if (!c->prev && !c->next) {
            free(list);
            list = 0;
            ws_save(desk);
            return;
        }

        if (!c->prev) {
            list = c->next;
            c->next->prev = 0;

        } else if (!c->next) {
            c->prev->next = 0;

        } else {
            c->prev->next = c->next;
            c->next->prev = c->prev;
        }

        free(c);
        ws_save(desk);
        return;
    }
}

void win_kill() {
    win_current();

    if (cur != root) XKillClient(dis, cur);
}

void win_center(Window w) {
    XGetWindowAttributes(dis, w, &attr);

    XMoveWindow(dis, w, sw / 2 - attr.width  / 2,
                        sh / 2 - attr.height / 2);
}

void win_fs(Window w) {
    client *c;

    for WIN if (c->win == w) {
        if ((c->f = c->f == 0 ? 1 : 0)) {
            XGetWindowAttributes(dis, w, &c->a);
            XMoveResizeWindow(dis, w, 0, 0, sw, sh);

        } else
            XMoveResizeWindow(dis, w, c->a.x, c->a.y, c->a.width, c->a.height);
    }
}

void win_round_corners(Window w) {
    XWindowAttributes attr2;
    XGetWindowAttributes(dis, w, &attr2);

    if (!XGetWindowAttributes(dis, w, &attr2))
        return;

    int rad = ROUND_CORNERS;
    int dia = 2 * rad;

    if(attr2.width < dia || attr2.height < dia)
        return;

    Pixmap mask = XCreatePixmap(dis, w, attr2.width, attr2.height, 1);

    if (!mask) return;

    XGCValues xgcv;
    GC shape_gc = XCreateGC(dis, mask, 0, &xgcv);

    if (!shape_gc) {
        XFreePixmap(dis, mask);
        return;
    }

    XSetForeground(dis, shape_gc, 0);
    XFillRectangle(dis, mask, shape_gc, 0, 0, attr2.width, attr2.height);
    XSetForeground(dis, shape_gc, 1);
    XFillArc(dis, mask, shape_gc, 0, 0, dia, dia, 0, 23040);
    XFillArc(dis, mask, shape_gc, attr2.width-dia-1, 0, dia, dia, 0, 23040);
    XFillArc(dis, mask, shape_gc, 0, attr2.height-dia-1, dia, dia, 0, 23040);
    XFillArc(dis, mask, shape_gc, attr2.width-dia-1, attr2.height-dia-1, dia, dia,
        0, 23040);
    XFillRectangle(dis, mask, shape_gc, rad, 0, attr2.width-dia, attr2.height);
    XFillRectangle(dis, mask, shape_gc, 0, rad, attr2.width, attr2.height-dia);
    XShapeCombineMask(dis, w, ShapeBounding, 0, 0, mask, ShapeSet);
    XFreePixmap(dis, mask);
    XFreeGC(dis, shape_gc);
}

void win_to_ws(const Arg arg) {
    int tmp = desk;
    win_current();

    if (arg.i == tmp) return;

    ws_sel(arg.i);
    win_add(cur);
    ws_save(arg.i);

    ws_sel(tmp);
    win_del(cur);
    XUnmapWindow(dis, cur);
    ws_save(tmp);

    if (list) FOC(list->win);
}

void win_next() {
    win_current();
    client *c;

    if (list) {
        for WIN if (c->win == cur) break;

        c = c->next ? c->next : list;

        FOC(c->win);
        XRaiseWindow(dis, c->win);
    }
}

void win_fs_current() {
   win_fs(win_current());
}

void win_center_current() {
   win_center(win_current());
}

void ws_go(const Arg arg) {
    client *c;
    int tmp = desk;

    if (arg.i == desk) return;

    ws_save(desk);
    ws_sel(arg.i);

    if (list) for WIN XMapWindow(dis, c->win);

    ws_sel(tmp);

    if (list) for WIN XUnmapWindow(dis, c->win);

    ws_sel(arg.i);

    if (list) FOC(list->win);
}

void ws_save(int i) {
    ws_list[i].list = list;
}

void ws_sel(int i) {
    list = ws_list[i].list;
    desk = i;
}

void configure_request(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    wc.width      = ev->width;
    wc.height     = ev->height;
    wc.sibling    = ev->above;
    wc.stack_mode = ev->detail;

    XConfigureWindow(dis, ev->window, ev->value_mask, &wc);
}

void map_request(XEvent *e) {
    Window w = e->xmaprequest.window;

    XSelectInput(dis, w, PropertyChangeMask|StructureNotifyMask|
                         EnterWindowMask|FocusChangeMask);
    win_center(w);
    XMapWindow(dis, w);
    win_round_corners(w);
    FOC(w);
    win_add(w);
}

void run(const Arg arg) {
    if (fork()) return;
    if (dis)    close(ConnectionNumber(dis));

    setsid();
    execvp((char*)arg.com[0], (char**)arg.com);
}

int xerror(Display *dis, XErrorEvent *ee) {
    return 0;
}

int main(void) {
    XEvent ev;

    if (!(dis = XOpenDisplay(0x0))) return 0;

    signal(SIGCHLD, SIG_IGN);
    XSetErrorHandler(xerror);

    s    = DefaultScreen(dis);
    root = RootWindow(dis, s);
    sw   = XDisplayWidth(dis, s);
    sh   = XDisplayHeight(dis, s);

    key_grab();

    for (int i=0; i < sizeof(ws_list)/sizeof(*ws_list); ++i)
        ws_list[i].list = 0;

    ws_go((Arg){.i = 1});

    XSelectInput(dis, root, SubstructureNotifyMask|
        SubstructureRedirectMask|EnterWindowMask|LeaveWindowMask);

    XGrabButton(dis, 1, Mod4Mask, root, True,
        ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
        GrabModeAsync, GrabModeAsync, None, None);

    XGrabButton(dis, 3, Mod4Mask, root, True,
        ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
        GrabModeAsync, GrabModeAsync, None, None);

    while(1 && !XNextEvent(dis, &ev))
        if (events[ev.type]) events[ev.type](&ev);
}
