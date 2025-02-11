/* Copyright (C) 2013 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Version 2013-06-24.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <endian.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef _
#define _(x) x
#endif
#define static_assert(expr) \
  extern int never_defined_just_used_for_checking[(expr) ? 1 : -1]
#ifndef min
# define min(a, b) ((a) < (b) ? (a) : (b))
#endif

static_assert (sizeof (unsigned long) >= sizeof (uint32_t));

/* This is bfd_calc_gnu_debuglink_crc32 from bfd/opncls.c.  */
static unsigned long
    calc_gnu_debuglink_crc32 (unsigned long crc,
			      const unsigned char *buf,
			      size_t len)
{
  static const unsigned long crc32_table[256] =
    {
      0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
      0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
      0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
      0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
      0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
      0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
      0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
      0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
      0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
      0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
      0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
      0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
      0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
      0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
      0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
      0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
      0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
      0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
      0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
      0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
      0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
      0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
      0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
      0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
      0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
      0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
      0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
      0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
      0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
      0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
      0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
      0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
      0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
      0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
      0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
      0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
      0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
      0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
      0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
      0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
      0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
      0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
      0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
      0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
      0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
      0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
      0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
      0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
      0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
      0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
      0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
      0x2d02ef8d
    };
  const unsigned char *end;

  crc = ~crc & 0xffffffff;
  for (end = buf + len; buf < end; ++ buf)
    crc = crc32_table[(crc ^ *buf) & 0xff] ^ (crc >> 8);
  return ~crc & 0xffffffff;
}

static size_t updated_count, matched_count, failed_count;

static const char *usr_lib_debug;

static bool
crc32 (const char *fname, const char *base_fname, uint32_t *crcp)
{
  char *reldir = strdup (base_fname);
  if (reldir == NULL)
    error (1, 0, _("out of memory"));
  char *s = reldir + strlen (reldir);
  while (s > reldir && s[-1] != '/')
    *--s = '\0';
  char *debugname;
  if (asprintf (&debugname, "%s/%s/%s", usr_lib_debug, reldir, fname) <= 0)
    error (1, 0, _("out of memory"));
  free (reldir);
  int fd = open (debugname, O_RDONLY);
  if (fd == -1)
    {
      error (0, errno, _("cannot open \"%s\""), debugname);
      return false;
    }
  off_t size = lseek (fd, 0, SEEK_END);
  if (size == -1)
    {
      error (0, errno, _("cannot get size of \"%s\""), debugname);
      return false;
    }
  off_t offset = 0;
  uint32_t crc = 0;
  void *buf = NULL;
  while (offset < size)
    {
      const size_t maplen = min (0x10000, size - offset);
      void *map = NULL;
      if (buf == NULL)
	{
	  map = mmap (NULL, maplen, PROT_READ, MAP_PRIVATE
#ifdef MAP_POPULATE
      | MAP_POPULATE
#endif
      ,
		      fd, offset);
	  if (map == MAP_FAILED)
	    {
	      error (0, errno, _("cannot map 0x%llx bytes at offset 0x%llx "
				 "of file \"%s\""),
		     (unsigned long long) maplen, (unsigned long long) offset,
		     debugname);
	      map = NULL;
	    }
	}
      if (map == NULL)
	{
	  if (buf == NULL)
	    {
	      buf = malloc (maplen);
	      if (buf == NULL)
		error (1, 0, _("out of memory"));
	    }
	  ssize_t got = pread (fd, buf, maplen, offset);
	  if (got != maplen)
	    {
	      error (0, errno, _("cannot read 0x%llx bytes at offset 0x%llx "
				 "of file \"%s\""),
		     (unsigned long long) maplen, (unsigned long long) offset,
		     debugname);
	      free (buf);
	      free (debugname);
	      return false;
	    }
	}
      crc = calc_gnu_debuglink_crc32 (crc, map ?: buf, maplen);
      if (map && munmap (map, maplen) != 0)
	error (1, errno, _("cannot unmap 0x%llx bytes at offset 0x%llx "
			   "of file \"%s\""),
	       (unsigned long long) maplen, (unsigned long long) offset,
	       debugname);
      offset += maplen;
    }
  free (buf);
  if (close (fd) != 0)
    {
      error (0, errno, _("cannot close \"%s\""), debugname);
      free (debugname);
      return false;
    }
  free (debugname);
  *crcp = crc;
  return true;
}

static bool
process (Elf *elf, int fd, const char *fname)
{
  GElf_Ehdr ehdr_mem, *ehdr = gelf_getehdr (elf, &ehdr_mem);
  if (ehdr == NULL)
    {
      error (0, 0, _("cannot get ELF header of \"%s\""), fname);
      return false;
    }
  if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB
      && ehdr->e_ident[EI_DATA] != ELFDATA2MSB)
    {
      error (0, 0, _("invalid ELF endianity of \"%s\""), fname);
      return false;
    }
  Elf_Scn *scn = NULL;
  const char scnname[] = ".gnu_debuglink";
  while ((scn = elf_nextscn (elf, scn)) != NULL)
    {
      GElf_Shdr shdr_mem, *shdr = gelf_getshdr (scn, &shdr_mem);
      if (shdr == NULL)
	{
	  error (0, 0, _("cannot get section # %zu in \"%s\""),
		 elf_ndxscn (scn), fname);
	  continue;
	}
      const char *sname = elf_strptr (elf, ehdr->e_shstrndx, shdr->sh_name);
      if (sname == NULL)
	{
	  error (0, 0, _("cannot get name of section # %zu in \"%s\""),
		 elf_ndxscn (scn), fname);
	  continue;
	}
      if (strcmp (sname, scnname) != 0)
	continue;
      Elf_Data *data = elf_getdata (scn, NULL);
      if (data == NULL)
	{
	  error (0, 0, _("cannot get data of section \"%s\" # %zu in \"%s\""),
		 scnname, elf_ndxscn (scn), fname);
	  continue;
	}
      if ((data->d_size & 3) != 0)
	{
	  error (0, 0, _("invalid size of section \"%s\" # %zu in \"%s\""),
		 scnname, elf_ndxscn (scn), fname);
	  continue;
	}
      const uint8_t *zerop = memchr (data->d_buf, '\0', data->d_size);
      if (zerop == NULL)
	{
	  error (0, 0, _("no file string in section \"%s\" # %zu in \"%s\""),
		 scnname, elf_ndxscn (scn), fname);
	  continue;
	}
      const uint8_t *crcp = (const uint8_t *) ((uintptr_t) (zerop + 1 + 3) & -4);
      if (crcp + 4 != (uint8_t *) data->d_buf + data->d_size)
	{
	  error (0, 0, _("invalid format of section \"%s\" # %zu in \"%s\""),
		 scnname, elf_ndxscn (scn), fname);
	  continue;
	}
      uint32_t had_crc_targetendian = *(const uint32_t *) crcp;
      uint32_t had_crc = (ehdr->e_ident[EI_DATA] == ELFDATA2LSB
			  ? le32toh (had_crc_targetendian)
			  : be32toh (had_crc_targetendian));
      uint32_t crc;
      if (! crc32 (data->d_buf, fname, &crc))
	return false;
      if (crc == had_crc)
	{
	  matched_count++;
	  return true;
	}
      updated_count++;
      off_t seekto = (shdr->sh_offset + data->d_off
			+ (crcp - (const uint8_t *) data->d_buf));
      uint32_t crc_targetendian = (ehdr->e_ident[EI_DATA] == ELFDATA2LSB
				   ? htole32 (crc) : htobe32 (crc));
      ssize_t wrote = pwrite (fd, &crc_targetendian, sizeof (crc_targetendian),
			      seekto);
      if (wrote != sizeof (crc_targetendian))
	{
	  error (0, 0, _("cannot write new CRC to 0x%llx "
			 "inside section \"%s\" # %zu in \"%s\""),
		 (unsigned long long) seekto, scnname, elf_ndxscn (scn), fname);
	  return false;
	}
      return true;
    }
  error (0, 0, _("cannot find section \"%s\" in \"%s\""), scnname, fname);
  return false;
}

static void
version (void)
{
  printf("sepdebugcrcfix %s\n", VERSION);
  exit(EXIT_SUCCESS);
}

static const char *helpText =
  "Usage: %s DEBUG_DIR FILEs\n" \
  "Fixes CRC in .debug files under DEBUG_DIR for the given FILEs\n"	\
  "\n"									\
  "DEBUG_DIR is usually \"usr/lib/debug\".\n"				\
  "FILEs should have relative paths.\n"					\
  "The relative paths will be used to find the corresponding .debug\n"  \
  "files under DEBUGDIR\n";

static void
help (const char *progname, bool error)
{
  FILE *f = error ? stderr : stdout;
  fprintf (f, helpText, progname);
  exit (error ? EXIT_FAILURE : EXIT_SUCCESS);
}

int
main (int argc, char **argv)
{
  if (argc == 2 && strcmp (argv[1], "--version") == 0)
    version ();
  if (argc == 2 && strcmp (argv[1], "--help") == 0)
    help (argv[0], false);
  if (argc < 2)
    help (argv[0], true);

  usr_lib_debug = argv[1];
  if (elf_version (EV_CURRENT) == EV_NONE)
    error (1, 0, _("error initializing libelf: %s"), elf_errmsg (-1));
  for (int argi = 2; argi < argc; argi++)
    {
      const char *fname = argv[argi];
      struct stat stat_buf;
      if (stat(fname, &stat_buf) < 0)
	{
	  error (0, errno, _("cannot stat input \"%s\""), fname);
	  failed_count++;
	  continue;
	}

      /* Make sure we can read and write */
      if (chmod (fname, stat_buf.st_mode | S_IRUSR | S_IWUSR) != 0)
	error (0, errno, _("cannot chmod \"%s\" to make sure we can read and write"), fname);

      bool failed = false;
      int fd = open (fname, O_RDWR);
      if (fd == -1)
	{
	  error (0, errno, _("cannot open \"%s\""), fname);
	  failed = true;
	}
      else
	{
	  Elf *elf = elf_begin (fd, ELF_C_READ_MMAP, NULL);
	  if (elf == NULL)
	    {
	      error (0, 0, _("cannot open \"%s\" as ELF: %s"), fname,
		     elf_errmsg (-1));
	      failed = true;
	    }
	  else
	    {
	      if (! process (elf, fd, fname))
		failed = true;
	      if (elf_end (elf) != 0)
		{
		  error (0, 0, _("cannot close \"%s\" as ELF: %s"), fname,
			 elf_errmsg (-1));
		  failed = true;
		}
	    }
	  if (close (fd) != 0)
	    {
	      error (0, errno, _("cannot close \"%s\""), fname);
	      failed = true;
	    }
	}

      /* Restore old access rights. Including any suid bits reset. */
      if (chmod (fname, stat_buf.st_mode) != 0)
	error (0, errno, _("cannot chmod \"%s\" to restore old access rights"), fname);

      if (failed)
	failed_count++;
    }
  printf ("%s: Updated %zu CRC32s, %zu CRC32s did match.\n", argv[0],
	  updated_count, matched_count);
  if (failed_count)
    printf ("%s: Failed for %zu files.\n", argv[0], failed_count);
  return failed_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
