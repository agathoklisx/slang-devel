__L__->slang_init_path ();

() = evalfile ("./Tcc.sl");

private define __sysv_test__ ()
{
  variable retval = -1;
  loop (1) {
  ifnot (sysv->basename (__FILE__) == "test.sl")
    {
    () = fprintf (stderr, "basename failed\n");
    break;
    }

  if (NULL == sysv->dirname (__FILE__))
    {
    () = fprintf (stderr, "dirname failed\n");
    break;
    }

  variable orig_path = sprintf ("%s/../interp/../../slang-devel/interp/%s",
      sysv->dirname (__FILE__), sysv->basename (__FILE__));

  variable sanitized_path = sysv->realpath (orig_path);

  if (NULL == sanitized_path)
    {
    () = fprintf (stderr, "failed to canonicalize original path using realpath\n");
    break;
    }

  retval = 0;
  }

  retval;
}

private define re_init (tcc)
{
  if (-1 == tcc.new (;;__qualifiers))
    {
    () = fprintf (stderr, "failed to re-initialize tcc instance\n");
    return -1;
    }

  0;
}

private define __tcc_test__ ()
{
  variable t;

  % this is for convenience
  % use appropriate paths for the specific system
  variable
    lib_path = String_Type[0],
    incl_path = String_Type[0];

  % do not modify this
  variable __qual = struct {lib_path = lib_path, incl_path = incl_path};

  loop (1) {
  variable retval = -1;

  if (NULL == (t = Tcc.init (;;__qual), t))
    {
    () = fprintf (stderr, "failed to initialize tcc instance\n");
    break;
    }

  if (-1 == (retval = t.sladd_intrinsic_variable ("INT_VAR", "int_var",
      Integer_Type, `int int_var = 100;`), retval))
    {
    () = fprintf (stderr, "failed to add Integer_Type intrinsic variable\n");
    break;
    }

  ifnot (retval = (@__get_reference ("INT_VAR")), 100 == retval)
    {
    () = fprintf (stderr, "failed intrinsic variable test, awaiting 100 got %d\n",
        retval);
    break;
    }

  t.delete ();

  if (-1 == (retval = re_init (t;;__qual), retval))
    break;

  if (-1 == (retval = t.sladd_intrinsic_variable ("Array_Var_IntType", "int_arr_var",
      Array_Type, `const int int_arr_var[3] = {10, 10, 10};`;
      num_dims = 1, dims = 3, array_type = __class_id (Integer_Type)), retval))
    {
    () = fprintf (stderr, "failed to add Array_Type of Integer_Type intrinsic variable\n");
    break;
    }

  ifnot (retval = all (10 == (@__get_reference ("Array_Var_IntType"))), 1 == retval)
    {
    () = fprintf (stderr, "failed intrinsic Array_Type of Integer_Type variable test\n");
    retval = -1;
    break;
    }

  t.delete ();

  if (-1 == (retval = re_init (t;;__qual), retval))
    break;

  if (-1 == (retval = t.sladd_intrinsic_variable ("Array_Var_StrType", "str_arr_var",
      Array_Type, `const char *str_arr_var[] = {"a", "a", "a", '\0'};`), retval))
    {
    () = fprintf (stderr, "failed to add Array_Type of String_Type intrinsic variable\n");
    break;
    }

  ifnot (retval = all ("a" == (@__get_reference ("Array_Var_StrType"))), 1 == retval)
    {
    () = fprintf (stderr, "failed intrinsic Array_Type of String_Type variable test\n");
    retval = -1;
    break;
    }

  t.delete ();

  if (-1 == (retval = re_init (t;;__qual), retval))
    break;

  if (-1 == (retval = t.sladd_intrinsic_variable ("STR_VAR", "str_var", String_Type,
      `const char *str_var = "const_var";`), retval))
    {
    () = fprintf (stderr, "failed to add String_Type intrinsic variable\n");
    break;
    }

  ifnot (retval = (@__get_reference ("STR_VAR")), "const_var" == retval)
    {
    () = fprintf (stderr, "failed intrinsic variable test, awaiting \"const_var\" got %s\n",
        retval);
    retval = -1;
    break;
    }

  t.delete ();

  if (-1 == (retval = re_init (t;;__qual), retval))
    break;

  if (-1 == (retval = t.sladd_intrinsic_function ("add_two", 1, "c_add_two",
       Integer_Type, [Integer_Type],
       `int c_add_two (int *i) {return *i + 2;}`), retval))
    {
    () = fprintf (stderr, "failed to add intrinsic function\n");
    break;
    }

  ifnot (retval = (@__get_reference ("add_two")) (2), retval == 4)
    {
    () = fprintf (stderr, "failed intrinsic function test, awaiting 1 got %d\n",
        retval);
    break;
    }

  t.delete ();

  if (-1 == (retval = re_init (t;;__qual), retval))
    break;

  % create executable
  if (-1 == (retval = t.compile_string (`#include <stdio.h>
    int main (int argc, char **argv) {
        fprintf (stderr, "howl\n");
        return 0;}`
     ;create_exe, output_file = "/tmp/tcc_executable", overwrite), retval))
    {
    () = fprintf (stderr, "failed to create executable\n");
    break;
    }

  if (retval = system ("/tmp/tcc_executable"), retval == -1)
    () = fprintf (stderr, "failed executable test\n");
  else
    retval = 0;

  () = remove ("/tmp/tcc_executable");
  }

  ifnot (NULL == t)
    t.delete ();

  return (retval == -1);
}


private define main ()
{
  variable err_msg = "test has failed";
  variable suc_msg = "test has passed";

  variable exit_code = 0;

  loop (1){
  if (1 == (exit_code = __sysv_test__ (), exit_code))
    {
    () = fprintf (stderr, "sysv %s\n", err_msg);
    break;
    }
  else
    () = fprintf (stdout, "sysv %s\n", suc_msg);

  if (1 == (exit_code = __tcc_test__ (), exit_code))
    {
    () = fprintf (stderr, "tcc %s\n", err_msg);
    break;
    }
  else
    () = fprintf (stdout, "tcc %s\n", suc_msg);
    }

  exit (exit_code);
}

main ();
