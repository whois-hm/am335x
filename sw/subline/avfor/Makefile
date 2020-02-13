
FFMPEG_DIR=		$(FFDIR)
LIVE555_DIR=	$(LIVEDIR)
WQ_DIR=		$(WQDIR)
MAINLINE_DIR=	$(MAINLINEDIR)

#plugin gui embedded	using am335x option
#TSLIBE_DIR=	$(TSLIBDIR)
#DEF= 			-Dplatform_embedded -Dfbenv=\""/dev/fb0\"" -Dtsplug -DNO_OPENSSL
#INC=			-I $(WQ_DIR)/inc -I $(FFMPEG_DIR)/inc -I $(LIVE555_DIR)/inc  -I $(TSLIBE_DIR)/include  -I $(MAINLINE_DIR)
#LIB_DIR= 		-L $(WQ_DIR)/lib -L $(FFMPEG_DIR)/lib -L $(LIVE555_DIR)/lib -L $(TSLIBE_DIR)/lib 
#LIBS=			-lwq -lliveMedia -lgroupsock -lUsageEnvironment -lBasicUsageEnvironment -lavformat -lavformat -lavfilter -lavcodec -lavdevice -lavutil -lpostproc -lswresample -lswscale -lx264 -lts -ldl -lm -lpthread -lrt 

#plugin gui sdl using desktop option (assume sdl ttf has been installed)
SDLLIB_DIR=		$(SDLLIBDIR)
DEF= 			-Dplatform_sdl -Dsdl_ttf_fontpath=\""/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf\""
INC=			-I $(WQ_DIR)/inc -I $(FFMPEG_DIR)/inc -I $(LIVE555_DIR)/inc  -I $(SDLLIB_DIR)/inc   -I $(MAINLINE_DIR)
LIB_DIR= 		-L $(WQ_DIR)/lib -L $(FFMPEG_DIR)/lib -L $(LIVE555_DIR)/lib -L $(SDLLIB_DIR)/lib 
LIBS=			 -lwq -lliveMedia -lgroupsock -lUsageEnvironment -lBasicUsageEnvironment -lavformat -lavfilter -lavcodec -lavdevice -lavutil -lfdk-aac -lmp3lame -lopus -lpostproc -lswresample -lswscale -lvpx -lx264 -lx265 -lyasm -lz -lvorbis -ltheora -ldl -ltheoraenc -ltheoradec -lvorbisenc -lvdpau -lm -lX11 -lva -lva-drm -lva-x11 -lnuma  -lSDL2 -lSDL2_ttf -lfreetype -ldl -lm -lpthread -lrt -lcrypto -lssl

#CXX=			/home/hmkwon/ti_linux/toolchain/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++
CXX=			g++
CXX_SRCS=		$(wildcard *.cpp)
CXX_OBJS=		$(CXX_SRCS:.cpp=.o)



all : avfor

avfor : $(CXX_OBJS)
	$(CXX) -o avfor $(CXX_OBJS) $(LIB_DIR) $(LIBS) $(DEF)

$(CXX_OBJS) : $(CXX_SRCS)
	$(CXX) -std=c++0x -c -o  $(CXX_OBJS) $(CXX_SRCS) $(INC) $(DEF)

clean :
	rm -f *.o
	rm avfor
	
