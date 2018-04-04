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

private define main ()
{
  variable err_msg = "test has failed";
  variable suc_msg = "test has passed";

  if (-1 == __sysv_test__ ())
    () = fprintf (stderr, "sysv %s\n", err_msg);
  else
    () = fprintf (stdout, "sysv %s\n", suc_msg);
}

main ();
