
[![Translation status](https://translate.fedoraproject.org/widgets/camorama/-/svg-badge.svg)](https://translate.fedoraproject.org/engage/camorama/)

# Build

The build dependencies vary along distributions.

On Fedora:

```
sudo dnf install gcc make gettext-devel libv4l-devel gtk3-devel cairo-devel \
	gdk-pixbuf2-devel gnome-common
```

On Ubuntu/Debian:

```
sudo apt-get install gcc make gettext libv4l-dev libgtk-3-dev libcairo2-dev \
	libgdk-pixbuf2.0-dev gnome-common
```

Once the dependencies are installed, building and installing camorama can
be done with:

```
./autogen.sh
make
sudo make install
```

# Run

```
camorama (options)

Help Options:
  -h, --help                 Show help options
  --help-all                 Show all help options
  --help-gapplication        Show GApplication options
  --help-gtk                 Show GTK+ Options

Application Options:
  -V, --version              show version and exit
  -d, --device               v4l device to use
  -D, --debug                enable debugging code
  -x, --width                capture width
  -y, --height               capture height
  -M, --max                  maximum capture size
  -m, --min                  minimum capture size
  -H, --half                 middle capture size
  -R, --read                 use read() rather than mmap()
  -S, --disable-scaler       disable video scaler
  -U, --userptr              use userptr pointer rather than mmap()
  --dont-use-libv4l2         use userptr pointer rather than mmap()
  -i, --input                v4l device input to use
  --display=DISPLAY          X display to use
```

# GUI:

That's the Camorama GUI:

```
+---------------------------------------------------------------+
|     Camorama - HD Pro Webcam C920 - 800x600 (scale: 75%)      |
| File   Edit   View   Help                                     |
+------------------------------------------+--------------------+
|                                          |  Effects           |
|                    .~.                   |                    |
|                    /V\                   |                    |
|                   // \\                  |                    |
|                  /(   )\                 |                    |
|                   ^`~'^                  |                    |
|                                          |                    |
+------------------------------------------+--------------------+
| [Show Adjustments]        [Full Screen]        [Take Picture] |
| Contrast:   128 ++++++++++++++++++|-------------------------- |
| Brightness: 128 ++++++++++++++++++|-------------------------- |
| Zoom:         0 |-------------------------------------------- |
| Color:      128 ++++++++++++++++++|-------------------------- |
|                                                               |
| 30.00 fps - current   30.24 fps - average                     |
+---------------------------------------------------------------+
```

The menu contains:
- `File` option has 3 items:
  - `Take Picture`: Takes an instant shot;
  - `Toggle Full Screen`: Toggles between full screen and normal mode;
  - `Quit`: quits Camorama.
- `Edit`: Has a preferences menu that allows to setup periodic screen shots.
  See more details below.
- `View`: Allows disabling/enabling the Effects view and the sliders. It
   also allows selecting the video resolution, which also affects the frame
   rate.
- `Help`: Has an `About` button that show Camorama version, credits and
   license.

The camera image is shown at the top left, just below the menu.
Its dispayed image can be affected by several optional effects.

The slides cover some image controls that are supported by your camera.

When there's no effects applied, the "Effects" part of the window
will be empty. Right-clicking with the mouse should open an small menu
that allows adding/removing efect filters to the image.

The supported effect filters are:

- Invert:
  - invert image colors, making it a picture negative.
- Threshold (Overall):
  - any pixel with an average value `< x` turns black or `> x` turns white.
 `x` is adjusted via a "Theshold" slider.
- Threshold (Per Channel):
  - same as above, but sets threshold per each each color channel,
    e.g. red, green and blue. `x` is adjusted via a "Channel Theshold" slider.
- Mirror:
  - mirror image
- Wacky:
  - started out as an edge detection function, turned into, um, this ;-)
- Reichardt:
  - Hassenstein-Reichardt movement detection filter
- Smooth:
  - smooths image
- Laplace:
  - shoddy Laplace edge detection function
- Monochrome:
  - Remove colors from the image
- Monochrome (Weight):
  - Remove colors from the image
- Sobel:
  - didn't turn out right, but i thought this looked cool too :).

The buttons below the image and effects are:
- `Show Adjustments` show/hide the sliders
- `Full Screen`: Shows camorama in full screen mode;
- `Take Picture` capture image

The `Edit/Preferences` menu allows to setup several properties related
to image capture:

- `General`:
  - Automatic capture: enables periodic screen shots.
    When selected, allows to select the capture interval.

- `Local Capture`: adjust the parameters for local capture/screen shots.
   - directory - dir where captures will be saved
   - filename - filename for captures
   - append time to filename - should camorama append the time
     to the filename.  When enabled, the file name will use this format:
     %Y-%m-%d %H:%M:%S. It will also append a 3-digit number in order to
     allow capturing more than one frame per second.
     If not selected, a newly captured image will overwrite the last
     image if it has the same name.
   - add a timestamp to captured images - put a timestamp in the lower left
     corner of the image. The format of such timestamp is defined via
     timestamp properties (see below).
   - image type - what to save the image as.
 - `Remote Capture`: capture and upload capture image to a remote server.
   Please notice that the credentials are maintained by Gnome;
   - server: IP or domain for remote capture;
   - type: type of capture. Can be ftp, scp or smb;
   - save directory - where to save the file on the server.
     Using the full pathname seems to work better.
   - filename - filename for captures
   - append time to filename - should camorama append the time
     to the filename.  When enabled, the file name will use this format:
     %Y-%m-%d %H:%M:%S. It will also append a 3-digit number in order to
     allow capturing more than one frame per second.
     If not selected, a newly captured image will overwrite the last
     image if it has the same name.
   - add a timestamp to captured images - put a timestamp in the lower left
     corner of the image. The format of such timestamp is defined via
     timestamp properties (see below).
   - image type - what to save the image as.
- `Timestamp`:  you can use the date/time, a custom string,
  or both together.
  - use custom string - use a custom string in the timestamp
  - custom string - string you want to use in the timestamp
  - draw date/time - add date/time in the timestamp

Please add an issue at https://github.com/alessio/camorama if you
have any problems building or running camorama or if you have any
comments/questions.

# Credits

These are projects that the original author looked at when
creating Camorama:

- gspy (gspy.sourceforge.net) - gnome/v4l stuff, code for image timestamp
- gqcam  - gnome/v4l stuff  -  the more i work on camorama, the more it becomes gqcam2 ;)
- xawtv - v4l stuff
- metacity-setup (https://help.ubuntu.com/community/Metacity) - configure scripts and gnome2 stuff
- Mark McClelland for the code for YUV->RGB conversion
- gnomemeeting for the eggtray icon stuff (system tray applet).
