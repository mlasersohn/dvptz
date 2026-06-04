<img width="3840" height="2160" alt="four" src="https://github.com/user-attachments/assets/537132d5-01e1-4763-bd9b-0da114c8ffd4" />

DVPTZ is an elaborate video and audio management systems for Linux. It accepts video inputs through all sources accepted by OpenCV,
as well as several other sources including NDI® (NDI is registered trademark of Vizrt NDI AB), still and video files, real-time 3D
rendering, HTML rendering, pipes, plugins, and much else. Audio sources include all those supported by PulseAudio as well as several 
other audio file formats. It supports PTZ cameras using Visca, either through the network or USB. Optionally, PTZ functionality can 
be accessed through NDI or V4L functions. Output to various video file formats is supported as well as numerous streaming formats.
Realtime streaming to streaming sites, such as Twitch and Youtube is also provided. There are too many features to list here. See below 
for a cursory rundown. For those vaguely interested, DVPTZ stands for Digital Video Pan Tilt Zoom, intending to place emphasis on the program's support for Visca, NDI, and V4L PTZ capabilities. I discourage people from referring to it as "div-putz", but also recoginize that this is inevitable.

# Building
The Makefile defaults to mostly using the clang C++ compiler. Switching it over to only using g++ and gcc is a simple matter of editing
the lines, selecting which pair you mean to use:

MCXX=g++
MCC=gcc

or

MCXX=clang++
MCC=clang

One of the several files that need to be compiled, "libhtml_window.so" must be compiled with g++, not clang++, so you must
have the gnu compiler available to make this optional shared library.

To build DVPTZ, you must have installed the following libraries:

## Required Libraries
### OpenCV (4.5.2 or greater, likely required)
libopencv_imgproc
libopencv_videoio
libopencv_core
libopencv_imgcodecs
libopencv_highgui
libopencv_dnn
libopencv_objdetect

### AV/FFMPEG (58.76.100 or greater, might be required)
libavformat
libavcodec
libavutil
libswresample
libswscale
libavfilter

### Pulse Audio (0.24.1)
libpulse
libpulse-simple

### VLC
libvlc

### Curl
libcurl

### Cairo
libcairo

### FLTK (1.4 or greater, required)
libfltk
libfltk_images

### X11
libXcursor
libX11
libxcb
libXdmcp
libXau
libXext
libXtst
libm
libXft
libXrender
libXfixes
libXinerama
libXrender
libXcomposite

### VISCA
libvisca_ip

### JSON
libcjson

### Font
libfontconfig
libexpat
libfreetype

### Fourier
libfftw3

### XML
libxml2

### Blend2D
libblend2d

### JPEG
libjpeg

### IRC (should one day be made optional)
libircclient

### Misc
libuuid
librt
libz
liblzma
libbz2
libmagic
stdc++

## Optional Libraries
To build the optional library "libosg_camera.so" which supports OSG (rendered 3D models as video sources),
you must have the following OSG libraries installed:

libosg
libosgViewer
libosgDB
libosgGA

If you do not want to build in OSG support, edit or delete the following line in the Makefile:
OSG := 1

To build the optional library "libhtml_window.so" which supports rendering HTML as a video source, you
must set the value of CEF_PATH in the Makefile to point at the directory where CEF keeps its include
files and have the following CEF libraries installed:

libcef.so ; copied right into the DVPTZ build directory

CEF_PATH/build/libcef_dll_wrapper/libcef_dll_wrapper.a ; down under CEF_PATH somewhere

If you do not want to build in CEF support, edit or delete the following line in the Makefile:
CEF := 1

if libndi.so is available during runtime, DVPTZ will attempt to open it and use the NDI functions
it makes available. 

Once you have installed all of the necessary libraries and have edited the Makefile appropriately (if necessary), building 
DVPTZ is simply a matter of invoking make: 

$ make
or
$ make -f Makefile

# Running
In the simplest case, running DVPTZ is just a matter of invoking the executable, "dvptz". This will cause the program to
invoke the "intro" executable by default, which will display status messages as the dvptz program starts. During this
period, dvptz will scan your system \("/dev" directories\) for cameras and microphones and various other audio sources. There
are many startup flags and arguments allowing you to customize dvptz on startup as well as bypass the scan and specify
exactly which audio and video sources you wish to have available during startup.

## Arguments:
#### Specify a video device
dvptz --source=/dev/video0

#### Specify a device and a backend
dvptz --source=/dev/video0::V4L

#### Specify a device and an alias
dvptz --source=/dev/video0\[alias=Sony\]

####  Specify a device and a backend with fourcc code (must be 4 characters long)
dvptz --source=/dev/video0::V4L:MJPG

####  Specify an audio input device
dvptz --audio_source=alsa_input.usb-audio-technica____AT2020_USB-00.analog-stereo

####  Load a full setup. This will overwrite most other options.
dvptz --load=your_file.setup

####  Use an IP camera at a RTSP URL with login and password
dvptz --source=rtsp://your_login:your_password@url_for_your_webcam.org/live

####  Use USB camera number 4
dvptz --source=4

####  Set main window width to 1920, Defaults to fullscreen.
dvptz --main_width=1920

####  Set main window height to 1080, Defaults to fullscreen.
dvptz --main_width=1080

####  Set preferred video width to 640
dvptz --width=640

####  Set preferred video height to 480
dvptz --height=480

####  Scale the interface to system wide screen scaling.
dvptz --auto_scale

####  Force a scaling factor to the interface.
dvptz --forced_scale=2.0

####  Set preferred output video width to 1280. Output defaults to input.
dvptz --output_width=1280

####  Set preferred output video height to 720. Output defaults to input.
dvptz --output_height=720

####  Set preferred display video width to 1280. Display defaults to input.
dvptz --display_width=1280

####  Set preferred display video height to 720. Display defaults to input.
dvptz --display_height=720

####  Set the FPS to a specific value, avoiding testing
dvptz --fps=18

####  Sets the interval between frames to a rigid length. Specified in seconds.
dvptz --interval=0.01

####  Split the main screen between all the video sources
dvptz --split

####  Record audio and video to raw files (unmuxed) and mux as a separate step.
dvptz --buffered

####  Flip horizontally or vertically or both
dvptz --flip=horizontal --flip=vertical

####  Do not record audio.
dvptz --no_audio

####  Reopen old .bin (temporary files) to encode. Record is turned off.
dvptz --old

####  Do not scan for USB cameras.
dvptz --no_scan

####  Do not scan for input audio devices.
dvptz --no_audio_scan

####  Scan for PTZ serial ports.
dvptz --scan_for_ptz

####  Record entire main window, encoding on exit. Usually used to make tutorial videos for the program itself.
dvptz --record_main_window

####  Record the desktop as another camera.
dvptz --record_desktop

####  Record the desktop as another camera, specifying area to record as x,y,w,h.
dvptz --record_desktop=20,40,640,360

####  Record each recording camera to its own file.
dvptz --multi_stream

####  Display the camera that is currently recording.
dvptz --display_recording_camera

####  Recording follows the displayed camera.
dvptz --recording_follows_display

####  Blend automatically between cuts when editing with the previewer.
####  Also fades in and out from black at the beginning and end.
dvptz --transition=blend

####  Allows for PTZ camera control using VISCA over the specified serial port. A colon following the path
####  followed by a 1, indicates that this interface does not reply.
dvptz --ptz=/dev/ttyUSB0:1

####  Reset PTZ cameras to home position on launch.
dvptz --ptz_home.

####  Initialize object detection. Without this object detection is not offered.
dvptz --detect

####  Use alternate DNN/YOLO files for recognition. If anyone one of these is provided,
####  the other two must also be present. The default values are:
####  	"yolov3-openimages.cfg"
####  	"yolov3-openimages.weights"
####  	"openimages.names"
####  Or, depending on the value of --yolo_model=
####  	"yolov3.cfg"
####  	"yolov3.weights"
####  	"coco.names"
dvptz --yolo_cfg=cfg_filename --yolo_weights=weights_filename --yolo_names=names_filename

####  Use the default YOLO filenames for COCO or OpenImages. This defaults to OpenImages.
dvptz --yolo_model=coco

####  If JPEG streaming is turned on, stream the current camera's image to
####  a custom server running at www.example.com on port 20000. Port 20001
####  will be used as the PTZ control port. If port 10000 was selected
####  port 10001 would be the control port.
dvptz --jpeg_streaming=www.example.com:20000

####  Stream to a streaming server like Twitch or YouTube Live.
####  The entire URL must be specifed, including the protocol specifier
####  i.e.: smpt:// or rtmp://, etc. and any other URL embedded arguments
####  like your key
dvptz --streaming=rtmp://www.streaming_server.com/your_key

####  Stream to the server specified in --streaming= or save to a muxed file if no server is specified.
dvptz --streaming_only

####  Streaming audio quality expressed as a factor. The default is best. Base value is 11025 hz, so.
####  dvptz --streaming_audio_quality=good would be 11025 hz.
####  dvptz --streaming_audio_quality=better would be 22050 hz.
####  dvptz --streaming_audio_quality=best would be 44100 hz.
dvptz --streaming_audio_quality=good

####  Embed the picture in picture stream into the recorded output.
dvptz --embed_pip

####  Set the position of the picture in picture.
dvptz --pip_position=0.8,0.1

####  Set the rgb color of the border around the pip. Set it to -1,-1,-1 for no border.
dvptz --pip_highlight=255,128,64

####  Set the size of pip as fraction of its real size. The default is 1/4th (ie: 0.25)
dvptz --pip_size=0.35

####  Allows for more than one pip.
dvptz --multipip=right

####  Set the grid size for placement of sources. Default is 1 for no grid.
dvptz --grid_size=10

####  Set the desktop audio monitor device (a null sink in pulseaudio).
dvptz --desktop_monitor=my_null_sink

####  Do not query the system for available codecs and offer a menu of options.
dvptz --no_query_codecs

####  Test the codecs found at current settings to determining if they are usable,
####  eliminating those that are not from the offered options.
dvptz --test_codecs

####  Timestamp video images.
dvptz --timestamp

####  Disregard video setting when starting, using defaults and command line arguments instead.
dvptz --disregard_settings

####  Cause the primary interface elements to have a transparent background.
dvptz --transparent_interface

####  When scaling input images to output, keep aspect ratio and border excess.
dvptz --no_frame_scaling

####  Crop image to display size rather than scaling it.
dvptz --crop_display

####  Crop image to output size rather than scaling it.
dvptz --crop_output

####  Add a plugin transition library file to be searched.
dvptz --transition=filename.so

####  Add a plugin transition from the library specified in --transition_plugin_file=...
dvptz --transition_plugin=transition_name

####  Add a plugin filter library file to be searched.
dvptz --filter_plugin_file=filename.so

####  Add a plugin filter from the library specified in --filter_plugin_file=...
dvptz --filter_plugin=filter_name

####  Add a screen capture plugin library file to be searched.
dvptz --capture_plugin_file=filename.so

####  Force GUI elements to persist on the screen rather than appearing when the mouse approaches.
dvptz --retain_commands
dvptz --retain_cameras
dvptz --retain_audio
dvptz --retain_ptz

####  Force all GUI elements to persist on the screen rather than appearing when the mouse approaches.
dvptz --lock_panels

####  Animate (slide) the GUI elements when the mouse approaches.
dvptz --animate_panels

####  Do not allow the user to change directories in the file selection dialog.
dvptz --exclude_directories

####  Hide the status message.
dvptz --hide_status

####  Select a camera to be active.
dvptz --select_camera=camera_name

####  Select an audio source to be active.
dvptz --select_audio=audio_device_name

For example, when starting dvptz, I use the following options:
--animate_panels --no_scan --source=/dev/v4l/by-id/usb-054c_SRG-120DU_Series-video-index0\[alias="Sony SRG-120DU"\] --source=/dev/v4l/by-id/usb-AVer_Inc._CAM520_5308138900019-video-index0::V4L2:MJPG\[alias="Aver 520"\] "--source=ndi_p216://NDI_HX \(BZBGEAR-NDI-787264200195\)"\[alias="BZBGear"\] --no_frame_scaling --yolo_model=coco --streaming_only --width=1280 --height=720 --container=flv --detect --ptz=udp://192.168.0.100:52381\[alias="Sony"\]\[lock="Sony SRG-120DU"\]\[bind="Sony SRG-120DU"\] --ptz=/dev/ttyUSB0\[alias="Aver"\]\[lock="Aver 520"\]\[bind="Aver 520"\] --ptz=tcp://192.168.0.146:1259\[alias="BZBGear"\]\[lock="BZBGear"\]\[bind="BZBGear"\] --recording_follows_display --message_delay=250000

This allows me to open my Sony SRG-120DU as a USV/UVC device, my AVer CAM520 as another USB/UVC device, and my BZBGear camera as an NDI device. I am
setting up YOLO object detection using coco. I set the resolution to 1280 and 720 with a default container of FLV. I also setup PTZ over UDP for the
Sony, through USB for the Aver, and through TCP (NDI) for the BZBGear camera. Furthermore, I set the system to record which ever camera is displayed.

Most, maybe all of these options can be set once within the program, as well as many more. 

# Some of the Features
DVPTZ allows the user to save the state of the entire
system as well as the state of any of the cameras or other sources. Within the program, or using the --source= flag, you can select between dozens
of supported input sources. You can also set hundreds \(thousands?\) of combinations of containers, and audio and video codecs, for output files; the exact
number depending on how many you have licensed/installed on your system and made available to the ffmpeg library. Many container formats for images,
audio, and video are also available as input sources.

DVPTZ is also capable of streaming to the usual stream receiving websites, including Twitch, YouTube, and Facebook as well as any others that allow
for RTMP protocol. DVPTZ can also be used as a NDI source on output for other programs, devices, or sites.

DVPTZ has many features including transitions; various sorts of plug-ins for audio and video effects and GUI elements; render HTML both locally and 
from the websites, including entire websites; drawing various directly on the screen, including fonts and text; picture in picture from multiple
sources; detecting objects and labeling them; triggering recording based on object recognition, motion, time of day, interval, and changing light 
conditions. DVPTZ allows for multiple audio sources to be mixed or muted, including recording the output of other programs. The entire desktop
or individual windows can also be captured as video sources, allowing for game-play to be streamed.

DVPTZ supports control over PTZ cameras using VISCA. If the camera does not provide VISCA through either USB, RS232, or TCP/UDP, DVPTZ will 
attempt to control the camera through the PTZ functions supported by V4L2. Through VISCA, DVPTZ supports pan, tilt, zoom, focus, as well as
turning on and off many optional camera features such as autofocus, auto-exposure, and much more. Almost all of the VISCA parameters are
adjustable.

DVPTZ's user interface was built primarily with FLTK, with a lot of help from Cairo and OpenCV. It provides many examples of how to do things
with these libraries and also quite a few examples of how not to do things with the same libraries. As that much of this code is experimental
and some of it is ill-advised, borrow and run this code at your own risk. I will support you in a haphazard and willy-nilly sort of way, so
do not expect timely answers to questions, nor will I defend myself or this program from criticism of any sort. I'd rather have fun.

# LICENSE
This source code is copyrighted by Mark Lasersohn (c) 2025. You are licensed to use any portion of it for any non-nefarious
purpose. I am not responsible in any way for anything that happens to you or anyone else if you do. I make no claim that this
program or any of the code you might lift from it does much of anything you might want. It might, but then I am not entirely
sure you and I agree on terminology.

All of the libraries necessary to build and run DVPTZ are the property of the various license holders, and have nothing to do 
with me, really. I am attempting to abide by the terms of the LGPL (available all over the internet), as well as several other 
liberal open-source oriented licenses. It is possible to use DVPTZ to decode and encode audio and video using any codec you have 
installed on your system. That is up to you, not me. DVPTZ simply scans what you have made available and then uses whatever 
you tell it to use. If my program is using a library you authored and you would like some attribution, please let me know. 
I will be happy to include some words here or during startup to that effect. If you would like me to include your license tucked
in among the source code, I will do that as well.

# Bye
Farewell and have fun.
Mark Lasersohn
<img width="1280" height="720" alt="photo_1777579966201" src="https://github.com/user-attachments/assets/042971a2-eca0-4410-96ff-193751c25b6f" />

