#include "svmos.h"

void print_list()
{
    if (0 == window_list.size())
        return;

    list<Node>::iterator it = window_list.begin();
    while (it != window_list.end()) {
        printf("0x%0lx --> ", (*it).gID);
        it++;
    }
    puts("^");
}

bool isCounted(Window id)
{
    if (0 == window_list.size())
        return false;

    list<Node>::iterator it = window_list.begin();
    while (it != window_list.end()) {
        if (it->gID == id) {
            return true;
        }
        it++;
    }
    return false;
}

Window getGuestID(Window host_id)
{
    if (0 == window_list.size())
        return 0;

    list<Node>::iterator it = window_list.begin();
    while (it != window_list.end()) {
        if (it->hID == host_id) {
            return (*it).gID;
        }
        it++;
    }
    return 0;
}

list<Node>::iterator findNodeByGuestID(Window guest_id)
{
    list<Node>::iterator it = window_list.begin();
    while (it != window_list.end()) {
        if (it->gID == guest_id) {
            return it;
        }
        it++;
    }
    return it;
}

void deleteByGuestId(Window guest_id)
{
    list<Node>::iterator it = window_list.begin();
    while (it != window_list.end()) {
        if (it->gID == guest_id) {
            window_list.erase(it);
            return;
        }
        it++;
    }
}

