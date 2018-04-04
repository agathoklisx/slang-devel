variable env = get_environ ();

() = fprintf (stdout, "%f\n", _time);

tic;

loop (10000)
  () = fprintf (stdout, "%s\n", sysv->basename (env[0]));

() = fprintf (stdout, "%f\n", toc);
