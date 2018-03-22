/* repeat a string for count times 
 * 
 * Originally written by Agathoklis D.E. Chatzimanikas
 */

#include <string.h>

char *myStrCat (char *s, char *a)
{
  while (*s != '\0') s++;
  while (*a != '\0') *s++ = *a++;
  *s = '\0';
  return s;
}

/*
see
http://stackoverflow.com/questions/5770940/how-repeat-a-string-in-language-c?

By using above function instead of strcat, execution is multiply faster.
Also the following slang code is much faster than using strcat and sligtly slower
than the repeat intrinsic

define repeat (str, count)
{
  ifnot (0 < count)
    return "";

  variable ar = String_Type[count];
  ar[*] = str;
  return strjoin (ar);
}
*/

/* returns an empty string if (count < 0) */
static void repeat_intrin (char *str, int *count)
{
  char *res;
  char *tmp;

  if (0 >= *count)
    {
    (void) SLang_push_string ("");
    return;
    }

  res = (char *) SLmalloc (strlen (str) * (size_t) *count + 1);

  *res = '\0';

  tmp = myStrCat (res, str);

  while (--*count > 0)
    tmp = myStrCat (tmp, str);

  (void) SLang_push_malloced_string (res);
}

/*
  MAKE_INTRINSIC_SI("repeat", repeat_intrin, SLANG_VOID_TYPE),
 */
