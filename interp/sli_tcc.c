/*             (this is just for reference)
 *        a mini slang interpreter implementation
 *   (that requires a sensible open source environment)
 * (that uses the tinycc compiler (for dynamical (just in time)
 * access to the slang API)
 *
 * see: slang-modules/tcc/tcc-module.c (for details about tcc)
 *
 * This is a stripped down slsh from the distribution, that means:
 *
 *  - can not run in interactive mode (doesn't load readline code)
 *  - awaits a file for execution (doesn't use stdin)
 *  - doesn't understands -e
 *  - without stat_mode_to_string () and usage() functions
 *  - does not read initialization files
 *  - initializes tcc
 *  - initializes the minimun that can get from slang
 *  - initializes a SysV environment
 *
 * Compiled with cc (GCC) 6 version and later
 *  (it uses the following feature[s] from GNU cc:
 *
 *  - compound statements enclosed in parentheses as expressions
 *  )
 */

/*
 * Copyright (C) 2005-2017,2018 John E. Davis

 * The S-Lang Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.

 * The S-Lang Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/* tinycc is Licensed under:
	*	GNU LESSER GENERAL PUBLIC LICENSE
	* Version 2.1, February 1999
 */

/* 
 * originally written by Agathoklis D.E Chatzimanikas (spring of 2018)
 *    (and licensed for free) as freedom
 */

/*        (thou who enter here)                      * 
 * (have no expectations for formal C) this is Ci Ci *
 *   (patches at: aga dot chatzimanikas at gmail)    *
 *   (discussion To: slang-devel@jedsoft.org)        */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#include <libtcc.h>
#include <slang.h>

   /* private */

static char *__L__[256];

   /* abstraction */

extern const char *__LG__;

   /* portability */
/* adopted from tcc.h */
#ifdef _WIN32
# define IS_DIRSEP(c) (c == '/' || c == '\\')
# define IS_ABSPATH(p) (IS_DIRSEP(p[0]) || (p[0] && p[1] == ':' && IS_DIRSEP(p[2])))
# define PATHSEP ";"
#else
# define IS_DIRSEP(c) (c == '/')
# define IS_ABSPATH(p) IS_DIRSEP(p[0])
# define PATHSEP ":"
#endif

   /* */

   /* helper functions */

#define tostdout(...) fprintf (stdout, __VA_ARGS__)
#define tostderr(...) fprintf (stderr, __VA_ARGS__)

#define __DEBUG_INFO__                         \
({                                             \
  char debug__[1024];                          \
  sprintf (debug__,                            \
    "File: %s\nFunction: %s\nLineNr: %d\n",    \
      __FILE__, __func__, __LINE__);           \
  debug__;                                     \
})

#define  DebugMsg(...)                         \
  tostderr ("%s\n", __DEBUG_INFO__);           \
  tostderr (__VA_ARGS__)

   /* tcc interface */

#define TCC_CONFIG_TCC_DIR    1
#define TCC_ADD_INC_PATH      2
#define TCC_ADD_SYS_INC_PATH  3
#define TCC_ADD_LPATH         4
#define TCC_ADD_LIB           5
#define TCC_SET_OUTPUT_PATH   6
#define TCC_COMPILE_FILE      7

static int TCC_CLASS_ID = 0;

typedef struct
  {
  TCCState *handler;
  } TCC_Type;

static void free_tcc_type (TCC_Type *tcc)
{
  if (NULL == tcc)
    return;

  if (NULL != tcc->handler)
    tcc_delete (tcc->handler);

  SLfree ((char *) tcc);
}

static void __tcc_error_handler (void *opaque, const char *msg)
{
  SLang_Name_Type *f;
  (void) opaque;

  if (NULL == (f = SLang_get_function ("tcc_error_handler")))
    fprintf (stderr, "Caught tcc error: %s\n", msg);
  else
    {
    SLang_push_string ((char *) msg);
    SLexecute_function (f);
    }
}

static SLang_MMT_Type *allocate_tcc_type (TCCState *handler)
{
  TCC_Type *tcc;
  SLang_MMT_Type *mmt;

  if (NULL == (tcc = (TCC_Type *) SLmalloc (sizeof (TCC_Type))))
    return NULL;

  memset ((char *) tcc, 0, sizeof (TCC_Type));

  tcc->handler = handler;

  tcc_set_error_func (tcc->handler, NULL, __tcc_error_handler);

  if (NULL == (mmt = SLang_create_mmt (TCC_CLASS_ID, (VOID_STAR) tcc)))
    {
    free_tcc_type (tcc);
    return NULL;
    }

  return mmt;
}

static int __tcc_set_path (TCC_Type *tcc, char *path, int *type)
{
  switch (*type)
    {
    case TCC_CONFIG_TCC_DIR:
      tcc_set_lib_path (tcc->handler, path);
      return 0;

    case TCC_ADD_INC_PATH:
      return tcc_add_include_path (tcc->handler, path);

    case TCC_ADD_SYS_INC_PATH:
      return tcc_add_sysinclude_path (tcc->handler, path);

    case TCC_ADD_LPATH:
      return tcc_add_library_path (tcc->handler, path);

    case TCC_ADD_LIB:
      return tcc_add_library (tcc->handler, path);

    default:
      return -1;
    }
}

static void __tcc_set_options (void)
{
  TCC_Type *tcc;
  SLang_MMT_Type *mmt;
  char *opt;
  SLang_Array_Type *at;
  int num_opts = 0;
  char **opts = NULL;

  at = NULL;
  mmt = NULL;

 	switch (SLang_peek_at_stack())
 	  {
	   case SLANG_STRING_TYPE:
	     if (-1 == SLang_pop_slstring (&opt))
	       return;

      break;

    case SLANG_ARRAY_TYPE:
      if (-1 == SLang_pop_array_of_type (&at, SLANG_STRING_TYPE))
	       goto end;

	     num_opts = at->num_elements;
	     opts = (char **) at->data;
      break;
    }

  if (NULL == (mmt = SLang_pop_mmt (TCC_CLASS_ID)))
    goto end;

  tcc =  (TCC_Type *) SLang_object_from_mmt (mmt);

  if (NULL == at)
    {
    tcc_set_options (tcc->handler, opt);
    goto end;
    }

  int i;
  for (i = 0; i < num_opts; i++)
    {
 	  char *s = opts[i];
 	  if (s != NULL)
      tcc_set_options (tcc->handler, s);
    }

end:
  if (NULL != mmt)
    SLang_free_mmt (mmt);

  if (NULL != at)
    SLang_free_array (at);
  else
    SLang_free_slstring (opt);
}

static int __tcc_relocate (TCC_Type *tcc)
{
  void *def = TCC_RELOCATE_AUTO;
  return tcc_relocate (tcc->handler, def);
}

static int __tcc_set_output_file (TCC_Type *tcc, const char *path)
{
  return tcc_output_file (tcc->handler, path);
}

static int __tcc_set_output_type (TCC_Type *tcc, int *type)
{
  return tcc_set_output_type (tcc->handler, *type);
}

static void __tcc_new (void)
{
  SLang_MMT_Type *mmt;
  TCCState *handler;

  handler = tcc_new ();

  if (NULL == (mmt = allocate_tcc_type (handler)))
    goto error;

  if (-1 == SLang_push_mmt (mmt))
    {
    SLang_free_mmt (mmt);
    goto error;
    }

  return;

error:
  tcc_delete (handler);
  SLang_push_null ();
}

static void __tcc_delete (TCC_Type *tcc)
{
  tcc_delete (tcc->handler);
  tcc->handler = NULL;
}

static int __tcc_compile_string (TCC_Type *tcc, char *buf)
{
  return tcc_compile_string (tcc->handler, buf);
}

static int __tcc_compile_file (TCC_Type *tcc, char *file)
{
  return tcc_add_file (tcc->handler, file);
}

static int __tcc_run (void)
{
  TCC_Type *tcc;
  SLang_MMT_Type *mmt;
  int argc = 0;
  char **argv;
  SLang_Array_Type *at;
  int retval = -1;

  at = NULL;
  mmt = NULL;

  if (SLang_Num_Function_Args > 1)
    {
    if (-1 == SLang_pop_array_of_type (&at, SLANG_STRING_TYPE))
      return retval;

	   argc = at->num_elements;
	   argv = (char **) at->data;
    }
  else
    argv = NULL;

  if (NULL == (mmt = SLang_pop_mmt (TCC_CLASS_ID)))
    goto end;

  tcc = (TCC_Type *) SLang_object_from_mmt (mmt);

  retval = tcc_run (tcc->handler, argc, argv);

end:
  if (NULL != mmt)
    SLang_free_mmt (mmt);

  if (NULL != at)
   SLang_free_array (at);

  return retval;
}

static void __tcc_define_symbol (TCC_Type *tcc, char *sym, char *value)
{
  tcc_define_symbol (tcc->handler, sym, value);
}

static void __tcc_undefine_symbol (TCC_Type *tcc, char *sym)
{
  tcc_undefine_symbol (tcc->handler, sym);
}

/*
static int __tcc_add_symbol (TCC_Type *tcc, char *sym, char *value)
{
  return tcc_add_symbol (tcc->handler, sym, value);
}

static void __tcc_get_symbol (TCC_Type *tcc, char *sym)
{
  tcc_get_symbol (tcc->handler, sym);
}
*/

     /* slang interface */

/* a bit modified upstream's _pSLstrings_to_array() */
static SLang_Array_Type *__pSLstrings_to_array (char **strs, int n)
{
  char **data;
  SLindex_Type i, inum = n;
  SLang_Array_Type *at;

  if (inum < 0)
    while (strs[++inum] != NULL);

  if (NULL == (at = SLang_create_array (SLANG_STRING_TYPE, 0, NULL, &inum, 1)))
    return NULL;

  data = (char **)at->data;
  for (i = 0; i < inum; i++)
    {
   	if (strs[i] == NULL)
	     {
	     data[i] = NULL;
	     continue;
	     }

  	if (NULL == (data[i] = SLang_create_slstring (strs[i])))
	    {
	    SLang_free_array (at);
	    return NULL;
	    }
   }

  return at;
}

          /* read only */
static int
__sladd_intrinsic_variable (TCC_Type *tcc, char *name, char *sym,  int *dttype)
{
#define RDONLY 1
  void *symb;

  if (0 != tcc_relocate (tcc->handler, TCC_RELOCATE_AUTO))
    return -1;

  if (NULL == (symb = tcc_get_symbol (tcc->handler, sym)))
    return -1;

  int array_type, num_dims, dims, retval;
  SLang_Array_Type *at;


  switch (*dttype)
    {
    case SLANG_STRING_TYPE:
      return SLadd_intrinsic_variable (name, (char *) symb, *dttype, RDONLY);

    case SLANG_INT_TYPE:
      return SLadd_intrinsic_variable (name, (int *) symb, *dttype, RDONLY);

    case SLANG_ARRAY_TYPE:
      /* default SLANG_STRING_TYPE */
      (void) SLang_get_int_qualifier ("array_type", &array_type, SLANG_STRING_TYPE);
      (void) SLang_get_int_qualifier ("num_dims", &num_dims, 1);
      (void) SLang_get_int_qualifier ("dims", &dims, -1);

      if (num_dims > SLARRAY_MAX_DIMS)
        return -1;

      switch (array_type)
        {
        case SLANG_STRING_TYPE:
        at = __pSLstrings_to_array ((char **) symb, dims);
        if (NULL == at)
          return -1;

        if (-1 == (retval = SLadd_intrinsic_variable (name, (VOID_STAR) at,
            SLANG_ARRAY_TYPE, RDONLY)))
          SLang_free_array (at);

        return retval;

        case SLANG_INT_TYPE:
          return SLang_add_intrinsic_array (name, array_type, RDONLY, symb, num_dims, dims);

        default:
          return -1;
        }

    default:
      return -1;
    }
}

static int __sladd_intrinsic_function (
  TCC_Type *tcc, char *name, char *sym, int *rettype, int *nargs)
{
  void *symb;
  SLang_Array_Type *at;
  SLtype *types;
  SLtype arg_types[SLANG_MAX_INTRIN_ARGS];
  int depth = SLstack_depth ();
  int retval = -1;
  int i;

  at = NULL;

  if (*nargs > SLang_Num_Function_Args || *nargs < 0)
    return -1;

  if (*nargs && 0 == depth)
    return -1;

  for (i = *nargs; i < SLANG_MAX_INTRIN_ARGS; i++)
    arg_types[i] = 0;

  if (depth)
 	switch (SLang_peek_at_stack ())
 	  {
    case SLANG_ARRAY_TYPE:
      if (-1 == SLang_pop_array_of_type (&at, SLANG_INT_TYPE))
	       return -1;

      types = (SLtype *) at->data;

      for (i = 0; i < *nargs; i++)
        arg_types[i] = types[i];

      break;

    default:
      return -1;
    }

  if (0 != tcc_relocate (tcc->handler, TCC_RELOCATE_AUTO))
    goto end;

  if (NULL == (symb = tcc_get_symbol (tcc->handler, sym)))
    goto end;

  retval = SLadd_intrinsic_function (name, (FVOID_STAR) symb, *rettype, *nargs,
    arg_types[0], arg_types[1], arg_types[2], arg_types[3], arg_types[4],
    arg_types[5], arg_types[6]);

end:
  if (NULL != at)
    SLang_free_array (at);

  return retval;
}

/* tcc class */

static void destroy_tcc_type (SLtype type, VOID_STAR f)
{
  TCC_Type *tcc;
  (void) type;

  tcc = (TCC_Type *) f;
  free_tcc_type (tcc);
}

static SLang_IConstant_Type TCC_CONSTS [] =
{
  MAKE_ICONSTANT("TCC_OUTPUT_MEMORY", TCC_OUTPUT_MEMORY),
  MAKE_ICONSTANT("TCC_OUTPUT_EXE", TCC_OUTPUT_EXE),
  MAKE_ICONSTANT("TCC_OUTPUT_DLL", TCC_OUTPUT_DLL),
  MAKE_ICONSTANT("TCC_OUTPUT_OBJ", TCC_OUTPUT_OBJ),
  MAKE_ICONSTANT("TCC_OUTPUT_PREPROCESS", TCC_OUTPUT_PREPROCESS),
  MAKE_ICONSTANT("TCC_SET_OUTPUT_PATH", TCC_SET_OUTPUT_PATH),
  MAKE_ICONSTANT("TCC_COMPILE_FILE", TCC_COMPILE_FILE),
  MAKE_ICONSTANT("TCC_CONFIG_TCC_DIR", TCC_CONFIG_TCC_DIR),
  MAKE_ICONSTANT("TCC_ADD_INC_PATH", TCC_ADD_INC_PATH),
  MAKE_ICONSTANT("TCC_ADD_SYS_INC_PATH", TCC_ADD_SYS_INC_PATH),
  MAKE_ICONSTANT("TCC_ADD_LPATH", TCC_ADD_LPATH),
  MAKE_ICONSTANT("TCC_ADD_LIB", TCC_ADD_LIB),

  SLANG_END_ICONST_TABLE
};

#define DUMMY_TCC_TYPE ((SLtype)-1)
#define P DUMMY_TCC_TYPE
#define I SLANG_INT_TYPE
#define V SLANG_VOID_TYPE
#define S SLANG_STRING_TYPE

static SLang_Intrin_Fun_Type TCC_Intrinsics [] =
{
  MAKE_INTRINSIC_0("tcc_new", __tcc_new, V),
  MAKE_INTRINSIC_1("tcc_delete", __tcc_delete, V, P),
  MAKE_INTRINSIC_3("tcc_set_path", __tcc_set_path, I, P, S, I),
  MAKE_INTRINSIC_0("tcc_set_opt", __tcc_set_options, V),
  MAKE_INTRINSIC_2("tcc_set_output_file", __tcc_set_output_file, I, P, S),
  MAKE_INTRINSIC_2("tcc_set_output_type", __tcc_set_output_type, I, P, I),
  MAKE_INTRINSIC_2("tcc_compile_string", __tcc_compile_string, I, P, S),
  MAKE_INTRINSIC_2("tcc_compile_file",  __tcc_compile_file, I, P, S),
  MAKE_INTRINSIC_0("tcc_run", __tcc_run, V),
  MAKE_INTRINSIC_1("tcc_relocate", __tcc_relocate, I, P),
  MAKE_INTRINSIC_3("tcc_define_symbol", __tcc_define_symbol, V, P, S, S),
  MAKE_INTRINSIC_2("tcc_undefine_symbol", __tcc_undefine_symbol, V, P, S),

/*  MAKE_INTRINSIC_3("tcc_add_symbol", __tcc_add_symbol, I, P, S, S),
 *  MAKE_INTRINSIC_2("tcc_get_symbol", __tcc_get_symbol, I, P, S),
 */

  /* slang interface */
  MAKE_INTRINSIC_4("sladd_variable", __sladd_intrinsic_variable, I, P, S, S, I),
  MAKE_INTRINSIC_5("sladd_function", __sladd_intrinsic_function, I, P, S, S, I, I),

  SLANG_END_INTRIN_FUN_TABLE
};

static int register_tcc_type (void)
{
  SLang_Class_Type *cl;

  if (TCC_CLASS_ID)
    return 0;

  if (NULL == (cl = SLclass_allocate_class ("TCC_Type")))
    return -1;

  if (-1 == SLclass_set_destroy_function (cl, destroy_tcc_type))
    return -1;

  if (-1 == SLclass_register_class (cl, SLANG_VOID_TYPE,
      sizeof (TCC_Type*), SLANG_CLASS_TYPE_MMT))
    return -1;

  TCC_CLASS_ID = SLclass_get_class_id (cl);

  if (-1 == SLclass_patch_intrin_fun_table1 (TCC_Intrinsics, DUMMY_TCC_TYPE,
       TCC_CLASS_ID))
    return -1;

  return 0;
}

static int __init_tcc__ (void)
{
  SLang_NameSpace_Type *ns;

  if (NULL == (ns = SLns_create_namespace ("Tcc")))
    return -1;

  if (-1 == register_tcc_type ())
    return -1;

  if (-1 == SLns_add_intrin_fun_table (ns, TCC_Intrinsics, NULL))
    return -1;

  if (-1 == SLns_add_iconstant_table (ns, TCC_CONSTS, NULL))
    return -1;

  return 0;
}

      /* slsh code from the distribution */

static int loadfile (SLFUTURE_CONST char *path, char *file, char *ns)
{
  int status;

  if (file != NULL)
    {
    int free_path = 0;
	   if (path == NULL)
	     {
	     free_path = 1;
	     path = SLpath_getcwd ();

      if (path == NULL)
        {
	       path = ".";
     	  free_path = 0;
        }
   	  }

	   file = SLpath_find_file_in_path (path, file);
	   if (free_path)
      SLfree (path);

	   if (file == NULL)
	   return 0;
    }

  status = SLns_load_file (file, ns);

  SLfree (file);

  if (status == 0)
    return 1;

  return -1;
}

    /* exit interface */

typedef struct _AtExit_Type
  {
  SLang_Name_Type *nt;
  struct _AtExit_Type *next;
  } AtExit_Type;

static AtExit_Type *AtExit_Hooks;

static void at_exit (SLang_Ref_Type *ref)
{
  SLang_Name_Type *nt;
  AtExit_Type *a;

  if (NULL == (nt = SLang_get_fun_from_ref (ref)))
    return;

  a = (AtExit_Type *) SLmalloc (sizeof (AtExit_Type));
  if (a == NULL)
    return;

  a->nt = nt;
  a->next = AtExit_Hooks;
  AtExit_Hooks = a;
}

static void c_exit (int status)
{
  /* Clear the error to allow exit hooks to run */
  if (SLang_get_error ())
    SLang_restart (1);

  while (AtExit_Hooks != NULL)
    {
   	AtExit_Type *next = AtExit_Hooks->next;

   	if (SLang_get_error () == 0)
  	  (void) SLexecute_function (AtExit_Hooks->nt);

   	SLfree ((char *) AtExit_Hooks);
   	AtExit_Hooks = next;
    }

  if (SLang_get_error ())
    SLang_restart (1);

  exit (status);
}

static void exit_intrin (void)
{
  int status;

  if (SLang_Num_Function_Args == 0)
    status = 0;
  else if (-1 == SLang_pop_int (&status))
    return;

  c_exit (status);
}

    /*  sysv */

static void __realpath__ (char *path)
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

  if (NULL == (p = (char *) SLmalloc (path_max + 1)))
    {
    SLerrno_set_errno (SL_Malloc_Error);
    (void) SLang_push_null ();
    return;
    }

  if (NULL != realpath (path, p))
    {
    (void) SLang_push_malloced_string (p);
    return;
    }

   SLerrno_set_errno (errno);
   SLfree (p);
   (void) SLang_push_null ();
}

     /* adopted from libtcc.c  */
/* extract the basename of a file */
static void __basename__ (char *name)
{
  char *p = strchr (name, 0);

  while (p > name && !IS_DIRSEP (p[-1]))
    --p;

  if (-1 == SLang_push_string (p))
    (void) SLang_push_null ();
}

/*    a little bit modified glibc dirname(3)  */
  /* dirname - return directory part of PATH.
   * Copyright (C) 1996, 2000, 2001, 2002 Free Software Foundation, Inc.
   * This file is part of the GNU C Library.
   * Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.
   */
static void __dirname__ (char *name)
{
  static const char dot[] = ".";

  char *last_slash;
  char *path = strdup (name);

  /* Find last '/'. */

  last_slash = strrchr (path, '/');

  if (last_slash != NULL && last_slash != path && last_slash[1] == '\0')
    {
    /* Determine whether all remaining characters are slashes. */
    char *runp;

    for (runp = last_slash; runp != path; --runp)
      if (runp[-1] != '/')
        break;

    /* The '/' is the last character, we have to look further. */
    if (runp != path)
      last_slash = memrchr (path, '/', runp - path);
    }

  if (last_slash != NULL)
    {
    /* Determine whether all remaining characters are slashes. */
    char *runp;

    for (runp = last_slash; runp != path; --runp)
      if (runp[-1] != '/')
        break;

    /* Terminate the path. */
    if (runp == path)
      {
      /* The last slash is the first character in the string. We have to
       * return "/". As a special case we have to return "//" if there
       * are exactly two slashes at the beginning of the string. See
       * XBD 4.10 Path Name Resolution for more information. */

      if (last_slash == path + 1)
        ++last_slash;
      else
        last_slash = path + 1;
      }
    else
      last_slash = runp;

    last_slash[0] = '\0';
    }
  else
    /* This assignment is ill-designed but the XPG specs require to
     * return a string containing "." in any case no directory part is
     * found and so a static and constant string is required. */
    path = (char *) dot;

  if (-1 == SLang_push_string (path))
    (void) SLang_push_null ();

  free (path);
}

static SLang_Intrin_Fun_Type __SYSV_FUNCS__ [] =
{
  MAKE_INTRINSIC_S("realpath", __realpath__, SLANG_VOID_TYPE),
  MAKE_INTRINSIC_S("basename", __basename__, SLANG_VOID_TYPE),
  MAKE_INTRINSIC_S("dirname",  __dirname__,  SLANG_VOID_TYPE),

  SLANG_END_INTRIN_FUN_TABLE
};

static int __init_sysv__ ()
{
  SLang_NameSpace_Type *ns;

  if (NULL == (ns = SLns_create_namespace ("sysv")))
    return -1;

  if (-1 == SLns_add_intrin_fun_table (ns, __SYSV_FUNCS__, NULL))
    return -1;

  return 0;
}

    /* stdio */

static int __init_stdio__ ()
{
  return SLang_init_stdio ();
}

#undef P
#undef I
#undef V
#undef S

      /* simple main */

int main (int argc, char **argv)
{
  char *file = NULL;
  int exit_val, retval;

  if (argc == 1)
    {
    fprintf (stderr, "argument (a file with S-Lang code) is required\n");
    exit (1);
    }

  (void) SLutf8_enable (-1);

  if (-1 == SLang_init_slang ())
    {
    fprintf (stderr, "failed to initialize slang state\n");
    exit (1);
    }

  if (SLang_Version < SLANG_VERSION)
    {
    fprintf (stderr, "Executable compiled against S-Lang %s but linked to %s\n",
        SLANG_VERSION_STRING, SLang_Version_String);
    exit (1);
    }

  if (-1 == SLang_init_import ()) /* dynamic linking */
    {
    fprintf (stderr, "failed to initialize dynamic linking facility\n");
    exit (1);
    }

  (void) SLsignal (SIGPIPE, SIG_IGN);

  file = argv[1];

  argc--;
  argv++;

  if (-1 == SLang_set_argc_argv (argc, argv))
    exit (1);

  if (-1 == __init_tcc__ ())
    {
    fprintf (stderr, "Failed to initialize the tcc compiler\n");
    exit (1);
    }

  if (-1 == __init_sysv__ ())
    {
    fprintf (stderr, "Failed to initialize sysv environment\n");
    exit (1);
    }


  if (-1 == __init_stdio__ ())
    {
    fprintf (stderr, "Failed to initialize Standard IO\n");
    exit (1);
    }

  retval = loadfile (NULL, file, NULL);

  if (0 == retval)
    {
    fprintf (stderr, "%s: file not found\n", file);
    exit (1);
    }

  exit_val = SLang_get_error ();

  c_exit (exit_val);

  return SLang_get_error ();
}
