#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <Magick++.h>

#define BUFSIZE (1024*1024)

struct HEADER {
  int count;
  int bbox[4];
} header;

int main(int argc, char **argv) {
  if (argc!=3) {
    fprintf(stderr, "USAGE: createImageStream <list_of_images.txt> <output.streaming>\n");
    return 0;
  }
  FILE *fi = fopen(argv[1], "r");
  FILE *fo = fopen(argv[2], "wb");
  fwrite(&header, sizeof(header), 1, fo);
  header.count = 0;
  header.bbox[0] = INT_MAX;
  header.bbox[1] = INT_MIN;
  header.bbox[2] = INT_MAX; 
  header.bbox[3] = INT_MIN;
  char ifn[256], ofn[256];
  void *buf = malloc(BUFSIZE);
  int x1, y1, x2, y2, pos, bytesRead;
  fscanf(fi, "%s", ifn);
  while (!feof(fi)) {
    header.count++;
    fprintf(stderr, "%d: %s\n", header.count, ifn);
    char *ext = strrchr(ifn, '.');
    strncpy(ofn, ifn, ext-ifn);
    strcpy(ofn+(ext-ifn), ".offset");
    FILE *offsetFile = fopen(ofn, "r");
    fscanf(offsetFile, "%d %d %d %d", &x1, &y1, &x2, &y2);
    if (x1<header.bbox[0]) header.bbox[0] = x1;
    if (x2>header.bbox[1]) header.bbox[1] = x2;
    if (y1<header.bbox[2]) header.bbox[2] = y1;
    if (y2>header.bbox[3]) header.bbox[3] = y2;
    fclose(offsetFile);

    FILE *imageFile = fopen(ifn, "rb");
    fseek(imageFile, 0, SEEK_END);
    pos = ftell(imageFile);
    fwrite(&x1, sizeof(x1), 1, fo);
    fwrite(&y1, sizeof(y1), 1, fo);
    fwrite(&pos, sizeof(pos), 1, fo);
    fseek(imageFile, 0, SEEK_SET);
    while ((bytesRead = fread(buf, 1, BUFSIZE, imageFile))>0) {
      fwrite(buf, 1, bytesRead, fo);
    }
    pos = ftell(imageFile);
    fclose(imageFile);

    fscanf(fi, "%s", ifn);
  }
  fseek(fo, 0, SEEK_SET);
  fwrite(&header, sizeof(header), 1, fo);
  fclose(fi);
  fclose(fo);
  return 0;
}
