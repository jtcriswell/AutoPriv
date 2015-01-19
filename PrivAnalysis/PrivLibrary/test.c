/*
 * A small test case for cap wrapper library
 * Compile with gcc test.c priv.c -lcap -o test
 */

#include <stdio.h>
#include "priv.h"

#include <unistd.h>

int main (int argc, char** argv){

  int i;
  cap_t cap;
  cap_value_t cap_v;
  cap_flag_value_t value;

  if ( priv_lower (9,
  		  CAP_AUDIT_CONTROL,
  		  CAP_SYSLOG,
  		  CAP_FOWNER,
  		  CAP_FSETID,
  		  CAP_SYS_BOOT,
  		  CAP_SYS_CHROOT,
  		  CAP_CHOWN,
  		  CAP_DAC_OVERRIDE,
  		  CAP_DAC_READ_SEARCH
  		  ) != 0 ){
    fprintf (stderr, "Error dropping cap 0\n");
  }


  if ( priv_drop (3,
  		  CAP_SYS_ADMIN,
  		  CAP_AUDIT_WRITE,
  		  CAP_SYS_BOOT) != 0){
    fprintf (stderr, "Error raising cap 1\n");
  }

  if ( priv_raise (9,
  		  CAP_AUDIT_CONTROL,
  		  CAP_SYSLOG,
  		  CAP_FOWNER,
  		  CAP_FSETID,
  		  CAP_SYS_BOOT,
  		  CAP_CHOWN,
  		  CAP_SYS_CHROOT,
  		  CAP_AUDIT_WRITE,
  		  CAP_SYS_BOOT) != 0){
    fprintf (stderr, "Error raising cap 2\n");
  }

  if ( priv_lowerall() < 0 ){
    fprintf (stderr, "Error lowering all privs\n");
  }

  if ( priv_raise (1, CAP_CHOWN) < 0 ){
    fprintf (stderr, "Error raising priv again\n");
  }

   
  cap = cap_get_proc();
  printf ("pid %d\n", getpid ());

  printf ("\tPERM\tINHERIT\tEFFECT\n");
  for (i = 0; i <= CAP_LAST_CAP; i ++){

    printf ("%d\t", i);

    cap_get_flag (cap, i, CAP_PERMITTED, &value);
    printf ("%d\t", value == CAP_SET);

    cap_get_flag (cap, i, CAP_INHERITABLE, &value);
    printf ("%d\t", value == CAP_SET);

    cap_get_flag (cap, i, CAP_EFFECTIVE, &value);
    printf ("%d\t", value == CAP_SET);

    printf ("\n");

  }

  cap_free (cap);
  return 0;
}

