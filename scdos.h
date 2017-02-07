/*
  Select non-standard functionality commonly available in C compilers
  for DOS in <dos.h>, <dir.h>, <conio.h>, <bios.h>, <string.h>, which
  is not (yet?) provided by Smaller C's library.
*/

#ifndef __DOS_H
#define __DOS_H

#define far
#define near
#define interrupt __interrupt

/* max() and min() may come from <stdlib.h> */
#define max(a,b)        (((a) > (b)) ? (a) : (b))
#define min(a,b)        (((a) < (b)) ? (a) : (b))

#define FP_OFF(p)    ((unsigned long)(p) & 0xF)
#define FP_SEG(p)    ((unsigned)((unsigned long)(p) >> 4))
#define MK_FP(s,o)   ((void*)(((unsigned long)(s) << 4) + (unsigned)(o)))

#define poke(a,b,c)  (*((unsigned short far*)MK_FP((a),(b))) = (c))
#define pokeb(a,b,c) (*((unsigned char far*)MK_FP((a),(b))) = (c))
#define peek(a,b)    (*((unsigned short far*)MK_FP((a),(b))))
#define peekb(a,b)   (*((unsigned char far*)MK_FP((a),(b))))

#define MAXPATH   80
#define MAXDRIVE  3
#define MAXDIR    66
#define MAXFILE   9
#define MAXEXT    5

#define WILDCARDS 0x01
#define EXTENSION 0x02
#define FILENAME  0x04
#define DIRECTORY 0x08
#define DRIVE     0x10

#pragma pack(push, 1)
struct ffblk /* AKA Disk Transfer Address (DTA) */
{
  char ff_reserved[21];
  char ff_attrib;
  unsigned short ff_ftime;
  unsigned short ff_fdate;
  long ff_fsize;
  char ff_name[13];
};
#pragma pack(pop)

struct WORDREGS
{
  unsigned short ax, bx, cx, dx, si, di, cflag, flags;
};

struct BYTEREGS
{
  unsigned char al, ah, bl, bh, cl, ch, dl, dh;
};

union REGS
{
  struct WORDREGS x;
  struct BYTEREGS h;
};

struct INTREGS
{
  unsigned short gs, fs, es, ds;
  unsigned edi, esi, ebp, esp, ebx, edx, ecx, eax;
  unsigned short ss, ip, cs, flags;
};

unsigned char inportb(int port);
void outportb(int port, unsigned char value);
#define inp(port)       inportb(port)
#define outp(port,val)  outportb(port,val)

int int86(int intno, union REGS* inregs, union REGS* outregs);

int kbhit(void);
int bioskey(int cmd);

#ifdef _DOS
/*
  This header file is pulled by the huffc and fixhelp tools.
  In case they're not compiled for DOS, __interrupt won't be recognized.
*/
void setvect(int intno, void __interrupt isr());
void __interrupt (*getvect(int intno))();
void callisr(void __interrupt isr());
#endif

int stricmp(char* s1, char* s2);
void movedata(unsigned srcseg, unsigned srcoff, unsigned dstseg, unsigned dstoff, unsigned n);

int getdisk(void);
int setdisk(int drive);
char* getcwd(char* buf, int len);
int chdir(char* path);
int findfirst(char* path, struct ffblk* ffblk, int attrib);
int findnext(struct ffblk* ffblk);
void fnmerge(char* path, char* drive, char* dir, char* name, char* ext);
int fnsplit(char* path, char* drive, char* dir, char* name, char* ext);

#endif
