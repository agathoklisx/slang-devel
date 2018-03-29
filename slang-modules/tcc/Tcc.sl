% this is intended as a reference

% as it was pulled out of a complicated code, that cannot be in
% sync (some things might not work as intented)

import ("./tcc");

% assumes tcc default path 
% /usr/local/lib/tcc

% and now i see that needs realpath (fixed)
% commited a realpath implementation in clib

variable __realpath = __get_reference ("realpath");

public define tcc_error_handler (msg)
{
  () = fprintf (stderr, "caught tcc error: %s\n", msg);
}

static variable SLapi_Types = Assoc_Type[Integer_Type, -1];
                                      % this match Void_Type which is equal to 0
SLapi_Types["Undefined_Type"] =  0x1; % where SLANG_VOID_TYPE is equal to 1
SLapi_Types["String_Type"]    =  __class_id (String_Type);
SLapi_Types["Integer_Type"]   =  __class_id (Integer_Type);
SLapi_Types["Array_Type"]     =  __class_id (Array_Type);

private define new (s)
{
  ifnot (NULL == s.tcc)
    ifnot (qualifier_exists ("delete"))
      return -1;
    else
      s.delete ();

  s.tcc = tcc_new ();

  if (NULL == s.tcc)
    return -1;

  ifnot (qualifier_exists ("disable_tcc_support"))
    s.add_tcc_path ("/usr/local/lib/tcc");

  variable i;
  variable incl_path = [s.sys_include_path, qualifier ("include_path",
      String_Type[0])];

  _for i (0, length (incl_path) - 1)
    if (-1 == s.add_include_sys_path (incl_path[i]))
      return -1;

  variable lib_path = [s.sys_lib_path, qualifier ("lib_path",
      String_Type[0])];

  _for i (0, length (lib_path) - 1)
    if (-1 == s.add_library_path (lib_path[i]))
      return -1;

  if (-1 == s.link_against_library ("slang"))
    return -1;

  0;
}

private define link_against_library (s, lib)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return -1;

  tcc_set_path (s.tcc, lib, TCC_ADD_LIB);
}

private define add_library_path (s, path)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return -1;

  tcc_set_path (s.tcc, path, TCC_ADD_LPATH);
}

private define add_include (s, path)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return -1;

  tcc_set_path (s.tcc, path, TCC_ADD_INC_PATH);
}

private define add_include_sys_path (s, path)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return -1;

  tcc_set_path (s.tcc, path, TCC_ADD_SYS_INC_PATH);
}

private define add_tcc_path (s, path)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return;

  () = tcc_set_path (s.tcc, path, TCC_CONFIG_TCC_DIR);
}

private define set_output_type (s, type)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return -1;

  ifnot (tcc_set_output_type (s.tcc, type))
    s.output_type = type;
  else
    return -1;

  0;
}

private define set_output_file (s, file)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return -1;

  if (NULL == file)
    return -1;

  variable tmp, p;

  ifnot (path_is_absolute (file))
    {
    p = qualifier ("output_dir", getcwd);
    if (NULL == (p = (@__realpath) (p), p))
      return -1;

    file = path_concat (p, file);
    }
  else
    if (NULL == (p = (@__realpath) (file), p))
      if (NULL == (p = (@__realpath) (path_dirname (file)), p))
        return -1;
      else
        file = path_concat (p, path_basename (file));
    else
      file = p;

  ifnot (access (file, F_OK))
    ifnot (qualifier_exists ("overwrite"))
      return -1;

  ifnot (tcc_set_output_file (s.tcc, file))
    s.output_file = file;
  else
    return -1;

  0;
}

private define set_opt (s, opt)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return;

  tcc_set_opt (s.tcc, opt);
}

private define delete (s)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return;

  tcc_delete (s.tcc);
  s.tcc = NULL;
}

private define compile_string (s, cbuf)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return -1;

  variable isexe = qualifier_exists ("create_exe");
  variable output_file = qualifier ("output_file");

  if (isexe)
    {
    if (NULL == output_file)
      return -1;

    if (-1 == s.set_output_type (TCC_OUTPUT_EXE))
      return -1;
    }
  else
    if (-1 == s.set_output_type (TCC_OUTPUT_MEMORY))
      return -1;

  ifnot (0 == tcc_compile_string (s.tcc, cbuf))
    return -1;

  if (isexe)
    {
    s.set_opt ("-xn");

    if (-1 == s.set_output_file (output_file;;__qualifiers))
      return -1;
    }

  s.iscompiled = (0 == isexe);

  if (qualifier_exists ("verbose") && isexe)
    () = fprintf (stdout, "created executable: %s\n", output_file);

  0;
}

private define compile_file (s, file)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return -1;

  if (-1 == access (file, F_OK|R_OK))
    return -1;

  variable isexe = qualifier_exists ("create_exe");

  if (isexe)
    if (-1 == s.set_output_type (TCC_OUTPUT_EXE))
      return -1;
    else {}
  else
    if (-1 == s.set_output_type (TCC_OUTPUT_MEMORY))
      return -1;

  if (-1 == tcc_compile_file (s.tcc, file))
    return -1;

  if (isexe)
    {
    % this is to set s.tcc->filetype to AFF_TYPE_NONE
    % otherwise elf_output_file() -> tcc_add_runtime() tries to compile  
    % /usr/local/lib/tcc/libtcc1.a
    % when it calls tcc_add_support(s1, TCC_LIBTCC1);
    s.set_opt ("-xn");

    if (-1 == s.set_output_file (qualifier ("output_file");;
         __qualifiers))
      return -1;
    }

  s.iscompiled = (0 == isexe);

  0;
}

private define run (s, argv)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return -1;

  if (s.itwasrun)
    return -1;

  ifnot (s.iscompiled)
    return -1;

  if (NULL == argv || typeof (argv) != Array_Type ||
      _typeof (argv) != String_Type)
    tcc_run (s.tcc);
  else
    tcc_run (s.tcc, argv);

  s.itwasrun = 1;
}

private define execute_string (s, cbuf, argv)
{
  if (-1 == s.compile_string (cbuf))
    return -1;

  s.itwasrun = 0;
  s.run (argv);
}

private define execute_file (s, file, argv)
{
  if (-1 == s.compile_file (file))
    return -1;

  s.itwasrun = 0;
  s.run (argv);
}

private define relocate (s)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return -1;

  tcc_relocate (s.tcc);
}

private define cdefine (s, sym, value)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return;

  tcc_define_symbol (s.tcc, sym, value);
}

private define undefine (s, sym)
{
  if (NULL == s.tcc || typeof (s.tcc) != TCC_Type)
    return;

  tcc_undefine_symbol (s.tcc, sym);
}

private define sladd_intrinsic_variable (s, vname, sym, rettype, cbuf)
{
  ifnot (0 == is_defined (vname))
    return -1;

  rettype = SLapi_Types[string (rettype)];

  if (-1 == rettype)
    return -1;

  if (-1 == s.compile_string (cbuf))
    return -1;

  if (-1 == sladd_variable (s.tcc, vname, sym, rettype;;__qualifiers))
    return -1;

  is_defined (vname) == -1 ? 0 : -1;
}

private define sladd_intrinsic_function (s, funm, nargs, sym, rettype, argtypes, cbuf)
{
  ifnot (0 == is_defined (funm))
    ifnot (qualifier_exists ("redefine"))
      return -1;

  variable arg_types = Integer_Type[length (argtypes)];

  variable i;
   _for i (0, length (argtypes) - 1)
     arg_types[i] = SLapi_Types[string (argtypes[i])];

  rettype = SLapi_Types[string (rettype)];

  if (-1 == rettype)
    return -1;

  if (-1 == s.compile_string (cbuf))
    return -1;

  if (-1 == sladd_function (arg_types, s.tcc, funm, sym, rettype, nargs))
    return -1;

  is_defined (funm) == 1 ? 0 : -1;
}

private define init (self)
{
  variable s = struct
    {
    tcc,
    new = &new,
    link_against_library = &link_against_library,
    add_library_path = &add_library_path,
    add_include = &add_include,
    add_include_sys_path = &add_include_sys_path,
    add_tcc_path = &add_tcc_path,
    set_output_type = &set_output_type,
    set_output_file = &set_output_file,
    set_opt = &set_opt,
    relocate = &relocate,
    define = &cdefine,
    undefine = &undefine,
    delete = &delete,
    compile_file = &compile_file,
    compile_string = &compile_string,
    run = &run,
    execute_string = &execute_string,
    execute_file = &execute_file,
    sladd_intrinsic_function = &sladd_intrinsic_function,
    sladd_intrinsic_variable = &sladd_intrinsic_variable,
    sys_include_path = ["/usr/local/include", "/usr/include"],
    sys_lib_path = ["/usr/local/lib", "/usr/lib"],
    lib_path = String_Type[0],
    include_path = String_Type[0],
    output_type = TCC_OUTPUT_MEMORY,
    iscompiled = 0,
    itwasrun = 0,
    output_file,
    };

  ifnot (qualifier_exists ("no_init"))
    if (-1 == s.new (;;__qualifiers))
      return NULL;
  s;
}

public variable Tcc = struct {init = &init};

% this doesn't really belongs here

define add_required_functions ()
{
  if (1 == is_defined ("realpath"))
    return 0;

  variable tcc = Tcc.init ();

  if (NULL == tcc)
    return -1;

  variable retval = tcc.sladd_intrinsic_function (
    "realpath", 1, "realpath_intrin", Void_Type, [String_Type], `

/* realpath(3) implementation
 *
 * Originally written by Agathoklis D.E. Chatzimanikas
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <slang.h>

void realpath_intrin (char *path)
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
`);

 tcc.delete ();

 ifnot (retval)
   __realpath = __get_reference ("realpath");

 retval;
}

define test_required_functions ()
{
  ifnot (1 == is_defined ("realpath"))
    {
    () = fprintf (stderr, "realpath() intrinsic function has not been initialized\n");
    return -1;
    }

  variable orig_path = sprintf ("%s/../../clib/../slang-modules/tcc/%s",
      path_dirname (__FILE__), path_basename (__FILE__));
  variable sanitized_path = (@__realpath) (orig_path);

  if (NULL == sanitized_path)
    {
    () = fprintf (stderr, "failed to canonicalize original path using realpath\n");
    return -1;
    }

  0;
}

