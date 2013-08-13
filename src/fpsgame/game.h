#ifndef __GAME_H__
#define __GAME_H__

#include "cube.h"

// animations

enum
{
    ANIM_DEAD = ANIM_GAMESPECIFIC, ANIM_DYING, ANIM_IDLE,
    ANIM_FORWARD, ANIM_BACKWARD, ANIM_LEFT, ANIM_RIGHT,
    ANIM_CROUCH, ANIM_CROUCH_FORWARD, ANIM_CROUCH_BACKWARD, ANIM_CROUCH_LEFT, ANIM_CROUCH_RIGHT,

    ANIM_HOLD1, ANIM_HOLD2, ANIM_HOLD3,
    ANIM_ATTACK1, ANIM_ATTACK2, ANIM_ATTACK3,
    ANIM_PAIN,
    ANIM_JUMP, ANIM_SINK, ANIM_SWIM,
    ANIM_CROUCH_JUMP, ANIM_CROUCH_SINK, ANIM_CROUCH_SWIM,
    ANIM_EDIT, ANIM_LAG, ANIM_TAUNT, ANIM_WIN, ANIM_LOSE,
    ANIM_GUN_IDLE, ANIM_GUN_SHOOT,
    ANIM_VWEP_IDLE, ANIM_VWEP_SHOOT,
    NUMANIMS
};

static const char * const animnames[] =
{
    "mapmodel",
    "dead", "dying", "idle",
    "forward", "backward", "left", "right",
    "crouch", "crouch forward", "crouch backward", "crouch left", "crouch right",
    "hold 1", "hold 2", "hold 3",
    "attack 1", "attack 2", "attack 3",
    "pain",
    "jump", "sink", "swim",
    "crouch jump", "crouch sink", "crouch swim",
    "edit", "lag", "taunt", "win", "lose",
    "gun idle", "gun shoot",
    "vwep idle", "vwep shoot"
};

// console message types

enum
{
    CON_CHAT       = 1<<8,
    CON_TEAMCHAT   = 1<<9,
    CON_GAMEINFO   = 1<<10,
    CON_FRAG_SELF  = 1<<11,
    CON_FRAG_OTHER = 1<<12,
    CON_TEAMKILL   = 1<<13
};

// network quantization scale
#define DMF 16.0f                // for world locations
#define DNF 100.0f              // for normalized vectors
#define DVELF 1.0f              // for playerspeed based velocity vectors

enum                            // static entity types
{
    NOTUSED = ET_EMPTY,         // entity slot not in use in map
    LIGHT = ET_LIGHT,           // lightsource, attr1 = radius, attr2 = intensity
    MAPMODEL = ET_MAPMODEL,     // attr1 = idx, attr2 = yaw, attr3 = pitch, attr4 = roll, attr5 = scale
    PLAYERSTART,                // attr1 = angle, attr2 = team
    ENVMAP = ET_ENVMAP,         // attr1 = radius
    PARTICLES = ET_PARTICLES,
    MAPSOUND = ET_SOUND,
    SPOTLIGHT = ET_SPOTLIGHT,
    TELEPORT,                   // attr1 = idx, attr2 = model, attr3 = tag
    TELEDEST,                   // attr1 = angle, attr2 = idx
    JUMPPAD,                    // attr1 = zpush, attr2 = ypush, attr3 = xpush
    FLAG,                       // attr1 = angle, attr2 = team
    MAXENTTYPES,

    I_FIRST = 0,
    I_LAST = -1
};

struct gameentity : extentity
{
};

enum { GUN_MELEE = 0, GUN_ROCKET, GUN_RIFLE, NUMGUNS };

#define validgun(n) ((n) >= 0 && (n) < NUMGUNS)

enum
{
    M_TEAM       = 1<<0,
    M_CTF        = 1<<1,
    M_OVERTIME   = 1<<2,
    M_EDIT       = 1<<3,
    M_DEMO       = 1<<4,
    M_LOCAL      = 1<<5,
    M_LOBBY      = 1<<6
};

static struct gamemodeinfo
{
    const char *name;
    int flags;
    const char *info;
} gamemodes[] =
{
    { "demo", M_DEMO | M_LOCAL, NULL},
    { "coop edit", M_EDIT, "Cooperative Editing: Edit maps with multiple players simultaneously." },
    { "ffa", M_LOBBY, "Free For All: Collect items for ammo. Frag everyone to score points." },
    { "teamplay", M_TEAM, "Teamplay: Collect items for ammo. Frag \fs\f3the enemy team\fr to score points for \fs\f1your team\fr." },
    { "ctf", M_CTF | M_TEAM, "Capture The Flag: Capture \fs\f3the enemy flag\fr and bring it back to \fs\f1your flag\fr to score points for \fs\f1your team\fr. Collect items for ammo." }
};

#define STARTGAMEMODE (-1)
#define NUMGAMEMODES ((int)(sizeof(gamemodes)/sizeof(gamemodes[0])))

#define m_valid(mode)          ((mode) >= STARTGAMEMODE && (mode) < STARTGAMEMODE + NUMGAMEMODES)
#define m_check(mode, flag)    (m_valid(mode) && gamemodes[(mode) - STARTGAMEMODE].flags&(flag))
#define m_checknot(mode, flag) (m_valid(mode) && !(gamemodes[(mode) - STARTGAMEMODE].flags&(flag)))
#define m_checkall(mode, flag) (m_valid(mode) && (gamemodes[(mode) - STARTGAMEMODE].flags&(flag)) == (flag))

#define m_ctf          (m_check(gamemode, M_CTF))
#define m_teammode     (m_check(gamemode, M_TEAM))
#define m_overtime     (m_check(gamemode, M_OVERTIME))
#define isteam(a,b)    (m_teammode && a==b)

#define m_demo         (m_check(gamemode, M_DEMO))
#define m_edit         (m_check(gamemode, M_EDIT))
#define m_lobby        (m_check(gamemode, M_LOBBY))
#define m_timed        (m_checknot(gamemode, M_DEMO|M_EDIT|M_LOCAL))
#define m_botmode      (m_checknot(gamemode, M_DEMO|M_LOCAL))
#define m_mp(mode)     (m_checknot(mode, M_LOCAL))

enum { MM_AUTH = -1, MM_OPEN = 0, MM_VETO, MM_LOCKED, MM_PRIVATE, MM_PASSWORD, MM_START = MM_AUTH, MM_INVALID = MM_START - 1 };

static const char * const mastermodenames[] =  { "auth",   "open",   "veto",       "locked",     "private",    "password" };
static const char * const mastermodecolors[] = { "",       "\f0",    "\f2",        "\f2",        "\f3",        "\f3" };
static const char * const mastermodeicons[] =  { "server", "server", "serverlock", "serverlock", "serverpriv", "serverpriv" };

// hardcoded sounds, defined in sounds.cfg
enum
{
    S_JUMP = 0, S_LAND,
    S_SPLASHIN, S_SPLASHOUT, S_BURN,
    S_ITEMSPAWN, S_TELEPORT, S_JUMPPAD,
    S_MELEE, S_ROCKET, S_ROCKETEXPLODE, S_RIFLE, 
    S_WEAPLOAD, S_NOAMMO, S_HIT,
    S_PAIN1, S_PAIN2, S_DIE1, S_DIE2,

    S_FLAGPICKUP,
    S_FLAGDROP,
    S_FLAGRETURN,
    S_FLAGSCORE,
    S_FLAGRESET,
    S_FLAGFAIL
};

// network messages codes, c2s, c2c, s2c

enum { PRIV_NONE = 0, PRIV_MASTER, PRIV_AUTH, PRIV_ADMIN };

enum
{
    N_CONNECT = 0, N_SERVINFO, N_WELCOME, N_INITCLIENT, N_POS, N_TEXT, N_SOUND, N_CDIS,
    N_SHOOT, N_EXPLODE, N_SUICIDE,
    N_DIED, N_DAMAGE, N_HITPUSH, N_SHOTFX, N_EXPLODEFX,
    N_TRYSPAWN, N_SPAWNSTATE, N_SPAWN, N_FORCEDEATH,
    N_GUNSELECT, N_TAUNT,
    N_MAPCHANGE, N_MAPVOTE, N_TEAMINFO, N_ITEMSPAWN, N_ITEMPICKUP, N_ITEMACC, N_TELEPORT, N_JUMPPAD,
    N_PING, N_PONG, N_CLIENTPING,
    N_TIMEUP, N_FORCEINTERMISSION,
    N_SERVMSG, N_ITEMLIST, N_RESUME,
    N_EDITMODE, N_EDITENT, N_EDITF, N_EDITT, N_EDITM, N_FLIP, N_COPY, N_PASTE, N_ROTATE, N_REPLACE, N_DELCUBE, N_REMIP, N_NEWMAP, N_GETMAP, N_SENDMAP, N_CLIPBOARD, N_EDITVAR,
    N_MASTERMODE, N_KICK, N_CLEARBANS, N_CURRENTMASTER, N_SPECTATOR, N_SETMASTER, N_SETTEAM,
    N_LISTDEMOS, N_SENDDEMOLIST, N_GETDEMO, N_SENDDEMO,
    N_DEMOPLAYBACK, N_RECORDDEMO, N_STOPDEMO, N_CLEARDEMOS,
    N_TAKEFLAG, N_RETURNFLAG, N_RESETFLAG, N_TRYDROPFLAG, N_DROPFLAG, N_SCOREFLAG, N_INITFLAGS,
    N_SAYTEAM,
    N_CLIENT,
    N_AUTHTRY, N_AUTHKICK, N_AUTHCHAL, N_AUTHANS, N_REQAUTH,
    N_PAUSEGAME, N_GAMESPEED,
    N_ADDBOT, N_DELBOT, N_INITAI, N_FROMAI, N_BOTLIMIT, N_BOTBALANCE,
    N_MAPCRC, N_CHECKMAPS,
    N_SWITCHNAME, N_SWITCHMODEL, N_SWITCHTEAM,
    N_SERVCMD,
    N_DEMOPACKET,
    NUMMSG
};

static const int msgsizes[] =               // size inclusive message token, 0 for variable or not-checked sizes
{
    N_CONNECT, 0, N_SERVINFO, 0, N_WELCOME, 1, N_INITCLIENT, 0, N_POS, 0, N_TEXT, 0, N_SOUND, 2, N_CDIS, 2,
    N_SHOOT, 0, N_EXPLODE, 0, N_SUICIDE, 1,
    N_DIED, 5, N_DAMAGE, 5, N_HITPUSH, 7, N_SHOTFX, 10, N_EXPLODEFX, 4,
    N_TRYSPAWN, 1, N_SPAWNSTATE, 8, N_SPAWN, 3, N_FORCEDEATH, 2,
    N_GUNSELECT, 2, N_TAUNT, 1,
    N_MAPCHANGE, 0, N_MAPVOTE, 0, N_TEAMINFO, 0, N_ITEMSPAWN, 2, N_ITEMPICKUP, 2, N_ITEMACC, 3,
    N_PING, 2, N_PONG, 2, N_CLIENTPING, 2,
    N_TIMEUP, 2, N_FORCEINTERMISSION, 1,
    N_SERVMSG, 0, N_ITEMLIST, 0, N_RESUME, 0,
    N_EDITMODE, 2, N_EDITENT, 11, N_EDITF, 16, N_EDITT, 16, N_EDITM, 16, N_FLIP, 14, N_COPY, 14, N_PASTE, 14, N_ROTATE, 15, N_REPLACE, 17, N_DELCUBE, 14, N_REMIP, 1, N_NEWMAP, 2, N_GETMAP, 1, N_SENDMAP, 0, N_EDITVAR, 0,
    N_MASTERMODE, 2, N_KICK, 0, N_CLEARBANS, 1, N_CURRENTMASTER, 0, N_SPECTATOR, 3, N_SETMASTER, 0, N_SETTEAM, 0,
    N_LISTDEMOS, 1, N_SENDDEMOLIST, 0, N_GETDEMO, 2, N_SENDDEMO, 0,
    N_DEMOPLAYBACK, 3, N_RECORDDEMO, 2, N_STOPDEMO, 1, N_CLEARDEMOS, 2,
    N_TAKEFLAG, 3, N_RETURNFLAG, 4, N_RESETFLAG, 3, N_TRYDROPFLAG, 1, N_DROPFLAG, 7, N_SCOREFLAG, 9, N_INITFLAGS, 0,
    N_SAYTEAM, 0,
    N_CLIENT, 0,
    N_AUTHTRY, 0, N_AUTHKICK, 0, N_AUTHCHAL, 0, N_AUTHANS, 0, N_REQAUTH, 0,
    N_PAUSEGAME, 0, N_GAMESPEED, 0,
    N_ADDBOT, 2, N_DELBOT, 1, N_INITAI, 0, N_FROMAI, 2, N_BOTLIMIT, 2, N_BOTBALANCE, 2,
    N_MAPCRC, 0, N_CHECKMAPS, 1,
    N_SWITCHNAME, 0, N_SWITCHMODEL, 2, N_SWITCHTEAM, 0,
    N_SERVCMD, 0,
    N_DEMOPACKET, 0,
    -1
};

#define TESSERACT_SERVER_PORT 42000
#define TESSERACT_LANINFO_PORT 41998
#define TESSERACT_MASTER_PORT 41999
#define PROTOCOL_VERSION 1              // bump when protocol changes
#define DEMO_VERSION 1                  // bump when demo format changes
#define DEMO_MAGIC "TESSERACT_DEMO\0\0"

struct demoheader
{
    char magic[16];
    int version, protocol;
};

#define MAXNAMELEN 15

enum
{
    HICON_RED_FLAG = 0,
    HICON_BLUE_FLAG,

    HICON_X       = 20,
    HICON_Y       = 1650,
    HICON_TEXTY   = 1644,
    HICON_STEP    = 490,
    HICON_SIZE    = 120,
    HICON_SPACE   = 40
};

#if 0
static struct itemstat { int add, max, sound; const char *name; int icon, info; } itemstats[] =
{
};
#endif

#define validitem(n) false

#define MAXRAYS 4
#define EXP_SELFDAMDIV 2
#define EXP_SELFPUSH 2.5f
#define EXP_DISTSCALE 1

static const struct guninfo { int sound, attackdelay, damage, spread, projspeed, kickamount, range, rays, hitpush, exprad, ttl, use; const char *name, *file; } guns[NUMGUNS] =
{
    { S_MELEE,   250,   1,   0,   0,  0,   14,  1,   0,  0,    0, 0, "melee",  "melee"},
    { S_ROCKET,  800,   1,   0, 480, 10, 1024,  1,5000, 10,    0, 0, "rocket", "rocket"},
    { S_RIFLE,   800,   1,   0,   0, 30, 2048,  1,5000,  0,    0, 0, "rifle",  "rifle" }
};

#include "ai.h"

// inherited by gameent and server clients
struct gamestate
{
    int health, maxhealth;
    int gunselect, gunwait;
    int ammo[NUMGUNS];
    int aitype, skill;

    gamestate() : maxhealth(1), aitype(AI_NONE), skill(0) {}

    bool canpickup(int type)
    {
        return validitem(type);
    }

    void pickup(int type)
    {
    }

    void respawn()
    {
        health = maxhealth;
        gunselect = GUN_MELEE;
        gunwait = 0;
        loopi(NUMGUNS) ammo[i] = 0;
        ammo[GUN_MELEE] = 1;
    }

    void spawnstate(int gamemode)
    {
        if(m_demo)
        {
            gunselect = GUN_MELEE;
        }
        else
        {
            gunselect = GUN_RIFLE;
            ammo[GUN_RIFLE] = 1;
            ammo[GUN_ROCKET] = 1;
        }
    }

    // just subtract damage here, can set death, etc. later in code calling this
    int dodamage(int damage)
    {
        health -= damage;
        return damage;
    }

    int hasammo(int gun, int exclude = -1)
    {
        return validgun(gun) && gun != exclude && ammo[gun] > 0;
    }
};

#define MAXTEAMS 2
static const char * const teamnames[1+MAXTEAMS] = { "", "azul", "rojo" };
static const char * const teamtextcode[1+MAXTEAMS] = { "\f0", "\f1", "\f3" };
static const int teamtextcolor[1+MAXTEAMS] = { 0x1EC850, 0x6496FF, 0xFF4B19 };
static const int teamscoreboardcolor[1+MAXTEAMS] = { 0, 0x3030C0, 0xC03030 };
static const char * const teamblipcolor[1+MAXTEAMS] = { "_neutral", "_blue", "_red" };
static inline int teamnumber(const char *name) { loopi(MAXTEAMS) if(!strcmp(teamnames[1+i], name)) return 1+i; return 0; }
#define validteam(n) ((n) >= 1 && (n) <= MAXTEAMS)
#define teamname(n) (teamnames[validteam(n) ? (n) : 0])

struct gameent : dynent, gamestate
{
    int weight;                         // affects the effectiveness of hitpush
    int clientnum, privilege, lastupdate, plag, ping;
    int lifesequence;                   // sequence id for each respawn, used in damage test
    int respawned, suicided;
    int lastpain;
    int lastaction, lastattackgun;
    bool attacking;
    int lasttaunt;
    int lastpickup, lastpickupmillis, flagpickup;
    int frags, flags, deaths, totaldamage, totalshots;
    editinfo *edit;
    float deltayaw, deltapitch, deltaroll, newyaw, newpitch, newroll;
    int smoothmillis;

    string name, info;
    int team, playermodel;
    ai::aiinfo *ai;
    int ownernum, lastnode;

    vec muzzle;

    gameent() : weight(100), clientnum(-1), privilege(PRIV_NONE), lastupdate(0), plag(0), ping(0), lifesequence(0), respawned(-1), suicided(-1), lastpain(0), frags(0), flags(0), deaths(0), totaldamage(0), totalshots(0), edit(NULL), smoothmillis(-1), team(0), playermodel(-1), ai(NULL), ownernum(-1), muzzle(-1, -1, -1)
    {
        name[0] = info[0] = 0;
        respawn();
    }
    ~gameent()
    {
        freeeditinfo(edit);
        if(ai) delete ai;
    }

    void hitpush(int damage, const vec &dir, gameent *actor, int gun)
    {
        vec push(dir);
        push.mul((actor==this && guns[gun].exprad ? EXP_SELFPUSH : 1.0f)*guns[gun].hitpush*damage/weight);
        vel.add(push);
    }

    void respawn()
    {
        dynent::reset();
        gamestate::respawn();
        respawned = suicided = -1;
        lastaction = 0;
        lastattackgun = gunselect;
        attacking = false;
        lasttaunt = 0;
        lastpickup = -1;
        lastpickupmillis = 0;
        flagpickup = 0;
        lastnode = -1;
    }
};

struct teamscore
{
    int team, score;
    teamscore() {}
    teamscore(int team, int n) : team(team), score(n) {}

    static bool compare(const teamscore &x, const teamscore &y)
    {
        if(x.score > y.score) return true;
        if(x.score < y.score) return false;
        return x.team < y.team;
    }
};

static inline uint hthash(const teamscore &t) { return hthash(t.team); }
static inline bool htcmp(int team, const teamscore &t) { return team == t.team; }

struct teaminfo
{
    int frags;

    teaminfo() { reset(); }

    void reset() { frags = 0; }
};

namespace entities
{
    extern vector<extentity *> ents;

    extern const char *entmdlname(int type);
    extern const char *itemname(int i);
    extern int itemicon(int i);

    extern void preloadentities();
    extern void renderentities();
    extern void checkitems(gameent *d);
    extern void resetspawns();
    extern void spawnitems(bool force = false);
    extern void putitems(packetbuf &p);
    extern void setspawn(int i, bool on);
    extern void teleport(int n, gameent *d);
    extern void pickupeffects(int n, gameent *d);
    extern void teleporteffects(gameent *d, int tp, int td, bool local = true);
    extern void jumppadeffects(gameent *d, int jp, bool local = true);
}

namespace game
{
    extern int gamemode;

    struct clientmode
    {
        virtual ~clientmode() {}

        virtual void preload() {}
        virtual float clipconsole(float w, float h) { return 0; }
        virtual void drawhud(gameent *d, int w, int h) {}
        virtual void rendergame() {}
        virtual void respawned(gameent *d) {}
        virtual void setup() {}
        virtual void checkitems(gameent *d) {}
        virtual int respawnwait(gameent *d) { return 0; }
        virtual void pickspawn(gameent *d) { findplayerspawn(d, -1, m_teammode ? d->team : 0); }
        virtual void senditems(packetbuf &p) {}
        virtual void removeplayer(gameent *d) {}
        virtual void gameover() {}
        virtual bool hidefrags() { return false; }
        virtual int getteamscore(int team) { return 0; }
        virtual void getteamscores(vector<teamscore> &scores) {}
        virtual void aifind(gameent *d, ai::aistate &b, vector<ai::interest> &interests) {}
        virtual bool aicheck(gameent *d, ai::aistate &b) { return false; }
        virtual bool aidefend(gameent *d, ai::aistate &b) { return false; }
        virtual bool aipursue(gameent *d, ai::aistate &b) { return false; }
    };

    extern clientmode *cmode;
    extern void setclientmode();

    // game
    extern int nextmode;
    extern string clientmap;
    extern bool intermission;
    extern int maptime, maprealtime, maplimit;
    extern gameent *player1;
    extern vector<gameent *> players, clients;
    extern int lastspawnattempt;
    extern int lasthit;
    extern int following;
    extern int smoothmove, smoothdist;

    extern bool clientoption(const char *arg);
    extern gameent *getclient(int cn);
    extern gameent *newclient(int cn);
    extern const char *colorname(gameent *d, const char *name = NULL, const char *alt = NULL, const char *color = "");
    extern const char *teamcolorname(gameent *d, const char *alt = "you");
    extern const char *teamcolor(const char *prefix, const char *suffix, int team, const char *alt);
    extern gameent *pointatplayer();
    extern gameent *hudplayer();
    extern gameent *followingplayer();
    extern void stopfollowing();
    extern void clientdisconnected(int cn, bool notify = true);
    extern void clearclients(bool notify = true);
    extern void startgame();
    extern void spawnplayer(gameent *);
    extern void deathstate(gameent *d, bool restore = false);
    extern void damaged(int damage, gameent *d, gameent *actor, bool local = true);
    extern void killed(gameent *d, gameent *actor);
    extern void timeupdate(int timeremain);
    extern void msgsound(int n, physent *d = NULL);
    extern void drawicon(int icon, float x, float y, float sz = 120);
    const char *mastermodecolor(int n, const char *unknown);
    const char *mastermodeicon(int n, const char *unknown);

    // client
    extern bool connected, remote, demoplayback;
    extern string servdesc;

    extern int parseplayer(const char *arg);
    extern void ignore(int cn);
    extern void unignore(int cn);
    extern bool isignored(int cn);
    extern void addmsg(int type, const char *fmt = NULL, ...);
    extern void switchname(const char *name);
    extern void switchteam(const char *name);
    extern void switchplayermodel(int playermodel);
    extern void sendmapinfo();
    extern void stopdemo();
    extern void changemap(const char *name, int mode);
    extern void c2sinfo(bool force = false);
    extern void sendposition(gameent *d, bool reliable = false);

    // weapon
    extern int getweapon(const char *name);
    extern void shoot(gameent *d, const vec &targ);
    extern void shoteffects(int gun, const vec &from, const vec &to, gameent *d, bool local, int id, int prevaction);
    extern void explode(bool local, gameent *owner, const vec &v, const vec &vel, dynent *safe, int dam, int gun);
    extern void explodeeffects(int gun, gameent *d, bool local, int id = 0);
    extern void damageeffect(int damage, gameent *d, bool thirdperson = true);
    extern void gibeffect(int damage, const vec &vel, gameent *d);
    extern float intersectdist;
    extern bool intersect(dynent *d, const vec &from, const vec &to, float &dist = intersectdist);
    extern dynent *intersectclosest(const vec &from, const vec &to, gameent *at, float &dist = intersectdist);
    extern void clearbouncers();
    extern void updatebouncers(int curtime);
    extern void removebouncers(gameent *owner);
    extern void renderbouncers();
    extern void clearprojectiles();
    extern void updateprojectiles(int curtime);
    extern void removeprojectiles(gameent *owner);
    extern void renderprojectiles();
    extern void preloadbouncers();
    extern void removeweapons(gameent *owner);
    extern void updateweapons(int curtime);
    extern void gunselect(int gun, gameent *d);
    extern void weaponswitch(gameent *d);
    extern void avoidweapons(ai::avoidset &obstacles, float radius);

    // scoreboard
    extern void showscores(bool on);
    extern void getbestplayers(vector<gameent *> &best);
    extern void getbestteams(vector<int> &best);
    extern void clearteaminfo();
    extern void setteaminfo(int team, int frags);
    extern void removegroupedplayer(gameent *d);

    // render
    struct playermodelinfo
    {
        const char *model[1+MAXTEAMS], *hudguns[1+MAXTEAMS],
                   *icon[1+MAXTEAMS];
        bool ragdoll;
    };

    extern int playermodel, teamskins, testteam;

    extern void saveragdoll(gameent *d);
    extern void clearragdolls();
    extern void moveragdolls();
    extern void changedplayermodel();
    extern const playermodelinfo &getplayermodelinfo(gameent *d);
    extern int chooserandomplayermodel(int seed);
    extern void swayhudgun(int curtime);
    extern vec hudgunorigin(int gun, const vec &from, const vec &to, gameent *d);
}

namespace server
{
    extern const char *modename(int n, const char *unknown = "unknown");
    extern const char *mastermodename(int n, const char *unknown = "unknown");
    extern void startintermission();
    extern void stopdemo();
    extern void forcemap(const char *map, int mode);
    extern void forcepaused(bool paused);
    extern void forcegamespeed(int speed);
    extern void hashpassword(int cn, int sessionid, const char *pwd, char *result, int maxlen = MAXSTRLEN);
    extern int msgsizelookup(int msg);
    extern bool serveroption(const char *arg);
    extern bool delayspawn(int type);
}

#endif

