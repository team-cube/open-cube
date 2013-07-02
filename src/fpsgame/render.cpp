#include "game.h"

namespace game
{      
    vector<fpsent *> bestplayers;
    vector<int> bestteams;

    VARP(ragdoll, 0, 1, 1);
    VARP(ragdollmillis, 0, 10000, 300000);
    VARP(ragdollfade, 0, 1000, 300000);
    VARFP(playermodel, 0, 0, 0, changedplayermodel());
    VARP(forceplayermodels, 0, 0, 1);
    VARP(hidedead, 0, 0, 1);

    vector<fpsent *> ragdolls;

    void saveragdoll(fpsent *d)
    {
        if(!d->ragdoll || !ragdollmillis || (!ragdollfade && lastmillis > d->lastpain + ragdollmillis)) return;
        fpsent *r = new fpsent(*d);
        r->lastupdate = ragdollfade && lastmillis > d->lastpain + max(ragdollmillis - ragdollfade, 0) ? lastmillis - max(ragdollmillis - ragdollfade, 0) : d->lastpain;
        r->edit = NULL;
        r->ai = NULL;
        if(d==player1) r->playermodel = playermodel;
        ragdolls.add(r);
        d->ragdoll = NULL;   
    }

    void clearragdolls()
    {
        ragdolls.deletecontents();
    }

    void moveragdolls()
    {
        loopv(ragdolls)
        {
            fpsent *d = ragdolls[i];
            if(lastmillis > d->lastupdate + ragdollmillis)
            {
                delete ragdolls.remove(i--);
                continue;
            }
            moveragdoll(d);
        }
    }

    static const playermodelinfo playermodels[1] =
    {
        { { "player", "player/blue", "player/red" }, { "player/hudguns", "player/hudguns/blue", "player/hudguns/red" }, { "player", "player_blue", "player_red" }, true }
    };

    int chooserandomplayermodel(int seed)
    {
        return (seed&0xFFFF)%(sizeof(playermodels)/sizeof(playermodels[0]));
    }

    const playermodelinfo *getplayermodelinfo(int n)
    {
        if(size_t(n) >= sizeof(playermodels)/sizeof(playermodels[0])) return NULL;
        return &playermodels[n];
    }

    const playermodelinfo &getplayermodelinfo(fpsent *d)
    {
        const playermodelinfo *mdl = getplayermodelinfo(d==player1 || forceplayermodels ? playermodel : d->playermodel);
        if(!mdl) mdl = getplayermodelinfo(playermodel);
        return *mdl;
    }

    void changedplayermodel()
    {
        if(player1->clientnum < 0) player1->playermodel = playermodel;
        if(player1->ragdoll) cleanragdoll(player1);
        loopv(ragdolls) 
        {
            fpsent *d = ragdolls[i];
            if(!d->ragdoll) continue;
            if(!forceplayermodels)
            {
                const playermodelinfo *mdl = getplayermodelinfo(d->playermodel);
                if(mdl) continue;
            }
            cleanragdoll(d);
        }
        loopv(players)
        {
            fpsent *d = players[i];
            if(d == player1 || !d->ragdoll) continue;
            if(!forceplayermodels)
            {
                const playermodelinfo *mdl = getplayermodelinfo(d->playermodel);
                if(mdl) continue;
            }
            cleanragdoll(d);
        }
    }

    void preloadplayermodel()
    {
        loopi(3)
        {
            const playermodelinfo *mdl = getplayermodelinfo(i);
            if(!mdl) break;
            if(i != playermodel && (!multiplayer(false) || forceplayermodels)) continue;
            if(m_teammode)
            {
                loopj(MAXTEAMS) preloadmodel(mdl->model[1+j]);
            }
            else preloadmodel(mdl->model[0]);
        }
    }
    
    void renderplayer(fpsent *d, const playermodelinfo &mdl, int team, float fade, bool mainpass = true)
    {
        int lastaction = d->lastaction, hold = d->gunselect==GUN_PISTOL ? 0 : (ANIM_HOLD1+d->gunselect)|ANIM_LOOP, attack = ANIM_ATTACK1+d->gunselect, delay = guns[d->gunselect].attackdelay+50;
        if(intermission && d->state!=CS_DEAD)
        {
            lastaction = 0;
            hold = attack = ANIM_LOSE|ANIM_LOOP;
            delay = 0;
            if(m_teammode ? bestteams.htfind(d->team)>=0 : bestplayers.find(d)>=0) hold = attack = ANIM_WIN|ANIM_LOOP;
        }
        else if(d->state==CS_ALIVE && d->lasttaunt && lastmillis-d->lasttaunt<1000 && lastmillis-d->lastaction>delay)
        {
            lastaction = d->lasttaunt;
            hold = attack = ANIM_TAUNT;
            delay = 1000;
        }
        modelattach a[5];
        static const char *vweps[] = {"vwep/fist", "vwep/shotg", "vwep/chaing", "vwep/rocket", "vwep/rifle", "vwep/gl", "vwep/pistol"};
        int ai = 0;
        if(d->gunselect<=GUN_PISTOL)
        {
            int vanim = ANIM_VWEP_IDLE|ANIM_LOOP, vtime = 0;
            if(lastaction && d->lastattackgun==d->gunselect && lastmillis < lastaction + delay)
            {
                vanim = ANIM_VWEP_SHOOT;
                vtime = lastaction;
            }
            a[ai++] = modelattach("tag_weapon", vweps[d->gunselect], vanim, vtime);
        }
        if(mainpass)
        {
            d->muzzle = vec(-1, -1, -1);
            a[ai++] = modelattach("tag_muzzle", &d->muzzle);
        }
        const char *mdlname = mdl.model[validteam(team) ? team : 0];
        renderclient(d, mdlname, a[0].tag ? a : NULL, hold, attack, delay, lastaction, intermission && d->state!=CS_DEAD ? 0 : d->lastpain, fade, ragdoll && mdl.ragdoll);
    }

    void rendergame()
    {
        ai::render();

        if(intermission)
        {
            bestteams.shrink(0);
            bestplayers.shrink(0);
            if(m_teammode) getbestteams(bestteams);
            else getbestplayers(bestplayers);
        }

        fpsent *exclude = isthirdperson() ? NULL : followingplayer();
        loopv(players)
        {
            fpsent *d = players[i];
            if(d == player1 || d->state==CS_SPECTATOR || d->state==CS_SPAWNING || d->lifesequence < 0 || d == exclude || (d->state==CS_DEAD && hidedead)) continue;
            int team = m_teammode && validteam(d->team) ? d->team : 0;
            renderplayer(d, getplayermodelinfo(d), team, 1);
            copystring(d->info, colorname(d));
            if(d->state!=CS_DEAD) particle_text(d->abovehead(), d->info, PART_TEXT, 1, teamtextcolor[team], 2.0f);
        }
        loopv(ragdolls)
        {
            fpsent *d = ragdolls[i];
            float fade = 1.0f;
            if(ragdollmillis && ragdollfade) 
                fade -= clamp(float(lastmillis - (d->lastupdate + max(ragdollmillis - ragdollfade, 0)))/min(ragdollmillis, ragdollfade), 0.0f, 1.0f);
            int team = m_teammode && validteam(d->team) ? d->team : 0;
            renderplayer(d, getplayermodelinfo(d), team, fade);
        } 
        if(isthirdperson() && !followingplayer() && (player1->state!=CS_DEAD || !hidedead)) renderplayer(player1, getplayermodelinfo(player1), player1->team, 1);
        entities::renderentities();
        renderbouncers();
        renderprojectiles();
        if(cmode) cmode->rendergame();
    }

    VARP(hudgun, 0, 1, 1);
    VARP(hudgunsway, 0, 1, 1);

    FVAR(swaystep, 1, 35.0f, 100);
    FVAR(swayside, 0, 0.04f, 1);
    FVAR(swayup, -1, 0.05f, 1);

    float swayfade = 0, swayspeed = 0, swaydist = 0;
    vec swaydir(0, 0, 0);

    void swayhudgun(int curtime)
    {
        fpsent *d = hudplayer();
        if(d->state != CS_SPECTATOR)
        {
            if(d->physstate >= PHYS_SLOPE)
            {
                swayspeed = min(sqrtf(d->vel.x*d->vel.x + d->vel.y*d->vel.y), d->maxspeed);
                swaydist += swayspeed*curtime/1000.0f;
                swaydist = fmod(swaydist, 2*swaystep);
                swayfade = 1;
            }
            else if(swayfade > 0)
            {
                swaydist += swayspeed*swayfade*curtime/1000.0f;
                swaydist = fmod(swaydist, 2*swaystep);
                swayfade -= 0.5f*(curtime*d->maxspeed)/(swaystep*1000.0f);
            }

            float k = pow(0.7f, curtime/10.0f);
            swaydir.mul(k);
            vec vel(d->vel);
            vel.add(d->falling);
            swaydir.add(vec(vel).mul((1-k)/(15*max(vel.magnitude(), d->maxspeed))));
        }
    }

    struct hudent : dynent
    {
        hudent() { type = ENT_CAMERA; }
    } guninterp;

    void drawhudmodel(fpsent *d, int anim, float speed = 0, int base = 0)
    {
        if(d->gunselect>GUN_PISTOL) return;

        vec sway;
        vecfromyawpitch(d->yaw, 0, 0, 1, sway);
        float steps = swaydist/swaystep*M_PI;
        sway.mul(swayside*cosf(steps));
        sway.z = swayup*(fabs(sinf(steps)) - 1);
        sway.add(swaydir).add(d->o);
        if(!hudgunsway) sway = d->o;

        const playermodelinfo &mdl = getplayermodelinfo(d);
        defformatstring(gunname)("%s/%s", mdl.hudguns[m_teammode && validteam(d->team) ? d->team : 0], guns[d->gunselect].file);
        modelattach a[2];
        d->muzzle = vec(-1, -1, -1);
        a[0] = modelattach("tag_muzzle", &d->muzzle);
        rendermodel(gunname, anim, sway, d->yaw, d->pitch, 0, MDL_NOBATCH, NULL, a, base, (int)ceil(speed));
        if(d->muzzle.x >= 0) d->muzzle = calcavatarpos(d->muzzle, 12);
    }

    void drawhudgun()
    {
        fpsent *d = hudplayer();
        if(d->state==CS_SPECTATOR || d->state==CS_EDITING || !hudgun || editmode) 
        { 
            d->muzzle = player1->muzzle = vec(-1, -1, -1);
            return;
        }

        int rtime = guns[d->gunselect].attackdelay;
        if(d->lastaction && d->lastattackgun==d->gunselect && lastmillis-d->lastaction<rtime)
        {
            drawhudmodel(d, ANIM_GUN_SHOOT|ANIM_SETSPEED, rtime/17.0f, d->lastaction);
        }
        else
        {
            drawhudmodel(d, ANIM_GUN_IDLE|ANIM_LOOP);
        }
    }

    void renderavatar()
    {
        drawhudgun();
    }

    void renderplayerpreview(int model, int team, int weap)
    {
        static fpsent *previewent = NULL;
        if(!previewent)
        {
            previewent = new fpsent;
            previewent->o = vec(0, 0.9f*(previewent->eyeheight + previewent->aboveeye), previewent->eyeheight - (previewent->eyeheight + previewent->aboveeye)/2);
            loopi(GUN_PISTOL-GUN_FIST) previewent->ammo[GUN_FIST+1+i] = 1;
        }
        previewent->gunselect = clamp(weap, int(GUN_FIST), int(GUN_PISTOL));
        previewent->yaw = fmod(lastmillis/10000.0f*360.0f, 360.0f);
        const playermodelinfo *mdlinfo = getplayermodelinfo(model);
        if(!mdlinfo) return;
        renderplayer(previewent, *mdlinfo, validteam(team) ? team : 0, 1, false);
    }

    vec hudgunorigin(int gun, const vec &from, const vec &to, fpsent *d)
    {
        if(d->muzzle.x >= 0) return d->muzzle;
        vec offset(from);
        if(d!=hudplayer() || isthirdperson())
        {
            vec front, right;
            vecfromyawpitch(d->yaw, d->pitch, 1, 0, front);
            offset.add(front.mul(d->radius));
            offset.z += (d->aboveeye + d->eyeheight)*0.75f - d->eyeheight;
            vecfromyawpitch(d->yaw, 0, 0, -1, right);
            offset.add(right.mul(0.5f*d->radius));
            offset.add(front);
            return offset;
        }
        offset.add(vec(to).sub(from).normalize().mul(2));
        if(hudgun)
        {
            offset.sub(vec(camup).mul(1.0f));
            offset.add(vec(camright).mul(0.8f));
        }
        else offset.sub(vec(camup).mul(0.8f));
        return offset;
    }

    void preloadweapons()
    {
        const playermodelinfo &mdl = getplayermodelinfo(player1);
        loopi(NUMGUNS)
        {
            const char *file = guns[i].file;
            if(!file) continue;
            string fname;
            if(m_teammode)
            {
                loopj(MAXTEAMS)
                { 
                    formatstring(fname)("%s/%s", mdl.hudguns[1+j], file);
                    preloadmodel(fname);
                }
            }
            else
            {
                formatstring(fname)("%s/%s", mdl.hudguns[0], file);
                preloadmodel(fname);
            }
            formatstring(fname)("vwep/%s", file);
            preloadmodel(fname);
        }
    }

    void preloadsounds()
    {
        for(int i = S_JUMP; i <= S_SPLASH2; i++) preloadsound(i);
        for(int i = S_JUMPPAD; i <= S_PISTOL; i++) preloadsound(i);
        for(int i = S_BURN; i <= S_HIT; i++) preloadsound(i);
    }

    void preload()
    {
        if(hudgun) preloadweapons();
        preloadbouncers();
        preloadplayermodel();
        preloadsounds();
        entities::preloadentities();
    }

}

