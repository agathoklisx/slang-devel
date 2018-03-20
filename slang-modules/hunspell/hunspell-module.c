/* slang bindings to hunspell library
 * https://github.com/hunspell/
 *
 **************************************
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * Copyright (C) 2002-2017 Nmeth Lszl
 ************************************** 
 *
 * Originally written by Agathoklis D.E. Chatzimanikas
 * Last checked against hunspell ~revision a7be9d3
 * 
 * compiled with gcc (with debug flags):
 gcc hunspell-module.c -I/usr/local/include -g -O2    \
   -Wl,-R/usr/local/lib --shared -fPIC -lhunspell-1.6 \
   -Wall -Wformat=2 -W -Wunused -Wundef -pedantic     \
   -Wno-long-long -Winline -Wmissing-prototypes       \
   -Wnested-externs -Wpointer-arith -Wcast-align      \
   -Wshadow -Wstrict-prototypes -Wextra -Wc++-compat  \
   -Wlogical-op -o hunspell-module.so

 * all the operations checked with
 * valgrind --leak-check=full
 */

#include <stdlib.h>
#include <string.h>
#include <slang.h>
#include <hunspell/hunspell.h>

SLANG_MODULE(hunspell);

static int Hunspell_Id = 0;

typedef struct
  {
  Hunhandle *handler;
  } Hunspell_Type;

static void free_hunspell_type (Hunspell_Type *hsp)
{
  if (NULL == hsp)
    return;

  if (NULL != hsp->handler)
    Hunspell_destroy (hsp->handler);

  SLfree ((char *) hsp);
}

static SLang_MMT_Type *allocate_hunspell_type (Hunhandle *handler)
{
  SLang_MMT_Type *mmt;
  Hunspell_Type *hsp;

  if (NULL == (hsp = (Hunspell_Type *) SLmalloc (sizeof (Hunspell_Type))))
    return NULL;

  memset ((char *) hsp, 0, sizeof (Hunspell_Type));

  hsp->handler = handler;

  if (NULL == (mmt = SLang_create_mmt (Hunspell_Id, (VOID_STAR) hsp)))
    {
    free_hunspell_type (hsp);
    return NULL;
    }

  return mmt;
}

static int hunspell_spell_intrinsic (Hunspell_Type *hsp, char *str)
{
  if (NULL == hsp->handler)
    return -1;

  return Hunspell_spell (hsp->handler, str);
}

static void hunspell_suggest_intrinsic (Hunspell_Type *hsp, char *str)
{
 	int i;
 	char **lst = NULL;
  SLindex_Type idx;
  SLang_Array_Type *suggestions;

  if (NULL == hsp->handler)
    return;

	 idx = Hunspell_suggest (hsp->handler, &lst, str);

  suggestions = SLang_create_array (SLANG_STRING_TYPE, 0, NULL, &idx, 1);
  if (suggestions == NULL)
    {
    SLang_push_null ();
    goto end;
    }

	 if (idx > 0)
    {
    for (i = 0; i < idx; i++)
      if (-1 == SLang_set_array_element (suggestions, &i, &lst[i]))
        {
        SLang_free_array (suggestions);
        SLang_push_null ();
        goto end;
        }
    }

  SLang_push_array (suggestions, 1);

end:
  if (NULL != lst)
    Hunspell_free_list (hsp->handler, &lst, idx);
}

static void hunspell_add_dic_intrinsic (Hunspell_Type *hsp, char *str)
{
  if (NULL == hsp->handler)
    return;

  Hunspell_add_dic (hsp->handler, str);
}

static void hunspell_rm_word_intrinsic (Hunspell_Type *hsp, char *str)
{
  if (NULL == hsp->handler)
    return;

  Hunspell_remove (hsp->handler, str);
}

static void hunspell_add_word_intrinsic (Hunspell_Type *hsp, char *str)
{
  if (NULL == hsp->handler)
    return;

  Hunspell_add (hsp->handler, str);
}

static void hunspell_close_intrinsic (Hunspell_Type *hsp)
{
  if (NULL == hsp->handler)
    return;

  Hunspell_destroy (hsp->handler);
  hsp->handler = NULL;
}

static void hunspell_init_intrinsic (char *aff_dir, char *dic_dir)
{
  Hunhandle *handler;
  SLang_MMT_Type *mmt;

  handler = Hunspell_create (aff_dir, dic_dir);
  if (NULL == handler)
    {
    SLang_push_null ();
    return;
    }

  if (NULL == (mmt = allocate_hunspell_type (handler)))
    goto error;

  if (-1 == SLang_push_mmt (mmt))
    {
    SLang_free_mmt (mmt);
    goto error;
    }

 return;

error:
  Hunspell_destroy (handler);
  SLang_push_null ();
}

#define DUMMY_HUNSPELL_TYPE ((SLtype)-1)
#define P DUMMY_HUNSPELL_TYPE
#define I SLANG_INT_TYPE
#define V SLANG_VOID_TYPE
#define S SLANG_STRING_TYPE

static SLang_Intrin_Fun_Type hunspell_Intrinsics [] =
{
  MAKE_INTRINSIC_SS("hunspell_init", hunspell_init_intrinsic, V),
  MAKE_INTRINSIC_1("hunspell_close", hunspell_close_intrinsic, V, P),
  MAKE_INTRINSIC_2("hunspell_add_dic", hunspell_add_dic_intrinsic, V, P, S),
  MAKE_INTRINSIC_2("hunspell_check", hunspell_spell_intrinsic, I, P, S),
  MAKE_INTRINSIC_2("hunspell_suggest", hunspell_suggest_intrinsic, V, P, S),
  MAKE_INTRINSIC_2("hunspell_add_word", hunspell_add_word_intrinsic, V, P, S),
  MAKE_INTRINSIC_2("hunspell_remove_word", hunspell_rm_word_intrinsic, V, P, S),
  SLANG_END_INTRIN_FUN_TABLE
};

static void destroy_hunspell (SLtype type, VOID_STAR f)
{
  Hunspell_Type *hsp;
  (void) type;

  hsp = (Hunspell_Type *) f;
  free_hunspell_type (hsp);
}

static int register_hunspell_type (void)
{
  SLang_Class_Type *cl;

  if (Hunspell_Id)
    return 0;

  if (NULL == (cl = SLclass_allocate_class ("Hunspell_Type")))
    return -1;

  if (-1 == SLclass_set_destroy_function (cl, destroy_hunspell))
    return -1;

  if (-1 == SLclass_register_class (cl, SLANG_VOID_TYPE, sizeof (Hunspell_Type), SLANG_CLASS_TYPE_MMT))
    return -1;

  Hunspell_Id = SLclass_get_class_id (cl);

  if (-1 == SLclass_patch_intrin_fun_table1 (hunspell_Intrinsics, DUMMY_HUNSPELL_TYPE, Hunspell_Id))
     return -1;

  return 0;
}

#undef DUMMY_HUNSPELL_TYPE
#undef P
#undef I
#undef V
#undef S

int init_hunspell_module_ns (char *ns_name)
{
  SLang_NameSpace_Type *ns;

  if (NULL == (ns = SLns_create_namespace (ns_name)))
    return -1;

  if (-1 == register_hunspell_type ())
    return -1;

  if (-1 == SLns_add_intrin_fun_table (ns, hunspell_Intrinsics, NULL))
    return -1;

  return 0;
}
