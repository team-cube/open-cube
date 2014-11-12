VARP(softexplosion, 0, 1, 1);
VARP(softexplosionblend, 1, 16, 64);

namespace sphere
{
    struct vert
    {
        vec pos;
        ushort s, t;
    } *verts = NULL;
    GLushort *indices = NULL;
    int numverts = 0, numindices = 0;
    GLuint vbuf = 0, ebuf = 0;

    void init(int slices, int stacks)
    {
        numverts = (stacks+1)*(slices+1);
        verts = new vert[numverts];
        float ds = 1.0f/slices, dt = 1.0f/stacks, t = 1.0f;
        loopi(stacks+1)
        {
            float rho = M_PI*(1-t), s = 0.0f;
            loopj(slices+1)
            {
                float theta = j==slices ? 0 : 2*M_PI*s;
                vert &v = verts[i*(slices+1) + j];
                v.pos = vec(-sin(theta)*sin(rho), cos(theta)*sin(rho), cos(rho));
                v.s = ushort(s*0xFFFF);
                v.t = ushort(t*0xFFFF);
                s += ds;
            }
            t -= dt;
        }

        numindices = stacks*slices*3*2;
        indices = new ushort[numindices];
        GLushort *curindex = indices;
        loopi(stacks)
        {
            loopk(slices)
            {
                int j = i%2 ? slices-k-1 : k;

                *curindex++ = i*(slices+1)+j;
                *curindex++ = (i+1)*(slices+1)+j;
                *curindex++ = i*(slices+1)+j+1;

                *curindex++ = i*(slices+1)+j+1;
                *curindex++ = (i+1)*(slices+1)+j;
                *curindex++ = (i+1)*(slices+1)+j+1;
            }
        }

        if(!vbuf) glGenBuffers_(1, &vbuf);
        glBindBuffer_(GL_ARRAY_BUFFER, vbuf);
        glBufferData_(GL_ARRAY_BUFFER, numverts*sizeof(vert), verts, GL_STATIC_DRAW);
        DELETEA(verts);

        if(!ebuf) glGenBuffers_(1, &ebuf);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, ebuf);
        glBufferData_(GL_ELEMENT_ARRAY_BUFFER, numindices*sizeof(GLushort), indices, GL_STATIC_DRAW);
        DELETEA(indices);
    }

    void cleanup()
    {
        if(vbuf) { glDeleteBuffers_(1, &vbuf); vbuf = 0; }
        if(ebuf) { glDeleteBuffers_(1, &ebuf); ebuf = 0; }
    }

    void enable()
    {
        if(!vbuf) init(12, 6);

        glBindBuffer_(GL_ARRAY_BUFFER, vbuf);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, ebuf);

        gle::vertexpointer(sizeof(vert), &verts->pos);
        gle::texcoord0pointer(sizeof(vert), &verts->s, GL_UNSIGNED_SHORT, 2, GL_TRUE);
        gle::enablevertex();
        gle::enabletexcoord0();
    }

    void draw()
    {
        glDrawRangeElements_(GL_TRIANGLES, 0, numverts-1, numindices, GL_UNSIGNED_SHORT, indices);
        xtraverts += numindices;
        glde++;
    }

    void disable()
    {
        gle::disablevertex();
        gle::disabletexcoord0();

        glBindBuffer_(GL_ARRAY_BUFFER, 0);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}

static const float WOBBLE = 1.25f;

struct fireballrenderer : listrenderer
{
    fireballrenderer(const char *texname)
        : listrenderer(texname, 0, PT_FIREBALL|PT_SHADER)
    {}

    void startrender()
    {
        if(softparticles && softexplosion) SETSHADER(explosionsoft);
        else SETSHADER(explosion);

        sphere::enable();
    }

    void endrender()
    {
        sphere::disable();
    }

    void cleanup()
    {
        sphere::cleanup();
    }

    void seedemitter(particleemitter &pe, const vec &o, const vec &d, int fade, float size, int gravity)
    {
        pe.maxfade = max(pe.maxfade, fade);
        pe.extendbb(o, (size+1+pe.ent->attr2)*WOBBLE);
    }

    void renderpart(listparticle *p, const vec &o, const vec &d, int blend, int ts)
    {
        float pmax = p->val,
              size = p->fade ? float(ts)/p->fade : 1,
              psize = p->size + pmax * size;

        if(isfoggedsphere(psize*WOBBLE, p->o)) return;

        matrix4 m;
        m.identity();
        m.translate(o);

        bool inside = o.dist(camera1->o) <= psize*WOBBLE;
        vec oc(o);
        oc.sub(camera1->o);

        float yaw = inside ? camera1->yaw : atan2(oc.y, oc.x)/RAD - 90,
        pitch = (inside ? camera1->pitch : asin(oc.z/oc.magnitude())/RAD) - 90;

        vec s(1, 0, 0), t(0, 1, 0);
        s.rotate_around_x(pitch*-RAD);
        s.rotate_around_z(yaw*-RAD);
        t.rotate_around_x(pitch*-RAD);
        t.rotate_around_z(yaw*-RAD);

        vec rotdir = vec(-1, 1, -1).normalize();
        float rotangle = lastmillis/1000.0f*143;
        s.rotate(rotangle*-RAD, rotdir);
        t.rotate(rotangle*-RAD, rotdir);

        LOCALPARAM(texgenS, s);
        LOCALPARAM(texgenT, t);

        m.rotate(rotangle*RAD, vec(-rotdir.x, rotdir.y, -rotdir.z));
        m.scale(-psize, psize, inside ? psize : -psize);
        m.mul(camprojmatrix, m);
        LOCALPARAM(explosionmatrix, m);

        LOCALPARAM(center, o);
        LOCALPARAMF(blendparams, inside ? 0.5f : 4, inside ? 0.25f : 0);
        if(2*(p->size + pmax)*WOBBLE >= softexplosionblend)
        {
            LOCALPARAMF(softparams, -1.0f/softexplosionblend, 0, inside ? blend/(2*255.0f) : 0);
        }
        else
        {
            LOCALPARAMF(softparams, 0, -1, inside ? blend/(2*255.0f) : 0);
        }

        vec color = p->color.tocolor().mul(ldrscale);
        float alpha = blend/255.0f;

        loopi(inside ? 2 : 1)
        {
            gle::color(color, i ? alpha/2 : alpha);
            if(i) glDepthFunc(GL_GEQUAL);
            sphere::draw();
            if(i) glDepthFunc(GL_LESS);
        }
    }
};
static fireballrenderer fireballs("media/particle/explosion.png"), pulsebursts("media/particle/pulse_burst.png");

