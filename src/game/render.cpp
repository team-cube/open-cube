#include "game.h"

namespace game
{
    vector<gameent *> bestplayers;
    vector<int> bestteams;

    VARP(ragdoll, 0, 1, 1);
    VARP(ragdollmillis, 0, 10000, 300000);
    VARP(ragdollfade, 0, 1000, 300000);
    VARP(forceplayermodels, 0, 0, 1);
    VARP(hidedead, 0, 0, 1);

    extern int playermodel;

    vector<gameent *> ragdolls;

    void saveragdoll(gameent *d)
    {
        if(!d->ragdoll || !ragdollmillis || (!ragdollfade && lastmillis > d->lastpain + ragdollmillis)) return;
        gameent *r = new gameent(*d);
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
            gameent *d = ragdolls[i];
            if(lastmillis > d->lastupdate + ragdollmillis)
            {
                delete ragdolls.remove(i--);
                continue;
            }
            moveragdoll(d);
        }
    }

    static const int playercolors[] =
    {
        0x425E32,
        0x5E1F2A,   
        0xAB3E38,   
        0xB27436,   
        0x735E48,   
        0xABA561,   
        0x468046,   
        0x3B8180,   
        0x657F8A,   
        0x295182,   
        0x915E75,   
        0x5B4573,   
        0x392A54,   
        0x555453,   
        0x737577,   
        0xBFB8AC
    };

    static const int playercolorsazul[] =
    {
        0x2A4778,
        0x407187,
        0x393973,
        0x5D6EC6
    };

    static const int playercolorsrojo[] =
    {
        0xAC2C2A,
        0x7E211B,   
        0x772235,   
        0xA3435B
    };   

    extern void changedplayercolor();
    VARFP(playercolor, 0, 0, sizeof(playercolors)/sizeof(playercolors[0])-1, changedplayercolor());
    VARFP(playercolorazul, 0, 0, sizeof(playercolorsazul)/sizeof(playercolorsazul[0])-1, changedplayercolor());
    VARFP(playercolorrojo, 0, 0, sizeof(playercolorsrojo)/sizeof(playercolorsrojo[0])-1, changedplayercolor());

    static const playermodelinfo playermodels[] =
    {
        { { "player/bones", "player/bones", "player/bones" }, { "hudgun", "hudgun", "hudgun" }, { "player", "player_azul", "player_rojo" }, true }
    };

    extern void changedplayermodel();
    VARFP(playermodel, 0, 0, sizeof(playermodels)/sizeof(playermodels[0])-1, changedplayermodel());

    int chooserandomplayermodel(int seed)
    {
        return (seed&0xFFFF)%(sizeof(playermodels)/sizeof(playermodels[0]));
    }

    const playermodelinfo *getplayermodelinfo(int n)
    {
        if(size_t(n) >= sizeof(playermodels)/sizeof(playermodels[0])) return NULL;
        return &playermodels[n];
    }

    const playermodelinfo &getplayermodelinfo(gameent *d)
    {
        const playermodelinfo *mdl = getplayermodelinfo(d==player1 || forceplayermodels ? playermodel : d->playermodel);
        if(!mdl) mdl = getplayermodelinfo(playermodel);
        return *mdl;
    }

    int getplayercolor(int team, int color)
    {
        #define GETPLAYERCOLOR(name) \
            return playercolors##name[color%(sizeof(playercolors##name)/sizeof(playercolors##name[0]))];
        switch(team)
        {
            case 1: GETPLAYERCOLOR(azul)
            case 2: GETPLAYERCOLOR(rojo)
            default: GETPLAYERCOLOR()
        }
    }

    HVAR(testplayercolor, 0, 0, 0xFFFFFF);

    int getplayercolor(gameent *d, int team)
    {
        if(testplayercolor) return testplayercolor;
        else if(d==player1) switch(team)
        {
            case 1: return getplayercolor(1, playercolorazul);
            case 2: return getplayercolor(2, playercolorrojo);
            default: return getplayercolor(0, playercolor);
        }
        else return getplayercolor(team, (d->playercolor>>(8*team))&0xFF);
    }

    void changedplayermodel()
    {
        if(player1->clientnum < 0) player1->playermodel = playermodel;
        if(player1->ragdoll) cleanragdoll(player1);
        loopv(ragdolls)
        {
            gameent *d = ragdolls[i];
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
            gameent *d = players[i];
            if(d == player1 || !d->ragdoll) continue;
            if(!forceplayermodels)
            {
                const playermodelinfo *mdl = getplayermodelinfo(d->playermodel);
                if(mdl) continue;
            }
            cleanragdoll(d);
        }
    }

    void changedplayercolor()
    {
        if(player1->clientnum < 0) player1->playercolor = playercolor | (playercolorazul<<8) | (playercolorrojo<<16);
    }

    void syncplayer()
    {
        if(player1->playermodel != playermodel)
        {
            player1->playermodel = playermodel;
            addmsg(N_SWITCHMODEL, "ri", player1->playermodel);
        }

        int col = playercolor | (playercolorazul<<8) | (playercolorrojo<<16);
        if(player1->playercolor != col)
        {
            player1->playercolor = col;
            addmsg(N_SWITCHCOLOR, "ri", player1->playercolor);
        }
    }

    void preloadplayermodel()
    {
        loopi(sizeof(playermodels)/sizeof(playermodels[0]))
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

    int numanims() { return NUMANIMS; }

    void findanims(const char *pattern, vector<int> &anims)
    {
        loopi(sizeof(animnames)/sizeof(animnames[0])) if(matchanim(animnames[i], pattern)) anims.add(i);
    }

    VAR(animoverride, -1, 0, NUMANIMS-1);
    VAR(testanims, 0, 0, 1);
    VAR(testpitch, -90, 0, 90);

    void renderplayer(gameent *d, const playermodelinfo &mdl, int color, int team, float fade, bool mainpass = true)
    {
        int lastaction = d->lastaction, hold = d->gunselect != GUN_MELEE ? (ANIM_HOLD1+d->gunselect)|ANIM_LOOP : 0, attack = ANIM_ATTACK1+d->gunselect, delay = guns[d->gunselect].attackdelay+50;
        if(intermission && d->state!=CS_DEAD)
        {
            hold = attack = ANIM_LOSE|ANIM_LOOP;
            if(validteam(team) ? bestteams.htfind(team)>=0 : bestplayers.find(d)>=0) hold = attack = ANIM_WIN|ANIM_LOOP;
        }
        else if(d->state==CS_ALIVE && d->lasttaunt && lastmillis-d->lasttaunt<1000 && lastmillis-d->lastaction>delay)
        {
            lastaction = d->lasttaunt;
            hold = attack = ANIM_TAUNT;
            delay = 1000;
        }
        modelattach a[5];
        static const char * const vweps[] = {"worldgun/melee", "worldgun/rifle", "worldgun/rocket"};
        int ai = 0;
        if(vweps[d->gunselect])
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
        int anim = hold ? hold : ANIM_IDLE|ANIM_LOOP;
        float yaw = testanims && d==player1 ? 0 : d->yaw,
              pitch = testpitch && d==player1 ? testpitch : d->pitch;
        vec o = d->feetpos();
        int basetime = 0;
        if(animoverride) anim = (animoverride<0 ? ANIM_ALL : animoverride)|ANIM_LOOP;
        else if(d->state==CS_DEAD)
        {
            anim = ANIM_DYING|ANIM_NOPITCH;
            basetime = d->lastpain;
            if(ragdoll && mdl.ragdoll) anim |= ANIM_RAGDOLL;
            else if(lastmillis-basetime>1000) anim = ANIM_DEAD|ANIM_LOOP|ANIM_NOPITCH;
        }
        else if(d->state==CS_EDITING || d->state==CS_SPECTATOR) anim = ANIM_EDIT|ANIM_LOOP;
        else if(d->state==CS_LAGGED)                            anim = ANIM_LAG|ANIM_LOOP;
        else if(!intermission)
        {
            if(lastmillis-d->lastpain < 300)
            {
                anim = ANIM_PAIN;
                basetime = d->lastpain;
            }
            else if(d->lastpain < lastaction && (attack < 0 || lastmillis-lastaction < delay))
            {
                anim = attack < 0 ? -attack : attack;
                basetime = lastaction;
            }

            if(d->inwater && d->physstate<=PHYS_FALL) anim |= (((game::allowmove(d) && (d->move || d->strafe)) || d->vel.z+d->falling.z>0 ? ANIM_SWIM : ANIM_SINK)|ANIM_LOOP)<<ANIM_SECONDARY;
            else
            {
                static const int dirs[9] =
                {
                    ANIM_RUN_SE, ANIM_RUN_S, ANIM_RUN_SW,
                    ANIM_RUN_E,  0,          ANIM_RUN_W,
                    ANIM_RUN_NE, ANIM_RUN_N, ANIM_RUN_NW
                };
                int dir = dirs[(d->move+1)*3 + (d->strafe+1)];
                if(d->timeinair>100) anim |= ((dir ? dir+ANIM_JUMP_N-ANIM_RUN_N : ANIM_JUMP) | ANIM_END) << ANIM_SECONDARY;
                else if(dir && game::allowmove(d)) anim |= (dir | ANIM_LOOP) << ANIM_SECONDARY;
            }

            if(d->crouching) switch((anim>>ANIM_SECONDARY)&ANIM_INDEX)
            {
                case ANIM_IDLE: anim &= ~(ANIM_INDEX<<ANIM_SECONDARY); anim |= ANIM_CROUCH<<ANIM_SECONDARY; break;
                case ANIM_JUMP: anim &= ~(ANIM_INDEX<<ANIM_SECONDARY); anim |= ANIM_CROUCH_JUMP<<ANIM_SECONDARY; break;
                case ANIM_SWIM: anim &= ~(ANIM_INDEX<<ANIM_SECONDARY); anim |= ANIM_CROUCH_SWIM<<ANIM_SECONDARY; break;
                case ANIM_SINK: anim &= ~(ANIM_INDEX<<ANIM_SECONDARY); anim |= ANIM_CROUCH_SINK<<ANIM_SECONDARY; break;
                case 0: anim |= (ANIM_CROUCH|ANIM_LOOP)<<ANIM_SECONDARY; break;
                case ANIM_RUN_N: case ANIM_RUN_NE: case ANIM_RUN_E: case ANIM_RUN_SE: case ANIM_RUN_S: case ANIM_RUN_SW: case ANIM_RUN_W: case ANIM_RUN_NW:
                    anim += (ANIM_CROUCH_N - ANIM_RUN_N) << ANIM_SECONDARY;
                    break;
                case ANIM_JUMP_N: case ANIM_JUMP_NE: case ANIM_JUMP_E: case ANIM_JUMP_SE: case ANIM_JUMP_S: case ANIM_JUMP_SW: case ANIM_JUMP_W: case ANIM_JUMP_NW:
                    anim += (ANIM_CROUCH_JUMP_N - ANIM_JUMP_N) << ANIM_SECONDARY;
                    break;
            }

            if((anim&ANIM_INDEX)==ANIM_IDLE && (anim>>ANIM_SECONDARY)&ANIM_INDEX) anim >>= ANIM_SECONDARY;
        }
        if(!((anim>>ANIM_SECONDARY)&ANIM_INDEX)) anim |= (ANIM_IDLE|ANIM_LOOP)<<ANIM_SECONDARY;
        int flags = 0;
        if(d!=player1) flags |= MDL_CULL_VFC | MDL_CULL_OCCLUDED | MDL_CULL_QUERY;
        if(d->type==ENT_PLAYER) flags |= MDL_FULLBRIGHT;
        else flags |= MDL_CULL_DIST;
        if(!mainpass) flags &= ~(MDL_FULLBRIGHT | MDL_CULL_VFC | MDL_CULL_OCCLUDED | MDL_CULL_QUERY | MDL_CULL_DIST);
        float trans = d->state == CS_LAGGED ? 0.3f : 1.0f;
        rendermodel(mdlname, anim, o, yaw, pitch, 0, flags, d, a[0].tag ? a : NULL, basetime, 0, fade, vec4(vec::hexcolor(color), trans));
    }

    static inline void renderplayer(gameent *d, float fade = 1)
    {
        int team = m_teammode && validteam(d->team) ? d->team : 0;
        renderplayer(d, getplayermodelinfo(d), getplayercolor(d, team), team, fade);
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

        gameent *exclude = isthirdperson() ? NULL : followingplayer();
        loopv(players)
        {
            gameent *d = players[i];
            if(d == player1 || d->state==CS_SPECTATOR || d->state==CS_SPAWNING || d->lifesequence < 0 || d == exclude || (d->state==CS_DEAD && hidedead)) continue;
            renderplayer(d);
            copystring(d->info, colorname(d));
            if(d->state!=CS_DEAD)
            {
                int team = m_teammode && validteam(d->team) ? d->team : 0;
                particle_text(d->abovehead(), d->info, PART_TEXT, 1, teamtextcolor[team], 2.0f);
            }
        }
        loopv(ragdolls)
        {
            gameent *d = ragdolls[i];
            float fade = 1.0f;
            if(ragdollmillis && ragdollfade)
                fade -= clamp(float(lastmillis - (d->lastupdate + max(ragdollmillis - ragdollfade, 0)))/min(ragdollmillis, ragdollfade), 0.0f, 1.0f);
            renderplayer(d, fade);
        }
        if(isthirdperson() && !followingplayer() && (player1->state!=CS_DEAD || !hidedead))
            renderplayer(player1);
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
        gameent *d = hudplayer();
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

    void drawhudmodel(gameent *d, int anim, float speed = 0, int base = 0)
    {
        vec sway;
        vecfromyawpitch(d->yaw, 0, 0, 1, sway);
        float steps = swaydist/swaystep*M_PI;
        sway.mul(swayside*cosf(steps));
        sway.z = swayup*(fabs(sinf(steps)) - 1);
        sway.add(swaydir).add(d->o);
        if(!hudgunsway) sway = d->o;

        const playermodelinfo &mdl = getplayermodelinfo(d);
        defformatstring(gunname, "%s/%s", mdl.hudguns[m_teammode && validteam(d->team) ? d->team : 0], guns[d->gunselect].file);
        modelattach a[2];
        d->muzzle = vec(-1, -1, -1);
        a[0] = modelattach("tag_muzzle", &d->muzzle);
        rendermodel(gunname, anim, sway, d->yaw, d->pitch, 0, MDL_NOBATCH, NULL, a, base, (int)ceil(speed));
        if(d->muzzle.x >= 0) d->muzzle = calcavatarpos(d->muzzle, 12);
    }

    void drawhudgun()
    {
        gameent *d = hudplayer();
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

    void renderplayerpreview(int model, int color, int team, int weap)
    {
        static gameent *previewent = NULL;
        if(!previewent)
        {
            previewent = new gameent;
            previewent->o = vec(0, 0.9f*(previewent->eyeheight + previewent->aboveeye), previewent->eyeheight - (previewent->eyeheight + previewent->aboveeye)/2);
            loopi(NUMGUNS-1) previewent->ammo[i+1] = 1;
        }
        previewent->gunselect = validgun(weap) ? weap : GUN_MELEE;
        previewent->yaw = fmod(lastmillis/10000.0f*360.0f, 360.0f);
        const playermodelinfo *mdlinfo = getplayermodelinfo(model);
        if(!mdlinfo) return;
        renderplayer(previewent, *mdlinfo, getplayercolor(team, color), team, 1, false);
    }

    vec hudgunorigin(int gun, const vec &from, const vec &to, gameent *d)
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
                    formatstring(fname, "%s/%s", mdl.hudguns[1+j], file);
                    preloadmodel(fname);
                }
            }
            else
            {
                formatstring(fname, "%s/%s", mdl.hudguns[0], file);
                preloadmodel(fname);
            }
            formatstring(fname, "worldgun/%s", file);
            preloadmodel(fname);
        }
    }

    void preloadsounds()
    {
        for(int i = S_JUMP; i <= S_DIE2; i++) preloadsound(i);
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

