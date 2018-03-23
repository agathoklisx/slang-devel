/* this is an authentication mechanism which I hope i got right
 * it uses  pam_authenticate(3) and certainly needs expansion
 */ 

#include <security/pam_appl.h>

static int conversation (int num_msg, const struct pam_message** msg, struct pam_response** resp, void* appdata_ptr)
{
  struct pam_response *reply;

  reply = (struct pam_response* ) SLmalloc (sizeof (struct pam_response));

  if (reply == NULL)
    return PAM_BUF_ERR;

  reply[0].resp = (char*) appdata_ptr;
  reply[0].resp_retcode = 0;

  *resp = reply;

  return PAM_SUCCESS;
}

static int auth_intrin (const char *user, const char* pass)
{
  pam_handle_t* pamh;
  int retval;
  char* password = (char*) malloc (strlen (pass) + 1);

  strcpy (password, pass);

  struct pam_conv pamc = {conversation, password};

  retval = pam_start ("exit", user, &pamc, &pamh);

  if (retval != PAM_SUCCESS)
    {
    SLang_verror (SL_OS_Error, "pam_start failed: %s", pam_strerror (pamh, retval));
    (void) pam_end (pamh, 0);
    return -1;
    }

  retval = pam_authenticate (pamh, 0);

  (void) pam_end (pamh, 0);
  return retval == PAM_SUCCESS ? 0 : -1;
}

/*
   MAKE_INTRINSIC_SS("auth", auth_intrin, SLANG_INT_TYPE),
 */
