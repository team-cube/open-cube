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
        gle::attribf(x+w, y);   gle::attrib(tc[1]);
        gle::attribf(x,   y);   gle::attrib(tc[0]);
        gle::attribf(x+w, y+h); gle::attrib(tc[2]);
        gle::attribf(x,   y+h); gle::attrib(tc[3]);
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
        STATE_SCROLL_DOWN = 1<<11,
        STATE_HIDDEN      = 1<<12
    };

    struct Object;

    static Object *buildparent = NULL;
    static int buildchild = -1;

    #define BUILD(type, o, setup, contents) do { \
        type *o = buildparent->buildtype<type>(); \
        setup; \
        o->buildchildren(contents); \
    } while(0)

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

        virtual bool rawkey(int code, bool isdown)
        {
            loopchildrenrev(o,
            {
                if(o->rawkey(code, isdown)) return true;
            });
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

        void resetstate()
        {
            state = childstate = 0;
        }
        void resetchildstate()
        {
            resetstate();
            loopchildren(o, o->resetchildstate());
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
            #define DOSTATE(flags, func) case flags: func##children(cx, cy); return haschildstate(flags);
            DOSTATES
            #undef DOSTATE
            }
            return false;
        }

        #define DOSTATE(flags, func) \
            virtual void func##children(float cx, float cy) \
            { \
                loopinchildrenrev(o, cx, cy, \
                { \
                    o->func##children(ox, oy); \
                    childstate |= (o->state | o->childstate) & (flags); \
                }, ); \
                if(target(cx, cy)) state |= (flags); \
                func(cx, cy); \
            } \
            virtual void func(float cx, float cy) {}
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

        template<class T> T *buildtype()
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

        void buildchildren(uint *contents)
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

        virtual int childcolumns() const { return children.length(); }
    };

    struct Window;
    
    static Window *window = NULL;

    static float maxscale = 1;

    struct Window : Object
    {
        char *name;
        uint *contents, *onshow, *onhide;
        bool allowinput;
        float px, py, px2, py2;

        Window(const char *name, const char *contents, const char *onshow, const char *onhide, bool allowinput = true) :
            name(newstring(name)),
            contents(compilecode(contents)),
            onshow(onshow && onshow[0] ? compilecode(onshow) : NULL),
            onhide(onhide && onhide[0] ? compilecode(onhide) : NULL),
            allowinput(allowinput),
            px(0), py(0), px2(0), py2(0)
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
            if(onhide) execute(onhide);
        }

        void show()
        {
            state |= STATE_HIDDEN;
            if(onshow) execute(onshow);
        }

        void setup()
        {
            Object::setup();
            px = py = px2 = py2 = 0;
        }

        void layout()
        {
            if(state&STATE_HIDDEN) { w = h = 0; return; } 
            window = this;
            Object::layout();
            window = NULL;
        }

        void draw()
        {
            if(state&STATE_HIDDEN) return; 
            window = this;

            projection();
            resethudmatrix();
            hudshader->set();

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            gle::colorf(1, 1, 1);

            Object::draw(x, y);

            glDisable(GL_BLEND);

            window = NULL;
        }

        void adjustchildren()
        {
            if(state&STATE_HIDDEN) return;
            window = this;
            Object::adjustchildren();
            window = NULL;
        }

        void adjustlayout()
        {
            float aspect = float(screenw)/screenh,
                  sh = max(max(h, w/aspect), 1.0f),
                  sw = aspect*sh,
                  sx = 0.5f*(1 - sw),
                  sy = 0.5f*(1 - sh);
            Object::adjustlayout(sx, sy, sw, sh);
        }    
            
        #define DOSTATE(flags, func) \
            void func##children(float cx, float cy) \
            { \
                if(!allowinput || px >= px2 || py >= py2) return; \
                cx *= px2-px; cx += px - x; \
                cy *= py2-py; cy += py - y; \
                Object::func##children(cx, cy); \
            }
        DOSTATES
        #undef DOSTATE

        void escrelease(float cx, float cy);

        void projection()
        {
            float aspect = float(screenw)/screenh,
                  sh = max(max(h, w/aspect), 1.0f),
                  sw = aspect*sh,
                  sx = 0.5f*(1 - sw),
                  sy = 0.5f*(1 - sh),
                  scale = max((y + h)/(sh*maxscale), 1.0f);
            px = (sx - 0.5f)*scale + 0.5f;
            px2 = (sx + sw - 0.5f)*scale + 0.5f;
            py = (sy - 0.5f)*scale + 0.5f;
            py2 = (sy + sh - 0.5f)*scale + 0.5f;
            hudmatrix.ortho(px, px2, py2, py, -1, 1);
        }

        void calcscissor(float x1, float y1, float x2, float y2, int &sx1, int &sy1, int &sx2, int &sy2)
        {
            sx1 = clamp(int(floor((x1-px)/(px2-px)*screenw)), 0, screenw);
            sy1 = clamp(int(floor(screenh - (y2-py)/(py2-py)*screenh)), 0, screenh);
            sx2 = clamp(int(ceil((x2-px)/(px2-px)*screenw)), 0, screenw);
            sy2 = clamp(int(ceil(screenh - (y1-py)/(py2-py)*screenh)), 0, screenh);
        }
    };

    static inline bool htcmp(const char *key, const Window *w) { return !strcmp(key, w->name); }

    hashset<Window *> windows;

    void ClipArea::scissor()
    {
        int sx1, sy1, sx2, sy2;
        window->calcscissor(x1, y1, x2, y2, sx1, sy1, sx2, sy2);
        glScissor(sx1, sy1, sx2-sx1, sy2-sy1);
    }

    struct World : Object
    {
        static const char *typestr() { return "#World"; }
        const char *gettype() const { return typestr(); }
        const char *getname() const { return gettype(); }

        #define loopwindows(o, body) do { \
            loopv(children) \
            { \
                Window *o = (Window *)children[i]; \
                body; \
            } \
        } while(0)

        #define loopwindowsrev(o, body) do { \
            loopvrev(children) \
            { \
                Window *o = (Window *)children[i]; \
                body; \
            } \
        } while(0)

        void adjustchildren()
        {
            loopwindows(w, w->adjustlayout());
        }

        #define DOSTATE(flags, func) \
            void func##children(float cx, float cy) \
            { \
                loopwindowsrev(w, \
                { \
                    w->func##children(cx, cy); \
                    int wflags = (w->state | w->childstate) & (flags); \
                    if(wflags) { childstate |= wflags; break; } \
                }); \
            }
        DOSTATES
        #undef DOSTATE

        void build()
        {
            reset();
            setup();
            loopwindows(w,
            {
                w->build();
                if(!children.inrange(i)) break;
                if(children[i] != w) i--;
            });
        }

        bool show(Window *w)
        {
            if(children.find(w) >= 0) return false;
            w->resetchildstate();
            children.add(w);
            w->show();
            return true;
        }

        void hide(Window *w, int index)
        {
            children.remove(index);
            childstate = 0;
            loopchildren(o, childstate |= o->state | o->childstate);
            w->hide();
        }

        bool hide(Window *w)
        {
            int index = children.find(w);
            if(index < 0) return false;
            hide(w, index);
            return true;
        }

        int hideall()
        {
            int hidden = 0;
            loopwindowsrev(w,
            {
                hide(w, i);
                hidden++;
            });
            return hidden;
        }

        bool allowinput() const { loopwindows(w, { if(w->allowinput) return true; }); return false; }

        void draw()
        {
            if(children.empty()) return;

            loopwindows(w, w->draw());

            gle::disable();
        }
    };

    static World *world = NULL;

    void Window::escrelease(float cx, float cy)
    {
        world->hide(this);
    }

    void Window::build()
    {
        reset(world);
        setup();
        buildchildren(contents);
    }

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

    struct Grid : Object
    {
        int columns;
        float space;
        vector<float> widths, heights;

        static const char *typestr() { return "#Grid"; }
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

    struct TableHeader : Object
    {
        int columns;

        TableHeader() : columns(0) {}

        static const char *typestr() { return "#TableHeader"; }
        const char *gettype() const { return typestr(); }

        int childcolumns() const { return columns; }

        void buildchildren(uint *columndata, uint *contents)
        {
            Object *oldparent = buildparent;
            int oldchild = buildchild;
            buildparent = this;
            buildchild = 0;
            execute(columndata);
            if(columns != buildchild) while(children.length() > buildchild) delete children.pop();
            columns = buildchild;
            if((*contents&CODE_OP_MASK) != CODE_EXIT) execute(contents);
            while(children.length() > buildchild) delete children.pop();
            buildparent = oldparent;
            buildchild = oldchild;
            resetstate();
        }

        void adjustchildren()
        {
            for(int i = columns; i < children.length(); i++) children[i]->adjustlayout(0, 0, w, h);
        }

        void draw(float sx, float sy)
        {
            for(int i = columns; i < children.length(); i++)
            {
                Object *o = children[i];
                if(!isfullyclipped(sx + o->x, sy + o->y, o->w, o->h))
                    o->draw(sx + o->x, sy + o->y);
            }
            loopi(columns)
            {
                Object *o = children[i];
                if(!isfullyclipped(sx + o->x, sy + o->y, o->w, o->h))
                    o->draw(sx + o->x, sy + o->y);
            }
        }
    };

    struct TableRow : TableHeader
    {
        static const char *typestr() { return "#TableRow"; }
        const char *gettype() const { return typestr(); }

        bool target(float cx, float cy)
        {
            return true;
        }
    };

    #define BUILDCOLUMNS(type, o, setup, columndata, contents) do { \
        type *o = buildparent->buildtype<type>(); \
        setup; \
        o->buildchildren(columndata, contents); \
    } while(0)

    struct Table : Object
    {
        float space;
        vector<float> widths;

        static const char *typestr() { return "#Table"; }
        const char *gettype() const { return typestr(); }

        void setup(float space_ = 0)
        {
            Object::setup();
            space = space_;
        }

        void layout()
        {
            widths.setsize(0);

            w = h = 0;
            loopchildren(o,
            {
                o->layout();
                int cols = o->childcolumns();
                while(widths.length() <= cols) widths.add(0);
                loopj(cols)
                {
                    Object *c = o->children[j];
                    if(c->w > widths[j]) widths[j] = c->w;
                }
                w = max(w, o->w);
            });
            float rw = 0;
            loopv(widths) rw += widths[i];
            rw += space*max(widths.length() - 1, 0);
            w = max(w, rw);
            loopchildren(o,
            {
                o->x = 0;
                o->y = h;
                o->w = max(o->w, rw);
                float offset = 0;
                int cols = o->childcolumns();
                loopj(cols)
                {
                    Object *c = o->children[j];
                    c->x = offset;
                    c->adjustlayout(c->x, c->y, widths[j], o->h);
                    offset += widths[j];
                }
                h += o->h;
            });

            h += space*max(children.length() - 1, 0);
        }

        void adjustchildren()
        {
            if(children.empty()) return;

            float cspace = w, rspace = h;
            loopv(widths) cspace -= widths[i];
            loopchildren(o, rspace -= o->h);
            cspace /= max(widths.length() - 1, 1);
            rspace /= max(children.length() - 1, 1);

            float offsety = 0;
            loopchildren(o,
            {
                o->x = 0;
                o->y = offsety;
                o->w = w;
                o->adjustlayout(0, o->y, w, o->h);
                float offsetx = 0;
                int cols = o->childcolumns();
                loopj(cols)
                {
                    Object *c = o->children[j];
                    c->x = offsetx;
                    c->adjustlayout(c->x, c->y, widths[j], o->h);
                    offsetx += widths[j] + cspace;
                }
                offsety += o->h + rspace;
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

    struct Color
    {
        uchar r, g, b, a;
        
        Color() {}
        Color(uint c) : r((c>>16)&0xFF), g((c>>8)&0xFF), b(c&0xFF), a(c>>24 ? c>>24 : 0xFF) {}
        Color(uint c, uchar a) : r((c>>16)&0xFF), g((c>>8)&0xFF), b(c&0xFF), a(a) {}
        Color(uchar r, uchar g, uchar b, uchar a = 255) : r(r), g(g), b(b), a(a) {}
 
        void init() { gle::colorub(r, g, b, a); }
        void attrib() { gle::attribub(r, g, b, a); }
        
        static void def() { gle::defcolor(4, GL_UNSIGNED_BYTE); }
    };

    struct FillColor : Filler
    {
        enum { SOLID = 0, MODULATE };

        int type;
        Color color;

        void setup(int type_, const Color &color_, float minw_ = 0, float minh_ = 0)
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
            color.init();
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
        Color color2;

        void setup(int type_, int dir_, const Color &color_, const Color &color2_, float minw_ = 0, float minh_ = 0)
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
            Color::def();
            gle::begin(GL_TRIANGLE_STRIP);
            gle::attribf(sx+w, sy);   (dir == HORIZONTAL ? color2 : color).attrib();
            gle::attribf(sx,   sy);   color.attrib();
            gle::attribf(sx+w, sy+h); color2.attrib();
            gle::attribf(sx,   sy+h); (dir == HORIZONTAL ? color : color2).attrib();
            gle::end();
            hudshader->set();
            if(type==MODULATE) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            Object::draw(sx, sy);
        }
    };

    struct Line : Filler
    {
        Color color;

        void setup(const Color &color_, float minw_ = 0, float minh_ = 0)
        {
            Filler::setup(minw_, minh_);
            color = color_;
        }

        static const char *typestr() { return "#Line"; }
        const char *gettype() const { return typestr(); }

        void draw(float sx, float sy)
        {
            hudnotextureshader->set();
            color.init();
            gle::defvertex(2);
            gle::begin(GL_LINES);
            gle::attribf(sx,   sy);
            gle::attribf(sx+w, sy+h);
            gle::end();
            gle::colorf(1, 1, 1);
            hudshader->set();

            Object::draw(sx, sy);
        }
    };

    struct Outline : Filler
    {
        Color color;

        void setup(const Color &color_, float minw_ = 0, float minh_ = 0)
        {
            Filler::setup(minw_, minh_);
            color = color_;
        }

        static const char *typestr() { return "#Outline"; }
        const char *gettype() const { return typestr(); }

        void draw(float sx, float sy)
        {
            hudnotextureshader->set();
            color.init();
            gle::defvertex(2);
            gle::begin(GL_LINE_LOOP);
            gle::attribf(sx,   sy);
            gle::attribf(sx+w, sy);
            gle::attribf(sx+w, sy+h);
            gle::attribf(sx,   sy+h);
            gle::end();
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
        float scale;
        Color color;

        void setup(float scale_ = 1, const Color &color_ = Color(255, 255, 255))
        {
            Object::setup();

            scale = scale_;
            color = color_;
        }

        static const char *typestr() { return "#Text"; }
        const char *gettype() const { return typestr(); }

        float drawscale() const { return scale / (FONTH * uitextrows); }

        virtual const char *getstr() const { return ""; }

        void draw(float sx, float sy)
        {
            float oldscale = textscale;
            textscale = drawscale();
            draw_text(getstr(), sx/textscale, sy/textscale, color.r, color.g, color.b, color.a);
            textscale = oldscale;

            Object::draw(sx, sy);
        }

        void layout()
        {
            Object::layout();

            float k = drawscale(), tw, th;
            text_boundsf(getstr(), tw, th);
            w = max(w, tw*k);
            h = max(h, th*k);
        }
    };

    struct TextString : Text
    {
        char *str;

        TextString() : str(NULL) {}
        ~TextString() { delete[] str; }

        void setup(const char *str_, float scale_ = 1, const Color &color_ = Color(255, 255, 255))
        {
            Text::setup(scale_, color_);

            SETSTR(str, str_);
        }

        static const char *typestr() { return "#TextString"; }
        const char *gettype() const { return typestr(); }

        const char *getstr() const { return str; }
    };

    struct TextInt : Text
    {
        int val;
        char str[20];

        TextInt() : val(0) { str[0] = '0'; str[1] = '\0'; }

        void setup(int val_, float scale_ = 1, const Color &color_ = Color(255, 255, 255))
        {
            Text::setup(scale_, color_);

            if(val != val_) { val = val_; intformat(str, val, sizeof(str)); }
        }

        static const char *typestr() { return "#TextInt"; }
        const char *gettype() const { return typestr(); }

        const char *getstr() const { return str; }
    };

    struct TextFloat : Text
    {
        float val;
        char str[20];

        TextFloat() { memset(&val, -1, sizeof(val)); str[0] = '\0'; }

        void setup(float val_, float scale_ = 1, const Color &color_ = Color(255, 255, 255))
        {
            Text::setup(scale_, color_);

            if(val != val_) { val = val_; intformat(str, val, sizeof(str)); }
        }

        static const char *typestr() { return "#TextFloat"; }
        const char *gettype() const { return typestr(); }

        const char *getstr() const { return str; }
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

        #define DOSTATE(flags, func) \
            void func##children(float cx, float cy) \
            { \
                cx += offsetx; \
                cy += offsety; \
                if(cx < virtw && cy < virth) Clipper::func##children(cx, cy); \
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

        void scrollup(float cx, float cy);
        void scrolldown(float cx, float cy);
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
            ScrollButton *button = (ScrollButton *)find(ScrollButton::typestr(), false);
            if(button && button->haschildstate(STATE_HOLD)) movebutton(button, offsetx, offsety, cx - button->x, cy - button->y);
        }

        void press(float cx, float cy)
        {
            ScrollButton *button = (ScrollButton *)find(ScrollButton::typestr(), false);
            if(button && button->haschildstate(STATE_PRESS)) { offsetx = cx - button->x; offsety = cy - button->y; }
            else scrollto(cx, cy);
        }

        virtual void addscroll(Scroller *scroller, float dir) = 0;

        void addscroll(float dir)
        {
            Scroller *scroller = (Scroller *)findsibling(Scroller::typestr());
            if(scroller) addscroll(scroller, dir);
        }

        void arrowscroll(float dir) { addscroll(dir*curtime/1000.0f); }
        void wheelscroll(float step);
        virtual int wheelscrolldirection() const { return 1; }

        void scrollup(float cx, float cy) { wheelscroll(-wheelscrolldirection()); }
        void scrolldown(float cx, float cy) { wheelscroll(wheelscrolldirection()); }

        virtual void movebutton(Object *o, float fromx, float fromy, float tox, float toy) = 0;
    };

    void Scroller::scrollup(float cx, float cy)
    {
        ScrollBar *scrollbar = (ScrollBar *)findsibling(ScrollBar::typestr());
        if(scrollbar) scrollbar->wheelscroll(-scrollbar->wheelscrolldirection());
    }

    void Scroller::scrolldown(float cx, float cy)
    {
        ScrollBar *scrollbar = (ScrollBar *)findsibling(ScrollBar::typestr());
        if(scrollbar) scrollbar->wheelscroll(scrollbar->wheelscrolldirection());
    }

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
            ScrollBar *scrollbar = (ScrollBar *)findsibling(ScrollBar::typestr());
            if(scrollbar) scrollbar->arrowscroll(arrowspeed);
        }
    };

    VARP(uiscrollsteptime, 0, 50, 1000);

    void ScrollBar::wheelscroll(float step)
    {
        ScrollArrow *arrow = (ScrollArrow *)findsibling(ScrollArrow::typestr());
        if(arrow) addscroll(arrow->arrowspeed*step*uiscrollsteptime/1000.0f);
    }

    struct HorizontalScrollBar : ScrollBar
    {
        static const char *typestr() { return "#HorizontalScrollBar"; }
        const char *gettype() const { return typestr(); }

        void addscroll(Scroller *scroller, float dir)
        {
            scroller->addhscroll(dir);
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

        void addscroll(Scroller *scroller, float dir)
        {
            scroller->addvscroll(dir);
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

        int wheelscrolldirection() const { return -1; }
    };

    struct SliderButton : Object
    {
        static const char *typestr() { return "#SliderButton"; }
        const char *gettype() const { return typestr(); }
    };

    static double getfval(ident *id, double val = 0)
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

    static void setfval(ident *id, double val, uint *onchange = NULL)
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
                setfval(id, val, onchange);
                changed = false;
            }
            else val = getfval(id, vmin);
        }

        static const char *typestr() { return "#Slider"; }
        const char *gettype() const { return typestr(); }
        const char *gettypename() const { return typestr(); }

        bool target(float cx, float cy)
        {
            return true;
        }

        void arrowscroll(double dir)
        {
            double newval = clamp(val + dir*vstep, min(vmin, vmax), max(vmin, vmax));
            if(val != newval) changeval(newval);
        }

        void wheelscroll(float step);
        virtual int wheelscrolldirection() const { return 1; }

        void scrollup(float cx, float cy) { wheelscroll(-wheelscrolldirection()); }
        void scrolldown(float cx, float cy) { wheelscroll(wheelscrolldirection()); }

        virtual void scrollto(float cx, float cy) {}

        void hold(float cx, float cy)
        {
            scrollto(cx, cy);
        }

        void changeval(double newval)
        {
            val = newval;
            changed = true;
        }
    };

    VARP(uislidersteptime, 0, 50, 1000);

    struct SliderArrow : Object
    {
        double stepdir;
        int laststep;

        SliderArrow() : laststep(0) {}

        void setup(double dir_ = 0)
        {
            Object::setup();
            stepdir = dir_;
        }

        static const char *typestr() { return "#SliderArrow"; }
        const char *gettype() const { return typestr(); }

        void press(float cx, float cy)
        {
            laststep = totalmillis + 2*uislidersteptime;

            Slider *slider = (Slider *)findsibling(Slider::typestr());
            if(slider) slider->arrowscroll(stepdir);
        }

        void hold(float cx, float cy)
        {
            if(totalmillis < laststep + uislidersteptime)
                return;
            laststep = totalmillis;

            Slider *slider = (Slider *)findsibling(Slider::typestr());
            if(slider) slider->arrowscroll(stepdir);
        }
    };

    void Slider::wheelscroll(float step)
    {
        SliderArrow *arrow = (SliderArrow *)findsibling(SliderArrow::typestr());
        if(arrow) step *= arrow->stepdir;
        arrowscroll(step);
    }

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

        int wheelscrolldirection() const { return -1; }
    };

    struct TextEditor : Object
    {
        static TextEditor *focus;

        float scale, offsetx, offsety;
        editor *edit;
        char *keyfilter;

        TextEditor() : edit(NULL), keyfilter(NULL) {}

        void setup(const char *name, int length, int height, float scale_ = 1, const char *initval = NULL, int mode = EDITORUSED, const char *keyfilter_ = NULL)
        {
            Object::setup();
            editor *edit_ = useeditor(name, mode, false, initval);
            if(edit_ != edit)
            {
                if(edit) clearfocus();
                edit = edit_;
            }
            else if(isfocus() && !hasstate(STATE_HOVER)) commit();
            if(initval && edit->mode == EDITORFOCUSED && !isfocus()) edit->init(initval);
            edit->active = true;
            edit->linewrap = length < 0;
            edit->maxx = edit->linewrap ? -1 : length;
            edit->maxy = height <= 0 ? 1 : -1;
            edit->pixelwidth = abs(length)*FONTH;
            if(edit->linewrap && edit->maxy == 1) edit->updateheight();
            else edit->pixelheight = FONTH*max(height, 1);
            scale = scale_;
            if(keyfilter_) SETSTR(keyfilter, keyfilter_);
            else DELETEA(keyfilter);
        }
        ~TextEditor()
        {
            clearfocus();
            DELETEA(keyfilter);
        }

        static void setfocus(TextEditor *e)
        {
            if(focus == e) return;
            focus = e;
            bool allowtextinput = focus!=NULL && focus->allowtextinput();
            ::textinput(TI_GUI, allowtextinput);
            ::keyrepeat(KR_GUI, allowtextinput);
        }
        void setfocus() { setfocus(this); }
        void clearfocus() { if(focus == this) setfocus(NULL); }
        bool isfocus() const { return focus == this; }

        static const char *typestr() { return "#TextEditor"; }
        const char *gettype() const { return typestr(); }

        bool target(float cx, float cy)
        {
            return true;
        }

        float drawscale() const { return scale / (FONTH * uitextrows); }

        void draw(float sx, float sy)
        {
            float k = drawscale();
            pushhudmatrix();
            hudmatrix.translate(sx, sy, 0);
            hudmatrix.scale(k, k, 1);
            flushhudmatrix();

            edit->draw(FONTW/2, 0, 0xFFFFFF, isfocus());

            pophudmatrix();

            Object::draw(sx, sy);
        }

        void layout()
        {
            Object::layout();

            float k = drawscale();
            w = max(w, (edit->pixelwidth + FONTW)*k);
            h = max(h, edit->pixelheight*k);
        }

        virtual void resetmark(float cx, float cy)
        {
            edit->mark(false);
            offsetx = cx;
            offsety = cy;
        }

        void press(float cx, float cy)
        {
            setfocus();
            resetmark(cx, cy);
        }

        void hold(float cx, float cy)
        {
            if(isfocus())
            {
                float k = drawscale();
                bool dragged = max(fabs(cx - offsetx), fabs(cy - offsety)) > (FONTH/8.0f)*k;
                edit->hit(int(floor(cx/k - FONTW/2)), int(floor(cy/k)), dragged);
            }
        }

        void scrollup(float cx, float cy)
        {
            edit->scrollup();
        }

        void scrolldown(float cx, float cy)
        {
            edit->scrolldown();
        }

        virtual void cancel()
        {
            clearfocus();
        }

        virtual void commit()
        {
            clearfocus();
        }

        bool key(int code, bool isdown)
        {
            if(Object::key(code, isdown)) return true;
            if(!isfocus()) return false;
            switch(code)
            {
                case SDLK_ESCAPE:
                    if(isdown) cancel();
                    return true;
                case SDLK_RETURN:
                case SDLK_TAB:
                    if(edit->maxy != 1) break;
                    // fall-through
                case SDLK_KP_ENTER:
                    if(isdown) commit();
                    return true;
            }
            if(isdown) edit->key(code);
            return true;
        }

        virtual bool allowtextinput() const { return true; }

        bool textinput(const char *str, int len)
        {
            if(Object::textinput(str, len)) return true;
            if(!isfocus() || !allowtextinput()) return false;
            if(!keyfilter) edit->input(str, len);
            else while(len > 0)
            {
                int accept = min(len, (int)strspn(str, keyfilter));
                if(accept > 0) edit->input(str, accept);
                str += accept + 1;
                len -= accept + 1;
                if(len <= 0) break;
                int reject = (int)strcspn(str, keyfilter);
                str += reject;
                str -= reject;
            }
            return true;
        }
    };

    TextEditor *TextEditor::focus = NULL;

    static const char *getsval(ident *id, const char *val = "")
    {
        switch(id->type)
        {
            case ID_VAR: val = intstr(*id->storage.i); break;
            case ID_FVAR: val = floatstr(*id->storage.f); break;
            case ID_SVAR: val = *id->storage.s; break;
            case ID_ALIAS: val = id->getstr(); break;
        }
        return val;
    }

    static void setsval(ident *id, const char *val, uint *onchange = NULL)
    {
        switch(id->type)
        {
            case ID_VAR: setvarchecked(id, parseint(val)); break;
            case ID_FVAR: setfvarchecked(id, parsefloat(val)); break;
            case ID_SVAR: setsvarchecked(id, val); break;
            case ID_ALIAS: alias(id->name, val); break;
        }
        if(onchange && (*onchange&CODE_OP_MASK) != CODE_EXIT) execute(onchange);
    }

    struct Field : TextEditor
    {
        ident *id;
        bool changed;

        Field() : id(NULL), changed(false) {}

        void setup(ident *id_, int length, uint *onchange, float scale = 1, const char *keyfilter_ = NULL)
        {
            if(isfocus() && !hasstate(STATE_HOVER)) commit();
            if(changed)
            {
                if(id == id_) setsval(id, edit->lines[0].text, onchange);
                changed = false;
            }
            TextEditor::setup(id_->name, length, 0, scale, id != id_ || !isfocus() ? getsval(id_) : NULL, EDITORFOCUSED, keyfilter_);
            id = id_;
        }

        void commit()
        {
            TextEditor::commit();
            changed = true;
        }

        void cancel()
        {
            TextEditor::cancel();
            changed = false;
        }
    };

    struct KeyField : Field
    {
        void resetmark(float cx, float cy)
        {
            edit->clear();
            Field::resetmark(cx, cy);
        }

        void insertkey(int code)
        {
            const char *keyname = getkeyname(code);
            if(keyname)
            {
                if(!edit->empty()) edit->insert(" ");
                edit->insert(keyname);
            }
        }

        bool rawkey(int code, bool isdown)
        {
            if(Object::rawkey(code, isdown)) return true;
            if(!isfocus() || !isdown) return false;
            if(code == SDLK_ESCAPE) commit();
            else insertkey(code);
            return true;
        }

        bool allowtextinput() const { return false; }
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

        bool target(float cx, float cy)
        {
            return true;
        }

        void draw(float sx, float sy)
        {
            if(clipstack.length()) glDisable(GL_SCISSOR_TEST);
            int sx1, sy1, sx2, sy2;
            window->calcscissor(sx, sy, sx+w, sy+h, sx1, sy1, sx2, sy2);
            glDisable(GL_BLEND);
            gle::disable();
            modelpreview::start(sx1, sy1, sx2-sx1, sy2-sy1, false, clipstack.length() > 0);
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

        bool target(float cx, float cy)
        {
            return true;
        }

        void draw(float sx, float sy)
        {
            if(clipstack.length()) glDisable(GL_SCISSOR_TEST);
            int sx1, sy1, sx2, sy2;
            window->calcscissor(sx, sy, sx+w, sy+h, sx1, sy1, sx2, sy2);
            glDisable(GL_BLEND);
            gle::disable();
            modelpreview::start(sx1, sy1, sx2-sx1, sy2-sy1, false, clipstack.length() > 0);
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

        bool target(float cx, float cy)
        {
            return true;
        }

        void previewslot(Slot &slot, VSlot &vslot, float x, float y)
        {
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
            Slot &slot = lookupslot(index, false);
            previewslot(slot, *slot.variants, sx, sy);
            Object::draw(sx, sy);
        }
    };

    struct VSlotViewer : SlotViewer
    {
        static const char *typestr() { return "#VSlotViewer"; }
        const char *gettype() const { return typestr(); }

        void draw(float sx, float sy)
        {
            VSlot &vslot = lookupvslot(index, false);
            previewslot(*vslot.slot, vslot, sx, sy);
            Object::draw(sx, sy);
        }
    };

    ICOMMAND(newui, "ssssb", (char *name, char *contents, char *onshow, char *onhide, int *allowinput),
    {
        Window *window = windows.find(name, NULL);
        if(window) { world->hide(window); windows.remove(name); delete window; }
        windows[name] = new Window(name, contents, onshow, onhide, *allowinput!=0);
    });

    bool showui(const char *name)
    {
        Window *window = windows.find(name, NULL);
        return window && world->show(window);
    }

    bool hideui(const char *name)
    {
        if(!name) return world->hideall() > 0;
        Window *window = windows.find(name, NULL);
        return window && world->hide(window);
    }

    bool toggleui(const char *name)
    {
        if(showui(name)) return true;
        hideui(name);
        return false;
    }

    void holdui(const char *name, bool on)
    {
        if(on) showui(name);
        else hideui(name);
    }

    ICOMMAND(showui, "s", (char *name), intret(showui(name) ? 1 : 0));
    ICOMMAND(hideui, "s", (char *name), intret(hideui(name) ? 1 : 0));
    ICOMMAND(toggleui, "s", (char *name), intret(toggleui(name) ? 1 : 0));
    ICOMMAND(holdui, "sD", (char *name, int *down), holdui(name, *down!=0));

    #define IFSTATEVAL(state,t,f) { if(state) { if(t->type == VAL_NULL) intret(1); else result(*t); } else if(f->type == VAL_NULL) intret(0); else result(*f); }
    #define DOSTATE(flags, func) \
        ICOMMANDNS("ui!" #func, uinot##func##_, "ee", (uint *t, uint *f), \
            executeret(buildparent && buildparent->hasstate(flags) ? t : f)); \
        ICOMMANDNS("ui" #func, ui##func##_, "ee", (uint *t, uint *f), \
            executeret(buildparent && buildparent->haschildstate(flags) ? t : f)); \
        ICOMMANDNS("ui!" #func "?", uinot##func##__, "tt", (tagval *t, tagval *f), \
            IFSTATEVAL(buildparent && buildparent->hasstate(flags), t, f)); \
        ICOMMANDNS("ui" #func "?", ui##func##__, "tt", (tagval *t, tagval *f), \
            IFSTATEVAL(buildparent && buildparent->haschildstate(flags), t, f)); \
        ICOMMANDNS("ui!" #func "+", uinextnot##func##_, "ee", (uint *t, uint *f), \
            executeret(buildparent && buildparent->children.inrange(buildchild) && buildparent->children[buildchild]->hasstate(flags) ? t : f)); \
        ICOMMANDNS("ui" #func "+", uinext##func##_, "ee", (uint *t, uint *f), \
            executeret(buildparent && buildparent->children.inrange(buildchild) && buildparent->children[buildchild]->haschildstate(flags) ? t : f)); \
        ICOMMANDNS("ui!" #func "+?", uinextnot##func##__, "tt", (tagval *t, tagval *f), \
            IFSTATEVAL(buildparent && buildparent->children.inrange(buildchild) && buildparent->children[buildchild]->hasstate(flags), t, f)); \
        ICOMMANDNS("ui" #func "+?", uinext##func##__, "tt", (tagval *t, tagval *f), \
            IFSTATEVAL(buildparent && buildparent->children.inrange(buildchild) && buildparent->children[buildchild]->haschildstate(flags), t, f));
    DOSTATES
    #undef DOSTATE

    ICOMMANDNS("uifocus", uifocus_, "ee", (uint *t, uint *f),
        executeret(buildparent && TextEditor::focus == buildparent ? t : f));
    ICOMMANDNS("uifocus?", uifocus__, "tt", (tagval *t, tagval *f),
        IFSTATEVAL(buildparent && TextEditor::focus == buildparent, t, f));
    ICOMMANDNS("uifocus+", uinextfocus_, "ee", (uint *t, uint *f),
        executeret(buildparent && buildparent->children.inrange(buildchild) && TextEditor::focus == buildparent->children[buildchild] ? t : f));
    ICOMMANDNS("uifocus+?", uinextfocus__, "tt", (tagval *t, tagval *f),
        IFSTATEVAL(buildparent && buildparent->children.inrange(buildchild) && TextEditor::focus == buildparent->children[buildchild], t, f));

    ICOMMAND(uialign, "ii", (int *xalign, int *yalign),
    {
        if(buildparent) buildparent->setalign(*xalign, *yalign);
    });
    ICOMMANDNS("uialign-", uialign_, "ii", (int *xalign, int *yalign),
    {
        if(buildparent && buildchild > 0) buildparent->children[buildchild-1]->setalign(*xalign, *yalign);
    });
    ICOMMANDNS("uialign*", uialign__, "ii", (int *xalign, int *yalign),
    {
        if(buildparent) loopi(buildchild) buildparent->children[i]->setalign(*xalign, *yalign);
    });

    ICOMMAND(uiclamp, "iiii", (int *left, int *right, int *bottom, int *top),
    {
        if(buildparent) buildparent->setclamp(*left, *right, *bottom, *top);
    });
    ICOMMANDNS("uiclamp-", uiclamp_, "iiii", (int *left, int *right, int *bottom, int *top),
    {
        if(buildparent && buildchild > 0) buildparent->children[buildchild-1]->setclamp(*left, *right, *bottom, *top);
    });
    ICOMMANDNS("uiclamp*", uiclamp__, "iiii", (int *left, int *right, int *bottom, int *top),
    {
        if(buildparent) loopi(buildchild) buildparent->children[i]->setclamp(*left, *right, *bottom, *top);
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

    ICOMMAND(uigrid, "ife", (int *columns, float *space, uint *children),
        BUILD(Grid, o, o->setup(*columns, *space), children));

    ICOMMAND(uitableheader, "ee", (uint *columndata, uint *children),
        BUILDCOLUMNS(TableHeader, o, o->setup(), columndata, children));
    ICOMMAND(uitablerow, "ee", (uint *columndata, uint *children),
        BUILDCOLUMNS(TableRow, o, o->setup(), columndata, children));
    ICOMMAND(uitable, "fe", (float *space, uint *children),
        BUILD(Table, o, o->setup(*space), children));

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

    ICOMMAND(uisliderarrow, "fe", (float *dir, uint *children),
        BUILD(SliderArrow, o, o->setup(*dir), children));

    ICOMMAND(uisliderbutton, "e", (uint *children),
        BUILD(SliderButton, o, o->setup(), children));

    ICOMMAND(uicolor, "iffe", (int *c, float *minw, float *minh, uint *children),
        BUILD(FillColor, o, o->setup(FillColor::SOLID, Color(*c), *minw, *minh), children));

    ICOMMAND(uimodcolor, "iffe", (int *c, float *minw, float *minh, uint *children),
        BUILD(FillColor, o, o->setup(FillColor::MODULATE, Color(*c), *minw, *minh), children));

    ICOMMAND(uivgradient, "iiffe", (int *c, int *c2, float *minw, float *minh, uint *children),
        BUILD(Gradient, o, o->setup(Gradient::SOLID, Gradient::VERTICAL, Color(*c), Color(*c2), *minw, *minh), children));

    ICOMMAND(uimodvgradient, "iiffe", (int *c, int *c2, float *minw, float *minh, uint *children),
        BUILD(Gradient, o, o->setup(Gradient::MODULATE, Gradient::VERTICAL, Color(*c), Color(*c2), *minw, *minh), children));

    ICOMMAND(uihgradient, "iiffe", (int *c, int *c2, float *minw, float *minh, uint *children),
        BUILD(Gradient, o, o->setup(Gradient::SOLID, Gradient::HORIZONTAL, Color(*c), Color(*c2), *minw, *minh), children));

    ICOMMAND(uimodhgradient, "iiffe", (int *c, int *c2, float *minw, float *minh, uint *children),
        BUILD(Gradient, o, o->setup(Gradient::MODULATE, Gradient::HORIZONTAL, Color(*c), Color(*c2), *minw, *minh), children));

    ICOMMAND(uioutline, "iffe", (int *c, float *minw, float *minh, uint *children),
        BUILD(Outline, o, o->setup(Color(*c), *minw, *minh), children));

    ICOMMAND(uiline, "iffe", (int *c, float *minw, float *minh, uint *children),
        BUILD(Line, o, o->setup(Color(*c), *minw, *minh), children));

    static inline void buildtext(tagval &t, float scale, const Color &color, uint *children)
    {
        if(scale <= 0) scale = 1;
        switch(t.type)
        {
            case VAL_INT:
                BUILD(TextInt, o, o->setup(t.i, scale, color), children);
                break;
            case VAL_FLOAT:
                BUILD(TextFloat, o, o->setup(t.f, scale, color), children);
                break;
            case VAL_CSTR:
            case VAL_MACRO:
            case VAL_STR:
                if(t.s[0])
                {
                    BUILD(TextString, o, o->setup(t.s, scale, color), children);
                    break;
                }
                // fall-through
            default:
                BUILD(Object, o, o->setup(), children);
                break;
        }
    }

    ICOMMAND(uicolortext, "tfie", (tagval *text, float *scale, int *c, uint *children),
        buildtext(*text, *scale, Color(*c), children));

    ICOMMAND(uitext, "tfe", (tagval *text, float *scale, uint *children),
        buildtext(*text, *scale, Color(255, 255, 255), children));

    ICOMMAND(uitexteditor, "siifsie", (char *name, int *length, int *height, float *scale, char *initval, int *keep, uint *children),
        BUILD(TextEditor, o, o->setup(name, *length, *height, *scale <= 0 ? 1 : *scale, initval, *keep ? EDITORFOREVER : EDITORUSED), children));

    ICOMMAND(uifield, "riefe", (ident *var, int *length, uint *onchange, float *scale, uint *children),
        BUILD(Field, o, o->setup(var, *length, onchange, *scale <= 0 ? 1 : *scale), children));

    ICOMMAND(uikeyfield, "riefe", (ident *var, int *length, uint *onchange, float *scale, uint *children),
        BUILD(KeyField, o, o->setup(var, *length, onchange, *scale <= 0 ? 1 : *scale), children));

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

    ICOMMAND(uivslotview, "iffe", (int *index, float *minw, float *minh, uint *children),
        BUILD(VSlotViewer, o, o->setup(*index, *minw, *minh), children));

    FVAR(uisensitivity, 1e-3f, 1, 1e3f);

    bool hascursor()
    {
        return world->allowinput();
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

    bool movecursor(int dx, int dy)
    {
        if(!hascursor()) return false;
        cursorx = clamp(cursorx + dx*uisensitivity/screenw, 0.0f, 1.0f);
        cursory = clamp(cursory + dy*uisensitivity/screenh, 0.0f, 1.0f);
        return true;
    }

    bool keypress(int code, bool isdown)
    {
        if(world->rawkey(code, isdown)) return true;
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
                if(world->setstate(action, cursorx, cursory))
                {
                    if(hold >= 0) world->state |= hold;
                    return true;
                }
            }
            else if(hold < 0) return true;
            else if(world->state&hold)
            {
                world->setstate(action, cursorx, cursory);
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

    void limitscale(float scale)
    {
        maxscale = scale;
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
        readyeditors();

        world->setstate(STATE_HOVER, cursorx, cursory);
        if(world->state&STATE_HOLD) world->setstate(STATE_HOLD, cursorx, cursory);
        if(world->state&STATE_ALT_HOLD) world->setstate(STATE_ALT_HOLD, cursorx, cursory);
        if(world->state&STATE_ESC_HOLD) world->setstate(STATE_ESC_HOLD, cursorx, cursory);

        world->build();

        flusheditors();
    }

    void render()
    {
        world->layout();
        world->adjustchildren();
        world->draw();
    }
}

