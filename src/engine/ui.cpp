#include "engine.h"
#include "textedit.h"

namespace UI
{
    float cursorx = 0.5f, cursory = 0.5f;

    static void quads(float x, float y, float w, float h, float tx = 0, float ty = 0, float tw = 1, float th = 1)
    {
        gle::attribf(x,   y);   gle::attribf(tx,    ty);
        gle::attribf(x+w, y);   gle::attribf(tx+tw, ty);
        gle::attribf(x+w, y+h); gle::attribf(tx+tw, ty+th);
        gle::attribf(x,   y+h); gle::attribf(tx,    ty+th);
    }

    static void quad(float x, float y, float w, float h, float tx = 0, float ty = 0, float tw = 1, float th = 1)
    {
        gle::defvertex(2);
        gle::deftexcoord0();
        gle::begin(GL_TRIANGLE_STRIP);
        gle::attribf(x+w, y);   gle::attribf(tx+tw, ty);
        gle::attribf(x,   y);   gle::attribf(tx,    ty);
        gle::attribf(x+w, y+h); gle::attribf(tx+tw, ty+th);
        gle::attribf(x,   y+h); gle::attribf(tx,    ty+th);
        gle::end();
    }

    static void quad(float x, float y, float w, float h, const vec2 tc[4])
    {
        gle::defvertex(2);
        gle::deftexcoord0();
        gle::begin(GL_TRIANGLE_STRIP);
        gle::attribf(x+w, y);   gle::attrib(tc[0]);
        gle::attribf(x,   y);   gle::attrib(tc[1]);
        gle::attribf(x+w, y+h); gle::attrib(tc[3]);
        gle::attribf(x,   y+h); gle::attrib(tc[2]);
        gle::end();
    }

    struct ClipArea
    {
        float x1, y1, x2, y2;

        ClipArea(float x, float y, float w, float h) : x1(x), y1(y), x2(x+w), y2(y+h) {}

        void intersect(const ClipArea &c)
        {
            x1 = max(x1, c.x1);
            y1 = max(y1, c.y1);
            x2 = max(x1, min(x2, c.x2));
            y2 = max(y1, min(y2, c.y2));

        }

        bool isfullyclipped(float x, float y, float w, float h)
        {
            return x1 == x2 || y1 == y2 || x >= x2 || y >= y2 || x+w <= x1 || y+h <= y1;
        }

        void scissor();
    };

    static vector<ClipArea> clipstack;

    static void pushclip(float x, float y, float w, float h)
    {
        if(clipstack.empty()) glEnable(GL_SCISSOR_TEST);
        ClipArea &c = clipstack.add(ClipArea(x, y, w, h));
        if(clipstack.length() >= 2) c.intersect(clipstack[clipstack.length()-2]);
        c.scissor();
    }

    static void popclip()
    {
        clipstack.pop();
        if(clipstack.empty()) glDisable(GL_SCISSOR_TEST);
        else clipstack.last().scissor();
    }

    static bool isfullyclipped(float x, float y, float w, float h)
    {
        if(clipstack.empty()) return false;
        return clipstack.last().isfullyclipped(x, y, w, h);
    }

    enum
    {
        ALIGN_MASK    = 0xF,

        ALIGN_HMASK   = 0x3,
        ALIGN_HSHIFT  = 0,
        ALIGN_HNONE   = 0,
        ALIGN_LEFT    = 1,
        ALIGN_HCENTER = 2,
        ALIGN_RIGHT   = 3,

        ALIGN_VMASK   = 0xC,
        ALIGN_VSHIFT  = 2,
        ALIGN_VNONE   = 0<<2,
        ALIGN_BOTTOM  = 1<<2,
        ALIGN_VCENTER = 2<<2,
        ALIGN_TOP     = 3<<2,

        CLAMP_MASK    = 0xF0,
        CLAMP_LEFT    = 0x10,
        CLAMP_RIGHT   = 0x20,
        CLAMP_BOTTOM  = 0x40,
        CLAMP_TOP     = 0x80,

        NO_ADJUST     = ALIGN_HNONE | ALIGN_VNONE,
    };

    enum
    {
        STATE_HOVER       = 1<<0,
        STATE_PRESS       = 1<<1,
        STATE_HOLD        = 1<<2,
        STATE_RELEASE     = 1<<3,
        STATE_ALT_PRESS   = 1<<4,
        STATE_ALT_HOLD    = 1<<5,
        STATE_ALT_RELEASE = 1<<6,
        STATE_ESC_PRESS   = 1<<7,
        STATE_ESC_HOLD    = 1<<8,
        STATE_ESC_RELEASE = 1<<9,
        STATE_SCROLL_UP   = 1<<10,
        STATE_SCROLL_DOWN = 1<<11
    };

    struct Object
    {
        Object *parent;
        float x, y, w, h;
        uchar adjust;
        ushort state, childstate;
        vector<Object *> children;

        Object() : state(0), childstate(0) {}
        virtual ~Object()
        {
            clearchildren();
        }

        void reset(Object *parent_ = NULL)
        {
            parent = parent_;
            x = y = w = h = 0;
            adjust = ALIGN_HCENTER | ALIGN_VCENTER;
        }

        void setup()
        {
        }

        void clearchildren()
        {
            children.deletecontents();
        }

        #define loopchildren(o, body) do { \
            loopv(children) \
            { \
                Object *o = children[i]; \
                body; \
            } \
        } while(0)

        #define loopchildrenrev(o, body) do { \
            loopvrev(children) \
            { \
                Object *o = children[i]; \
                body; \
            } \
        } while(0)

        #define loopinchildren(o, cx, cy, inbody, outbody) \
            loopchildren(o, \
            { \
                float o##x = cx - o->x; \
                float o##y = cy - o->y; \
                if(o##x >= 0 && o##x < o->w && o##y >= 0 && o##y < o->h) \
                { \
                    inbody; \
                } \
                outbody; \
            })

        #define loopinchildrenrev(o, cx, cy, inbody, outbody) \
            loopchildrenrev(o, \
            { \
                float o##x = cx - o->x; \
                float o##y = cy - o->y; \
                if(o##x >= 0 && o##x < o->w && o##y >= 0 && o##y < o->h) \
                { \
                    inbody; \
                } \
                outbody; \
            })

        virtual void layout()
        {
            w = h = 0;
            loopchildren(o,
            {
                o->x = o->y = 0;
                o->layout();
                w = max(w, o->x + o->w);
                h = max(h, o->y + o->h);
            });
        }

        void adjustchildrento(float px, float py, float pw, float ph)
        {
            loopchildren(o, o->adjustlayout(px, py, pw, ph));
        }

        virtual void adjustchildren()
        {
            adjustchildrento(0, 0, w, h);
        }

        void adjustlayout(float px, float py, float pw, float ph)
        {
            switch(adjust&ALIGN_HMASK)
            {
                case ALIGN_LEFT:    x = px; break;
                case ALIGN_HCENTER: x = px + (pw - w) / 2; break;
                case ALIGN_RIGHT:   x = px + pw - w; break;
            }

            switch(adjust&ALIGN_VMASK)
            {
                case ALIGN_BOTTOM:  y = py; break;
                case ALIGN_VCENTER: y = py + (ph - h) / 2; break;
                case ALIGN_TOP:     y = py + ph - h; break;
            }

            if(adjust&CLAMP_MASK)
            {
                if(adjust&CLAMP_LEFT)   x = px;
                if(adjust&CLAMP_RIGHT)  w = px + pw - x;
                if(adjust&CLAMP_BOTTOM) y = py;
                if(adjust&CLAMP_TOP)    h = py + ph - y;
            }

            adjustchildren();
        }

        void setalign(int xalign, int yalign)
        {
            adjust &= ~ALIGN_MASK;
            adjust |= (clamp(xalign, -1, 1)+2)<<ALIGN_HSHIFT;
            adjust |= (clamp(yalign, -1, 1)+2)<<ALIGN_VSHIFT;
        }

        void setclamp(int left, int right, int bottom, int top)
        {
            adjust &= ~CLAMP_MASK;
            if(left) adjust |= CLAMP_LEFT;
            if(right) adjust |= CLAMP_RIGHT;
            if(bottom) adjust |= CLAMP_BOTTOM;
            if(top) adjust |= CLAMP_TOP;
        }

        virtual bool target(float cx, float cy)
        {
            return false;
        }

        virtual bool key(int code, bool isdown)
        {
            loopchildrenrev(o,
            {
                if(o->key(code, isdown)) return true;
            });
            return false;
        }

        virtual bool textinput(const char *str, int len)
        {
            loopchildrenrev(o,
            {
                if(o->textinput(str, len)) return true;
            });
            return false;
        }

        virtual void draw(float sx, float sy)
        {
            loopchildren(o,
            {
                if(!isfullyclipped(sx + o->x, sy + o->y, o->w, o->h))
                    o->draw(sx + o->x, sy + o->y);
            });
        }

        void draw()
        {
            draw(x, y);
        }

        void resetstate(bool full = false)
        {
            state = childstate = 0;
            if(full) loopchildren(o, o->resetstate(true));
        }

        bool hasstate(int flags) const { return ((state & ~childstate) & flags) != 0; }
        bool haschildstate(int flags) const { return ((state | childstate) & flags) != 0; }

        #define DOSTATES \
            DOSTATE(STATE_HOVER, hover) \
            DOSTATE(STATE_PRESS, press) \
            DOSTATE(STATE_HOLD, hold) \
            DOSTATE(STATE_RELEASE, release) \
            DOSTATE(STATE_ALT_HOLD, althold) \
            DOSTATE(STATE_ALT_PRESS, altpress) \
            DOSTATE(STATE_ALT_RELEASE, altrelease) \
            DOSTATE(STATE_ESC_HOLD, eschold) \
            DOSTATE(STATE_ESC_PRESS, escpress) \
            DOSTATE(STATE_ESC_RELEASE, escrelease) \
            DOSTATE(STATE_SCROLL_UP, scrollup) \
            DOSTATE(STATE_SCROLL_DOWN, scrolldown)

        bool setstate(int state, float cx, float cy)
        {
            switch(state)
            {
            #define DOSTATE(flags, func) case flags: func(cx, cy); return haschildstate(flags);
            DOSTATES
            #undef DOSTATE
            }
            return false;
        }

        #define DOSTATE(flags, func) \
            virtual void func(float cx, float cy) \
            { \
                loopinchildrenrev(o, cx, cy, \
                { \
                    o->func(ox, oy); \
                    childstate |= (o->state | o->childstate) & (flags); \
                }, ); \
                if(target(cx, cy)) state |= (flags); \
            }
        DOSTATES
        #undef DOSTATE

        static const char *typestr() { return "#Object"; }
        virtual const char *gettype() const { return typestr(); }
        virtual const char *getname() const { return gettype(); }
        virtual const char *gettypename() const { return gettype(); }

        template<class T> bool istype() const { return T::typestr() == gettype(); }
        bool isnamed(const char *name) const { return name[0] == '#' ? name == gettypename() : !strcmp(name, getname()); }

        Object *find(const char *name, bool recurse = true, const Object *exclude = NULL) const
        {
            loopchildren(o,
            {
                if(o != exclude && o->isnamed(name)) return o;
            });
            if(recurse) loopchildren(o,
            {
                if(o != exclude)
                {
                    Object *found = o->find(name);
                    if(found) return found;
                }
            });
            return NULL;
        }

        Object *findsibling(const char *name) const
        {
            for(const Object *prev = this, *cur = parent; cur; prev = cur, cur = cur->parent)
            {
                Object *o = cur->find(name, true, prev);
                if(o) return o;
            }
            return NULL;
        }

        template<class T> T *buildtype();
        void buildchildren(uint *contents);

        virtual void build() {}
    };

    struct Window : Object
    {
        char *name;
        uint *contents, *onshow, *onhide;

        Window(const char *name, const char *contents, const char *onshow, const char *onhide) :
            name(newstring(name)),
            contents(compilecode(contents)),
            onshow(onshow && onshow[0] ? compilecode(onshow) : NULL),
            onhide(onhide && onhide[0] ? compilecode(onhide) : NULL)
        {
        }
        ~Window()
        {
            delete[] name;
            freecode(contents);
            freecode(onshow);
            freecode(onhide);
        }

        static const char *typestr() { return "#Window"; }
        const char *gettype() const { return typestr(); }
        const char *getname() const { return name; }

        void build();

        void hide()
        {
            if(onshow) execute(onshow);
        }

        void show()
        {
            if(onshow) execute(onshow);
        }

        void escrelease(float cx, float cy);
    };

    static inline bool htcmp(const char *key, const Window *w) { return !strcmp(key, w->name); }

    hashset<Window *> windows;

    struct World : Object
    {
        static const char *typestr() { return "#World"; }
        const char *gettype() const { return typestr(); }
        const char *getname() const { return gettype(); }

        void layout()
        {
            Object::layout();

            float aspect = float(screenw)/screenh;
            h = max(max(h, w/aspect), 1.0f);
            w = aspect*h;
            x = 0.5f*(1 - w);
            y = 0.5f*(1 - h);

            adjustchildren();
        }

        #define DOSTATE(flags, func) \
            void func(float cx, float cy) \
            { \
                loopinchildrenrev(o, cx, cy, \
                { \
                    o->func(ox, oy); \
                }, \
                { \
                    int oflags = (o->state | o->childstate) & (flags); \
                    if(oflags) { childstate |= oflags; break; } \
                }); \
                if(target(cx, cy)) state |= (flags); \
            }
        DOSTATES
        #undef DOSTATE

        void build();

        bool show(Window *w)
        {
            if(children.find(w) >= 0) return false;
            w->resetstate(true);
            children.add(w);
            w->show();
            return true;
        }

        bool hide(Window *w)
        {
            int index = children.find(w);
            if(index < 0) return false;
            children.remove(index);
            childstate = 0;
            loopchildren(w, childstate |= w->state | w->childstate);
            w->hide();
            return true;
        }
    };

    World *world = NULL;

    void ClipArea::scissor()
    {
        int sx1 = clamp(int(floor((x1-world->x)/world->w*screenw)), 0, screenw),
            sy1 = clamp(int(floor((y1-world->y)/world->h*screenh)), 0, screenh),
            sx2 = clamp(int(ceil((x2-world->x)/world->w*screenw)), 0, screenw),
            sy2 = clamp(int(ceil((y2-world->y)/world->h*screenh)), 0, screenh);
        glScissor(sx1, screenh - sy2, sx2-sx1, sy2-sy1);
    }

    void Window::escrelease(float cx, float cy)
    {
        Object::escrelease(cx, cy);
        world->hide(this);
    }

    Object *buildparent = NULL;
    int buildchild = -1;

    template<class T> inline T *Object::buildtype()
    {
        T *t;
        if(children.inrange(buildchild))
        {
            Object *o = children[buildchild];
            if(o->istype<T>()) t = (T *)o;
            else
            {
                delete o;
                t = new T;
                children[buildchild] = t;
            }
        }
        else
        {
            t = new T;
            children.add(t);
        }
        t->reset(this);
        buildchild++;
        return t;
    }

    inline void Object::buildchildren(uint *contents)
    {
        if((*contents&CODE_OP_MASK) == CODE_EXIT) children.deletecontents();
        else
        {
            Object *oldparent = buildparent;
            int oldchild = buildchild;
            buildparent = this;
            buildchild = 0;
            execute(contents);
            while(children.length() > buildchild)
                delete children.pop();
            buildparent = oldparent;
            buildchild = oldchild;
        }
        resetstate();
    }

    void Window::build()
    {
        reset(world);
        setup();
        buildchildren(contents);
    }

    void World::build()
    {
        reset();
        setup();
        loopchildren(o, o->build());
    }

    #define BUILD(type, o, setup, contents) do { \
        type *o = buildparent->buildtype<type>(); \
        setup; \
        o->buildchildren(contents); \
    } while(0)

    struct HorizontalList : Object
    {
        float space;

        static const char *typestr() { return "#HorizontalList"; }
        const char *gettype() const { return typestr(); }

        void setup(float space_ = 0)
        {
            Object::setup();
            space = space_;
        }

        void layout()
        {
            w = h = 0;
            loopchildren(o,
            {
                o->x = w;
                o->y = 0;
                o->layout();
                w += o->w;
                h = max(h, o->y + o->h);
            });
            w += space*max(children.length() - 1, 0);
        }

        void adjustchildren()
        {
            if(children.empty()) return;

            float offset = 0;
            loopchildren(o,
            {
                o->x = offset;
                offset += o->w;
                o->adjustlayout(o->x, 0, offset - o->x, h);
                offset += space;
            });
        }
    };

    struct VerticalList : Object
    {
        float space;

        static const char *typestr() { return "#VerticalList"; }
        const char *gettype() const { return typestr(); }

        void setup(float space_ = 0)
        {
            Object::setup();
            space = space_;
        }

        void layout()
        {
            w = h = 0;
            loopchildren(o,
            {
                o->x = 0;
                o->y = h;
                o->layout();
                h += o->h;
                w = max(w, o->x + o->w);
            });
            h += space*max(children.length() - 1, 0);
        }

        void adjustchildren()
        {
            if(children.empty()) return;

            float offset = 0;
            loopchildren(o,
            {
                o->y = offset;
                offset += o->h;
                o->adjustlayout(0, o->y, w, offset - o->y);
                offset += space;
            });
        }
    };

    struct Table : Object
    {
        int columns;
        float space;
        vector<float> widths, heights;

        static const char *typestr() { return "#Table"; }
        const char *gettype() const { return typestr(); }

        void setup(int columns_, float space_ = 0)
        {
            Object::setup();
            columns = columns_;
            space = space_;
        }

        void layout()
        {
            widths.setsize(0);
            heights.setsize(0);

            int column = 0, row = 0;
            loopchildren(o,
            {
                o->layout();
                if(!widths.inrange(column)) widths.add(o->w);
                else if(o->w > widths[column]) widths[column] = o->w;
                if(!heights.inrange(row)) heights.add(o->h);
                else if(o->h > heights[row]) heights[row] = o->h;
                column = (column + 1) % columns;
                if(!column) row++;
            });
            w = h = 0;
            column = row = 0;
            float offset = 0;
            loopchildren(o,
            {
                o->x = offset;
                o->y = h;
                o->adjustlayout(o->x, o->y, widths[column], heights[row]);
                offset += widths[column];
                w = max(w, offset);
                column = (column + 1) % columns;
                if(!column)
                {
                    offset = 0;
                    h += heights[row];
                    row++;
                }
            });
            if(column) h += heights[row];

            w += space*max(widths.length() - 1, 0);
            h += space*max(heights.length() - 1, 0);
        }

        void adjustchildren()
        {
            if(children.empty()) return;

            float cspace = w, rspace = h;
            loopv(widths) cspace -= widths[i];
            loopv(heights) rspace -= heights[i];
            cspace /= max(widths.length() - 1, 1);
            rspace /= max(heights.length() - 1, 1);

            int column = 0, row = 0;
            float offsetx = 0, offsety = 0;

            loopchildren(o,
            {
                o->x = offsetx;
                o->y = offsety;
                o->adjustlayout(offsetx, offsety, widths[column], heights[row]);
                offsetx += widths[column] + cspace;
                column = (column + 1) % columns;
                if(!column)
                {
                    offsetx = 0;
                    offsety += heights[row] + rspace;
                    row++;
                }
            });
        }
    };

    struct Spacer : Object
    {
        float spacew, spaceh;

        void setup(float spacew_, float spaceh_)
        {
            Object::setup();
            spacew = spacew_;
            spaceh = spaceh_;
        }

        static const char *typestr() { return "#Spacer"; }
        const char *gettype() const { return typestr(); }

        void layout()
        {
            w = spacew;
            h = spaceh;
            loopchildren(o,
            {
                o->x = spacew;
                o->y = spaceh;
                o->layout();
                w = max(w, o->x + o->w);
                h = max(h, o->y + o->h);
            });
            w += spacew;
            h += spaceh;
        }

        void adjustchildren()
        {
            adjustchildrento(spacew, spaceh, w - 2*spacew, h - 2*spaceh);
        }
    };

    struct Offsetter : Object
    {
        float offsetx, offsety;

        void setup(float offsetx_, float offsety_)
        {
            Object::setup();
            offsetx = offsetx_;
            offsety = offsety_;
        }

        static const char *typestr() { return "#Offsetter"; }
        const char *gettype() const { return typestr(); }

        void layout()
        {
            Object::layout();

            loopchildren(o,
            {
                o->x += offsetx;
                o->y += offsety;
            });

            w += offsetx;
            h += offsety;
        }

        void adjustchildren()
        {
            adjustchildrento(offsetx, offsety, w - offsetx, h - offsety);
        }
    };

    struct Filler : Object
    {
        float minw, minh;

        void setup(float minw_, float minh_)
        {
            Object::setup();
            minw = minw_;
            minh = minh_;
        }

        static const char *typestr() { return "#Filler"; }
        const char *gettype() const { return typestr(); }

        void layout()
        {
            Object::layout();

            w = max(w, minw);
            h = max(h, minh);
        }
    };

    struct FillColor : Filler
    {
        enum { SOLID = 0, MODULATE };

        int type;
        vec4 color;

        void setup(int type_, const vec4 &color_, float minw_ = 0, float minh_ = 0)
        {
            Filler::setup(minw_, minh_);
            type = type_;
            color = color_;
        }

        static const char *typestr() { return "#FillColor"; }
        const char *gettype() const { return typestr(); }

        bool target(float cx, float cy)
        {
            return true;
        }

        void draw(float sx, float sy)
        {
            if(type==MODULATE) glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            hudnotextureshader->set();
            gle::color(color);
            gle::defvertex(2);
            gle::begin(GL_TRIANGLE_STRIP);
            gle::attribf(sx+w, sy);
            gle::attribf(sx,   sy);
            gle::attribf(sx+w, sy+h);
            gle::attribf(sx,   sy+h);
            gle::end();
            gle::colorf(1, 1, 1);
            hudshader->set();
            if(type==MODULATE) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            Object::draw(sx, sy);
        }
    };

    struct Gradient : FillColor
    {
        enum { VERTICAL, HORIZONTAL };

        int dir;
        vec4 color2;

        void setup(int type_, int dir_, const vec4 &color_, const vec4 &color2_, float minw_ = 0, float minh_ = 0)
        {
            FillColor::setup(type_, color_, minw_, minh_);
            dir = dir_;
            color2 = color2_;
        }

        static const char *typestr() { return "#Gradient"; }
        const char *gettype() const { return typestr(); }

        void draw(float sx, float sy)
        {
            if(type==MODULATE) glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            hudnotextureshader->set();
            gle::defvertex(2);
            gle::defcolor(4);
            gle::begin(GL_TRIANGLE_STRIP);
            gle::attribf(sx+w, sy);   gle::attrib(dir == HORIZONTAL ? color2 : color);
            gle::attribf(sx,   sy);   gle::attrib(color);
            gle::attribf(sx+w, sy+h); gle::attrib(color2);
            gle::attribf(sx,   sy+h); gle::attrib(dir == HORIZONTAL ? color : color2);
            gle::end();
            hudshader->set();
            if(type==MODULATE) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            Object::draw(sx, sy);
        }
    };

    struct Outline : Filler
    {
        float thick;
        vec4 color;

        void setup(float thick_, const vec4 &color_, float minw_ = 0, float minh_ = 0)
        {
            Filler::setup(minw_, minh_);
            thick = thick_;
            color = color_;
        }

        static const char *typestr() { return "#Outline"; }
        const char *gettype() const { return typestr(); }

        void draw(float sx, float sy)
        {
            hudnotextureshader->set();
            gle::color(color);
            gle::defvertex(2);
            if(!thick)
            {
                gle::begin(GL_LINE_LOOP);
                gle::attribf(sx,   sy);
                gle::attribf(sx+w, sy);
                gle::attribf(sx+w, sy+h);
                gle::attribf(sx,   sy+h);
                gle::end();
            }
            else
            {
                gle::begin(GL_QUADS);
                float tw = min(thick, w/2), th = min(thick, h/2);
                // top
                gle::attribf(sx, sy); gle::attribf(sx+w, sy); gle::attribf(sx+w-tw, sy+th); gle::attribf(sx+tw, sy+th);
                // bottom
                gle::attribf(sx+tw, sy+h-th); gle::attribf(sx+w-tw, sy+h-th); gle::attribf(sx+w, sy+h); gle::attribf(sx, sy+h);
                // left
                gle::attribf(sx, sy); gle::attribf(sx+tw, sy+th); gle::attribf(sx+tw, sy+h-th); gle::attribf(sx, sy+h);
                // right
                gle::attribf(sx+w-tw, sy+th); gle::attribf(sx+w, sy); gle::attribf(sx+w, sy+h); gle::attribf(sx+w-tw, sy+h-th);
                gle::end();
            }
            gle::colorf(1, 1, 1);
            hudshader->set();

            Object::draw(sx, sy);
        }
    };

    static inline bool checkalphamask(Texture *tex, float x, float y)
    {
        if(!tex->alphamask)
        {
            loadalphamask(tex);
            if(!tex->alphamask) return true;
        }
        int tx = clamp(int(x*tex->xs), 0, tex->xs-1),
            ty = clamp(int(y*tex->ys), 0, tex->ys-1);
        if(tex->alphamask[ty*((tex->xs+7)/8) + tx/8] & (1<<(tx%8))) return true;
        return false;
    }

    struct Image : Filler
    {
        Texture *tex;

        void setup(Texture *tex_, float minw_ = 0, float minh_ = 0)
        {
            Filler::setup(minw_, minh_);
            tex = tex_;
        }

        static const char *typestr() { return "#Image"; }
        const char *gettype() const { return typestr(); }

        bool target(float cx, float cy)
        {
            return !(tex->type&Texture::ALPHA) || checkalphamask(tex, cx/w, cy/h);
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);

            quad(sx, sy, w, h);

            Object::draw(sx, sy);
        }
    };

    struct CroppedImage : Image
    {
        float cropx, cropy, cropw, croph;

        void setup(Texture *tex_, float minw_ = 0, float minh_ = 0, float cropx_ = 0, float cropy_ = 0, float cropw_ = 1, float croph_ = 1)
        {
            Image::setup(tex_, minw_, minh_);
            cropx = cropx_;
            cropy = cropy_;
            cropw = cropw_;
            croph = croph_;
        }

        static const char *typestr() { return "#CroppedImage"; }
        const char *gettype() const { return typestr(); }

        bool target(float cx, float cy)
        {
            return !(tex->type&Texture::ALPHA) || checkalphamask(tex, cropx + cx/w*cropw, cropy + cy/h*croph);
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);

            quad(sx, sy, w, h, cropx, cropy, cropw, croph);

            Object::draw(sx, sy);
        }
    };

    struct StretchedImage : Image
    {
        static const char *typestr() { return "#StretchedImage"; }
        const char *gettype() const { return typestr(); }

        bool target(float cx, float cy)
        {
            if(!(tex->type&Texture::ALPHA)) return false;

            float mx, my;
            if(w <= minw) mx = cx/w;
            else if(cx < minw/2) mx = cx/minw;
            else if(cx >= w - minw/2) mx = 1 - (w - cx) / minw;
            else mx = 0.5f;
            if(h <= minh) my = cy/h;
            else if(cy < minh/2) my = cy/minh;
            else if(cy >= h - minh/2) my = 1 - (h - cy) / minh;
            else my = 0.5f;

            return checkalphamask(tex, mx, my);
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);

            gle::defvertex(2);
            gle::deftexcoord0();
            gle::begin(GL_QUADS);
            float splitw = (minw ? min(minw, w) : w) / 2,
                  splith = (minh ? min(minh, h) : h) / 2,
                  vy = sy, ty = 0;
            loopi(3)
            {
                float vh = 0, th = 0;
                switch(i)
                {
                    case 0: if(splith < h - splith) { vh = splith; th = 0.5f; } else { vh = h; th = 1; } break;
                    case 1: vh = h - 2*splith; th = 0; break;
                    case 2: vh = splith; th = 0.5f; break;
                }
                float vx = sx, tx = 0;
                loopj(3)
                {
                    float vw = 0, tw = 0;
                    switch(j)
                    {
                        case 0: if(splitw < w - splitw) { vw = splitw; tw = 0.5f; } else { vw = w; tw = 1; } break;
                        case 1: vw = w - 2*splitw; tw = 0; break;
                        case 2: vw = splitw; tw = 0.5f; break;
                    }
                    quads(vx, vy, vw, vh, tx, ty, tw, th);
                    vx += vw;
                    tx += tw;
                    if(tx >= 1) break;
                }
                vy += vh;
                ty += th;
                if(ty >= 1) break;
            }
            gle::end();

            Object::draw(sx, sy);
        }
    };

    struct BorderedImage : Image
    {
        float texborder, screenborder;

        void setup(Texture *tex_, float texborder_, float screenborder_)
        {
            Image::setup(tex_);
            texborder = texborder_;
            screenborder = screenborder_;
        }

        static const char *typestr() { return "#BorderedImage"; }
        const char *gettype() const { return typestr(); }

        bool target(float cx, float cy)
        {
            if(!(tex->type&Texture::ALPHA)) return false;

            float mx, my;
            if(cx < screenborder) mx = cx/screenborder*texborder;
            else if(cx >= w - screenborder) mx = 1-texborder + (cx - (w - screenborder))/screenborder*texborder;
            else mx = texborder + (cx - screenborder)/(w - 2*screenborder)*(1 - 2*texborder);
            if(cy < screenborder) my = cy/screenborder*texborder;
            else if(cy >= h - screenborder) my = 1-texborder + (cy - (h - screenborder))/screenborder*texborder;
            else my = texborder + (cy - screenborder)/(h - 2*screenborder)*(1 - 2*texborder);

            return checkalphamask(tex, mx, my);
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);

            gle::defvertex(2);
            gle::deftexcoord0();
            gle::begin(GL_QUADS);
            float vy = sy, ty = 0;
            loopi(3)
            {
                float vh = 0, th = 0;
                switch(i)
                {
                    case 0: vh = screenborder; th = texborder; break;
                    case 1: vh = h - 2*screenborder; th = 1 - 2*texborder; break;
                    case 2: vh = screenborder; th = texborder; break;
                }
                float vx = sx, tx = 0;
                loopj(3)
                {
                    float vw = 0, tw = 0;
                    switch(j)
                    {
                        case 0: vw = screenborder; tw = texborder; break;
                        case 1: vw = w - 2*screenborder; tw = 1 - 2*texborder; break;
                        case 2: vw = screenborder; tw = texborder; break;
                    }
                    quads(vx, vy, vw, vh, tx, ty, tw, th);
                    vx += vw;
                    tx += tw;
                }
                vy += vh;
                ty += th;
            }
            gle::end();

            Object::draw(sx, sy);
        }
    };

    struct TiledImage : Image
    {
        float tilew, tileh;

        void setup(Texture *tex_, float minw_ = 0, float minh_ = 0, float tilew_ = 0, float tileh_ = 0)
        {
            Image::setup(tex_, minw_, minh_);
            tilew = tilew_;
            tileh = tileh_;
        }

        static const char *typestr() { return "#TiledImage"; }
        const char *gettype() const { return typestr(); }

        bool target(float cx, float cy)
        {
            if(!(tex->type&Texture::ALPHA)) return false;

            return checkalphamask(tex, fmod(cx/tilew, 1), fmod(cy/tileh, 1));
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);

            if(tex->clamp)
            {
                gle::defvertex(2);
                gle::deftexcoord0();
                gle::begin(GL_QUADS);
                for(float dy = 0; dy < h; dy += tileh)
                {
                    float dh = min(tileh, h - dy);
                    for(float dx = 0; dx < w; dx += tilew)
                    {
                        float dw = min(tilew, w - dx);
                        quads(sx + dx, sy + dy, dw, dh, 0, 0, dw / tilew, dh / tileh);
                    }
                }
                gle::end();
            }
            else quad(sx, sy, w, h, 0, 0, w/tilew, h/tileh);

            Object::draw(sx, sy);
        }
    };

    // default size of text in terms of rows per screenful
    VARP(uitextrows, 1, 24, 200);

    #define SETSTR(dst, src) do { \
        if(dst) { if(dst != src && strcmp(dst, src)) { delete[] dst; dst = newstring(src); } } \
        else dst = newstring(src); \
    } while(0)

    struct Text : Object
    {
        char *str;
        float scale;
        vec color;

        Text() : str(NULL) {}
        ~Text() { delete[] str; }

        void setup(const char *str_, float scale_ = 1, float r = 1, float g = 1, float b = 1)
        {
            Object::setup();

            SETSTR(str, str_);
            scale = scale_;
            color = vec(r, g, b);
        }

        static const char *typestr() { return "#Text"; }
        const char *gettype() const { return typestr(); }

        float drawscale() const { return scale / (FONTH * uitextrows); }

        void draw(float sx, float sy)
        {
            float oldscale = textscale;
            textscale = drawscale();
            draw_text(str, sx/textscale, sy/textscale, int(color.x*255), int(color.y*255), int(color.z*255));
            textscale = oldscale;

            Object::draw(sx, sy);
        }

        void layout()
        {
            Object::layout();

            float k = drawscale(), tw, th;
            text_boundsf(str, tw, th);
            w = max(w, tw*k);
            h = max(h, th*k);
        }
    };

    struct Clipper : Object
    {
        float clipw, cliph, virtw, virth;

        void setup(float clipw_ = 0, float cliph_ = 0)
        {
            Object::setup();
            clipw = clipw_;
            cliph = cliph_;
            virtw = virth = 0;
        }

        static const char *typestr() { return "#Clipper"; }
        const char *gettype() const { return typestr(); }

        void layout()
        {
            Object::layout();

            virtw = w;
            virth = h;
            if(clipw) w = min(w, clipw);
            if(cliph) h = min(h, cliph);
        }

        void adjustchildren()
        {
            adjustchildrento(0, 0, virtw, virth);
        }

        void draw(float sx, float sy)
        {
            if((clipw && virtw > clipw) || (cliph && virth > cliph))
            {
                pushclip(sx, sy, w, h);
                Object::draw(sx, sy);
                popclip();
            }
            else Object::draw(sx, sy);
        }
    };

    struct Scroller : Clipper
    {
        float offsetx, offsety;

        Scroller() : offsetx(0), offsety(0) {}

        void setup(float clipw_ = 0, float cliph_ = 0)
        {
            Clipper::setup(clipw_, cliph_);
        }

        static const char *typestr() { return "#Scroller"; }
        const char *gettype() const { return typestr(); }

        void layout()
        {
            Clipper::layout();
            offsetx = min(offsetx, hlimit());
            offsety = min(offsety, vlimit());
        }

        bool target(float cx, float cy)
        {
            if(cx + offsetx >= virtw || cy + offsety >= virth) return false;
            return Object::target(cx + offsetx, cy + offsety);
        }

        #define DOSTATE(flags, func) \
            void func(float cx, float cy) \
            { \
                if(cx + offsetx >= virtw || cy + offsety >= virth) return; \
                loopinchildrenrev(o, cx + offsetx, cy + offsety, \
                { \
                    o->func(ox, oy); \
                    childstate |= (o->state | o->childstate) & (flags); \
                }, ); \
                if(target(cx, cy)) state |= (flags); \
            }
        DOSTATES
        #undef DOSTATE

        void draw(float sx, float sy)
        {
            if((clipw && virtw > clipw) || (cliph && virth > cliph))
            {
                pushclip(sx, sy, w, h);
                Object::draw(sx - offsetx, sy - offsety);
                popclip();
            }
            else Object::draw(sx, sy);
        }

        float hlimit() const { return max(virtw - w, 0.0f); }
        float vlimit() const { return max(virth - h, 0.0f); }
        float hoffset() const { return offsetx / max(virtw, w); }
        float voffset() const { return offsety / max(virth, h); }
        float hscale() const { return w / max(virtw, w); }
        float vscale() const { return h / max(virth, h); }

        void addhscroll(float hscroll) { sethscroll(offsetx + hscroll); }
        void addvscroll(float vscroll) { setvscroll(offsety + vscroll); }
        void sethscroll(float hscroll) { offsetx = clamp(hscroll, 0.0f, hlimit()); }
        void setvscroll(float vscroll) { offsety = clamp(vscroll, 0.0f, vlimit()); }
    };

    struct ScrollButton : Object
    {
        static const char *typestr() { return "#ScrollButton"; }
        const char *gettype() const { return typestr(); }
    };

    struct ScrollBar : Object
    {
        float offsetx, offsety;

        ScrollBar() : offsetx(0), offsety(0) {}

        static const char *typestr() { return "#ScrollBar"; }
        const char *gettype() const { return typestr(); }
        const char *gettypename() const { return typestr(); }

        bool target(float cx, float cy)
        {
            return true;
        }

        virtual void scrollto(float cx, float cy) {}

        void hold(float cx, float cy)
        {
            Object::hold(cx, cy);
            ScrollButton *button = (ScrollButton *)find(ScrollButton::typestr(), false);
            if(button && button->haschildstate(STATE_HOLD)) movebutton(button, offsetx, offsety, cx - button->x, cy - button->y);
        }

        void press(float cx, float cy)
        {
            Object::press(cx, cy);
            ScrollButton *button = (ScrollButton *)find(ScrollButton::typestr(), false);
            if(button && button->haschildstate(STATE_PRESS)) { offsetx = cx - button->x; offsety = cy - button->y; }
            else scrollto(cx, cy);
        }

        virtual void arrowscroll(float dir) = 0;

        virtual void movebutton(Object *o, float fromx, float fromy, float tox, float toy) = 0;
    };

    struct ScrollArrow : Object
    {
        float arrowspeed;

        void setup(float arrowspeed_ = 0)
        {
            Object::setup();
            arrowspeed = arrowspeed_;
        }

        static const char *typestr() { return "#ScrollArrow"; }
        const char *gettype() const { return typestr(); }

        void hold(float cx, float cy)
        {
            Object::hold(cx, cy);
            ScrollBar *scrollbar = (ScrollBar *)findsibling(ScrollBar::typestr());
            if(scrollbar) scrollbar->arrowscroll(arrowspeed);
        }
    };

    struct HorizontalScrollBar : ScrollBar
    {
        static const char *typestr() { return "#HorizontalScrollBar"; }
        const char *gettype() const { return typestr(); }

        void arrowscroll(float dir)
        {
            Scroller *scroller = (Scroller *)findsibling(Scroller::typestr());
            if(!scroller) return;
            scroller->addhscroll(dir*curtime/1000.0f);
        }

        void scrollto(float cx, float cy)
        {
            Scroller *scroller = (Scroller *)findsibling(Scroller::typestr());
            if(!scroller) return;
            ScrollButton *button = (ScrollButton *)find(ScrollButton::typestr(), false);
            if(!button) return;
            float bscale = (w - button->w) / (1 - scroller->hscale()),
                  offset = bscale > 1e-3f ? cx/bscale : 0;
            scroller->sethscroll(offset*scroller->virtw);
        }

        void adjustchildren()
        {
            Scroller *scroller = (Scroller *)findsibling(Scroller::typestr());
            if(!scroller) return;
            ScrollButton *button = (ScrollButton *)find(ScrollButton::typestr(), false);
            if(!button) return;
            float bw = w*scroller->hscale();
            button->w = max(button->w, bw);
            float bscale = scroller->hscale() < 1 ? (w - button->w) / (1 - scroller->hscale()) : 1;
            button->x = scroller->hoffset()*bscale;
            button->adjust &= ~ALIGN_HMASK;

            ScrollBar::adjustchildren();
        }

        void movebutton(Object *o, float fromx, float fromy, float tox, float toy)
        {
            scrollto(o->x + tox - fromx, o->y + toy);
        }
    };

    struct VerticalScrollBar : ScrollBar
    {
        static const char *typestr() { return "#VerticalScrollBar"; }
        const char *gettype() const { return typestr(); }

        void arrowscroll(float dir)
        {
            Scroller *scroller = (Scroller *)findsibling(Scroller::typestr());
            if(!scroller) return;
            scroller->addvscroll(dir*curtime/1000.0f);
        }

        void scrollto(float cx, float cy)
        {
            Scroller *scroller = (Scroller *)findsibling(Scroller::typestr());
            if(!scroller) return;
            ScrollButton *button = (ScrollButton *)find(ScrollButton::typestr(), false);
            if(!button) return;
            float bscale = (h - button->h) / (1 - scroller->vscale()),
                  offset = bscale > 1e-3f ? cy/bscale : 0;
            scroller->setvscroll(offset*scroller->virth);
        }

        void adjustchildren()
        {
            Scroller *scroller = (Scroller *)findsibling(Scroller::typestr());
            if(!scroller) return;
            ScrollButton *button = (ScrollButton *)find(ScrollButton::typestr(), false);
            if(!button) return;
            float bh = h*scroller->vscale();
            button->h = max(button->h, bh);
            float bscale = scroller->vscale() < 1 ? (h - button->h) / (1 - scroller->vscale()) : 1;
            button->y = scroller->voffset()*bscale;
            button->adjust &= ~ALIGN_VMASK;

            ScrollBar::adjustchildren();
        }

        void movebutton(Object *o, float fromx, float fromy, float tox, float toy)
        {
            scrollto(o->x + tox, o->y + toy - fromy);
        }
    };

    struct SliderButton : Object
    {
        static const char *typestr() { return "#SliderButton"; }
        const char *gettype() const { return typestr(); }
    };

    static double getval(ident *id, double val = 0)
    {
        switch(id->type)
        {
            case ID_VAR: val = *id->storage.i; break;
            case ID_FVAR: val = *id->storage.f; break;
            case ID_SVAR: val = parsefloat(*id->storage.s); break;
            case ID_ALIAS: val = id->getfloat(); break;
        }
        return val;
    }

    static void setval(ident *id, double val, uint *onchange = NULL)
    {
        switch(id->type)
        {
            case ID_VAR: setvarchecked(id, int(clamp(val, double(INT_MIN), double(INT_MAX)))); break;
            case ID_FVAR: setfvarchecked(id, val); break;
            case ID_SVAR: setsvarchecked(id, floatstr(val)); break;
            case ID_ALIAS: alias(id->name, floatstr(val)); break;
        }
        if(onchange && (*onchange&CODE_OP_MASK) != CODE_EXIT) execute(onchange);
    }

    struct Slider : Object
    {
        ident *id;
        double val, vmin, vmax, vstep;
        bool changed;

        Slider() : id(NULL), val(0), vmin(0), vmax(0), vstep(0), changed(false) {}

        void setup(ident *id_, double vmin_ = 0, double vmax_ = 0, double vstep_ = 1, uint *onchange = NULL)
        {
            Object::setup();
            if(!vmin_ && !vmax_) switch(id_->type)
            {
                case ID_VAR: vmin_ = id_->minval; vmax_ = id_->maxval; break;
                case ID_FVAR: vmin_ = id_->minvalf; vmax_ = id_->maxvalf; break;
            }
            if(id != id_) changed = false;
            id = id_;
            vmin = vmin_;
            vmax = vmax_;
            vstep = vstep_ > 0 ? vstep_ : 1;
            if(changed)
            {
                setval(id, val, onchange);
                changed = false;
            }
            else val = getval(id, vmin);
        }

        static const char *typestr() { return "#Slider"; }
        const char *gettype() const { return typestr(); }
        const char *gettypename() const { return typestr(); }

        bool target(float cx, float cy)
        {
            return true;
        }

        void arrowscroll(float dir)
        {
            double newval = clamp(val + dir*vstep, min(vmin, vmax), max(vmin, vmax));
            if(val != newval) changeval(newval);
        }

        virtual void scrollto(float cx, float cy) {}

        void hold(float cx, float cy)
        {
            Object::hold(cx, cy);
            scrollto(cx, cy);
        }

        void changeval(double newval)
        {
            val = newval;
            changed = true;
        }
    };

    struct SliderArrow : Object
    {
        float stepdir;
        int laststep, steptime;

        SliderArrow() : laststep(0) {}

        void setup(float dir_ = 0, int steptime_ = 0)
        {
            Object::setup();
            stepdir = dir_;
            steptime = steptime_;
        }

        static const char *typestr() { return "#SliderArrow"; }
        const char *gettype() const { return typestr(); }

        void press(float cx, float cy)
        {
            Object::press(cx, cy);

            laststep = totalmillis + 2*steptime;

            Slider *slider = (Slider *)findsibling(Slider::typestr());
            if(slider) slider->arrowscroll(stepdir);
        }

        void hold(float cx, float cy)
        {
            Object::hold(cx, cy);

            if(totalmillis < laststep + steptime)
                return;
            laststep = totalmillis;

            Slider *slider = (Slider *)findsibling(Slider::typestr());
            if(slider) slider->arrowscroll(stepdir);
        }
    };

    struct HorizontalSlider : Slider
    {
        static const char *typestr() { return "#HorizontalSlider"; }
        const char *gettype() const { return typestr(); }

        void scrollto(float cx, float cy)
        {
            SliderButton *button = (SliderButton *)find(SliderButton::typestr(), false);
            if(!button) return;
            float offset = w > button->w ? clamp((cx - button->w/2)/(w - button->w), 0.0f, 1.0f) : 0.0f;
            int step = int((val - vmin) / vstep),
                bstep = int(offset * (vmax - vmin) / vstep);
            if(step != bstep) changeval(bstep * vstep + vmin);
        }

        void adjustchildren()
        {
            SliderButton *button = (SliderButton *)find(SliderButton::typestr(), false);
            if(!button) return;
            int step = int((val - vmin) / vstep),
                bstep = int(button->x / (w - button->w) * (vmax - vmin) / vstep);
            if(step != bstep) button->x = (w - button->w) * step * vstep / (vmax - vmin);
            button->adjust &= ~ALIGN_HMASK;

            Slider::adjustchildren();
        }
    };

    struct VerticalSlider : Slider
    {
        static const char *typestr() { return "#VerticalSlider"; }
        const char *gettype() const { return typestr(); }

        void scrollto(float cx, float cy)
        {
            SliderButton *button = (SliderButton *)find(SliderButton::typestr(), false);
            if(!button) return;
            float offset = h > button->h ? clamp((cx - button->h/2)/(h - button->h), 0.0f, 1.0f) : 0.0f;
            int step = int((val - vmin) / vstep),
                bstep = int(offset * (vmax - vmin) / vstep);
            if(step != bstep) changeval(bstep * vstep + vmin);
        }

        void adjustchildren()
        {
            SliderButton *button = (SliderButton *)find(SliderButton::typestr(), false);
            if(!button) return;
            int step = int((val - vmin) / vstep),
                bstep = int(button->y / (h - button->h) * (vmax - vmin) / vstep);
            if(step != bstep) button->y = (h - button->h) * step * vstep / (vmax - vmin);
            button->adjust &= ~ALIGN_VMASK;

            Slider::adjustchildren();
        }
    };

    struct ModelPreview : Filler
    {
        char *name;
        int anim;

        ModelPreview() : name(NULL) {}
        ~ModelPreview() { delete[] name; }

        void setup(const char *name_, const char *animspec, float minw_, float minh_)
        {
            Filler::setup(minw_, minh_);
            SETSTR(name, name_);

            anim = ANIM_ALL;
            if(animspec[0])
            {
                if(isdigit(animspec[0]))
                {
                    anim = parseint(animspec);
                    if(anim >= 0) anim %= ANIM_INDEX;
                    else anim = ANIM_ALL;
                }
                else
                {
                    vector<int> anims;
                    game::findanims(animspec, anims);
                    if(anims.length()) anim = anims[0];
                }
            }
            anim |= ANIM_LOOP;
        }

        static const char *typestr() { return "#ModelPreview"; }
        const char *gettype() const { return typestr(); }

        void draw(float sx, float sy)
        {
            if(clipstack.length()) glDisable(GL_SCISSOR_TEST);
            int sx1 = clamp(int(floor((sx-world->x)/world->w*screenw)), 0, screenw),
                sy1 = clamp(int(floor(screenh - (sy-world->y)/world->h*screenh)), 0, screenh),
                sx2 = clamp(int(ceil((sx+w-world->x)/world->w*screenw)), 0, screenw),
                sy2 = clamp(int(ceil(screenh - (sy+h-world->y)/world->h*screenh)), 0, screenh);
            glDisable(GL_BLEND);
            gle::disable();
            modelpreview::start(sx1, sy1, sx2-sx1, sy2-sy1, true, clipstack.length() > 0);
            model *m = loadmodel(name);
            if(m)
            {
                vec center, radius;
                m->boundbox(center, radius);
                float dist =  2.0f*max(radius.magnitude2(), 1.1f*radius.z),
                      yaw = fmod(lastmillis/10000.0f*360.0f, 360.0f);
                vec o(-center.x, dist - center.y, -0.1f*dist - center.z);
                rendermodel(name, anim, o, yaw, 0, 0, 0, NULL, NULL, 0);
            }
            if(clipstack.length()) clipstack.last().scissor();
            modelpreview::end();
            hudshader->set();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
            if(clipstack.length()) glEnable(GL_SCISSOR_TEST);

            Object::draw(sx, sy);
        }
    };

    struct PlayerPreview : Filler
    {
        int model, team, weapon;

        void setup(int model_, int team_, int weapon_, float minw_, float minh_)
        {
            Filler::setup(minw_, minh_);
            model = model_;
            team = team_;
            weapon = weapon_;
        }

        static const char *typestr() { return "#PlayerPreview"; }
        const char *gettype() const { return typestr(); }

        void draw(float sx, float sy)
        {
            if(clipstack.length()) glDisable(GL_SCISSOR_TEST);
            int sx1 = clamp(int(floor((sx-world->x)/world->w*screenw)), 0, screenw),
                sy1 = clamp(int(floor(screenh - (sy+h-world->y)/world->h*screenh)), 0, screenh),
                sx2 = clamp(int(ceil((sx+w-world->x)/world->w*screenw)), 0, screenw),
                sy2 = clamp(int(ceil(screenh - (sy-world->y)/world->h*screenh)), 0, screenh);
            glDisable(GL_BLEND);
            gle::disable();
            modelpreview::start(sx1, sy1, sx2-sx1, sy2-sy1, true, clipstack.length() > 0);
            game::renderplayerpreview(model, team, weapon);
            if(clipstack.length()) clipstack.last().scissor();
            modelpreview::end();
            hudshader->set();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
            if(clipstack.length()) glEnable(GL_SCISSOR_TEST);

            Object::draw(sx, sy);
        }
    };

    VARP(uislotviewtime, 0, 25, 1000);
    static int lastthumbnail = 0;

    struct SlotViewer : Filler
    {
        int index;

        void setup(int index_, float minw_ = 0, float minh_ = 0)
        {
            Filler::setup(minw_, minh_);
            index = index_;
        }

        static const char *typestr() { return "#SlotViewer"; }
        const char *gettype() const { return typestr(); }

        void previewslot(VSlot &vslot, float x, float y)
        {
            Slot &slot = *vslot.slot;
            if(slot.sts.empty()) return;
            VSlot *layer = NULL, *decal = NULL;
            Texture *t = NULL, *glowtex = NULL, *layertex = NULL, *decaltex = NULL;
            if(slot.loaded)
            {
                t = slot.sts[0].t;
                if(t == notexture) return;
                Slot &slot = *vslot.slot;
                if(slot.texmask&(1<<TEX_GLOW)) { loopvj(slot.sts) if(slot.sts[j].type==TEX_GLOW) { glowtex = slot.sts[j].t; break; } }
                if(vslot.layer)
                {
                    layer = &lookupvslot(vslot.layer);
                    if(!layer->slot->sts.empty()) layertex = layer->slot->sts[0].t;
                }
                if(vslot.decal)
                {
                    decal = &lookupvslot(vslot.decal);
                    if(!decal->slot->sts.empty()) decaltex = decal->slot->sts[0].t;
                }
            }
            else
            {
                if(!slot.thumbnail)
                {
                    if(totalmillis - lastthumbnail < uislotviewtime) return;
                    loadthumbnail(slot);
                    lastthumbnail = totalmillis;
                }
                if(slot.thumbnail != notexture) t = slot.thumbnail;
                else return;
            }
            SETSHADER(hudrgb);
            vec2 tc[4] = { vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 1) };
            int xoff = vslot.offset.x, yoff = vslot.offset.y;
            if(vslot.rotation)
            {
                if((vslot.rotation&5) == 1) { swap(xoff, yoff); loopk(4) swap(tc[k].x, tc[k].y); }
                if(vslot.rotation >= 2 && vslot.rotation <= 4) { xoff *= -1; loopk(4) tc[k].x *= -1; }
                if(vslot.rotation <= 2 || vslot.rotation == 5) { yoff *= -1; loopk(4) tc[k].y *= -1; }
            }
            float xt = min(1.0f, t->xs/float(t->ys)), yt = min(1.0f, t->ys/float(t->xs));
            loopk(4) { tc[k].x = tc[k].x/xt - float(xoff)/t->xs; tc[k].y = tc[k].y/yt - float(yoff)/t->ys; }
            glBindTexture(GL_TEXTURE_2D, t->id);
            if(slot.loaded) gle::color(vslot.colorscale);
            quad(x, y, w, h, tc);
            if(decaltex)
            {
                glBindTexture(GL_TEXTURE_2D, decaltex->id);
                quad(x + w/2, y + h/2, w/2, h/2, tc);
            }
            if(glowtex)
            {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glBindTexture(GL_TEXTURE_2D, glowtex->id);
                gle::color(vslot.glowcolor);
                quad(x, y, w, h, tc);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            if(layertex)
            {
                glBindTexture(GL_TEXTURE_2D, layertex->id);
                gle::color(layer->colorscale);
                quad(x, y, w/2, h/2, tc);
            }
            gle::colorf(1, 1, 1);
            hudshader->set();
        }

        void draw(float sx, float sy)
        {
            previewslot(lookupvslot(index, false), sx, sy);
            Object::draw(sx, sy);
        }
    };

    ICOMMAND(newui, "ssss", (char *name, char *contents, char *onshow, char *onhide),
    {
        Window *window = windows.find(name, NULL);
        if(window) { world->hide(window); windows.remove(name); delete window; }
        windows[name] = new Window(name, contents, onshow, onhide);
    });

    bool showui(const char *name)
    {
        Window *window = windows.find(name, NULL);
        return window && world->show(window);
    }

    bool hideui(const char *name)
    {
        Window *window = windows.find(name, NULL);
        return window && world->hide(window);
    }

    ICOMMAND(showui, "s", (char *name), intret(showui(name) ? 1 : 0));
    ICOMMAND(hideui, "s", (char *name), intret(hideui(name) ? 1 : 0));

    #define DOSTATE(flags, func) \
        ICOMMANDNS("ui!" #func, uinot##func##_, "ee", (uint *t, uint *f), \
            executeret(buildparent && buildparent->hasstate(flags) ? t : f)); \
        ICOMMANDNS("ui" #func, ui##func##_, "ee", (uint *t, uint *f), \
            executeret(buildparent && buildparent->haschildstate(flags) ? t : f)); \
        ICOMMANDNS("ui!" #func "?", uinot##func##__, "tt", (tagval *t, tagval *f), \
            { if(buildparent && buildparent->hasstate(flags)) { if(t->type == VAL_NULL) intret(1); else result(*t); } else if(f->type == VAL_NULL) intret(0); else result(*f); }); \
        ICOMMANDNS("ui" #func "?", ui##func##__, "tt", (tagval *t, tagval *f), \
            { if(buildparent && buildparent->haschildstate(flags)) { if(t->type == VAL_NULL) intret(1); else result(*t); } else if(f->type == VAL_NULL) intret(0); else result(*f); }); \
        ICOMMANDNS("ui!" #func "+", uichildnot##func##_, "ee", (uint *t, uint *f), \
            executeret(buildparent && buildparent->children.inrange(buildchild) && buildparent->children[buildchild]->hasstate(flags) ? t : f)); \
        ICOMMANDNS("ui" #func "+", uichild##func##_, "ee", (uint *t, uint *f), \
            executeret(buildparent && buildparent->children.inrange(buildchild) && buildparent->children[buildchild]->haschildstate(flags) ? t : f)); \
        ICOMMANDNS("ui!" #func "+?", uichildnot##func##__, "tt", (tagval *t, tagval *f), \
            { if(buildparent && buildparent->children.inrange(buildchild) && buildparent->children[buildchild]->hasstate(flags)) { if(t->type == VAL_NULL) intret(1); else result(*t); } else if(f->type == VAL_NULL) intret(0); else result(*f); }); \
        ICOMMANDNS("ui" #func "+?", uichild##func##__, "tt", (tagval *t, tagval *f), \
            { if(buildparent && buildparent->children.inrange(buildchild) && buildparent->children[buildchild]->haschildstate(flags)) { if(t->type == VAL_NULL) intret(1); else result(*t); } else if(f->type == VAL_NULL) intret(0); else result(*f); });
    DOSTATES
    #undef DOSTATE

    ICOMMAND(uialign, "ii", (int *xalign, int *yalign),
    {
        if(buildparent) buildparent->setalign(*xalign, *yalign);
    });
    ICOMMANDNS("uialign-", uialign_, "ii", (int *xalign, int *yalign),
    {
        if(buildparent && buildchild > 0) buildparent->children[buildchild-1]->setalign(*xalign, *yalign);
    });

    ICOMMAND(uiclamp, "iiii", (int *left, int *right, int *bottom, int *top),
    {
        if(buildparent) buildparent->setclamp(*left, *right, *bottom, *top);
    });
    ICOMMANDNS("uiclamp-", uiclamp_, "iiii", (int *left, int *right, int *bottom, int *top),
    {
        if(buildparent && buildchild > 0) buildparent->children[buildchild-1]->setclamp(*left, *right, *bottom, *top);
    });

    ICOMMAND(uigroup, "e", (uint *children),
        BUILD(Object, o, o->setup(), children));

    ICOMMAND(uihlist, "fe", (float *space, uint *children),
        BUILD(HorizontalList, o, o->setup(*space), children));

    ICOMMAND(uivlist, "fe", (float *space, uint *children),
        BUILD(VerticalList, o, o->setup(*space), children));

    ICOMMAND(uilist, "fe", (float *space, uint *children),
    {
        for(Object *parent = buildparent; parent && !parent->istype<VerticalList>(); parent = parent->parent)
        {
            if(parent->istype<HorizontalList>())
            {
                BUILD(VerticalList, o, o->setup(*space), children);
                return;
            }
        }
        BUILD(HorizontalList, o, o->setup(*space), children);
    });

    ICOMMAND(uitable, "ife", (int *columns, float *space, uint *children),
        BUILD(Table, o, o->setup(*columns, *space), children));

    ICOMMAND(uispace, "ffe", (float *spacew, float *spaceh, uint *children),
        BUILD(Spacer, o, o->setup(*spacew, *spaceh), children));

    ICOMMAND(uioffset, "ffe", (float *offsetx, float *offsety, uint *children),
        BUILD(Offsetter, o, o->setup(*offsetx, *offsety), children));

    ICOMMAND(uifill, "ffe", (float *minw, float *minh, uint *children),
        BUILD(Filler, o, o->setup(*minw, *minh), children));

    ICOMMAND(uiclip, "ffe", (float *clipw, float *cliph, uint *children),
        BUILD(Clipper, o, o->setup(*clipw, *cliph), children));

    ICOMMAND(uiscroll, "ffe", (float *clipw, float *cliph, uint *children),
        BUILD(Scroller, o, o->setup(*clipw, *cliph), children));

    ICOMMAND(uihscrollbar, "e", (uint *children),
        BUILD(HorizontalScrollBar, o, o->setup(), children));

    ICOMMAND(uivscrollbar, "e", (uint *children),
        BUILD(VerticalScrollBar, o, o->setup(), children));

    ICOMMAND(uiscrollarrow, "fe", (float *dir, uint *children),
        BUILD(ScrollArrow, o, o->setup(*dir), children));

    ICOMMAND(uiscrollbutton, "e", (uint *children),
        BUILD(ScrollButton, o, o->setup(), children));

    ICOMMAND(uihslider, "rfffee", (ident *var, float *vmin, float *vmax, float *vstep, uint *onchange, uint *children),
        BUILD(HorizontalSlider, o, o->setup(var, *vmin, *vmax, *vstep, onchange), children));

    ICOMMAND(uivslider, "rfffee", (ident *var, float *vmin, float *vmax, float *vstep, uint *onchange, uint *children),
        BUILD(VerticalSlider, o, o->setup(var, *vmin, *vmax, *vstep, onchange), children));

    ICOMMAND(uisliderarrow, "fie", (float *dir, int *time, uint *children),
        BUILD(SliderArrow, o, o->setup(*dir, *time), children));

    ICOMMAND(uisliderbutton, "e", (uint *children),
        BUILD(SliderButton, o, o->setup(), children));

    ICOMMAND(uicolor, "ffffffe", (float *r, float *g, float *b, float *a, float *minw, float *minh, uint *children),
        BUILD(FillColor, o, o->setup(FillColor::SOLID, vec4(*r, *g, *b, *a), *minw, *minh), children));

    ICOMMAND(uimodcolor, "fffffe", (float *r, float *g, float *b, float *minw, float *minh, uint *children),
        BUILD(FillColor, o, o->setup(FillColor::MODULATE, vec4(*r, *g, *b, 1), *minw, *minh), children));

    ICOMMAND(uivgradient, "ffffffffffe", (float *r, float *g, float *b, float *a, float *r2, float *g2, float *b2, float *a2, float *minw, float *minh, uint *children),
        BUILD(Gradient, o, o->setup(Gradient::SOLID, Gradient::VERTICAL, vec4(*r, *g, *b, *a), vec4(*r2, *g2, *b2, *a2), *minw, *minh), children));

    ICOMMAND(uimodvgradient, "ffffffffe", (float *r, float *g, float *b, float *r2, float *g2, float *b2, float *minw, float *minh, uint *children),
        BUILD(Gradient, o, o->setup(Gradient::MODULATE, Gradient::VERTICAL, vec4(*r, *g, *b, 1), vec4(*r2, *g2, *b2, 1), *minw, *minh), children));

    ICOMMAND(uihgradient, "ffffffffffe", (float *r, float *g, float *b, float *a, float *r2, float *g2, float *b2, float *a2, float *minw, float *minh, uint *children),
        BUILD(Gradient, o, o->setup(Gradient::SOLID, Gradient::HORIZONTAL, vec4(*r, *g, *b, *a), vec4(*r2, *g2, *b2, *a2), *minw, *minh), children));

    ICOMMAND(uimodhgradient, "ffffffffe", (float *r, float *g, float *b, float *r2, float *g2, float *b2, float *minw, float *minh, uint *children),
        BUILD(Gradient, o, o->setup(Gradient::MODULATE, Gradient::HORIZONTAL, vec4(*r, *g, *b, 1), vec4(*r2, *g2, *b2, 1), *minw, *minh), children));

    ICOMMAND(uioutline, "ffffffe", (float *thick, float *r, float *g, float *b, float *minw, float *minh, uint *children),
        BUILD(Outline, o, o->setup(*thick, vec4(*r, *g, *b, 1), *minw, *minh), children));

    ICOMMAND(uicolortext, "sffffe", (char *text, float *scale, float *r, float *g, float *b, uint *children),
        BUILD(Text, o, o->setup(text, *scale <= 0 ? 1 : *scale, *r, *g, *b), children));

    ICOMMAND(uitext, "sfe", (char *text, float *scale, uint *children),
        BUILD(Text, o, o->setup(text, *scale <= 0 ? 1 : *scale), children));

    ICOMMAND(uiimage, "sffe", (char *texname, float *minw, float *minh, uint *children),
        BUILD(Image, o, o->setup(textureload(texname, 3, true, false), *minw, *minh), children));

    ICOMMAND(uistretchedimage, "sffe", (char *texname, float *minw, float *minh, uint *children),
        BUILD(StretchedImage, o, o->setup(textureload(texname, 3, true, false), *minw, *minh), children));

    ICOMMAND(uicroppedimage, "sffsssse", (char *texname, float *minw, float *minh, char *cropx, char *cropy, char *cropw, char *croph, uint *children),
        BUILD(CroppedImage, o, {
            Texture *tex = textureload(texname, 3, true, false);
            o->setup(tex, *minw, *minh,
                strchr(cropx, 'p') ? atof(cropx) / tex->xs : atof(cropx),
                strchr(cropy, 'p') ? atof(cropy) / tex->ys : atof(cropy),
                strchr(cropw, 'p') ? atof(cropw) / tex->xs : atof(cropw),
                strchr(croph, 'p') ? atof(croph) / tex->ys : atof(croph));
        }, children));

    ICOMMAND(uiborderedimage, "ssfe", (char *texname, char *texborder, float *screenborder, uint *children),
        BUILD(BorderedImage, o, {
            Texture *tex = textureload(texname, 3, true, false);
            o->setup(tex,
                strchr(texborder, 'p') ? atof(texborder) / tex->xs : atof(texborder),
                *screenborder);
        }, children));

    ICOMMAND(uitiledimage, "sffffe", (char *texname, float *tilew, float *tileh, float *minw, float *minh, uint *children),
        BUILD(TiledImage, o, {
            Texture *tex = textureload(texname, 0, true, false);
            o->setup(tex, *minw, *minh, *tilew <= 0 ? 1 : *tilew, *tileh <= 0 ? 1 : *tileh);
        }, children));

    ICOMMAND(uimodelpreview, "ssffe", (char *model, char *animspec, float *minw, float *minh, uint *children),
        BUILD(ModelPreview, o, o->setup(model, animspec, *minw, *minh), children));

    ICOMMAND(uiplayerpreview, "iiiffe", (int *model, int *team, int *weapon, float *minw, float *minh, uint *children),
        BUILD(PlayerPreview, o, o->setup(*model, *team, *weapon, *minw, *minh), children));

    ICOMMAND(uislotview, "iffe", (int *index, float *minw, float *minh, uint *children),
        BUILD(SlotViewer, o, o->setup(*index, *minw, *minh), children));

    FVAR(uisensitivity, 1e-3f, 1, 1e3f);

    bool hascursor()
    {
        return world->children.length() > 0;
    }

    void getcursorpos(float &x, float &y)
    {
        if(hascursor())
        {
            x = cursorx;
            y = cursory;
        }
        else x = y = 0.5f;
    }

    void resetcursor()
    {
        cursorx = cursory = 0.5f;
    }

    bool movecursor(int &dx, int &dy)
    {
        if(!hascursor()) return false;
        float scale = 500.0f / uisensitivity;
        cursorx = clamp(cursorx+dx*(screenh/(screenw*scale)), 0.0f, 1.0f);
        cursory = clamp(cursory+dy/scale, 0.0f, 1.0f);
        return true;
    }

    bool keypress(int code, bool isdown)
    {
        int action = -1, hold = -1;
        switch(code)
        {
        case -1: action = isdown ? STATE_PRESS : STATE_RELEASE; hold = STATE_HOLD; break;
        case -2: action = isdown ? STATE_ALT_PRESS : STATE_ALT_RELEASE; hold = STATE_ALT_HOLD; break;
        case -3: action = isdown ? STATE_ESC_PRESS : STATE_ESC_RELEASE; hold = STATE_ESC_HOLD; break;
        case -4: action = STATE_SCROLL_UP; break;
        case -5: action = STATE_SCROLL_DOWN; break;
        }
        if(action >= 0)
        {
            if(isdown)
            {
                if(world->setstate(action, cursorx*world->w, cursory*world->h))
                {
                    if(hold >= 0) world->state |= hold;
                    return true;
                }
            }
            else if(hold < 0) return true;
            else if(world->state&hold)
            {
                world->setstate(action, cursorx*world->w, cursory*world->h);
                world->state &= ~hold;
                return true;
            }
            return false;
        }
        return world->key(code, isdown);
    }

    bool textinput(const char *str, int len)
    {
        return world->textinput(str, len);
    }

    void setup()
    {
        world = new World;
    }

    void cleanup()
    {
        world->children.setsize(0);
        enumerate(windows, Window *, w, delete w);
        windows.clear();
        DELETEP(world);
    }

    void update()
    {
        world->hover(cursorx*world->w, cursory*world->h);
        if(world->state&STATE_HOLD) world->hold(cursorx*world->w, cursory*world->h);
        if(world->state&STATE_ALT_HOLD) world->althold(cursorx*world->w, cursory*world->h);
        if(world->state&STATE_ESC_HOLD) world->eschold(cursorx*world->w, cursory*world->h);

        world->build();
        world->layout();
    }

    void render()
    {
        if(world->children.empty()) return;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        hudmatrix.ortho(world->x, world->x + world->w, world->y + world->h, world->y, -1, 1);
        resethudmatrix();
        hudshader->set();

        gle::colorf(1, 1, 1);

        world->draw();

        glDisable(GL_BLEND);

        gle::disable();
    }
}

