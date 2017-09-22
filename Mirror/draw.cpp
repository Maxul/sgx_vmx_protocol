#include "svmos.h"

/* Place the new window's top-left corner at the given 'x,y' coordinates. */
static Window create_simple_window(Display *display, int width, int height,
                                   int x, int y, char override) {
  int screen_num = DefaultScreen(display);
  int win_border_width = 0;
  Window win;

  if (override == '1')
    win_border_width = color_border;

  win = XCreateSimpleWindow(display, RootWindow(display, screen_num), x, y,
                            width, height, win_border_width, border_color,
                            WhitePixel(display, screen_num));

  /* we need to realize override property */
  if (override == '1')
    XChangeWindowAttributes(display, win, CWOverrideRedirect, &xs);

  /* make the window actually appear on the screen. */
  XMapWindow(display, win);
  /* flush all pending requests to the X server. */
  XFlush(display);
  return win;
}

static GC create_gc(Display *display, Window win, int reverse_video) {
  GC gc;                       /* handle of newly created GC.  */
  unsigned long valuemask = 0; /* GC component mask bits.      */

  /* check when creating the GC.  */
  XGCValues values;            /* initial values for the GC.   */
  unsigned int line_width = 2; /* line width for the GC.       */
  int line_style = LineSolid;  /* style for lines drawing and  */
  int cap_style = CapButt;     /* style of the line's edje and */
  int join_style = JoinBevel;  /* joined lines.                */
  int screen_num = DefaultScreen(display);

  gc = XCreateGC(display, win, valuemask, &values);
  if (gc < 0) {
    fprintf(stderr, "XCreateGC Error\n");
    exit(1);
  }

  /* allocate foreground and background colors for this GC. */
  if (reverse_video) {
    XSetForeground(display, gc, WhitePixel(display, screen_num));
    XSetBackground(display, gc, BlackPixel(display, screen_num));
  } else {
    XSetForeground(display, gc, BlackPixel(display, screen_num));
    XSetBackground(display, gc, WhitePixel(display, screen_num));
  }

  /* define the style of lines that will be drawn using this GC. */
  XSetLineAttributes(display, gc, line_width, line_style, cap_style,
                     join_style);
  /* define the fill style for the GC. to be 'solid filling'. */
  XSetFillStyle(display, gc, FillSolid);
  return gc;
}

/* Draw border Rectangle */
static void draw_rectangle(Display *display, Window win, int x, int y,
                           int width, int height) {
  unsigned long valuemask = 0;

  XGCValues values;

  int line_style = LineSolid;
  int cap_style = CapRound;
  int join_style = JoinRound;

  GC gc = XCreateGC(display, win, valuemask, &values);
  XSetLineAttributes(display, gc, color_border * 2, line_style, cap_style,
                     join_style);
  XSetForeground(display, gc, border_color);
  XDrawRectangle(display, win, gc, x, y, width, height);
  XFreeGC(display, gc);
}

static void construct_name(XTextProperty *name, size_t len) {
  if (8 != name->format) {
    char *utf8_title_string = (char *)malloc(len);
    XStringListToTextProperty(&utf8_title_string, 1, name);
    free(utf8_title_string);
  }
}

static void set_wm_name(Display *dpy, Window win) {
  unsigned char *title_string;
  int title_len;

  XTextProperty window_name_property;
  XGetWMName(dpy, desktop, &window_name_property);
  XFree(window_name_property.value);

  title_len = imem->title_len;
  title_string = (unsigned char *)malloc(title_len);

  memcpy((void *)title_string, &(imem->title), title_len);
  window_name_property.value = title_string;
  window_name_property.format = imem->format;
  window_name_property.nitems = imem->nitems;
  construct_name(&window_name_property, title_len);

  XSetWMName(dpy, win, &window_name_property);
  free(title_string);
}

#define destroy_local_win(dpy)                                                 \
  {                                                                            \
    XFreeGC(dpy, gc);                                                          \
    XUnmapWindow(dpy, win);                                                    \
    XDestroyWindow(dpy, win);                                                  \
                                                                               \
    XSync(dpy, False);                                                         \
    XFlush(dpy);                                                               \
    XCloseDisplay(dpy);                                                        \
                                                                               \
    semaphore_p(sem_id);                                                       \
    shmem[1] = '0';                                                              \
    semaphore_v(sem_id);                                                       \
    /** newly fixed 2017/1/30 */                                               \
    pthread_detach(pthread_self());                                            \
    pthread_join(thread_input_event, 0);                                       \
    return NULL;                                                               \
  }

/* TODO: clean this nasty shit!!! */
void *draw_window(void *arg) {
  pthread_t thread_input_event = 0;

  Display *dpy_root = XOpenDisplay(NULL);
  Window root = DefaultRootWindow(dpy_root);
  int screen = DefaultScreen(dpy_root);

  /* this is for new window from guest, if close, close this one */
  Display *dpy_wnd = XOpenDisplay(NULL);
  Visual *visual = DefaultVisual(dpy_wnd, screen);

  XWindowAttributes win_attr_ret;

  XRenderPictFormat *format = None;

  /* replace this with that from the guest */
  XImage *image_tmp = None;
  Picture src_pic, dst_pic;
  Pixmap src_pm = None;

  Window win_relative;
  list<Node>::iterator find_result;

  int dx = 0, dy = 0;
  int color_border_native = color_border;

  char override = imem->override;
  Window guest_wid = imem->win;

  unsigned int width = imem->width;
  unsigned int height = imem->height;

  /* 如果属于二类窗口 */
  if ('1' == override) {
    /* 找到相对偏移 */
    dx = imem->dx;
    dy = imem->dy;
    win_relative = imem->win_related;

    color_border_native = 0;

    pthread_mutex_lock(&mutex_link);
    find_result = findNodeByGuestID(win_relative);
    pthread_mutex_unlock(&mutex_link);

    // printf("0x%lx -> father 0x%lx\n", guest_wid, win_relative);
    if (find_result != window_list.end()) {
      int local_x, local_y;
      Window junk_win, local_win = find_result->hID;

      XGetWindowAttributes(dpy_root, local_win, &win_attr_ret);

      XTranslateCoordinates(
          dpy_root, local_win, win_attr_ret.root, -win_attr_ret.border_width,
          -win_attr_ret.border_width, &local_x, &local_y, &junk_win);

      dx += local_x + color_border;
      dy += local_y + color_border;
    }
  } else {
    /* 记住最好把窗口放在屏幕中央 */
    dx = 0;
    dy = 0;
  }
  /* 这个信号量应该是用来同步shm的，其他操作别往里放 */
  semaphore_v(sem_id); // V

  Window win = create_simple_window(dpy_wnd, width + color_border_native * 2,
                                    height + color_border_native * 2,
                                    dx + color_border_native,
                                    dy + color_border_native, override);

  /* GC (graphics context) used for drawing */
  GC gc = create_gc(dpy_root, win, 0);

  /* update Node for guest winID's newly created <wid,width,height> */
  pthread_mutex_lock(&mutex_link);
  find_result = findNodeByGuestID(guest_wid);
  if (window_list.end() == find_result) {
    destroy_local_win(dpy_wnd);
  } else {
    find_result->hID = win;
    find_result->width = width;
    find_result->height = height;
  }
  pthread_mutex_unlock(&mutex_link);

  XSync(dpy_wnd, False);

  format = XRenderFindVisualFormat(dpy_wnd, visual);
  dst_pic = XRenderCreatePicture(dpy_wnd, win, format, 0, 0);

  /* get any to fill the window */
  image_tmp = XGetImage(dpy_root, root, 0, 0, width, height, 0L, ZPixmap);

  XSelectInput(dpy_wnd, win, ExposureMask | ButtonPressMask |
                                 ButtonReleaseMask | PointerMotionMask |
                                 KeyPressMask | KeyReleaseMask |
                                 StructureNotifyMask);

  /* can be closed??? */
  Atom delWindow = XInternAtom(dpy_wnd, "WM_DELETE_WINDOW", 0);
  XSetWMProtocols(dpy_wnd, win, &delWindow, 1);

  /* now that we have a window, we should listen to what happens to it. */
  /* 创建一个新线程专门负责收集这个窗口的一举一动，而本线程负责发送 */
  struct event_arg *event_arg = (struct event_arg *)malloc(sizeof(struct event_arg));
  event_arg->dpy = dpy_wnd;
  event_arg->wnd = win;
  pthread_create(&thread_input_event, NULL, &input_event_queue,
                 (void *)event_arg);

#define _DEBUG
#ifdef DEBUG
  clock_t now = clock();
#endif

//////////////////////////// this loop is to update the output
  for (;;) {
    pthread_mutex_lock(&mutex_link);
    find_result = findNodeByGuestID(guest_wid);
    /* guest window is not existing */
    if (find_result == window_list.end()) {
      pthread_mutex_unlock(&mutex_link);
      destroy_local_win(dpy_root);
    }
#if 0
    if (1 == find_result->resize) /* this window has been resized */
    {
      width = find_result->width;
      height = find_result->height;
      XDestroyImage(image_tmp);
      image_tmp =
          XGetImage(dpy_root, root, 0, 0, width, height, AllPlanes, ZPixmap);
      find_result->resize = 0;
    }
#endif
    pthread_mutex_unlock(&mutex_link);

    /* window content */
    semaphore_p(sem_id); // P
    if (imem->win == (int)guest_wid && shmem[1] == '1' && imem->msg_type == SVM_WIN_DESTROY) {
        printf("Window 0x%lx is to be closed\n", imem->win);
        pthread_mutex_lock(&mutex_link);
        deleteByGuestId(guest_wid);
        pthread_mutex_unlock(&mutex_link);
    } else if (imem->win == (int)guest_wid && shmem[1] == '1' && imem->msg_type == SVM_WIN_UPDATE) {
      /* window title */
      if (override == '0')
        set_wm_name(dpy_root, win);
#ifdef WIN32
      printf("depth %d\n", *(int *)(shmem + 2 + sizeof(int) * 6 + 200));
      /** @IMPORTANT: DEPTH ISSUE reannounced by Maxul Lee <Apr 28th 2015> */
      if (24 == *(int *)(mem + 2 + sizeof(int) * 6 + 200)) {
        image_tmp->bits_per_pixel = 24;
        int dummy = (image_tmp->width * 3) % 4;
        if (dummy)
          dummy = 4 - dummy;
        image_tmp->bytes_per_line = image_tmp->width * 3 + dummy;
      }
#endif
      /* draw */
      memcpy((image_tmp->data), &(imem->image), malloc_usable_size((void *)(image_tmp->data)));
      shmem[1] = '0'; // tell qemu-kvm has received data completely

#ifdef DEBUG
      printf("Win: 0x%lx \t size: %lfMB \t ", guest_wid,
             (double)malloc_usable_size((void *)(image_tmp->data)) / 1024 / 1024);
      printf("time: %ld us\n", clock() - now);
      now = clock();
#endif
      /* update the window */
      src_pm = XCreatePixmap(dpy_root, win, width, height, image_tmp->depth);
      XPutImage(dpy_root, src_pm, gc, image_tmp, 0, 0, 0, 0, width, height);
      src_pic = XRenderCreatePicture(dpy_root, src_pm, format, 0, 0);
      XRenderComposite(dpy_root, PictOpSrc, src_pic, 0, dst_pic, 0, 0, 0, 0,
                       color_border_native, color_border_native, width, height);

      if (color_border_native != 0)
        draw_rectangle(dpy_root, win, 0, 0, width + color_border_native * 2,
                       height + color_border_native * 2);
      XSync(dpy_root, False);
      XRenderFreePicture(dpy_root, src_pic);
      XFreePixmap(dpy_root, src_pm);
      /* update finished */
    }
    semaphore_v(sem_id);

    /* send events from event queue to QEMU-KVM */
    pthread_mutex_lock(&mutex_queue);
    if ('-' == shmem[0] && event_queue.size() > 0) {
      Item item_tmp;
      item_tmp = event_queue.front();
      event_queue.pop_front();
      //printf("window %ld x:%ld,y:%ld type:%ld\n", item_tmp.source_wid, item_tmp.x, item_tmp.y, item_tmp.event_type);
      memcpy(shmem + 2, (void *)&item_tmp, sizeof(item_tmp));
      shmem[0] = '+';
      pthread_mutex_unlock(&mutex_queue);
    } else {
      pthread_mutex_unlock(&mutex_queue);
    }
  }
}
