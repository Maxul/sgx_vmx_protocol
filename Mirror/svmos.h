#ifndef SVMOS_H
#define SVMOS_H

#include <fcntl.h>
#include <locale.h>
#include <malloc.h>
#include <memory.h>
#include <pthread.h>
#include <regex.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>

#include <list>
#include <queue>
using namespace std;

enum {
    SVM_WIN_UPDATE = 0xa,
    SVM_WIN_DESTROY,
};

#define GUEST_IN_OFF 1024
#define GUEST_OUT_OFF 1024
#define SHM_IN_OFF 2
#define SHM_OUT_OFF 2
#define REST_SIZE (KMEM_SIZE - GUEST_OUT_OFF)

#define SHARED_MEM_SIZE (1 << 22)
#define KMEM_SIZE SHARED_MEM_SIZE

enum {
    OUTPUT_READY,
    OUTPUT_DONE,
    INPUT_READY,
    INPUT_DONE,
};

#define write_input(val) {shmem[0] = val;}
#define write_output(val) {shmem[1] = val;}

struct output_prot {
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

#define imem ((struct output_prot *)(shmem + 1024))

/*thread for draw picture*/
void *draw_window(void *arg);

struct event_arg {
  Display *dpy;
  Window wnd;
};

/* add Xevent to queue*/
void *input_event_queue(void *arg);

union semun {
  int val;
  unsigned short *array;
  struct semid_ds *buf;
};

int set_semvalue(int semid);
int semaphore_p(int semid);
int semaphore_v(int semid);
extern int sem_id;

typedef struct List_Node {
  int close;
  int resize;
  int width, height;
  Window gID; // guest winID
  Window hID; // local winID
  struct List_Node *next;
} Node;

typedef struct {
  int64_t x, y;
  int64_t button;
  int64_t source_wid; // no good
  int64_t event_type;
} Item;

void get_window(Display *dpy, Window window, const char *winname);

typedef struct node *PNode;
typedef struct node {
  Item data;
  PNode next;
} qNode;

typedef struct {
  PNode front;
  PNode rear;
  int size;
} Queue;

/////////////

extern char *shmem;
extern sem_t *mutex; /* mutex for shared memory */
extern Display *dpy; /* for title */

extern unsigned int border_color; // color
extern int color_border;          // border

//extern Node *head;

extern pthread_mutex_t mutex_link;
extern pthread_mutex_t mutex_queue;
extern deque<Item> event_queue;
extern list<Node> window_list;

extern Window desktop;
extern XSetWindowAttributes xs;

/////////////
void print_list(void);
bool isCounted(Window id);
Window getGuestID(Window host_id);
list<Node>::iterator findNodeByGuestID(Window guest_id);
void deleteByGuestId(Window guest_id);

#endif
