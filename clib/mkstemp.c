/* implementation of mkstemp(3)
 * Originally written by Agathoklis D.E. Chatzimanikas
 */

#include <stdlib.h>

static void mkstemp_intrin (char *template)
{
  int fd;

  if (-1 == (fd = mkstemp (template)))
    {
    (void) SLang_push_null ();
    return;
    }

  SLFile_FD_Type *f;

  if (NULL == (f = SLfile_create_fd (NULL, fd)))
    {
    (void) SLang_push_null ();
    return;
    }

  if (-1 == SLfile_push_fd (f))
    (void) SLang_push_null ();

  SLfile_free_fd (f);
}

/*
   MAKE_INTRINSIC_S("mkstemp", mkstemp_intrin, VOID_TYPE),
 */
