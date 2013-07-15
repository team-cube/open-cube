#include "engine.h"

void notifywelcome()
{
}

struct change
{
    int type;
    const char *desc;

    change() {}
    change(int type, const char *desc) : type(type), desc(desc) {}
};
static vector<change> needsapply;

VARP(applydialog, 0, 1, 1);

void addchange(const char *desc, int type)
{
    if(!applydialog) return;
    needsapply.add(change(type, desc));
}

void clearchanges(int type)
{
    loopv(needsapply)
    {
        if(needsapply[i].type&type)
        {
            needsapply[i].type &= ~type;
            if(!needsapply[i].type) needsapply.remove(i--);
        }
    }
}

void menuprocess()
{
}

VAR(mainmenu, 1, 1, 0);

void clearmainmenu()
{
    if(mainmenu && isconnected())
    {
        mainmenu = 0;
    }
}

