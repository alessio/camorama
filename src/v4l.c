#include"v4l.h"
#include<time.h>
#include<errno.h>
#include<gnome.h>
#include <libv4l1.h>
#include "support.h"

extern int frame_number;
extern int errno;

void print_cam(cam *cam){
	printf("\nCamera Info\n");
	printf("-------------\n");
	printf("device = %s, x = %d, y = %d\n",cam->video_dev, cam->x,cam->y);
	printf("depth = %d, desk_depth = %d, size = %d\n",cam->depth,cam->desk_depth,cam->size);
	printf("capture directory = %s, capture file = %s\n",cam->pixdir, cam->capturefile);
	printf("remote capture directory = %s, remote capture file = %s\n",cam->rpixdir, cam->rcapturefile);
	printf("remote host = %s, remote login = %s\n",cam->rhost,cam->rlogin);
	printf("timestamp = %s\n\n",cam->ts_string);
	
}
void print_palette(int p)
{

   switch (p) {
   case VIDEO_PALETTE_HI240:
      printf("High 240 cube (BT848)\n");
      break;

   case VIDEO_PALETTE_RGB565:
      printf("565 16 bit RGB\n");
      break;

   case VIDEO_PALETTE_RGB24:
      printf("24bit RGB\n");
      break;

   case VIDEO_PALETTE_RGB32:
      printf("32bit RGB\n");
      break;

   case VIDEO_PALETTE_RGB555:
      printf("555 15bit RGB\n");
      break;

   case VIDEO_PALETTE_YUV422:
      printf("YUV422 capture");
      break;

   case VIDEO_PALETTE_YUYV:
      printf("YUYV\n");
      break;

   case VIDEO_PALETTE_UYVY:
      printf("UYVY\n");
      break;

   case VIDEO_PALETTE_YUV420:
      printf("YUV420\n");
      break;

   case VIDEO_PALETTE_YUV411:
      printf("YUV411 capture\n");
      break;

   case VIDEO_PALETTE_RAW:
      printf("RAW capture (BT848)\n");
      break;

   case VIDEO_PALETTE_YUV422P:
      printf("YUV 4:2:2 Planar");
      break;

   case VIDEO_PALETTE_YUV411P:
      printf("YUV 4:1:1 Planar\n");
      break;

   case VIDEO_PALETTE_YUV420P:
      printf("YUV 4:2:0 Planar\n");
      break;

   case VIDEO_PALETTE_YUV410P:
      printf("YUV 4:1:0 Planar\n");
      break;
   }
}

void camera_cap(cam * cam)
{
   char *msg;
   if(v4l1_ioctl(cam->dev, VIDIOCGCAP, &cam->vid_cap) == -1) {
      if(cam->debug == TRUE) {
         fprintf(stderr, "VIDIOCGCAP  --  could not get camera capabilities, exiting.....\n");
      }
      msg = g_strdup_printf(_("Could not connect to video device (%s).\nPlease check connection."), cam->video_dev);
      error_dialog(msg);
      g_free(msg);
      exit(0);
   }
   if(cam->x > 0 && cam->y > 0) {
      if(cam->vid_cap.maxwidth < cam->x) {
         cam->x = cam->vid_cap.maxwidth;
      }
      if(cam->vid_cap.minwidth > cam->x) {
         cam->x = cam->vid_cap.minwidth;
      }
      if(cam->vid_cap.maxheight < cam->y) {
         cam->y = cam->vid_cap.maxheight;
      }
      if(cam->vid_cap.minheight > cam->y) {
         cam->y = cam->vid_cap.minheight;
      }
   } else {
      switch (cam->size) {
      case PICMAX:
         cam->x = cam->vid_cap.maxwidth;
         cam->y = cam->vid_cap.maxheight;
         break;

      case PICMIN:
         cam->x = cam->vid_cap.minwidth;
         cam->y = cam->vid_cap.minheight;
         break;

      case PICHALF:
         cam->x = cam->vid_cap.maxwidth / 2;
         cam->y = cam->vid_cap.maxheight / 2;
         break;

      default:
         cam->x = cam->vid_cap.maxwidth / 2;
         cam->y = cam->vid_cap.maxheight / 2;
         break;
      }
   }
   if((cam->vid_cap.type & VID_TYPE_CAPTURE) != 1) {
      cam->read = TRUE;
   }

   if(cam->debug == TRUE) {
      printf("\nVIDIOCGCAP\n");
      printf("device name = %s\n", cam->vid_cap.name);
      printf("device type = %d\n", cam->vid_cap.type);
      if(cam->read == FALSE){
		  printf("can use mmap()\n");
	  }
      printf("# of channels = %d\n", cam->vid_cap.channels);
      printf("# of audio devices = %d\n", cam->vid_cap.audios);
      printf("max width = %d\n", cam->vid_cap.maxwidth);
      printf("max height = %d\n", cam->vid_cap.maxheight);
      printf("min width = %d\n", cam->vid_cap.minwidth);
      printf("min height = %d\n", cam->vid_cap.minheight);
   }
}

void
set_pic_info(cam* cam) {
	char *msg;
	if(cam->debug) {
		g_message("SET PIC");
	}
	cam->vid_pic.palette = VIDEO_PALETTE_RGB24;
	cam->vid_pic.depth = 24;
	//cam->vid_pic.palette = VIDEO_PALETTE_YUV420P;
	if(v4l1_ioctl(cam->dev, VIDIOCSPICT, &cam->vid_pic) == -1) {
		if(cam->debug) {
			g_message("VIDIOCSPICT  --  could not set picture info, exiting....");
		}
		msg = g_strdup_printf(_("Could not connect to video device (%s).\nPlease check connection."), cam->video_dev);
		error_dialog(msg);
		g_free(msg);
		exit(0);
	}
}

void get_pic_info(cam * cam){
//set_pic_info(cam);
   char *msg;
	
   if(v4l1_ioctl(cam->dev, VIDIOCGPICT, &cam->vid_pic) == -1) {
      msg = g_strdup_printf(_("Could not connect to video device (%s).\nPlease check connection."), cam->video_dev);
      error_dialog(msg);
      if(cam->debug == TRUE) {
         fprintf(stderr, "VIDIOCGPICT  --  could not get picture info, exiting....\n");
      }
      g_free(msg);
      exit(0);
   }
	
   if(cam->debug == TRUE) {
      printf("\nVIDIOCGPICT:\n");
      printf("bright = %d\n", cam->vid_pic.brightness);
      printf("hue = %d\n", cam->vid_pic.hue);
      printf("colour = %d\n", cam->vid_pic.colour);
      printf("contrast = %d\n", cam->vid_pic.contrast);
      printf("whiteness = %d\n", cam->vid_pic.whiteness);
      printf("colour depth = %d\n", cam->vid_pic.depth);
      print_palette(cam->vid_pic.palette);
   }
}

void get_win_info(cam * cam)
{
   gchar *msg;
   if(v4l1_ioctl(cam->dev, VIDIOCGWIN, &cam->vid_win) == -1) {
      msg = g_strdup_printf(_("Could not connect to video device (%s).\nPlease check connection."), cam->video_dev);
      error_dialog(msg);
      if(cam->debug == TRUE) {
         fprintf(stderr, "VIDIOCGWIN  --  could not get window info, exiting....\n");
      }
      exit(0);
   }
   if(cam->debug == TRUE) {
      printf("\nVIDIOCGWIN\n");
      printf("x = %d\n", cam->vid_win.x);
      printf("y = %d\n", cam->vid_win.y);
      printf("width = %d\n", cam->vid_win.width);
      printf("height = %d\n", cam->vid_win.height);
      printf("chromakey = %d\n", cam->vid_win.chromakey);
      printf("flags = %d\n", cam->vid_win.flags);
   }
}
void set_win_info(cam * cam)
{
   gchar *msg;
   if(v4l1_ioctl(cam->dev, VIDIOCSWIN, &cam->vid_win) == -1) {
      msg = g_strdup_printf(_("Could not connect to video device (%s).\nPlease check connection."), cam->video_dev);
      error_dialog(msg);
      if(cam->debug == TRUE) {
         fprintf(stderr, "VIDIOCSWIN  --  could not set window info, exiting....\nerrno = %d", errno);
      }
      g_free(msg);
      exit(0);
   }

   cam->x = cam->vid_win.width;
   cam->y = cam->vid_win.height;
}

void set_buffer(cam * cam)
{
   char *msg;
   if(v4l1_ioctl(cam->dev, VIDIOCGMBUF, &cam->vid_buf) == -1) {
      msg = g_strdup_printf(_("Could not connect to video device (%s).\nPlease check connection."), cam->video_dev);
      error_dialog(msg);
      if(cam->debug == TRUE) {
         fprintf(stderr, "VIDIOCGMBF  --  could not set buffer info, exiting...\n");
      }
      g_free(msg);
      exit(0);

   }
   
   if(cam->debug == TRUE) {
      printf("\nVIDIOCGMBUF\n");
      printf("mb.size = %d\n", cam->vid_buf.size);
      printf("mb.frames = %d\n", cam->vid_buf.frames);
      printf("mb.offset = %d\n", cam->vid_buf.offsets[1]);
   }

}
