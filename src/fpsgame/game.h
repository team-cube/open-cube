#ifndef __GAME_H__
#define __GAME_H__

#include "cube.h"

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
    I_SHELLS, I_BULLETS, I_ROCKETS, I_ROUNDS, I_GRENADES, I_CARTRIDGES,
    I_HEALTH,
    I_GREENARMOUR, I_YELLOWARMOUR,
    TELEPORT,                   // attr1 = idx, attr2 = model, attr3 = tag
    TELEDEST,                   // attr1 = angle, attr2 = idx
    JUMPPAD,                    // attr1 = zpush, attr2 = ypush, attr3 = xpush
    FLAG,                       // attr1 = angle, attr2 = team
    MAXENTTYPES
};

struct fpsentity : extentity
{
};

enum { GUN_FIST = 0, GUN_SG, GUN_CG, GUN_RL, GUN_RIFLE, GUN_GL, GUN_PISTOL, NUMGUNS };
enum { A_BLUE, A_GREEN, A_YELLOW };     // armour types... take 20/40/60 % off

enum
{
    M_TEAM       = 1<<0,
    M_NOITEMS    = 1<<1,
    M_NOAMMO     = 1<<2,
    M_INSTA      = 1<<3,
    M_EFFICIENCY = 1<<4,
    M_CTF        = 1<<5,
    M_OVERTIME   = 1<<6,
    M_EDIT       = 1<<7,
    M_DEMO       = 1<<8,
    M_LOCAL      = 1<<9,
    M_LOBBY      = 1<<10,
    M_COLLECT    = 1<<11
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
    { "instagib", M_NOITEMS | M_INSTA, "Instagib: You spawn with full rifle ammo and die instantly from one shot. There are no items. Frag everyone to score points." },
    { "insta team", M_NOITEMS | M_INSTA | M_TEAM, "Instagib Team: You spawn with full rifle ammo and die instantly from one shot. There are no items. Frag \fs\f3the enemy team\fr to score points for \fs\f1your team\fr." },
    { "efficiency", M_NOITEMS | M_EFFICIENCY, "Efficiency: You spawn with all weapons and armour. There are no items. Frag everyone to score points." },
    { "effic team", M_NOITEMS | M_EFFICIENCY | M_TEAM, "Efficiency Team: You spawn with all weapons and armour. There are no items. Frag \fs\f3the enemy team\fr to score points for \fs\f1your team\fr." },
    { "ctf", M_CTF | M_TEAM, "Capture The Flag: Capture \fs\f3the enemy flag\fr and bring it back to \fs\f1your flag\fr to score points for \fs\f1your team\fr. Collect items for ammo." },
    { "insta ctf", M_NOITEMS | M_INSTA | M_CTF | M_TEAM, "Instagib Capture The Flag: Capture \fs\f3the enemy flag\fr and bring it back to \fs\f1your flag\fr to score points for \fs\f1your team\fr. You spawn with full rifle ammo and die instantly from one shot. There are no items." },
    { "effic ctf", M_NOITEMS | M_EFFICIENCY | M_CTF | M_TEAM, "Efficiency Capture The Flag: Capture \fs\f3the enemy flag\fr and bring it back to \fs\f1your flag\fr to score points for \fs\f1your team\fr. You spawn with all weapons and armour. There are no items." },
    { "collect", M_COLLECT | M_TEAM, "Skull Collector: Frag \fs\f3the enemy team\fr to drop \fs\f3skulls\fr. Collect them and bring them to \fs\f3the enemy base\fr to score points for \fs\f1your team\fr or steal back \fs\f1your skulls\fr. Collect items for ammo." },
    { "insta collect", M_NOITEMS | M_INSTA | M_COLLECT | M_TEAM, "Instagib Skull Collector: Frag \fs\f3the enemy team\fr to drop \fs\f3skulls\fr. Collect them and bring them to \fs\f3the enemy base\fr to score points for \fs\f1your team\fr or steal back \fs\f1your skulls\fr. You spawn with full rifle ammo and die instantly from one shot. There are no items." },
    { "effic collect", M_NOITEMS | M_EFFICIENCY | M_COLLECT | M_TEAM, "Efficiency Skull Collector: Frag \fs\f3the enemy team\fr to drop \fs\f3skulls\fr. Collect them and bring them to \fs\f3the enemy base\fr to score points for \fs\f1your team\fr or steal back \fs\f1your skulls\fr. You spawn with all weapons and armour. There are no items." }
};

#define STARTGAMEMODE (-1)
#define NUMGAMEMODES ((int)(sizeof(gamemodes)/sizeof(gamemodes[0])))

#define m_valid(mode)          ((mode) >= STARTGAMEMODE && (mode) < STARTGAMEMODE + NUMGAMEMODES)
#define m_check(mode, flag)    (m_valid(mode) && gamemodes[(mode) - STARTGAMEMODE].flags&(flag))
#define m_checknot(mode, flag) (m_valid(mode) && !(gamemodes[(mode) - STARTGAMEMODE].flags&(flag)))
#define m_checkall(mode, flag) (m_valid(mode) && (gamemodes[(mode) - STARTGAMEMODE].flags&(flag)) == (flag))

#define m_noitems      (m_check(gamemode, M_NOITEMS))
#define m_noammo       (m_check(gamemode, M_NOAMMO|M_NOITEMS))
#define m_insta        (m_check(gamemode, M_INSTA))
#define m_efficiency   (m_check(gamemode, M_EFFICIENCY))
#define m_ctf          (m_check(gamemode, M_CTF))
#define m_collect      (m_check(gamemode, M_COLLECT))
#define m_teammode     (m_check(gamemode, M_TEAM))
#define m_overtime     (m_check(gamemode, M_OVERTIME))
#define isteam(a,b)    (m_teammode && a==b)

#define m_demo         (m_check(gamemode, M_DEMO))
#define m_edit         (m_check(gamemode, M_EDIT))
#define m_lobby        (m_check(gamemode, M_LOBBY))
#define m_timed        (m_checknot(gamemode, M_DEMO|M_EDIT|M_LOCAL))
#define m_botmode      (m_checknot(gamemode, M_DEMO|M_LOCAL))
#define m_mp(mode)     (m_checknot(mode, M_LOCAL))

enum { MM_AUTH = -1, MM_OPEN = 0, MM_VETO, MM_LOCKED, MM_PRIVATE, MM_PASSWORD, MM_START = MM_AUTH };

static const char * const mastermodenames[] =  { "auth",   "open",   "veto",       "locked",     "private",    "password" };
static const char * const mastermodecolors[] = { "",       "\f0",    "\f2",        "\f2",        "\f3",        "\f3" };
static const char * const mastermodeicons[] =  { "server", "server", "serverlock", "serverlock", "serverpriv", "serverpriv" };

// hardcoded sounds, defined in sounds.cfg
enum
{
    S_JUMP = 0, S_LAND, S_RIFLE, S_PUNCH1, S_SG, S_CG,
    S_RLFIRE, S_RLHIT, S_WEAPLOAD, S_ITEMAMMO, S_ITEMHEALTH,
    S_ITEMARMOUR, S_ITEMPUP, S_ITEMSPAWN, S_TELEPORT, S_NOAMMO, S_PUPOUT,
    S_PAIN1, S_PAIN2, S_PAIN3, S_PAIN4, S_PAIN5, S_PAIN6,
    S_DIE1, S_DIE2,
    S_FLAUNCH, S_FEXPLODE,
    S_SPLASH1, S_SPLASH2,
    S_JUMPPAD, S_PISTOL,

    S_FLAGPICKUP,
    S_FLAGDROP,
    S_FLAGRETURN,
    S_FLAGSCORE,
    S_FLAGRESET,
    S_FLAGFAIL,

    S_BURN,
    S_CHAINSAW_ATTACK,
    S_CHAINSAW_IDLE,

    S_HIT
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
    N_INITTOKENS, N_TAKETOKEN, N_EXPIRETOKENS, N_DROPTOKENS, N_DEPOSITTOKENS, N_STEALTOKENS,
    N_SERVCMD,
    N_DEMOPACKET,
    NUMMSG
};

static const int msgsizes[] =               // size inclusive message token, 0 for variable or not-checked sizes
{
    N_CONNECT, 0, N_SERVINFO, 0, N_WELCOME, 1, N_INITCLIENT, 0, N_POS, 0, N_TEXT, 0, N_SOUND, 2, N_CDIS, 2,
    N_SHOOT, 0, N_EXPLODE, 0, N_SUICIDE, 1,
    N_DIED, 5, N_DAMAGE, 6, N_HITPUSH, 7, N_SHOTFX, 10, N_EXPLODEFX, 4,
    N_TRYSPAWN, 1, N_SPAWNSTATE, 14, N_SPAWN, 3, N_FORCEDEATH, 2,
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
    N_INITTOKENS, 0, N_TAKETOKEN, 2, N_EXPIRETOKENS, 0, N_DROPTOKENS, 0, N_DEPOSITTOKENS, 2, N_STEALTOKENS, 0,
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
    HICON_BLUE_ARMOUR = 0,
    HICON_GREEN_ARMOUR,
    HICON_YELLOW_ARMOUR,

    HICON_HEALTH,

    HICON_FIST,
    HICON_SG,
    HICON_CG,
    HICON_RL,
    HICON_RIFLE,
    HICON_GL,
    HICON_PISTOL,

    HICON_QUAD,

    HICON_RED_FLAG,
    HICON_BLUE_FLAG,
    HICON_NEUTRAL_FLAG,

    HICON_TOKEN,

    HICON_X       = 20,
    HICON_Y       = 1650,
    HICON_TEXTY   = 1644,
    HICON_STEP    = 490,
    HICON_SIZE    = 120,
    HICON_SPACE   = 40
};

static struct itemstat { int add, max, sound; const char *name; int icon, info; } itemstats[] =
{
    {10,    30,    S_ITEMAMMO,   "SG", HICON_SG, GUN_SG},
    {20,    60,    S_ITEMAMMO,   "CG", HICON_CG, GUN_CG},
    {5,     15,    S_ITEMAMMO,   "RL", HICON_RL, GUN_RL},
    {5,     15,    S_ITEMAMMO,   "RI", HICON_RIFLE, GUN_RIFLE},
    {10,    30,    S_ITEMAMMO,   "GL", HICON_GL, GUN_GL},
    {30,    120,   S_ITEMAMMO,   "PI", HICON_PISTOL, GUN_PISTOL},
    {25,    100,   S_ITEMHEALTH, "H", HICON_HEALTH},
    {100,   100,   S_ITEMARMOUR, "GA", HICON_GREEN_ARMOUR, A_GREEN},
    {200,   200,   S_ITEMARMOUR, "YA", HICON_YELLOW_ARMOUR, A_YELLOW}
};

#define MAXRAYS 20
#define EXP_SELFDAMDIV 2
#define EXP_SELFPUSH 2.5f
#define EXP_DISTSCALE 1.5f

static const struct guninfo { int sound, attackdelay, damage, spread, projspeed, kickamount, range, rays, hitpush, exprad, ttl; const char *name, *file; } guns[NUMGUNS] =
{
    { S_PUNCH1,    250,  50,   0,   0,  0,   14,  1,  80,  0,    0, "fist",            "fist"   },
    { S_SG,       1400,  10, 400,   0, 20, 1024, 20,  80,  0,    0, "shotgun",         "shotg"  },
    { S_CG,        100,  30, 100,   0,  7, 1024,  1,  80,  0,    0, "chaingun",        "chaing" },
    { S_RLFIRE,    800, 120,   0, 320, 10, 1024,  1, 160, 40,    0, "rocketlauncher",  "rocket" },
    { S_RIFLE,    1500, 100,   0,   0, 30, 2048,  1,  80,  0,    0, "rifle",           "rifle"  },
    { S_FLAUNCH,   600,  90,   0, 200, 10, 1024,  1, 250, 45, 1500, "grenadelauncher", "gl"     },
    { S_PISTOL,    500,  35,  50,   0,  7, 1024,  1,  80,  0,    0, "pistol",          "pistol" }
};

#include "ai.h"

// inherited by fpsent and server clients
struct fpsstate
{
    int health, maxhealth;
    int armour, armourtype;
    int gunselect, gunwait;
    int ammo[NUMGUNS];
    int aitype, skill;

    fpsstate() : maxhealth(100), aitype(AI_NONE), skill(0) {}

    void baseammo(int gun, int k = 2, int scale = 1)
    {
        ammo[gun] = (itemstats[gun-GUN_SG].add*k)/scale;
    }

    void addammo(int gun, int k = 1, int scale = 1)
    {
        itemstat &is = itemstats[gun-GUN_SG];
        ammo[gun] = min(ammo[gun] + (is.add*k)/scale, is.max);
    }

    bool hasmaxammo(int type)
    {
       const itemstat &is = itemstats[type-I_SHELLS];
       return ammo[type-I_SHELLS+GUN_SG]>=is.max;
    }

    bool canpickup(int type)
    {
        if(type<I_SHELLS || type>I_YELLOWARMOUR) return false;
        itemstat &is = itemstats[type-I_SHELLS];
        switch(type)
        {
            case I_HEALTH: return health<maxhealth;
            case I_GREENARMOUR:
                // (100h/100g only absorbs 200 damage)
                if(armourtype==A_YELLOW && armour>=100) return false;
            case I_YELLOWARMOUR: return !armourtype || armour<is.max;
            default: return ammo[is.info]<is.max;
        }
    }

    void pickup(int type)
    {
        if(type<I_SHELLS || type>I_YELLOWARMOUR) return;
        itemstat &is = itemstats[type-I_SHELLS];
        switch(type)
        {
            case I_HEALTH:
                health = min(health+is.add, maxhealth);
                break;
            case I_GREENARMOUR:
            case I_YELLOWARMOUR:
                armour = min(armour+is.add, is.max);
                armourtype = is.info;
                break;
            default:
                ammo[is.info] = min(ammo[is.info]+is.add, is.max);
                break;
        }
    }

    void respawn()
    {
        health = maxhealth;
        armour = 0;
        armourtype = A_BLUE;
        gunselect = GUN_PISTOL;
        gunwait = 0;
        loopi(NUMGUNS) ammo[i] = 0;
        ammo[GUN_FIST] = 1;
    }

    void spawnstate(int gamemode)
    {
        if(m_demo)
        {
            gunselect = GUN_FIST;
        }
        else if(m_insta)
        {
            armour = 0;
            health = 1;
            gunselect = GUN_RIFLE;
            ammo[GUN_RIFLE] = 100;
        }
        else if(m_efficiency)
        {
            armourtype = A_GREEN;
            armour = 100;
            loopi(5) baseammo(i+1);
            gunselect = GUN_CG;
            ammo[GUN_CG] /= 2;
        }
        else if(m_ctf || m_collect)
        {
            armourtype = A_BLUE;
            armour = 50;
            ammo[GUN_PISTOL] = 40;
            ammo[GUN_GL] = 1;
        }
        else
        {
            armourtype = A_BLUE;
            armour = 25;
            ammo[GUN_PISTOL] = 40;
            ammo[GUN_GL] = 1;
        }
    }

    // just subtract damage here, can set death, etc. later in code calling this
    int dodamage(int damage)
    {
        int ad = damage*(armourtype+1)*25/100; // let armour absorb when possible
        if(ad>armour) ad = armour;
        armour -= ad;
        damage -= ad;
        health -= damage;
        return damage;
    }

    int hasammo(int gun, int exclude = -1)
    {
        return gun >= 0 && gun <= NUMGUNS && gun != exclude && ammo[gun] > 0;
    }
};

#define MAXTEAMS 2
static const char * const teamnames[1+MAXTEAMS] = { "", "azul", "rojo" };
static const char * const teamtextcode[1+MAXTEAMS] = { "\f0", "\f1", "\f3" };
static const int teamtextcolor[1+MAXTEAMS] = { 0x1EC850, 0x6496FF, 0xFF4B19 };
static const int teamscoreboardcolor[1+MAXTEAMS] = { 0, 0x3030C0, 0xC03030 };
static inline int teamnumber(const char *name) { loopi(MAXTEAMS) if(!strcmp(teamnames[1+i], name)) return 1+i; return 0; }
#define validteam(n) ((n) >= 1 && (n) <= MAXTEAMS)
#define teamname(n) (teamnames[validteam(n) ? (n) : 0])

struct fpsent : dynent, fpsstate
{
    int weight;                         // affects the effectiveness of hitpush
    int clientnum, privilege, lastupdate, plag, ping;
    int lifesequence;                   // sequence id for each respawn, used in damage test
    int respawned, suicided;
    int lastpain;
    int lastaction, lastattackgun;
    bool attacking;
    int attacksound, attackchan, idlesound, idlechan;
    int lasttaunt;
    int lastpickup, lastpickupmillis, lastbase, lastrepammo, flagpickup, tokens;
    vec lastcollect;
    int frags, flags, deaths, totaldamage, totalshots;
    editinfo *edit;
    float deltayaw, deltapitch, deltaroll, newyaw, newpitch, newroll;
    int smoothmillis;

    string name, info;
    int team, playermodel;
    ai::aiinfo *ai;
    int ownernum, lastnode;

    vec muzzle;

    fpsent() : weight(100), clientnum(-1), privilege(PRIV_NONE), lastupdate(0), plag(0), ping(0), lifesequence(0), respawned(-1), suicided(-1), lastpain(0), attacksound(-1), attackchan(-1), idlesound(-1), idlechan(-1), frags(0), flags(0), deaths(0), totaldamage(0), totalshots(0), edit(NULL), smoothmillis(-1), team(0), playermodel(-1), ai(NULL), ownernum(-1), muzzle(-1, -1, -1)
    {
        name[0] = info[0] = 0;
        respawn();
    }
    ~fpsent()
    {
        freeeditinfo(edit);
        if(attackchan >= 0) stopsound(attacksound, attackchan);
        if(idlechan >= 0) stopsound(idlesound, idlechan);
        if(ai) delete ai;
    }

    void hitpush(int damage, const vec &dir, fpsent *actor, int gun)
    {
        vec push(dir);
        push.mul((actor==this && guns[gun].exprad ? EXP_SELFPUSH : 1.0f)*guns[gun].hitpush*damage/weight);
        vel.add(push);
    }

    void stopattacksound()
    {
        if(attackchan >= 0) stopsound(attacksound, attackchan, 250);
        attacksound = attackchan = -1;
    }

    void stopidlesound()
    {
        if(idlechan >= 0) stopsound(idlesound, idlechan, 100);
        idlesound = idlechan = -1;
    }

    void respawn()
    {
        dynent::reset();
        fpsstate::respawn();
        respawned = suicided = -1;
        lastaction = 0;
        lastattackgun = gunselect;
        attacking = false;
        lasttaunt = 0;
        lastpickup = -1;
        lastpickupmillis = 0;
        lastbase = lastrepammo = -1;
        flagpickup = 0;
        tokens = 0;
        lastcollect = vec(-1e10f, -1e10f, -1e10f);
        stopattacksound();
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
    extern void checkitems(fpsent *d);
    extern void resetspawns();
    extern void spawnitems(bool force = false);
    extern void putitems(packetbuf &p);
    extern void setspawn(int i, bool on);
    extern void teleport(int n, fpsent *d);
    extern void pickupeffects(int n, fpsent *d);
    extern void teleporteffects(fpsent *d, int tp, int td, bool local = true);
    extern void jumppadeffects(fpsent *d, int jp, bool local = true);

    extern void repammo(fpsent *d, int type, bool local = true);
}

namespace game
{
    struct clientmode
    {
        virtual ~clientmode() {}

        virtual void preload() {}
        virtual int clipconsole(int w, int h) { return 0; }
        virtual void drawhud(fpsent *d, int w, int h) {}
        virtual void rendergame() {}
        virtual void respawned(fpsent *d) {}
        virtual void setup() {}
        virtual void checkitems(fpsent *d) {}
        virtual int respawnwait(fpsent *d) { return 0; }
        virtual void pickspawn(fpsent *d) { findplayerspawn(d); }
        virtual void senditems(packetbuf &p) {}
        virtual void removeplayer(fpsent *d) {}
        virtual void gameover() {}
        virtual bool hidefrags() { return false; }
        virtual int getteamscore(int team) { return 0; }
        virtual void getteamscores(vector<teamscore> &scores) {}
        virtual void aifind(fpsent *d, ai::aistate &b, vector<ai::interest> &interests) {}
        virtual bool aicheck(fpsent *d, ai::aistate &b) { return false; }
        virtual bool aidefend(fpsent *d, ai::aistate &b) { return false; }
        virtual bool aipursue(fpsent *d, ai::aistate &b) { return false; }
    };

    extern clientmode *cmode;
    extern void setclientmode();

    // fps
    extern int gamemode, nextmode;
    extern string clientmap;
    extern bool intermission;
    extern int maptime, maprealtime, maplimit;
    extern fpsent *player1;
    extern vector<fpsent *> players, clients;
    extern int lastspawnattempt;
    extern int lasthit;
    extern int respawnent;
    extern int following;
    extern int smoothmove, smoothdist;

    extern bool clientoption(const char *arg);
    extern fpsent *getclient(int cn);
    extern fpsent *newclient(int cn);
    extern const char *colorname(fpsent *d, const char *name = NULL, const char *alt = NULL, const char *color = "");
    extern const char *teamcolorname(fpsent *d, const char *alt = "you");
    extern const char *teamcolor(const char *prefix, const char *suffix, int team, const char *alt);
    extern fpsent *pointatplayer();
    extern fpsent *hudplayer();
    extern fpsent *followingplayer();
    extern void stopfollowing();
    extern void clientdisconnected(int cn, bool notify = true);
    extern void clearclients(bool notify = true);
    extern void startgame();
    extern void spawnplayer(fpsent *);
    extern void deathstate(fpsent *d, bool restore = false);
    extern void damaged(int damage, fpsent *d, fpsent *actor, bool local = true);
    extern void killed(fpsent *d, fpsent *actor);
    extern void timeupdate(int timeremain);
    extern void msgsound(int n, physent *d = NULL);
    extern void drawicon(int icon, float x, float y, float sz = 120);
    const char *mastermodecolor(int n, const char *unknown);
    const char *mastermodeicon(int n, const char *unknown);

    // client
    extern bool connected, remote, demoplayback;
    extern string servinfo;

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
    extern void sendposition(fpsent *d, bool reliable = false);

    // weapon
    extern int getweapon(const char *name);
    extern void shoot(fpsent *d, const vec &targ);
    extern void shoteffects(int gun, const vec &from, const vec &to, fpsent *d, bool local, int id, int prevaction);
    extern void explode(bool local, fpsent *owner, const vec &v, dynent *safe, int dam, int gun);
    extern void explodeeffects(int gun, fpsent *d, bool local, int id = 0);
    extern void damageeffect(int damage, fpsent *d, bool thirdperson = true);
    extern void gibeffect(int damage, const vec &vel, fpsent *d);
    extern float intersectdist;
    extern bool intersect(dynent *d, const vec &from, const vec &to, float &dist = intersectdist);
    extern dynent *intersectclosest(const vec &from, const vec &to, fpsent *at, float &dist = intersectdist);
    extern void clearbouncers();
    extern void updatebouncers(int curtime);
    extern void removebouncers(fpsent *owner);
    extern void renderbouncers();
    extern void clearprojectiles();
    extern void updateprojectiles(int curtime);
    extern void removeprojectiles(fpsent *owner);
    extern void renderprojectiles();
    extern void preloadbouncers();
    extern void removeweapons(fpsent *owner);
    extern void updateweapons(int curtime);
    extern void gunselect(int gun, fpsent *d);
    extern void weaponswitch(fpsent *d);
    extern void avoidweapons(ai::avoidset &obstacles, float radius);

    // scoreboard
    extern void showscores(bool on);
    extern void getbestplayers(vector<fpsent *> &best);
    extern void getbestteams(vector<int> &best);
    extern void clearteaminfo();
    extern void setteaminfo(int team, int frags);

    // render
    struct playermodelinfo
    {
        const char *model[1+MAXTEAMS], *hudguns[1+MAXTEAMS],
                   *vwep, *armour[3],
                   *icon[1+MAXTEAMS];
        bool ragdoll;
    };

    extern int playermodel, teamskins, testteam;

    extern void saveragdoll(fpsent *d);
    extern void clearragdolls();
    extern void moveragdolls();
    extern void changedplayermodel();
    extern const playermodelinfo &getplayermodelinfo(fpsent *d);
    extern int chooserandomplayermodel(int seed);
    extern void swayhudgun(int curtime);
    extern vec hudgunorigin(int gun, const vec &from, const vec &to, fpsent *d);
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

