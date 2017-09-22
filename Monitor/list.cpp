#include "svmos.h"

bool isCounted(Window guest_id)
{
    list<Node>::iterator it = head.begin();
    while (it != head.end()) {
        if (it->wid == guest_id) {
            return true;
        }
        it++;
    }
    return false;
}

void deleteByGuestId(Window guest_id)
{
    list<Node>::iterator it = head.begin();
    while (it != head.end()) {
        //cout << guest_id << endl;
        //cout <<  it->wid << endl;
        if (it->wid == guest_id) {
            head.erase(it);
            return;
        }
        it++;
    }
}

