#ifndef SVMOS_H
#define SVMOS_H

#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>
#include <regex.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/XTest.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>

#include <list>
#include <queue>
#include <iostream>
using namespace std;

#define SHAREDSIZE (1024*1024*4)
#define LOW_ADDR (0x0)
#define LOW_LEN (1024)

void *openRAM(void);
void *openKernelMemory(void);

void *recv_events(void *arg);
void send_notify(Window win);

void enum_new_windows(Display *disp);
void get_relate_window(Display *dpy, Window *window_ret);

#define GUEST_OUT_OFF 1024

enum {
    SVM_WINDOW_UPDATE = 0xa,
    SVM_WINDOW_DESTROY,
};

/* for 4MB kernel memory */
struct output_prot
{
    int64_t msg_type;

    int64_t win;
    int64_t width, height;
    int64_t size, depth;
    int64_t dx, dy, win_related;

    int64_t title_len, encode, format, nitems;
    int64_t title[30];

    int64_t override;
    int64_t image[0];
} __attribute__((packed));

typedef struct {
    Display *dpy;
    Window wid;
} arg_struct;

/* thread for print guest window image */
void* print_window(void* arg);

void* emulate_event(void* arg);

typedef struct list_node {
    int close;
    Window wid;
    struct list_node *next;
} Node;

typedef struct {
    int64_t x, y, button;
    int64_t source_wid;
    int64_t type;
} Item;

typedef struct node * PNode;

typedef struct node {
    Item data;
    PNode next;
} qNode;

typedef struct {
    int size;
    PNode front;
    PNode rear;
} Queue;

extern char *mem;
extern char *ram_start;
extern Display *dpy;
extern pthread_mutex_t mutex_actwin;
extern pthread_mutex_t mutex_closenotify;
extern pthread_mutex_t mutex_fd;
extern pthread_mutex_t mutex_link;
extern pthread_mutex_t mutex_mem;
extern pthread_mutex_t mutex_queue;
extern deque<Item> event_queue;
extern list<Node> head, head_new;

bool isCounted(Window guest_id);
void deleteByGuestId(Window guest_id);

#endif

