/*
  Select non-standard functionality commonly available in C compilers
  for DOS in <dos.h>, <dir.h>, <conio.h>, <bios.h>, <string.h>, which
  is not (yet?) provided by Smaller C's library.
*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "scdos.h"

#ifndef __HUGE__
#ifndef __UNREAL__
#error This program requires the huge or unreal mode(l)
#endif
#endif

unsigned char inportb(int port)
{
  asm("mov dx, [bp+8]\n"
      "in al, dx\n"
      "movzx eax, al");
}

void outportb(int port, unsigned char value)
{
  asm("mov dx, [bp+8]\n"
      "mov al, [bp+12]\n"
      "out dx, al");
}

void setvect(int intno, void __interrupt isr())
{
  unsigned addr = (unsigned)isr;
  unsigned ofs = (addr >> 20 << 4) | (addr & 0xF);
  unsigned seg = (addr >> 4) & 0xFFFF;
  asm("pushfd\n"
      "cli");
  poke(0, intno * 4, ofs);
  poke(0, intno * 4 + 2, seg);
  asm("popfd");
}

void __interrupt (*getvect(int intno))()
{
  asm("pushfd\n"
      "cli");
  unsigned ofs = peek(0, intno * 4);
  unsigned seg = peek(0, intno * 4 + 2);
  unsigned addr = (ofs & 0xF) | (seg << 4) | (ofs >> 4 << 20);
  asm("popfd");
  return (void __interrupt (*)())addr;
}

/* Calls the old ISR from the new ISR */
void callisr(void __interrupt isr())
{
  asm("pushfd\n"
      "cli\n"
      "rol dword [bp + 8], 12\n"
      "rol word [bp + 8], 4\n"
      "pushf\n"
      "call far [bp + 8]\n"
      "popfd");
}

int stricmp(char* s1, char* s2)
{
  while (toupper((unsigned char)*s1) == toupper((unsigned char)*s2))
  {
    if (!*s1)
      return 0;
    ++s1;
    ++s2;
  }

  return ((unsigned char)*s1) - ((unsigned char)*s2);
}

void movedata(unsigned srcseg, unsigned srcoff, unsigned dstseg, unsigned dstoff, unsigned n)
{
  memcpy(MK_FP(dstseg, dstoff), MK_FP(srcseg, srcoff), n);
}

/* REGS: unsigned short ax, bx, cx, dx, si, di, cflag, flags */
int int86(int intno, union REGS* inregs, union REGS* outregs)
{
  asm("  db        0x66, 0x68\n" /* push dword next */
      "patch_next:\n"
      "  dd        next\n"
      "section .relot\n"
      "  dd        patch_next\n"

      "section .text\n"
      "  movzx     ax, byte [bp+8]\n"
      "  shl       eax, 16\n"
      "  or        eax, 0xEA00CD90\n" /* nop; int intno; jmp far absolute next */
      "  push      eax\n"
      "  mov       ax, sp\n"
      "  push      ebp\n"
      "  push      ss\n"
      "  push      ax\n"

      "  mov       edi, [bp+12]\n"
      "  ror       edi, 4\n"
      "  mov       ds, di\n"
      "  shr       edi, 28\n"
      "  mov       ax, [di]\n"
      "  mov       bx, [di+2]\n"
      "  mov       cx, [di+4]\n"
      "  mov       dx, [di+6]\n"
      "  mov       si, [di+8]\n"
      "  mov       di, [di+10]\n"

      "  retf\n"
      "next:\n"
      "  pop       ebp\n"
      "  pushf\n"
      "  mov       [bp+8], di\n"
      "  pop       word [bp+12]\n"
      "  mov       edi, [bp+16]\n"
      "  ror       edi, 4\n"
      "  mov       ds, di\n"
      "  shr       edi, 28\n"
      "  mov       [di], ax\n"
      "  mov       [di+2], bx\n"
      "  mov       [di+4], cx\n"
      "  mov       [di+6], dx\n"
      "  mov       [di+8], si\n"
      "  mov       bx, [bp+8]\n"
      "  mov       [di+10], bx\n"
      "  mov       bx, [bp+12]\n"
      "  mov       [di+14], bx\n"
      "  and       bx, 1\n"
      "  mov       [di+12], bx\n"

      "  add       sp, 8\n"
      "  movzx     eax, ax");

#ifdef __UNREAL__
  asm("push dword 0\n"
      "pop  es\n"
      "pop  ds");
#endif
}

int kbhit(void)
{
  asm("mov   ah, 1\n"
      "int   0x16\n"
      "setnz al\n"
      "movzx eax, al");
}

int bioskey(int cmd)
{
  assert(cmd == 0);
  asm("mov   ah, 0\n"
      "int   0x16\n"
      "movzx eax, ax");
}

int getdisk(void)
{
  asm("mov   ah, 0x19\n"
      "int   0x21\n"
      "movzx eax, al");
}

int setdisk(int drive)
{
  asm("mov   ah, 0xE\n"
      "mov   dl, [bp+8]\n"
      "int   0x21\n"
      "movzx eax, al");
}

static
int DosGetCurDir(char buf[64], int drive, unsigned* error)
{
  asm("mov ah, 0x47\n"
      "mov esi, [bp + 8]\n"
      "ror esi, 4\n"
      "mov ds, si\n"
      "shr esi, 28\n"
      "mov dl, [bp + 12]\n"
      "int 0x21");
  asm("movzx ebx, ax\n"
      "cmc\n"
      "sbb ax, ax\n"
      "and eax, 1\n"
      "mov esi, [bp + 16]");
#ifdef __HUGE__
  asm("ror esi, 4\n"
      "mov ds, si\n"
      "shr esi, 28\n"
      "mov [si], ebx");
#else
  asm("push word 0\n"
      "pop  ds\n"
      "mov  [esi], ebx");
#endif
}

char* getcwd(char* buf, int len)
{
  /* Not setting errno! */
  char b[3 + 64];
  unsigned err;
  int l;
  b[0] = getdisk() + 'A';
  b[1] = ':';
  b[2] = '\\';
  if (!DosGetCurDir(b + 3, 0/*current drive*/, &err))
    return NULL;
  l = strlen(b) + 1;
  if (!buf)
  {
    if (!(buf = malloc(l)))
      return NULL;
  }
  else if (len < l)
  {
    return NULL;
  }
  return memcpy(buf, b, l);
}

static
int DosChDir(char* path, unsigned* error)
{
  asm("mov ah, 0x3B\n"
      "mov edx, [bp + 8]\n"
      "ror edx, 4\n"
      "mov ds, dx\n"
      "shr edx, 28\n"
      "int 0x21");
  asm("movzx ebx, ax\n"
      "cmc\n"
      "sbb ax, ax\n"
      "and eax, 1\n"
      "mov esi, [bp + 12]");
#ifdef __HUGE__
  asm("ror esi, 4\n"
      "mov ds, si\n"
      "shr esi, 28\n"
      "mov [si], ebx");
#else
  asm("push word 0\n"
      "pop  ds\n"
      "mov  [esi], ebx");
#endif
}

int chdir(char* path)
{
  /* Not setting errno! */
  unsigned err;
  if (!path || !DosChDir(path, &err))
    return -1;
  return 0;
}

static
void DosSetDTA(struct ffblk* dta)
{
  asm("mov ah, 0x1A\n"
      "mov edx, [bp + 8]\n"
      "ror edx, 4\n"
      "mov ds, dx\n"
      "shr edx, 28\n"
      "int 0x21");
#ifdef __UNREAL__
  asm("push word 0\n"
      "pop  ds");
#endif
}

static
int DosFindFirst(char* path, int attr, unsigned* error)
{
  asm("mov ah, 0x4E\n"
      "mov edx, [bp + 8]\n"
      "ror edx, 4\n"
      "mov ds, dx\n"
      "shr edx, 28\n"
      "mov cx, [bp + 12]\n"
      "int 0x21");
  asm("movzx ebx, ax\n"
      "cmc\n"
      "sbb ax, ax\n"
      "and eax, 1\n"
      "mov esi, [bp + 16]");
#ifdef __HUGE__
  asm("ror esi, 4\n"
      "mov ds, si\n"
      "shr esi, 28\n"
      "mov [si], ebx");
#else
  asm("push word 0\n"
      "pop  ds\n"
      "mov  [esi], ebx");
#endif
}

static
int DosFindNext(struct ffblk* dta, unsigned* error)
{
  asm("mov ah, 0x4F\n"
      "mov edx, [bp + 8]\n"
      "ror edx, 4\n"
      "mov ds, dx\n"
      "shr edx, 28\n"
      "int 0x21");
  asm("movzx ebx, ax\n"
      "cmc\n"
      "sbb ax, ax\n"
      "and eax, 1\n"
      "mov esi, [bp + 12]");
#ifdef __HUGE__
  asm("ror esi, 4\n"
      "mov ds, si\n"
      "shr esi, 28\n"
      "mov [si], ebx");
#else
  asm("push word 0\n"
      "pop  ds\n"
      "mov  [esi], ebx");
#endif
}

int findfirst(char* path, struct ffblk* ffblk, int attrib)
{
  unsigned err;
  if (!path || !ffblk)
    return -1;
  DosSetDTA(ffblk);
  if (!DosFindFirst(path, attrib, &err))
    return -1;
  return 0;
}

int findnext(struct ffblk* ffblk)
{
  unsigned err;
  if (!ffblk || !DosFindNext(ffblk, &err))
    return -1;
  return 0;
}

void fnmerge(char* path, char* drive, char* dir, char* name, char* ext)
{
  if (drive && *drive)
  {
    *path++ = *drive;
    *path++ = ':';
  }
  if (dir && *dir)
  {
    size_t len = strlen(dir);
    int c = dir[len - 1];
    memcpy(path, dir, len);
    path += len;
    if (c != '\\' && c != '/')
      *path++ = '\\';
  }
  if (name && *name)
  {
    size_t len = strlen(name);
    memcpy(path, name, len);
    path += len;
  }
  if (ext && *ext)
  {
    size_t len = strlen(ext);
    if (*ext != '.')
      *path++ = '.';
    memcpy(path, ext, len);
    path += len;
  }
  *path = '\0';
}

int fnsplit(char* path, char* drive, char* dir, char* name, char* ext)
{
  int flags = 0;
  char* p = path;

  if (drive)
    *drive = '\0';
  if (dir)
    *dir = '\0';
  if (name)
    *name = '\0';
  if (ext)
    *ext = '\0';

  if (isalpha((unsigned char)*p) && p[1] == ':')
  {
    if (drive)
    {
      *drive++ = *p++;
      *drive++ = *p++;
      *drive = '\0';
    }
    flags |= DRIVE;
  }

{
  char* lastSlash = strrchr(p, '/');
  char* lastBackslash = strrchr(p, '\\');
  char* dirEnd;
  /* Find the last (back)slash, if any. */
  if (lastSlash && lastBackslash)
    dirEnd = (lastSlash < lastBackslash) ? lastBackslash : lastSlash;
  else
    dirEnd = lastBackslash ? lastBackslash : lastSlash;
  if (dirEnd)
  {
    size_t len = dirEnd - p + 1;
    if (dir)
    {
      memcpy(dir, p, len);
      dir[len] = '\0';
    }
    p += len;
    flags |= DIRECTORY;
  }
}

{
  /* "." and ".." are directory names, not extensions. */
  size_t len = 0;
  if (*p == '.')
  {
    if (p[1] == '\0')
      len = 1;
    else if (p[1] == '.' && p[2] == '\0')
      len = 2;
  }
  /* Filename is before the last dot, if any. */
  if (!len)
  {
    char* lastDot = strrchr(p, '.');
    len = lastDot ? (size_t)(lastDot - p) : strlen(p);
  }
  if (len)
  {
    if (name)
    {
      memcpy(name, p, len);
      name[len] = '\0';
    }
    p += len;
    flags |= FILENAME;
  }
}

  if (*p == '.')
  {
    if (ext)
      strcpy(ext, p);
    flags |= EXTENSION;
  }

  if (path[strcspn(path, "?*")])
    flags |= WILDCARDS;

  return flags;
}
