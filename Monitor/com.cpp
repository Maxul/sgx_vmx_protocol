#include "svmos.h"

void *recv_events(void *unused)
{
    char c;
    char *event_flag = mem;
    Item item_tmp;

    for (;;)
    {
        pthread_mutex_lock(&mutex_fd);//P
        c = *event_flag;
        pthread_mutex_unlock(&mutex_fd);//V

        if ('+' == c) // receiving new event
        {
            memcpy((void *)&item_tmp, mem + 2, sizeof(Item));

            pthread_mutex_lock(&mutex_queue);
            event_queue.push_back(item_tmp);
            pthread_mutex_unlock(&mutex_queue);

            pthread_mutex_lock(&mutex_fd);//P
            *event_flag = '-';
            pthread_mutex_unlock(&mutex_fd);//V
        }
    }
}

void send_notify(Window win)
{
    printf("new version : 0x%lx should be closed\n", win);
    
    pthread_mutex_lock(&mutex_mem);
    for (;;) if ('0' == mem[1]) break; //qemu ready to receive
    struct output_prot wi;
    memset(&wi, 0, sizeof(struct output_prot));
    wi.msg_type = SVM_WINDOW_DESTROY;
    wi.win = win;
    memcpy((void *)(mem + GUEST_OUT_OFF), (void *)&wi, sizeof(wi));
    mem[1] = '1';
    pthread_mutex_unlock(&mutex_mem);
}

