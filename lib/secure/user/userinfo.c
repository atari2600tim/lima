/* Do not remove the headers from this file! see /USAGE for more info. */

/*
** userinfo.c -- user information
**
** 950821, Deathblade: created.
*/

#include <config.h>
#include <mudlib.h>
#include <security.h>

void sw_body_handle_new_logon();
nomask string query_userid();
object query_body();
void save_me();
mixed unguarded(mixed priv, function func);

varargs void modal_push(function input_func, mixed prompt, int secure, function return_to_func);
varargs void modal_func(function input_func, mixed prompt, int secure);
void modal_pop();

/* states */
#define GENDER_QUERY 0
#define GOT_GENDER 1
#define GOT_EMAIL 2
#define GOT_REAL_NAME 3
#define GOT_URL 4
#define GOT_REFERRAL 5

private
string email;
private
string real_name;
private
int ed_setup;
private
string url;

// temporary new user vars
nosave private int n_gen = -1;

// ### wah! get rid of this. need by sw_body.c; should move to NEW_USER_D
protected
nomask int query_n_gen()
{
   return n_gen;
}

nomask void set_ed_setup(int code)
{
   ed_setup = code;
   save_me();
}

nomask int query_ed_setup()
{
   return ed_setup;
}

// ### ACK!  should disappear. need something more secure than this
nomask string query_email()
{
   return email;
}

nomask void set_real_name(string new_name)
{
   if (this_user() != this_object())
      error("illegal attempt to set real name\n");

   real_name = new_name;
   save_me();
}

nomask void set_email(string new_email)
{
   if (this_user() != this_object())
      error("illegal attempt to set email address\n");

   email = new_email;
   save_me();
}

nomask void set_url(string new_url)
{
   if (this_user() != this_object())
      error("illegal attempt to set URL\n");

   url = new_url;
   save_me();
}

nomask void set_referral(string reftxt)
{
   if (!reftxt || strlen(trim(reftxt)) == 0)
      return;

   unguarded(1, ( : write_file, REFERRALS_LOG, ctime(time()) + ":" + query_userid() + ": " + reftxt + "\n" :));
}

nomask string query_url()
{
   return url;
}

protected
nomask varargs void userinfo_handle_logon(int state, mixed extra, string arg)
{
   object tmp;

   switch (state)
   {
   case GENDER_QUERY:
      modal_push(( : userinfo_handle_logon, GOT_GENDER, 0 :), "Are you male or female? ");
      break;

   case GOT_GENDER:
      arg = lower_case(arg);
      if (arg == "y" || arg == "yes")
      {
         write("Ha, ha, ha. Which one are you?\n");
         return;
      }
      if (arg == "n" || arg == "no")
      {
         write("Well, which one would you have liked to be, then?\n");
         return;
      }
      if (arg == "f" || arg == "female")
         n_gen = 2;
      else if (arg != "m" && arg != "male")
      {
         write("I've never heard of that gender.  Please try again.\n");
         return;
      }
      else
      {
         n_gen = 1;
      }

      write("\n"
            "The following info is only seen by you and administrators\n"
            "  if you prepend a # to your response.\n"
            "\n"
            "You cannot gain wizard status without valid responses to these questions:\n");

      modal_func(( : userinfo_handle_logon, GOT_EMAIL, 0 :), "Your email address: ");
      break;

   case GOT_EMAIL:
      email = arg;
      modal_func(( : userinfo_handle_logon, GOT_REAL_NAME, 0 :), "Your real name: ");
      break;

   case GOT_REAL_NAME:
      real_name = arg;

      modal_func(( : userinfo_handle_logon, GOT_URL, 0 :), "Your home page address (if any): ");
      break;

   case GOT_URL:
      url = arg;

      write("\n\nThat is almost all!\n"
            "Would you mind telling us how you found out about " +
            mud_name() +
            "?\n"
            "(This question is completely optional, but we appreciate an answer)\n");
      modal_func(( : userinfo_handle_logon, GOT_REFERRAL, 0 :), "How did you find us: ");
      break;
   case GOT_REFERRAL:
      set_referral(arg);
      modal_pop();
      tmp = new (USER_MENU);
      tmp->start_menu();
#ifndef USE_USER_MENU
      tmp->char_name(query_userid());
#endif
      return;
   }
}
