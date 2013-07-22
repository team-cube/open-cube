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
    loopvrev(needsapply)
    {
        change &c = needsapply[i];    
        if(c.type&type)
        {
            c.type &= ~type;
            if(!c.type) needsapply.remove(i);
        }
    }
}

void applychanges()
{
    int changetypes = 0;
    loopv(needsapply) changetypes |= needsapply[i].type;
    if(changetypes&CHANGE_GFX) execident("resetgl");
    else if(changetypes&CHANGE_SHADERS) execident("resetshaders");
    if(changetypes&CHANGE_SOUND) execident("resetsound");
}
COMMAND(applychanges, "");

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

