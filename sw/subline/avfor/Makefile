#plugin gui embedded
TSLIBE_DIR=		$(TSLIBDIR)
DEF= 			-Dplatform_embedded -Dfbenv=\""/dev/fb0\"" -Dtsplug -DNO_OPENSSL

FFMPEG_DIR=		$(FFDIR)
LIVE555_DIR=	$(LIVEDIR)
WQ_DIR=		$(WQDIR)
MAINLINE_DIR=	$(MAINLINEDIR)

CXX=			/home/hmkwon/ti_linux/toolchain/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++
INC=			-I $(WQ_DIR)/inc -I $(FFMPEG_DIR)/inc -I $(LIVE555_DIR)/inc  -I $(TSLIBE_DIR)/include  -I $(MAINLINE_DIR)
CXX_SRCS=		$(wildcard *.cpp)
CXX_OBJS=		$(CXX_SRCS:.cpp=.o)
LIB_DIR= 		-L $(WQ_DIR)/lib -L $(FFMPEG_DIR)/lib -L $(LIVE555_DIR)/lib -L $(TSLIBE_DIR)/lib 
LIBS=			-lwq -lliveMedia -lgroupsock -lUsageEnvironment -lBasicUsageEnvironment -lavformat -lavformat -lavfilter -lavcodec -lavdevice -lavutil -lpostproc -lswresample -lswscale -lx264 -lts -ldl -lm -lpthread -lrt 


all : avfor

avfor : $(CXX_OBJS)
	$(CXX) -o avfor $(CXX_OBJS) $(LIB_DIR) $(LIBS) $(DEF)

$(CXX_OBJS) : $(CXX_SRCS)
	$(CXX) -std=c++0x -c -o  $(CXX_OBJS) $(CXX_SRCS) $(INC) $(DEF)

clean :
	rm -f *.o
	rm avfor
	
