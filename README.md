
[![Translation status](https://translate.fedoraproject.org/widgets/camorama/-/svg-badge.svg)](https://translate.fedoraproject.org/engage/camorama/)

# camorama - view, alter and save images from a webcam

This branch of camorama has been adapted to try out various computer-vision
filters, starting with a Reichardt motion detection filter.

These filters are added in addition to the ones already in camorama.

Adrian Bowyer
12 September 2016

----------------------------


# Build

```
./configure
make
su
make install
```

# Run

```
camorama (options)

options:

 -h  usage
 -V  version info
 -D  debugging info
 -d  <video_device> use <video_device> instead of the default (/dev/video0)
 -M  use maximum capture size (depends on camera)
 -m  use minimum capture size (depends on camera)
 -H  use middle capture size (depends on camera)
 -x  <width> width of capture
 -y  <height> height of capture

gui:
   
   sliders for various image properties.  if something doesn't work, it is not supported by your camera/driver.

   buttons turn on various filters
     fix colour:  bgr->rgb conversion
     wacky:       started out as an edge detection function, turned into, um, this ;-)
     threshold:      any pixel with an averave value < x turns black or > x turns white.  x is adjustable with the dither slider
     channel threshold:   same as above, but does it for each channel, red, green and blue.
     sobel:       didn't turn out right, but i thought this looked cool too :).
     edge detect: shoddy laplace edge detection function
     negative:    makes picture negative
     mirror:      mirror image
     colour:      colour or bw
     smooth:      smooths image

    other buttons:  capture - capture image - see prefs. 

preferences:

	general:
	
		- local capture - capture to hard drive
		- remote capture - capture and upload via ftp
		- automatic capture - capture images automatically
		- capture interval - time between captures, in minutes
		
	local capture:
	
		- directory - dir where captures will be saved
		- filename - filename for captures
		- append time to filename - should camorama append the time to the filename.  if not, it will overwrite the last image if it has the same name.
		- image type - what to save the image as.
		- add timestamp - put a timestamp in the lower left corner of the image (can be customized)
		
	remote capture:
	
		- ftp host - where you want to upload your image
		- username - username on ftp server
		- password - password on server.  this will be stored by gconf, in ~/.gconf/apps/camorama/preferences, so if you are concerned about security, don't use this feature.
		- save directory - where to save the file on the server.  using the full pathname seems to work better.
		- filename - filename for captures
		- append time to filename - should camorama append the time to the filename.  if not, it will overwrite the last image if it has the same name.
		- image type - what to save the image as.
		- add timestamp - put a timestamp in the lower left corner of the image (can be customized)
		
	timestamp:  you can use the date/time, a custom string, or both together.
		
		- use custom string - use a custom string in the timestamp
		- custom string - string you want to use in the timestamp
		- draw date/time - add date/time in the timestamp
		
```		


# Known issues

- runs slow on quickcam and when the capture size is large - getting much better....

please email me if you have any problems building or running camorama or if you have any comments/questions <greg@fixedgear.org>.

tested cams:

- creative webcam 3
- quickcam express
- 3com homeconnect camera - using the 3comhc driver, not the default kernel driver

# Requirements

- a working version of gnome 2 (http://www.gnome.org)
- video for linux (http://www.exploits.org/v4l)

# Credits

these are projects that i looked at when creating camorama:

- gspy (gspy.sourceforge.net) - gnome/v4l stuff, code for image timestamp
- gqcam  - gnome/v4l stuff  -  the more i work on camorama, the more it becomes gqcam2 ;)
- xawtv - v4l stuff
- metacity-setup (https://help.ubuntu.com/community/Metacity) - configure scripts and gnome2 stuff
- Mark McClelland for the code for YUV->RGB conversion
- gnomemeeting for the eggtray icon stuff (system tray applet). 
