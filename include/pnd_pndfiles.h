
#ifndef h_pnd_pndfiles_h
#define h_pnd_pndfiles_h

#ifdef __cplusplus
extern "C" {
#endif

// the filename of PND files must end with a well defined (case insensitive!) extension
#define PND_PACKAGE_FILEEXT ".pnd" /* case insensitive due to SD FAT fs */

// when seeking the PXML appended (or embedded within if they forgot to append it)
// to a PND file, this buffer size will be used in the seek. It'll actually under-seek
// a bit, so in the odd chance the "<PXML>" tag borders right on the window size, we'll
// still find it.
//   Being SD reads, it might be nice to pick a decent size .. SD is constant read regardless
// of read size for certain sizes, but of course strstr() within a giant buffer is no good
// either if the goods are near the end. How big is an average .png for an average icon
// size?
#define PND_PXML_WINDOW_SIZE 4096

// pnd_seek_pxml should vaguely work like fseek, trying to position at begin of the appended/found PXML
// On return of 0, assuming nothing.
// On 1, assume that the FILE pointer is positioned for next read to pull in the PXML line by line
unsigned char pnd_pnd_seek_pxml ( FILE *f );

#ifdef __cplusplus
} /* "C" */
#endif

#endif
