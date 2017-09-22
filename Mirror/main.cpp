#include "svmos.h"

#define PRJ_ID 0xf
#define BORDER 3 // color border

/* shared memory with qemu */
char *shmem;
/* mutex that synchronises for shm */
sem_t *mutex;

/* convenience for XGetTextProperty to obtain the WM_NAME property */
Window desktop;
Display *dpy = NULL;

int color_border = BORDER;
unsigned int border_color;

list<Node> window_list;
deque<Item> event_queue;

pthread_mutex_t mutex_queue;
pthread_mutex_t mutex_link;

XSetWindowAttributes xs;

static int sem_id_close; /* FIXME: no need to be static */

void connect_to_guest(char *auth) {
  int shm_id;

  key_t key_close; // ==> sem_id_close
  key_t key;       // ==> sem_id

  /* init all ipcs */
  key_close = ftok(auth, 0x03);
  key = ftok(auth, PRJ_ID);
  if (key_close == -1 || key == -1) {
    perror("ftok error");
    exit(1);
  }

  sem_id_close = semget(key_close, 1, IPC_CREAT | 0600);
  sem_id = semget(key, 1, IPC_CREAT | 0600);
  if (sem_id_close == -1 || sem_id == -1) {
    perror("semget error");
    exit(1);
  }

  shm_id = shmget(key, SHARED_MEM_SIZE, IPC_CREAT | 0600);
  if (shm_id == -1) {
    perror("shmget error");
    exit(1);
  }
  printf("VM: %s\tsem: 0x%0x\n", auth, sem_id);

  shmem = (char *)shmat(shm_id, NULL, 0);
  // memset(mem, 0, SHARED_MEM_SIZE); /* 2017/2/12 for sake of re-connection */
  shmem[0] = '-'; // this flag is for user input switch
  shmem[1] = '0';
  //write_input(INPUT_READY);
  //write_output(OUTPUT_READY);
}

int main(int argc, char **argv)
{
  Window win_receive;
  pthread_t thread_draw;

  xs.override_redirect = True;

  border_color = (unsigned int)strtoul(argv[1], 0, 0);

  /* Check Xrender Support */
  int major_opcode, first_event, first_error;
  XInitThreads();
  dpy = XOpenDisplay(NULL);
  if (XQueryExtension(dpy, RENDER_NAME, &major_opcode, &first_event,
                      &first_error) == False) {
    fprintf(stderr, "No Xrender-Support\n");
    return 1;
  }

  get_window(dpy, DefaultRootWindow(dpy), "桌面");

  /** @MUST: try connecting with guest agent */
  connect_to_guest(argv[argc - 1]);

  pthread_mutex_init(&mutex_queue, NULL);
  pthread_mutex_init(&mutex_link, NULL);

  int flag_value;

  for (;;) {
    flag_value = semctl(sem_id_close, 0, GETVAL);
    if (flag_value)
      goto exit_loop;

    semaphore_p(sem_id); // P
    pthread_mutex_lock(&mutex_link);

    win_receive = imem->win; // new guest window

    if (0x0 != win_receive && isCounted(win_receive) == false && imem->msg_type == SVM_WIN_UPDATE) {

      Node this_win;
      this_win.close = 0;
      this_win.gID = win_receive;
      window_list.push_back(this_win);
      
      pthread_mutex_unlock(&mutex_link);

      //print_list();

      /* create a new thread to draw */
      pthread_create(&thread_draw, NULL, &draw_window, (void *)NULL);
    } else {
      pthread_mutex_unlock(&mutex_link);
      semaphore_v(sem_id); // V
    }
  }

exit_loop:
  semctl(sem_id_close, 0, IPC_RMID);
  XCloseDisplay(dpy);
  printf("Awesome mirror exits successfully!\n");
  return 0;
}
