# -*- makefile -*-

CEF := 1
OSG := 1

CEF_PATH=/home/laser/Downloads/OffScreenCEF/thirdparty/cef_binary

MARCH=-m64

# DBUG=-O3 $(MARCH) -D_FILE_OFFSET_BITS=64
DBUG=-ggdb $(MARCH) -D_FILE_OFFSET_BITS=64 -DDEBUG=1

EXAMPLES = dvptz intro

# MCXX=g++
# MCC=gcc
MCXX=clang++
MCC=clang

INCS = folder.h argv_split.h dr_mp3.h embed_app.h html_window.h muxer.h PulseAudio.h dvptz.h dr_flac.h dr_wav.h image_memory.h osg.h render_html.h vlc_window.h video_player.h common.h

CFLAGS = $(DBUG) -fno-diagnostics-color -Wno-c++17-extensions -Wno-deprecated-declarations -Wno-unused-result -Wno-write-strings -c -DFLTK_HAVE_CAIRO -D_GNU_SOURCE -D_REENTRANT -DFLTK_1_1 -I. -I/usr/include/python3.10 -I/usr/local/include/opencv4 -I/usr/local/include -I/usr/X11R6/include -I/usr/include/cairo -I/usr/local/include/ndi -I/usr/local/include/lunasvg -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/local/include/numpy

LD = $(MCC)
LDFLAGS = $(DBUG) -L/usr/local/lib -L/usr/lib -L/usr/X11R6/lib

AR = lib

CVLIBS = -lrt -ljpeg -lm -lxml2 -lfontconfig -lexpat -lfreetype -lpng -lz

STDDYN = -lopencv_imgproc -lopencv_videoio -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_dnn -lopencv_objdetect -lavformat -lavcodec -lavutil -lswresample -lswscale -lavfilter -lpulse -lpulse-simple -lvlc -lpython3.10 -lcurl -luuid -lblend2d -lcairo -lfltk -lfltk_images -llunasvg -lplutovg -lrt -ljpeg -lXcursor -lX11 -lxcb -lXdmcp -lXau -lXext -lXtst -lm -lXft -lXrender -lXfixes -lXinerama -lXrender -lXcomposite -lxml2 -lfontconfig -lexpat -lfreetype -lfftw3 -lz -llzma -lvisca_ip -lbz2 -lircclient -lcjson -lmagic -lFLAC -lpangocairo-1.0 -lgobject-2.0 -lpango-1.0 -lasound -ludev -lstdc++

all: $(EXAMPLES)

ifeq ($(CEF),1)
# NOTE libhtml_window.so is compiled with g++ (clang++ will core dump it)
libhtml_window.so : html_window.cpp $(INCS)
	@echo
	@echo "Compile HTML Window (CEF)"
	g++ $(DBUG) -fPIC -shared -DCEF_USE_SANDBOX -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -Wno-unused-parameter -Wno-write-strings -Wno-class-memaccess -I/usr/include/cairo -I$(CEF_PATH) -I$(CEF_PATH)/include html_window.cpp ./libcef.so $(CEF_PATH)/build/libcef_dll_wrapper/libcef_dll_wrapper.a -o libhtml_window.so
	cp libhtml_window.so ..
endif

ifeq ($(OSG),1)
libosg_camera.so: osg.cpp osg.h
	@echo
	@echo "Compile OSG Renderer"
	$(MCXX) $(DBUG) -fPIC -shared -Wno-deprecated-declarations -Wno-unused-parameter -Wno-write-strings osg.cpp -I/usr/include/cairo -L/usr/local/lib -L/usr/lib -L/usr/X11R6/lib -losg -losgViewer -losgDB -losgGA -o libosg_camera.so
	cp libosg_camera.so ..
endif

test_serial_ports.o: test_serial_ports.cpp
	@echo
	@echo "Compile Serial Port Tester"
	$(MCXX) -c test_serial_ports.cpp

cow_simple_pulse.o: cow_simple_pulse.c
	@echo
	@echo "Compile Simple PulseAudio"
	$(MCC) $(DBUG) -c cow_simple_pulse.c

curl.o: curl.cpp
	@echo
	@echo "Compile Curl Module"
	$(MCXX) $(DBUG) -c curl.cpp

run_python.o: run_python.cpp
	@echo
	@echo "Compile Python Module"
	$(MCXX) $(DBUG) $(CFLAGS) -c run_python.cpp

irc.o: irc.c
	@echo
	@echo "Compile IRC Module"
	$(MCC) $(DBUG) -c irc.c

read_wave.o: read_wave.cpp
	@echo
	@echo "Compile Read Audio Wave"
	$(MCXX) -Wno-deprecated-declarations $(CFLAGS) read_wave.cpp

extract_video.o: extract_video.cpp
	@echo
	@echo "Compile Video Extractor"
	$(MCXX) -Wno-deprecated-declarations $(CFLAGS) extract_video.cpp

extract_audio.o: extract_audio.cpp
	@echo
	@echo "Compile Audio Extractor"
	$(MCXX) -Wno-deprecated-declarations $(CFLAGS) extract_audio.cpp

vlc_window.o: vlc_window.cpp
	@echo
	@echo "Compile VLC Window"
	$(MCXX) $(CFLAGS) vlc_window.cpp

video_player.o: video_player.cpp video_player.h common.h
	@echo
	@echo "Compile Video Player"
	$(MCXX) $(CFLAGS) video_player.cpp

muxer.o: muxer.cpp $(INCS)
	@echo
	@echo "Compile Muxer Module"
	$(MCXX) -D__STDC_CONSTANT_MACROS $(CFLAGS) -I$(CEF_PATH) -I$(CEF_PATH)/include muxer.cpp

PulseAudio.o: PulseAudio.cpp $(INCS)
	@echo
	@echo "Compile PulseAudio Module"
	$(MCXX) -D__STDC_CONSTANT_MACROS $(CFLAGS) PulseAudio.cpp

pulse_devices.o: pulse_devices.cpp $(INCS)
	@echo
	@echo "Compile Pulse Device Detection"
	$(MCXX) -D__STDC_CONSTANT_MACROS $(CFLAGS) pulse_devices.cpp

embed_app.o : embed_app.cpp $(INCS)
	@echo
	@echo "Compile Embedded App Module"
	$(MCXX) $(CFLAGS) embed_app.cpp

dvptz.o: dvptz.cpp $(INCS) 
	@echo
	@echo "Compile dvptz"
	$(MCXX) -D__STDC_CONSTANT_MACROS $(CFLAGS) -I$(CEF_PATH) -I$(CEF_PATH)/include dvptz.cpp 

networking.o: networking.cpp $(INCS)
	@echo
	@echo "Compile Networking Module"
	$(MCC) $(CFLAGS) networking.cpp 

dvptz: dvptz.o test_serial_ports.o embed_app.o networking.o pulse_devices.o PulseAudio.o muxer.o vlc_window.o video_player.o extract_video.o extract_audio.o run_python.o curl.o cow_simple_pulse.o irc.o read_wave.o libhtml_window.so libosg_camera.so
	@echo
	@echo "Link dvptz"
	$(LD) -o dvptz $(LDFLAGS) dvptz.o test_serial_ports.o embed_app.o networking.o pulse_devices.o PulseAudio.o muxer.o vlc_window.o video_player.o extract_video.o extract_audio.o run_python.o curl.o cow_simple_pulse.o irc.o read_wave.o $(STDDYN)
	@echo "Copy dvptz"
	cp -v dvptz ../dvptz

intro: intro.cpp intro.h
	@echo
	@echo "Make intro"
	$(MCXX) -I/usr/include/cairo intro.cpp -o intro -L/usr/local/lib -lfltk
	cp -v intro ..
