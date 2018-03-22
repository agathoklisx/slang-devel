/* realpath(3) implementation
 *
 * Originally written by Agathoklis D.E. Chatzimanikas
 */

#include <limits.h>
#include <stdlib.h>

static void realpath_intrin (char *path)
{
  long path_max;
  char *p;

#ifdef PATH_MAX
  path_max = PATH_MAX;
#else
  path_max = pathconf (path, _PC_PATH_MAX);
  if (path_max <= 0)
    path_max = 4096;
#endif

  if (NULL == (p = (char *)SLmalloc (path_max+1)))
    return;

  if (NULL != realpath (path, p))
    {
    (void) SLang_push_malloced_string (p);
    return;
    }

   SLerrno_set_errno (errno);
   SLfree (p);
   (void) SLang_push_null ();
}

/* 
  MAKE_INTRINSIC_S("realpath", realpath_intrin, SLANG_VOID_TYPE),
 */
