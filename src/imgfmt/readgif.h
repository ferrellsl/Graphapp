/*
 *  Loading GIF files:
 */

Image * app_load_gif(const char *filename);
ImageList * app_load_gif_completely(const char *filename);

int app_read_gif (ImageReader *reader);
