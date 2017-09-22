#include "svmos.h"

#define MAGIC "#*"

/* guest RAM */
char *ram_start;
/* kernel memory */
char *mem;

int ErrorFlag = 0;

uint64_t offset;
Display *dpy = NULL;

/* head is all we've maintained so far */
list<Node> head;
/* head_new is what's new on desktop */
list<Node> head_new;

pthread_mutex_t mutex_mem;
pthread_mutex_t mutex_queue;
pthread_mutex_t mutex_link;
pthread_mutex_t mutex_closenotify;
pthread_mutex_t mutex_fd;

static int bad_window_handler(Display *dsp, XErrorEvent *err)
{
    char buff[256];

    XGetErrorText(dsp, err->error_code, buff, sizeof(buff));
    //printf("X Error %s\n", buff);
    ErrorFlag = 1;
    return 0;
}

int main(int argc, char** argv)
{
    pthread_t thread_window;
    pthread_t thread_recv_event;
    pthread_t thread_emulate_event;

    int major_opcode, first_event, first_error;

    /* Check Xrender Support */
    XInitThreads();
    dpy = XOpenDisplay(":0.0");
    if (XQueryExtension(dpy, RENDER_NAME, &major_opcode, &first_event, &first_error) == False)
    {
        fprintf(stderr, "No Xrender-Support\n");
        return 0;
    }

    /* open RAM and obtain the kmem's offset in of RAM */
    ram_start = (char *)openRAM();
    while (memcmp(ram_start, MAGIC, sizeof(MAGIC)) != 0)
    {
        sleep(1);
        printf(" .");
        fflush(stdout);
    }
    offset = *(uint64_t *)(ram_start + 0x2);
    printf("kernel memory allocated at 0x%lx.\n", offset);

    /* then we obtain the physically contiguous kernel memory allocated by our driver */
    mem = (char *)openKernelMemory();

    mem[1] = '0';// ==> This flag stands for output switch

    /* Init all */
    pthread_mutex_init(&mutex_fd, NULL);
    pthread_mutex_init(&mutex_link, NULL);
    pthread_mutex_init(&mutex_queue, NULL);

    pthread_create(&thread_recv_event, NULL, &recv_events, NULL);
    pthread_create(&thread_emulate_event, NULL, &emulate_event, NULL);

    arg_struct *arg;
    list <Node>::iterator p;

    XSetErrorHandler(bad_window_handler);

    for (;;) {
        enum_new_windows(dpy);

        for (p = head_new.begin(); p != head_new.end(); p++) {
            if (ErrorFlag) {
                pthread_mutex_lock(&mutex_link);
                deleteByGuestId(p->wid);
                pthread_mutex_unlock(&mutex_link);
                ErrorFlag = 0;
            } else {
                arg = (arg_struct *)malloc(sizeof(arg_struct));
                arg->dpy = XOpenDisplay(":0.0");
                arg->wid = p->wid;
                pthread_create(&thread_window, NULL, &print_window, (void *)arg);
                //printf("New Guest Window ID: 0x%lx\n", arg->wid);
                //printf("[Thread 0x%lx for Win 0x%lx inited]\n", thread_window, arg->wid);
            }
        }
        head_new.clear();
    }
    
    XCloseDisplay(dpy);
    return 0;
}

