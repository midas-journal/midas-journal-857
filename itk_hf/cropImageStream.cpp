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
  if (argc!=6) {
    fprintf(stderr, "USAGE: cropImageStream <input.image> <output.prefix> <column.count> <row.count> <multiplication>\n");
    return 0;
  }
  Magick::Image im;
  im.read(argv[1]);

  int nCol = atoi(argv[3]);
  int nRow = atoi(argv[4]);
  int mult = atoi(argv[5]);
  int dX = im.size().width()/nCol;
  int dY = im.size().height()/nRow;
  char filename[128];

  for (int i=0; i<nRow; i++) {
    for (int j=0; j<nCol; j++) {
      i = 2;
      j = 7;
      Magick::Image tile = im;
      tile.crop(Magick::Geometry(dX, dY, dX*j, dY*i));
      tile.scale(Magick::Geometry(dX*mult, dY*mult,
                                  tile.page().xOff(),
                                  tile.page().yOff()));
      for (int ii=0; ii<mult; ii++) {
        ii = 1;
//         for (int jj=0; jj<mult; jj++) {
        for (int jj=0; jj<2; jj++) {
          Magick::Image subTile = tile;
          subTile.crop(Magick::Geometry(dX, dY,
                                        subTile.page().xOff()+jj*dX,
                                        subTile.page().yOff()+ii*dY));
          sprintf(filename, "%s_%d_%d.png", argv[2], i*mult + ii, j*mult + jj);
          fprintf(stderr, "%s\n", filename);
          subTile.write(filename);
          sprintf(filename, "%s_%d_%d.offset", argv[2], i*mult + ii, j*mult + jj);
          fprintf(stderr, "%s\n", filename);
          FILE *fo = fopen(filename, "w");
          fprintf(fo, "%d\n%d\n%d\n%d\n",
                  j*dX*mult + jj*dX, i*dY*mult + ii*dY,
                  j*dX*mult + jj*dX + dX, i*dY*mult + ii*dY + dY);
          fclose(fo);
        }
        exit(0);
      }
    }
  }
  return 0;
  
  
//   fwrite(&header, sizeof(header), 1, fo);
//   header.count = 0;
//   header.bbox[0] = INT_MAX;
//   header.bbox[1] = INT_MIN;
//   header.bbox[2] = INT_MAX; 
//   header.bbox[3] = INT_MIN;
//   char ifn[256], ofn[256];
//   void *buf = malloc(BUFSIZE);
//   int x1, y1, x2, y2, pos, bytesRead;
//   fscanf(fi, "%s", ifn);
//   while (!feof(fi)) {
//     header.count++;
//     fprintf(stderr, "%d: %s\n", header.count, ifn);
//     char *ext = strrchr(ifn, '.');
//     strncpy(ofn, ifn, ext-ifn);
//     strcpy(ofn+(ext-ifn), ".offset");
//     FILE *offsetFile = fopen(ofn, "r");
//     fscanf(offsetFile, "%d %d %d %d", &x1, &y1, &x2, &y2);
//     if (x1<header.bbox[0]) header.bbox[0] = x1;
//     if (x2>header.bbox[1]) header.bbox[1] = x2;
//     if (y1<header.bbox[2]) header.bbox[2] = y1;
//     if (y2>header.bbox[3]) header.bbox[3] = y2;
//     fclose(offsetFile);

//     FILE *imageFile = fopen(ifn, "rb");
//     fseek(imageFile, 0, SEEK_END);
//     pos = ftell(imageFile);
//     fwrite(&x1, sizeof(x1), 1, fo);
//     fwrite(&y1, sizeof(y1), 1, fo);
//     fwrite(&pos, sizeof(pos), 1, fo);
//     fseek(imageFile, 0, SEEK_SET);
//     while ((bytesRead = fread(buf, 1, BUFSIZE, imageFile))>0) {
//       fwrite(buf, 1, bytesRead, fo);
//     }
//     pos = ftell(imageFile);
//     fclose(imageFile);

//     fscanf(fi, "%s", ifn);
//   }
//   fseek(fo, 0, SEEK_SET);
//   fwrite(&header, sizeof(header), 1, fo);
//   fclose(fi);
//   fclose(fo);
//   return 0;
}
