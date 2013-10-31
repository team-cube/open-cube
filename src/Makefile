CXXFLAGS= -O3 -fomit-frame-pointer -ffast-math
override CXXFLAGS+= -Wall -fsigned-char -fno-exceptions -fno-rtti

PLATFORM= $(shell uname -s)
PLATFORM_ARCH= $(shell uname -m)
PLATFORM_PREFIX= native

INCLUDES= -Ishared -Iengine -Igame -Ienet/include

STRIP=
ifeq (,$(findstring -g,$(CXXFLAGS)))
ifeq (,$(findstring -pg,$(CXXFLAGS)))
  STRIP=strip
endif
endif

MV=mv

ifneq (,$(findstring MINGW,$(PLATFORM)))
WINDRES= windres
ifneq (,$(findstring 64,$(PLATFORM)))
ifneq (,$(findstring CROSS,$(PLATFORM)))
  CXX=x86_64-w64-mingw32-g++
  WINDRES=x86_64-w64-mingw32-windres
ifneq (,$(STRIP))
  STRIP=x86_64-w64-mingw32-strip
endif
endif
WINLIB=lib64
WINBIN=../bin64
override CXX+= -m64
override WINDRES+= -F pe-x86-64
else
ifneq (,$(findstring CROSS,$(PLATFORM)))
  CXX=i686-w64-mingw32-g++
  WINDRES=i686-w64-mingw32-windres
ifneq (,$(STRIP))
  STRIP=i686-w64-mingw32-strip
endif
endif
WINLIB=lib
WINBIN=../bin
override CXX+= -m32
override WINDRES+= -F pe-i386
endif
CLIENT_INCLUDES= $(INCLUDES) -Iinclude
ifneq (,$(findstring TDM,$(PLATFORM)))
STD_LIBS=
else
STD_LIBS= -static-libgcc -static-libstdc++
endif
CLIENT_LIBS= -mwindows $(STD_LIBS) -L$(WINBIN) -L$(WINLIB) -lSDL2 -lSDL2_image -lSDL2_mixer -lzlib1 -lopengl32 -lenet -lws2_32 -lwinmm
else	
CLIENT_INCLUDES= $(INCLUDES) -I/usr/X11R6/include `sdl2-config --cflags`
CLIENT_LIBS= -Lenet -lenet -L/usr/X11R6/lib -lX11 `sdl2-config --libs` -lSDL2_image -lSDL2_mixer -lz -lGL
endif
ifeq ($(PLATFORM),Linux)
CLIENT_LIBS+= -lrt
	ifneq (,$(OVR))
		CLIENT_INCLUDES+= -ILibOVR/Include -DHAS_OVR=1
		ifneq (,$(findstring 64,$(PLATFORM_ARCH)))
			CLIENT_LIBS+= -LLibOVR/Lib/Linux/Release/x86_64 -lovr -ludev -lXinerama
		else
			CLIENT_LIBS+= -LLibOVR/Lib/Linux/Release/i386 -lovr -ludev -lXinerama
		endif
	endif
else
ifneq (,$(findstring GNU,$(PLATFORM))) 
CLIENT_LIBS+= -lrt 
endif 
endif

CLIENT_OBJS= \
	shared/crypto.o \
	shared/geom.o \
	shared/glemu.o \
	shared/stream.o \
	shared/tools.o \
	shared/zip.o \
	engine/aa.o \
	engine/bih.o \
	engine/blend.o \
	engine/client.o	\
	engine/command.o \
	engine/console.o \
	engine/decal.o \
	engine/dynlight.o \
	engine/grass.o \
	engine/light.o \
	engine/main.o \
	engine/material.o \
	engine/menus.o \
	engine/movie.o \
	engine/normal.o	\
	engine/octa.o \
	engine/octaedit.o \
	engine/octarender.o \
	engine/ovr.o \
	engine/physics.o \
	engine/pvs.o \
	engine/rendergl.o \
	engine/renderlights.o \
	engine/rendermodel.o \
	engine/renderparticles.o \
	engine/rendersky.o \
	engine/rendertext.o \
	engine/renderva.o \
	engine/server.o	\
	engine/serverbrowser.o \
	engine/shader.o \
	engine/sound.o \
	engine/texture.o \
	engine/ui.o \
	engine/water.o \
	engine/world.o \
	engine/worldio.o \
	game/ai.o \
	game/client.o \
	game/entities.o \
	game/game.o \
	game/render.o \
	game/scoreboard.o \
	game/server.o \
	game/waypoint.o \
	game/weapon.o

CLIENT_PCH= shared/cube.h.gch engine/engine.h.gch game/game.h.gch

ifneq (,$(findstring MINGW,$(PLATFORM)))
SERVER_INCLUDES= -DSTANDALONE $(INCLUDES) -Iinclude
SERVER_LIBS= -mwindows $(STD_LIBS) -L$(WINBIN) -L$(WINLIB) -lzlib1 -lenet -lws2_32 -lwinmm
MASTER_LIBS= $(STD_LIBS) -L$(WINBIN) -L$(WINLIB) -lzlib1 -lenet -lws2_32 -lwinmm
else
SERVER_INCLUDES= -DSTANDALONE $(INCLUDES)
SERVER_LIBS= -Lenet -lenet -lz
MASTER_LIBS= $(SERVER_LIBS)
endif
SERVER_OBJS= \
	shared/crypto-standalone.o \
	shared/stream-standalone.o \
	shared/tools-standalone.o \
	engine/command-standalone.o \
	engine/server-standalone.o \
	engine/worldio-standalone.o \
	game/entities-standalone.o \
	game/server-standalone.o

MASTER_OBJS= \
	shared/crypto-standalone.o \
	shared/stream-standalone.o \
	shared/tools-standalone.o \
	engine/command-standalone.o \
	engine/master-standalone.o

ifeq ($(PLATFORM),SunOS)
CLIENT_LIBS+= -lsocket -lnsl -lX11
SERVER_LIBS+= -lsocket -lnsl
endif

default: all

all: client server

clean:
	-$(RM) $(CLIENT_PCH) $(CLIENT_OBJS) $(SERVER_OBJS) $(MASTER_OBJS) tess_client tess_server tess_master

fixspace:
	sed -i 's/[ \t]*$$//; :rep; s/^\([ ]*\)\t/\1    /g; trep' shared/*.c shared/*.cpp shared/*.h engine/*.cpp engine/*.h game/*.cpp game/*.h

%.h.gch: %.h
	$(CXX) $(CXXFLAGS) -x c++-header -o $(subst .h.gch,.tmp.h.gch,$@) $(subst .h.gch,.h,$@)
	$(MV) $(subst .h.gch,.tmp.h.gch,$@) $@

%-standalone.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $(subst -standalone.o,.cpp,$@)

$(CLIENT_OBJS): CXXFLAGS += $(CLIENT_INCLUDES)
$(filter shared/%,$(CLIENT_OBJS)): $(filter shared/%,$(CLIENT_PCH))
$(filter engine/%,$(CLIENT_OBJS)): $(filter engine/%,$(CLIENT_PCH))
$(filter game/%,$(CLIENT_OBJS)): $(filter game/%,$(CLIENT_PCH))

$(SERVER_OBJS): CXXFLAGS += $(SERVER_INCLUDES)
$(filter-out $(SERVER_OBJS),$(MASTER_OBJS)): CXXFLAGS += $(SERVER_INCLUDES)

ifneq (,$(findstring MINGW,$(PLATFORM)))
client: $(CLIENT_OBJS)
	$(WINDRES) -I vcpp -i vcpp/mingw.rc -J rc -o vcpp/mingw.res -O coff 
	$(CXX) $(CXXFLAGS) -o $(WINBIN)/tesseract.exe vcpp/mingw.res $(CLIENT_OBJS) $(CLIENT_LIBS)

server: $(SERVER_OBJS)
	$(WINDRES) -I vcpp -i vcpp/mingw.rc -J rc -o vcpp/mingw.res -O coff
	$(CXX) $(CXXFLAGS) -o $(WINBIN)/tess_server.exe vcpp/mingw.res $(SERVER_OBJS) $(SERVER_LIBS)

master: $(MASTER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(WINBIN)/tess_master.exe $(MASTER_OBJS) $(MASTER_LIBS)

install: all
else
client:	libenet $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o tess_client $(CLIENT_OBJS) $(CLIENT_LIBS)

server:	libenet $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o tess_server $(SERVER_OBJS) $(SERVER_LIBS)  
	
master: libenet $(MASTER_OBJS)
	$(CXX) $(CXXFLAGS) -o tess_master $(MASTER_OBJS) $(MASTER_LIBS)  

shared/tessfont.o: shared/tessfont.c
	$(CXX) $(CXXFLAGS) -c -o $@ $< `freetype-config --cflags`

tessfont: shared/tessfont.o
	$(CXX) $(CXXFLAGS) -o tessfont shared/tessfont.o `freetype-config --libs` -lz

install: all
	cp -f tess_client	../bin_unix/$(PLATFORM_PREFIX)_client
	cp -f tess_server	../bin_unix/$(PLATFORM_PREFIX)_server
ifneq (,$(STRIP))
	$(STRIP) ../bin_unix/$(PLATFORM_PREFIX)_client
	$(STRIP) ../bin_unix/$(PLATFORM_PREFIX)_server
endif
endif

depend:
	makedepend -Y -Ishared -Iengine -Igame $(subst .o,.cpp,$(CLIENT_OBJS))
	makedepend -a -o.h.gch -Y -Ishared -Iengine -Igame $(subst .h.gch,.h,$(CLIENT_PCH))
	makedepend -a -o-standalone.o -Y -DSTANDALONE -Ishared -Iengine -Igame $(subst -standalone.o,.cpp,$(SERVER_OBJS))
	makedepend -a -o-standalone.o -Y -DSTANDALONE -Ishared -Iengine -Igame $(subst -standalone.o,.cpp,$(filter-out $(SERVER_OBJS), $(MASTER_OBJS)))

engine/engine.h.gch: shared/cube.h.gch
game/game.h.gch: shared/cube.h.gch

CC= $(CXX) -x c
ENET_CFLAGS:= -Ienet/include -O3 -fomit-frame-pointer $(shell enet/check_cflags.sh $(CC))
ENET_OBJS= \
	enet/callbacks.o \
	enet/host.o \
	enet/list.o \
	enet/packet.o \
	enet/peer.o \
	enet/protocol.o \
	enet/unix.o \
	enet/win32.o
$(ENET_OBJS): CFLAGS += $(ENET_CFLAGS)
enet/libenet.a: $(ENET_OBJS)
	$(AR) rcs $@ $(ENET_OBJS)
libenet: enet/libenet.a
			
# DO NOT DELETE

shared/crypto.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/crypto.o: shared/command.h shared/glexts.h shared/glemu.h
shared/crypto.o: shared/iengine.h shared/igame.h
shared/geom.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/geom.o: shared/command.h shared/glexts.h shared/glemu.h
shared/geom.o: shared/iengine.h shared/igame.h
shared/glemu.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/glemu.o: shared/command.h shared/glexts.h shared/glemu.h
shared/glemu.o: shared/iengine.h shared/igame.h
shared/stream.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/stream.o: shared/command.h shared/glexts.h shared/glemu.h
shared/stream.o: shared/iengine.h shared/igame.h
shared/tools.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/tools.o: shared/command.h shared/glexts.h shared/glemu.h
shared/tools.o: shared/iengine.h shared/igame.h
shared/zip.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/zip.o: shared/command.h shared/glexts.h shared/glemu.h
shared/zip.o: shared/iengine.h shared/igame.h
engine/aa.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/aa.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/aa.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/aa.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/bih.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/bih.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/bih.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/bih.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/blend.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/blend.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/blend.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/blend.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/client.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/client.o: shared/ents.h shared/command.h shared/glexts.h
engine/client.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/client.o: engine/world.h engine/octa.h engine/light.h engine/texture.h
engine/client.o: engine/bih.h engine/model.h
engine/command.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/command.o: shared/ents.h shared/command.h shared/glexts.h
engine/command.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/command.o: engine/world.h engine/octa.h engine/light.h
engine/command.o: engine/texture.h engine/bih.h engine/model.h
engine/console.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/console.o: shared/ents.h shared/command.h shared/glexts.h
engine/console.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/console.o: engine/world.h engine/octa.h engine/light.h
engine/console.o: engine/texture.h engine/bih.h engine/model.h
engine/decal.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/decal.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/decal.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/decal.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/dynlight.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/dynlight.o: shared/ents.h shared/command.h shared/glexts.h
engine/dynlight.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/dynlight.o: engine/world.h engine/octa.h engine/light.h
engine/dynlight.o: engine/texture.h engine/bih.h engine/model.h
engine/grass.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/grass.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/grass.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/grass.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/light.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/light.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/light.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/light.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/main.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/main.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/main.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/main.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/material.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/material.o: shared/ents.h shared/command.h shared/glexts.h
engine/material.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/material.o: engine/world.h engine/octa.h engine/light.h
engine/material.o: engine/texture.h engine/bih.h engine/model.h
engine/menus.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/menus.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/menus.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/menus.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/movie.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/movie.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/movie.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/movie.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/normal.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/normal.o: shared/ents.h shared/command.h shared/glexts.h
engine/normal.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/normal.o: engine/world.h engine/octa.h engine/light.h engine/texture.h
engine/normal.o: engine/bih.h engine/model.h
engine/octa.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/octa.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/octa.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/octa.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/octaedit.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/octaedit.o: shared/ents.h shared/command.h shared/glexts.h
engine/octaedit.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/octaedit.o: engine/world.h engine/octa.h engine/light.h
engine/octaedit.o: engine/texture.h engine/bih.h engine/model.h
engine/octarender.o: engine/engine.h shared/cube.h shared/tools.h
engine/octarender.o: shared/geom.h shared/ents.h shared/command.h
engine/octarender.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/octarender.o: shared/igame.h engine/world.h engine/octa.h
engine/octarender.o: engine/light.h engine/texture.h engine/bih.h
engine/octarender.o: engine/model.h
engine/ovr.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/ovr.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/ovr.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/ovr.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/physics.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/physics.o: shared/ents.h shared/command.h shared/glexts.h
engine/physics.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/physics.o: engine/world.h engine/octa.h engine/light.h
engine/physics.o: engine/texture.h engine/bih.h engine/model.h engine/mpr.h
engine/pvs.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/pvs.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/pvs.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/pvs.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/rendergl.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/rendergl.o: shared/ents.h shared/command.h shared/glexts.h
engine/rendergl.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/rendergl.o: engine/world.h engine/octa.h engine/light.h
engine/rendergl.o: engine/texture.h engine/bih.h engine/model.h
engine/renderlights.o: engine/engine.h shared/cube.h shared/tools.h
engine/renderlights.o: shared/geom.h shared/ents.h shared/command.h
engine/renderlights.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/renderlights.o: shared/igame.h engine/world.h engine/octa.h
engine/renderlights.o: engine/light.h engine/texture.h engine/bih.h
engine/renderlights.o: engine/model.h
engine/rendermodel.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendermodel.o: shared/geom.h shared/ents.h shared/command.h
engine/rendermodel.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/rendermodel.o: shared/igame.h engine/world.h engine/octa.h
engine/rendermodel.o: engine/light.h engine/texture.h engine/bih.h
engine/rendermodel.o: engine/model.h engine/ragdoll.h engine/animmodel.h
engine/rendermodel.o: engine/vertmodel.h engine/skelmodel.h engine/hitzone.h
engine/rendermodel.o: engine/md2.h engine/md3.h engine/md5.h engine/obj.h
engine/rendermodel.o: engine/smd.h engine/iqm.h
engine/renderparticles.o: engine/engine.h shared/cube.h shared/tools.h
engine/renderparticles.o: shared/geom.h shared/ents.h shared/command.h
engine/renderparticles.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/renderparticles.o: shared/igame.h engine/world.h engine/octa.h
engine/renderparticles.o: engine/light.h engine/texture.h engine/bih.h
engine/renderparticles.o: engine/model.h engine/explosion.h
engine/renderparticles.o: engine/lensflare.h engine/lightning.h
engine/rendersky.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendersky.o: shared/geom.h shared/ents.h shared/command.h
engine/rendersky.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/rendersky.o: shared/igame.h engine/world.h engine/octa.h
engine/rendersky.o: engine/light.h engine/texture.h engine/bih.h
engine/rendersky.o: engine/model.h
engine/rendertext.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendertext.o: shared/geom.h shared/ents.h shared/command.h
engine/rendertext.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/rendertext.o: shared/igame.h engine/world.h engine/octa.h
engine/rendertext.o: engine/light.h engine/texture.h engine/bih.h
engine/rendertext.o: engine/model.h
engine/renderva.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/renderva.o: shared/ents.h shared/command.h shared/glexts.h
engine/renderva.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/renderva.o: engine/world.h engine/octa.h engine/light.h
engine/renderva.o: engine/texture.h engine/bih.h engine/model.h
engine/server.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/server.o: shared/ents.h shared/command.h shared/glexts.h
engine/server.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/server.o: engine/world.h engine/octa.h engine/light.h engine/texture.h
engine/server.o: engine/bih.h engine/model.h
engine/serverbrowser.o: engine/engine.h shared/cube.h shared/tools.h
engine/serverbrowser.o: shared/geom.h shared/ents.h shared/command.h
engine/serverbrowser.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/serverbrowser.o: shared/igame.h engine/world.h engine/octa.h
engine/serverbrowser.o: engine/light.h engine/texture.h engine/bih.h
engine/serverbrowser.o: engine/model.h
engine/shader.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/shader.o: shared/ents.h shared/command.h shared/glexts.h
engine/shader.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/shader.o: engine/world.h engine/octa.h engine/light.h engine/texture.h
engine/shader.o: engine/bih.h engine/model.h
engine/sound.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/sound.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/sound.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/sound.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/texture.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/texture.o: shared/ents.h shared/command.h shared/glexts.h
engine/texture.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/texture.o: engine/world.h engine/octa.h engine/light.h
engine/texture.o: engine/texture.h engine/bih.h engine/model.h engine/scale.h
engine/ui.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/ui.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/ui.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/ui.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/ui.o: engine/textedit.h
engine/water.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/water.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/water.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/water.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/world.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/world.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/world.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/world.o: engine/light.h engine/texture.h engine/bih.h engine/model.h
engine/worldio.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/worldio.o: shared/ents.h shared/command.h shared/glexts.h
engine/worldio.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/worldio.o: engine/world.h engine/octa.h engine/light.h
engine/worldio.o: engine/texture.h engine/bih.h engine/model.h
game/ai.o: game/game.h shared/cube.h shared/tools.h shared/geom.h
game/ai.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
game/ai.o: shared/iengine.h shared/igame.h game/ai.h
game/client.o: game/game.h shared/cube.h shared/tools.h shared/geom.h
game/client.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
game/client.o: shared/iengine.h shared/igame.h game/ai.h game/ctf.h
game/entities.o: game/game.h shared/cube.h shared/tools.h shared/geom.h
game/entities.o: shared/ents.h shared/command.h shared/glexts.h
game/entities.o: shared/glemu.h shared/iengine.h shared/igame.h game/ai.h
game/game.o: game/game.h shared/cube.h shared/tools.h shared/geom.h
game/game.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
game/game.o: shared/iengine.h shared/igame.h game/ai.h
game/render.o: game/game.h shared/cube.h shared/tools.h shared/geom.h
game/render.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
game/render.o: shared/iengine.h shared/igame.h game/ai.h
game/scoreboard.o: game/game.h shared/cube.h shared/tools.h shared/geom.h
game/scoreboard.o: shared/ents.h shared/command.h shared/glexts.h
game/scoreboard.o: shared/glemu.h shared/iengine.h shared/igame.h game/ai.h
game/server.o: game/game.h shared/cube.h shared/tools.h shared/geom.h
game/server.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
game/server.o: shared/iengine.h shared/igame.h game/ai.h game/ctf.h
game/server.o: game/extinfo.h game/aiman.h
game/waypoint.o: game/game.h shared/cube.h shared/tools.h shared/geom.h
game/waypoint.o: shared/ents.h shared/command.h shared/glexts.h
game/waypoint.o: shared/glemu.h shared/iengine.h shared/igame.h game/ai.h
game/weapon.o: game/game.h shared/cube.h shared/tools.h shared/geom.h
game/weapon.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
game/weapon.o: shared/iengine.h shared/igame.h game/ai.h

shared/cube.h.gch: shared/tools.h shared/geom.h shared/ents.h
shared/cube.h.gch: shared/command.h shared/glexts.h shared/glemu.h
shared/cube.h.gch: shared/iengine.h shared/igame.h
engine/engine.h.gch: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
engine/engine.h.gch: shared/command.h shared/glexts.h shared/glemu.h
engine/engine.h.gch: shared/iengine.h shared/igame.h engine/world.h
engine/engine.h.gch: engine/octa.h engine/light.h engine/texture.h
engine/engine.h.gch: engine/bih.h engine/model.h
game/game.h.gch: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
game/game.h.gch: shared/command.h shared/glexts.h shared/glemu.h
game/game.h.gch: shared/iengine.h shared/igame.h game/ai.h

shared/crypto-standalone.o: shared/cube.h shared/tools.h shared/geom.h
shared/crypto-standalone.o: shared/ents.h shared/command.h shared/iengine.h
shared/crypto-standalone.o: shared/igame.h
shared/stream-standalone.o: shared/cube.h shared/tools.h shared/geom.h
shared/stream-standalone.o: shared/ents.h shared/command.h shared/iengine.h
shared/stream-standalone.o: shared/igame.h
shared/tools-standalone.o: shared/cube.h shared/tools.h shared/geom.h
shared/tools-standalone.o: shared/ents.h shared/command.h shared/iengine.h
shared/tools-standalone.o: shared/igame.h
engine/command-standalone.o: engine/engine.h shared/cube.h shared/tools.h
engine/command-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/command-standalone.o: shared/iengine.h shared/igame.h engine/world.h
engine/server-standalone.o: engine/engine.h shared/cube.h shared/tools.h
engine/server-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/server-standalone.o: shared/iengine.h shared/igame.h engine/world.h
engine/worldio-standalone.o: engine/engine.h shared/cube.h shared/tools.h
engine/worldio-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/worldio-standalone.o: shared/iengine.h shared/igame.h engine/world.h
game/entities-standalone.o: game/game.h shared/cube.h shared/tools.h
game/entities-standalone.o: shared/geom.h shared/ents.h shared/command.h
game/entities-standalone.o: shared/iengine.h shared/igame.h game/ai.h
game/server-standalone.o: game/game.h shared/cube.h shared/tools.h
game/server-standalone.o: shared/geom.h shared/ents.h shared/command.h
game/server-standalone.o: shared/iengine.h shared/igame.h game/ai.h
game/server-standalone.o: game/ctf.h game/extinfo.h game/aiman.h

engine/master-standalone.o: shared/cube.h shared/tools.h shared/geom.h
engine/master-standalone.o: shared/ents.h shared/command.h shared/iengine.h
engine/master-standalone.o: shared/igame.h
