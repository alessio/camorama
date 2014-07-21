void yuv420p_to_rgb(unsigned char *image, unsigned char *temp, int x, int y, int z);
void fix_colour(char *image, int x, int y);
void negative(unsigned char *image, int x, int y, int z);
void threshold(unsigned char *image, int x, int y, int threshold_value);
void threshold_channel(unsigned char *image, int x, int y, int threshold_value);
void mirror(unsigned char *image, int x, int y, int z);
void wacky(unsigned char *image, int z, int x, int y);
void smooth(unsigned char *image, int z, int x, int y);

void bw(unsigned char *image, int x, int y);
void bw2(unsigned char *image, int x, int y);
void laplace(unsigned char *image, int z, int x, int y);
void sobel(unsigned char *image, int x, int y);
