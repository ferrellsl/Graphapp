
#include <stdio.h>

int main(int argc, char *argv[])
{
	int i, len;
	unsigned char ch[3];

	for (i = 0; i < 256; i++)
	{
		if (i < 128) {
			ch[0] = i;
			len = 1;
		}
		else {
			ch[0] = 0xC0 | ((i >> 6) & 0x03); /* 0b110zzzxx */
			ch[1] = 0x80 | (i & 0x3F);        /* 0b10xxxxxx */
			len = 2;
		}
		if (len == 1)
			printf("%03d 0x%02x UTF8: %03d 0x%02x\n",
				i, i, ch[0], ch[0]);
		else if (len == 2)
			printf("%03d 0x%02x UTF8: %03d,%03d 0x%02x,0x%02x\n",
				i, i, ch[0], ch[1], ch[0], ch[1]);
	}
	return 0;
}
