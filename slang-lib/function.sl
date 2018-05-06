% this is a bundled function interface 
% usage:

%  f = fun (`(a, b) return a + b;`);
%  f.call (1, 2);
%  (@f.__funcref) (1, 2);

%  f = funref (`envbeg variable c = 1; envend c++; return c;`);
%  (@f);

%  frun (1, 2, `(a, b) return a + b + qualifier ("c", 1);`;c = 2);

public define __is_substrbytes (src, byteseq, offset)
{
  variable occur = {};
  if (NULL == offset || 1 > offset)
    offset = 1;

  offset--;

  while (offset = is_substrbytes (src, byteseq, offset + 1), offset)
    list_append (occur, offset);

  occur;
}

public define __eval (buf, ns)
{
  variable e;
  try (e)
    eval (buf, ns);
  catch AnyError:
    {
    variable err_buf;
    variable fun = (fun = qualifier ("fun"),
      NULL == fun
        ? _function_name
        : String_Type == typeof (fun)
          ? fun
          : _function_name);

    err_buf = (err_buf = strchop (buf, '\n', 0),
         strjoin (array_map (String_Type, &sprintf, "%d| %s",
         [1:length (err_buf)], err_buf), "\n"));

    if (qualifier_exists ("print_err"))
      () = fprintf (stderr, "%s\nmessage: %s\nline: %d\nfunction: %s\n",
        err_buf, e.message, e.line, e.function);

    throw qualifier ("error", AnyError), sprintf (
      "%s: Evaluation Error\n%S\nmessage: %S\nline: %d\n %s\n",
         fun, [err_buf, ""][qualifier_exists ("print_err")],
         e.message, e.line, e.function),
      e;
    }
}

private define exc_isnot (self, exc)
{
  NULL == exc || Struct_Type != typeof (exc) ||
  NULL == wherefirst (get_struct_field_names (exc) == "object") ||
  8 != length (get_struct_field_names (exc));
}

private define exc_print (self, exc)
{
  if (NULL == exc)
    exc = __get_exception_info;

  while (self.isnot (exc) == 0 == self.isnot (exc.object))
    {
    () = fprintf (stderr, "%s\n", self.fmt (exc.object;to_string));
    exc = exc.object;
    }
}

private define exc_fmt (self, exc)
{
  if (NULL == exc)
    exc = __get_exception_info;

  if (self.isnot (exc))
    exc = struct
      {
      error = 0,
      description = "",
      file = "",
      line = 0,
      function = "",
      object,
      message = "",
      Exception = "No exception in the stack"
      };

  sprintf ("Exception: %s\n\
Message:     %s\n\
Object:      %S\n\
Function:    %s\n\
Line:        %d\n\
File:        %s\n\
Description: %s\n\
Error:       %d",
    _push_struct_field_values (exc));

  if (qualifier_exists ("to_string"))
    return;

  "\n";
  strtok ();
}

public variable Exc = struct
  {
  fmt = &exc_fmt,
  print = &exc_print,
  isnot = &exc_isnot
  };

try
{
__eval (`

% This is an evaluation "on the fly", that means that the private scope
% cannot access (or the oposite) nothing from the private environment,
% that is defined on a compilation unit

% the state should reset at any invocation on success or on error
% or else is a bug
static variable __DATA__;
private variable __f__;
private variable __fun__;
private variable __env__;
private variable __i__;
private variable __len__;
private variable __as__;
private variable __scope__;
private variable __func_name__ = "__dev_fun_";
private variable __anon_name__ = "__FUNCTION__";
private variable __slots__ = [0:511];
private variable __FUNS__ = Assoc_Type[Struct_Type];
private variable __fid__;
private variable __DEPTH__ = 0;
private variable __INSTANCES__ = {};
private variable __ENV_BEG_TOKEN__ = "envbeg";
private variable __ENV_END_TOKEN__ = "envend";
private variable __ENV_BEG_TOKEN_LEN__ = strlen (__ENV_BEG_TOKEN__);
private variable __ENV_END_TOKEN_LEN__ = strlen (__ENV_END_TOKEN__);

private define __my_err_handler__ (e)
{
  ifnot (Exc.isnot (e))
    {
    Exc.print (e);
    throw AnyError;
    }
  else
    {
    if (String_Type == typeof (e))
      () = fprintf (stderr, "%s\n", e);

    throw AnyError;
    }
}

private define __clear__ ()
{
  array_map (&__uninitialize,
    [&__i__, &__len__, &__fun__, &__env__, &__as__, &__scope__]);

  __DEPTH__ = 0;
  __INSTANCES__ = {};
}

private define __ferror__ (e)
{
  loop (_stkdepth) pop;

  __clear__;

  if (qualifier_exists ("unhandled"))
    {
    variable retval = qualifier_exists ("return_on_err");
    ifnot (retval)
      return;

    return qualifier ("return_on_err");
    }

  variable handler;

  if (NULL == (handler = qualifier ("err_handler"), handler))
    if (NULL == (handler = __get_reference ("F_ERROR_HANDLER"), handler))
      handler = &__my_err_handler__;

  if (__is_callable (handler))
    (@handler) (e;;__qualifiers);
}

private define declare__ ()
{
  __i__   = 0;
  __len__ = strlen (__fun__);

  variable buf = __scope__ + " define " +  __as__;
  variable args = qualifier ("args", String_Type[0]);

  % can be used for declarations
  ifnot (__len__)
    return buf + " (" + strjoin (args, ", ") + ");";

  ifnot ('(' == __fun__[0])
    return buf + " (" + strjoin (args, ", ") + ")\n{\n" +
      qualifier ("fun", "") + "  ";

  if (__len__ > 4)
    % there are more to catch - this is weakness
    if (any (0 == array_map (Integer_Type, &strncmp, __fun__,
       ["() =", "()="], [4, 3])))
        return buf + " (" + strjoin (args, ", ") + ")\n{\n" +
          qualifier ("fun", "") + "  ";

  buf += " (";
  args =  strjoin (args, ", ") + ", ";

  loop (1)
    {
    _for __i__ (1, __len__ - 1)
      ifnot (')' == __fun__[__i__])
        args += char (__fun__[__i__]);
      else
        {
        __i__++;
        break 2;
        }

    __ferror__ ("function declaration failed, syntax error, " +
       "expected \")\""; error = SyntaxError);
   }

  buf + strjoin (strtok (args, ","), ", ") + ")\n{\n" +
    qualifier ("fun", "") + "  ";
}

private define __compile__ ()
{
  __fun__ = declare__ (;;__qualifiers) + (__tmp (__len__)
    ? substr (__fun__, __tmp (__i__) + 1, -1) + "\n}\n"
    : "");
}

private define __eval_fun__ ()
{
  variable e;
  try (e)
    {
    __eval (__tmp (__fun__) +
        "__F__->__DATA__ = &" + __tmp (__as__) + ";",
      __f__.__ns;;__qualifiers);
    }
  catch AnyError:
    __ferror__ (e);

  __f__.__funcref = __tmp (__DATA__);
}

static define __call ()
{
  variable args = __pop_list (_NARGS - 1);
  variable f = ();

  try
    {
    (@f.__funcref) (__push_list (args);;__qualifiers);
    }
  catch AnyError:
    __ferror__ (__get_exception_info;;__qualifiers);

  if (qualifier_exists ("destroy"))
    f.__destroy ();
}

static define __call_unhandled ()
{
  variable args = __pop_list (_NARGS - 1);
  variable f = ();

  (@f.__funcref) (__push_list (args);;__qualifiers);

  if (qualifier_exists ("destroy"))
    f.__destroy ();
}

private variable __F_Type = struct
  {
  call     = &__call,
  __ns     = "__F__",
  __fun,
  __env,
  __funcref,
  };

private define __save_instance__ ()
{
  list_insert (__INSTANCES__, struct
    {
    __fun   =  __fun__,
    __env   =  __env__,
    __f     =  @__f__,
    __as    =  __as__,
    __scope =  __scope__
    });
}

private define __restore_instance__ ()
{
  variable i = list_pop (__INSTANCES__);
  __fun__   =  i.__fun;
  __env__   =  i.__env;
  __f__     =  @i.__f;
  __as__    =  i.__as;
  __scope__ =  i.__scope;
}

private define __find_env__ ()
{
  variable
    env_beg = __is_substrbytes (__fun__, __ENV_BEG_TOKEN__, 1),
    env_end = __is_substrbytes (__fun__, __ENV_END_TOKEN__, 1);

  variable i, idx, env;
  variable len = length (env_beg);
  ifnot (len == length (env_end))
    __ferror__ (sprintf ("%d %d  %s\nunmatched envbeg envend delimiters",
    len, length (env_end), __fun__)
    ;error = SyntaxError);

  if (1 == len)
    {
    __env__ += substr (__fun__, __ENV_BEG_TOKEN_LEN__ + 1,
        env_end[0] -  (__ENV_BEG_TOKEN_LEN__ + 1)) + "\n";

    __fun__ = strtrim_beg (substr (__fun__,
        env_end[0] + __ENV_END_TOKEN_LEN__, - 1));

    return;
    }

  idx = 0;
  while (idx++, idx < len)
    {
    i = 0;

    while (i++, i < len)
      {
      if (env_end[idx] < env_beg[i])
        {
        __env__ += substr (__fun__, __ENV_BEG_TOKEN_LEN__ + 1,
            env_end[idx] -  (__ENV_BEG_TOKEN_LEN__ + 1)) + "\n";

        __fun__ = strtrim_beg (substr (__fun__,
            env_end[idx] + __ENV_END_TOKEN_LEN__, -1));

        return;
        }
      }
    }

  __env__ += substr (__fun__, __ENV_BEG_TOKEN_LEN__ + 1,
      env_end[-1] -  (__ENV_BEG_TOKEN_LEN__ + 1)) + "\n";

  __fun__ = strtrim_beg (substr (__fun__,
      env_end[-1] + __ENV_END_TOKEN_LEN__, -1));
}

private define __free_slot__ ()
{
  % this code can be easily done in C?
  ifnot (NULL == (__fid__ = wherefirst (__slots__), __fid__))
    return (__slots__[__fid__] = 0, __fid__);

  __fid__ = length (__slots__);
  __slots__ = Integer_Type[__fid__ + (__fid__ / 2)];
  __slots__[[0:__fid__]] = 0;
  __fid__;
}

private define __destroy__ (f)
{
  __fid__      = string (f.__funcref);
  variable key = f.__ns + "->" + __fid__;
  eval (__FUNS__[key].scope + " define " + __fid__[[1:]] + " ();", f.__ns);

  ifnot (NULL == __FUNS__[key].slot)
    __slots__[   __FUNS__[key].slot] = 1;

  assoc_delete_key (__FUNS__, key);
  set_struct_fields (f, NULL, NULL, NULL, NULL);
}

private define __function__ ()
{
  if (__DEPTH__)
    __save_instance__;

  __fun__ = strtrim (());

  __DEPTH__++;

  __env__ = qualifier ("env", "") + "\n";

  if (strlen (__fun__) > __ENV_BEG_TOKEN_LEN__)
    if (__fun__[[:__ENV_BEG_TOKEN_LEN__ - 1]] == __ENV_BEG_TOKEN__)
       __find_env__;

  __fid__ = NULL;

  __as__        = qualifier ("as", __func_name__ + string (__free_slot__));
  __f__         = @__F_Type;
  __f__.__ns    = qualifier ("ns", __as__);
  __scope__     = qualifier ("scope", "private");

  if (qualifier_exists ("unhandled"))
    __f__.call = &__call_unhandled;

  __compile__ (;;__qualifiers);
  __fun__ = __tmp (__env__) + __fun__;

  __eval_fun__ (;;__qualifiers);

  ifnot (qualifier_exists ("discard"))
    struct
      {
      __funcref = __f__.__funcref,
      call      = __f__.call,
      __ns      = __f__.__ns,
      __destroy = &__destroy__,
      };

  __FUNS__[__f__.__ns + "->" + string (__f__.__funcref)] = struct
    {
    slot   =  __fid__,
    scope  =  __scope__,
    };

  __uninitialize (&__f__);

   __DEPTH__--;

  if (__DEPTH__)
    __restore_instance__;
}

public define fun ()
{
  __function__ (;;__qualifiers);
}

public define funref ()
{
  __function__ (;;__qualifiers).__funcref;
}

public define frun ()
{
  variable e, f;
  f = __function__ (;; struct
    {
    @__qualifiers,
    as = __anon_name__,
    });

  if (__is_initialized (&f))
    try (e)
      {
      (@f.__funcref) (;;__qualifiers);
      }
    catch AnyError:
      throw AnyError, "", e;
    finally:
      f.__destroy ();
}
`, "__F__");
  }
catch AnyError:
  {
  Exc.print (NULL);
  throw AnyError;
  }
