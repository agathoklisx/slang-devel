/* implementation of fstat(3)
 * Originally written by Agathoklis D.E. Chatzimanikas
 */

#include <sys/stat.h>

static void fstat_intrin (void)
{
  struct stat st;
  int status, fd;

  SLang_MMT_Type *mmt = NULL;
  SLFile_FD_Type *f = NULL;

  switch (SLang_peek_at_stack ())
    {
    case SLANG_FILE_FD_TYPE:
      if (-1 == SLfile_pop_fd (&f))
        return;
      if (-1 == SLfile_get_fd (f, &fd))
        {
        SLfile_free_fd (f);
        return;
        }
      break;

    case SLANG_FILE_PTR_TYPE:
      {
      FILE *fp;
      if (-1 == SLang_pop_fileptr (&mmt, &fp))
        return;
      fd = fileno (fp);
      }
      break;

    case SLANG_INT_TYPE:
      if (-1 == SLang_pop_int (&fd))
        {
        (void) SLerrno_set_errno (SL_TYPE_MISMATCH);
        return;
        }
      break;

    default:
      SLdo_pop_n (SLang_Num_Function_Args);
      (void) SLerrno_set_errno (SL_TYPE_MISMATCH);
      (void) SLang_push_null ();
      return;
    }

  status = fstat (fd, &st);

  if (status == 0)
    SLang_push_cstruct ((VOID_STAR) &st, Fstat_Struct);
  else
    {
    (void) SLerrno_set_errno (errno);
    (void) SLang_push_null ();
    }

  if (f != NULL) SLfile_free_fd (f);
  if (mmt != NULL) SLang_free_mmt (mmt);
}

/*
    MAKE_INTRINSIC_0("fstat", fstat_intrin, SLANG_VOID_TYPE),
 */
