
#include <stdio.h> /* for FILE etc */
#include <string.h>
#include <unistd.h> /* for unlink */
#include <stdlib.h> /* for free */
#include <sys/stat.h> /* for stat */

#include "pnd_apps.h"
#include "pnd_container.h"
#include "pnd_pxml.h"
#include "pnd_discovery.h"
#include "pnd_pndfiles.h"
#include "pnd_conf.h"
#include "pnd_desktop.h"
#include "pnd_logger.h"

unsigned char pnd_emit_dotdesktop ( char *targetpath, char *pndrun, pnd_disco_t *p ) {
  char filename [ FILENAME_MAX ];
  char buffer [ 4096 ];
  FILE *f;

  // specification
  // http://standards.freedesktop.org/desktop-entry-spec

  // validation

  if ( ! p -> unique_id ) {
    pnd_log ( PND_LOG_DEFAULT, "Can't emit dotdesktop for %s, missing unique-id\n", targetpath );
    return ( 0 );
  }

  if ( ! p -> exec ) {
    pnd_log ( PND_LOG_DEFAULT, "Can't emit dotdesktop for %s, missing exec\n", targetpath );
    return ( 0 );
  }

  if ( ! targetpath ) {
    pnd_log ( PND_LOG_DEFAULT, "Can't emit dotdesktop for %s, missing target path\n", targetpath );
    return ( 0 );
  }

  if ( ! pndrun ) {
    pnd_log ( PND_LOG_DEFAULT, "Can't emit dotdesktop for %s, missing pnd_run.sh\n", targetpath );
    return ( 0 );
  }

  // set up

  sprintf ( filename, "%s/%s#%u.desktop", targetpath, p -> unique_id, p -> subapp_number );

  // emit

  //printf ( "EMIT DOTDESKTOP '%s'\n", filename );

  f = fopen ( filename, "w" );

  if ( ! f ) {
    return ( 0 );
  }

  fprintf ( f, "%s\n", PND_DOTDESKTOP_HEADER );

  if ( p -> title_en ) {
    snprintf ( buffer, 1020, "Name=%s\n", p -> title_en );
    fprintf ( f, "%s", buffer );
  }

  fprintf ( f, "Type=Application\n" );
  fprintf ( f, "Version=1.0\n" );

  if ( p -> icon ) {
    snprintf ( buffer, 1020, "Icon=%s\n", p -> icon );
    fprintf ( f, "%s", buffer );
  }

  if ( p -> unique_id ) {
    fprintf ( f, "X-Pandora-UID=%s\n", p -> unique_id );
  }

#if 1
  if ( p -> desc_en && p -> desc_en [ 0 ] ) {
    snprintf ( buffer, 1020, "Comment=%s\n", p -> desc_en ); // no [en] needed I suppose, yet
    fprintf ( f, "%s", buffer );
  }
#endif

  if ( p -> preview_pic1 ) {
    fprintf ( f, "X-Pandora-Preview-Pic-1=%s\n", p -> preview_pic1 );
  }

  if ( p -> clockspeed ) {
    fprintf ( f, "X-Pandora-Clockspeed=%s\n", p -> clockspeed );
  }

  if ( p -> startdir ) {
    fprintf ( f, "X-Pandora-Startdir=%s\n", p -> startdir );
  }

  if ( p -> main_category ) {
    fprintf ( f, "X-Pandora-MainCategory=%s\n", p -> main_category );
  }
  if ( p -> main_category1 ) {
    fprintf ( f, "X-Pandora-MainCategory1=%s\n", p -> main_category1 );
  }
  if ( p -> main_category2 ) {
    fprintf ( f, "X-Pandora-MainCategory2=%s\n", p -> main_category2 );
  }

  if ( p -> alt_category ) {
    fprintf ( f, "X-Pandora-AltCategory=%s\n", p -> alt_category );
  }
  if ( p -> alt_category1 ) {
    fprintf ( f, "X-Pandora-AltCategory1=%s\n", p -> alt_category1 );
  }
  if ( p -> alt_category2 ) {
    fprintf ( f, "X-Pandora-AltCategory2=%s\n", p -> alt_category2 );
  }
  if ( p -> info_filename ) {
    fprintf ( f, "X-Pandora-Info-Filename=%s\n", p -> info_filename );
  }
  if ( p -> info_name ) {
    fprintf ( f, "X-Pandora-Info-Name=%s\n", p -> info_name );
  }

#if 0 // we let pnd_run.sh command line handle this instead of in .desktop
  if ( p -> startdir ) {
    snprintf ( buffer, 1020, "Path=%s\n", p -> startdir );
    fprintf ( f, "%s", buffer );
  } else {
    fprintf ( f, "Path=%s\n", PND_DEFAULT_WORKDIR );
  }
#endif

  // association requests
  if ( p -> associationitem1_filetype ) {
    fprintf ( f, "MimeType=%s\n", p -> associationitem1_filetype );
  }

  if ( p -> exec ) {
    char *nohup;

    if ( p -> option_no_x11 ) {
      nohup = "/usr/bin/nohup ";
    } else {
      nohup = "";
    }

    // basics
    if ( p -> object_type == pnd_object_type_directory ) {
      snprintf ( buffer, 1020, "Exec=%s%s -p \"%s\" -e \"%s\" -b \"%s\"",
		 nohup, pndrun, p -> object_path, p -> exec,
		 p -> appdata_dirname ? p -> appdata_dirname : p -> unique_id );
    } else if ( p -> object_type == pnd_object_type_pnd ) {
      snprintf ( buffer, 1020, "Exec=%s%s -p \"%s/%s\" -e \"%s\" -b \"%s\"",
		 nohup, pndrun, p -> object_path, p -> object_filename, p -> exec,
		 p -> appdata_dirname ? p -> appdata_dirname : p -> unique_id );
    }

    // start dir
    if ( p -> startdir ) {
      strncat ( buffer, " -s ", 1020 );
      strncat ( buffer, p -> startdir, 1020 );
    }

    // args (regular args, for pnd_run.sh to handle)
    if ( p -> execargs ) {
      char argbuf [ 1024 ];
      snprintf ( argbuf, 1000, " -a \"%s\"", p -> execargs );
      strncat ( buffer, argbuf, 1020 );
    }

    // clockspeed
    if ( p -> clockspeed && atoi ( p -> clockspeed ) != 0 ) {
      strncat ( buffer, " -c ", 1020 );
      strncat ( buffer, p -> clockspeed, 1020 );
    }

    // exec options
    if ( pnd_pxml_get_x11 ( p -> option_no_x11 ) == pnd_pxml_x11_stop ) {
      strncat ( buffer, " -x ", 1020 );
    }

    // args (dashdash -- args, that the shell can screw with before pnd_run.sh gets them)
    if ( p -> exec_dashdash_args ) {
      char argbuf [ 4096 ];
      snprintf ( argbuf, 4096, " -- %s", p -> exec_dashdash_args );
      strncat ( buffer, argbuf, 4096 );
    }

    // newline
    strncat ( buffer, "\n", 1020 );

    // emit
    fprintf ( f, "%s", buffer );

    // and lets copy in some stuff in case it makes .desktop consumers life easier
    if ( p -> exec ) { fprintf ( f, "X-Pandora-Exec=%s\n", p -> exec ); }
    if ( p -> appdata_dirname ) { fprintf ( f, "X-Pandora-Appdata-Dirname=%s\n", p -> appdata_dirname ); }
    if ( p -> execargs ) { fprintf ( f, "X-Pandora-ExecArgs=%s\n", p -> execargs ); }
    if ( p -> object_flags & PND_DISCO_FLAG_OVR ) { fprintf ( f, "X-Pandora-Object-Flag-OVR=%s\n", "Yes" ); }
    if ( p -> object_type == pnd_object_type_pnd ) {
      fprintf ( f, "X-Pandora-Object-Path=%s\n", p -> object_path );
      fprintf ( f, "X-Pandora-Object-Filename=%s\n", p -> object_filename );
    }

  }

  // categories
  {
    char cats [ 512 ] = "";
    int n;
    pnd_conf_handle c;
    char *confpath;

    // uuuuh, defaults?
    // "Application" used to be in the standard and is commonly supported still
    // Utility and Network should ensure the app is visible 'somewhere' :/
    char *defaults = PND_DOTDESKTOP_DEFAULT_CATEGORY;

    // determine searchpath (for conf, not for apps)
    confpath = pnd_conf_query_searchpath();

    // inhale the conf file
    c = pnd_conf_fetch_by_id ( pnd_conf_categories, confpath );

    // if we can find a default category set, pull it in; otherwise assume
    // the hardcoded one
    if ( pnd_conf_get_as_char ( c, "default" ) ) {
      defaults = pnd_conf_get_as_char ( c, "default" );
    }

    // ditch the confpath
    free ( confpath );

    // attempt mapping
    if ( c ) {

      n = pnd_map_dotdesktop_categories ( c, cats, 511, p );

      if ( n ) {
	fprintf ( f, "Categories=%s\n", cats );
      } else {
	fprintf ( f, "Categories=%s\n", defaults );
      }

    } else {
      fprintf ( f, "Categories=%s\n", defaults );
    }

  }

  fprintf ( f, "%s\n", PND_DOTDESKTOP_SOURCE ); // should we need to know 'who' created the file during trimming

  fclose ( f );

  return ( 1 );
}

unsigned char pnd_emit_dotinfo ( char *targetpath, char *pndrun, pnd_disco_t *p ) {
  char filename [ FILENAME_MAX ];
  char buffer [ 1024 ];
  FILE *f;
  char *viewer, *searchpath;
  pnd_conf_handle desktoph;

  // specification
  // http://standards.freedesktop.org/desktop-entry-spec

  // validation
  //

  // viewer
  searchpath = pnd_conf_query_searchpath();

  desktoph = pnd_conf_fetch_by_id ( pnd_conf_desktop, searchpath );

  if ( ! desktoph ) {
    return ( 0 );
  }

  viewer = pnd_conf_get_as_char ( desktoph, "info.viewer" );

  if ( ! viewer ) {
    return ( 0 ); // no way to view the file
  }

  // etc
  if ( ! p -> unique_id ) {
    pnd_log ( PND_LOG_DEFAULT, "Can't emit dotdesktop for %s, missing unique-id\n", targetpath );
    return ( 0 );
  }

  if ( ! p -> info_filename ) {
    pnd_log ( PND_LOG_DEFAULT, "Can't emit dotdesktop for %s, missing info_filename\n", targetpath );
    return ( 0 );
  }

  if ( ! p -> info_name ) {
    pnd_log ( PND_LOG_DEFAULT, "Can't emit dotdesktop for %s, missing info_name\n", targetpath );
    return ( 0 );
  }

  if ( ! targetpath ) {
    pnd_log ( PND_LOG_DEFAULT, "Can't emit dotdesktop for %s, missing target path\n", targetpath );
    return ( 0 );
  }

  if ( ! pndrun ) {
    pnd_log ( PND_LOG_DEFAULT, "Can't emit dotdesktop for %s, missing pnd_run.sh\n", targetpath );
    return ( 0 );
  }

  // set up

  sprintf ( filename, "%s/%s#%uinfo.desktop", targetpath, p -> unique_id, p -> subapp_number );

  // emit

  f = fopen ( filename, "w" );

  if ( ! f ) {
    return ( 0 );
  }

  fprintf ( f, "%s\n", PND_DOTDESKTOP_HEADER );

  if ( p -> info_name ) {
    snprintf ( buffer, 1020, "Name=%s\n", p -> info_name );
    fprintf ( f, "%s", buffer );
  }

  fprintf ( f, "Type=Application\n" );
  fprintf ( f, "Version=1.0\n" );

#if 0
  if ( p -> icon ) {
    snprintf ( buffer, 1020, "Icon=%s\n", p -> icon );
    fprintf ( f, "%s", buffer );
  }
#endif

  if ( p -> unique_id ) {
    fprintf ( f, "X-Pandora-UID=%s\n", p -> unique_id );
  }

  if ( p -> title_en && p -> title_en [ 0 ] ) {
    snprintf ( buffer, 1020, "Comment=Automatic menu info entry for %s\n", p -> title_en );
    fprintf ( f, "%s", buffer );
  }

#if 0 // we let pnd_run.sh command line handle this instead of in .desktop
  if ( p -> startdir ) {
    snprintf ( buffer, 1020, "Path=%s\n", p -> startdir );
    fprintf ( f, "%s", buffer );
  } else {
    fprintf ( f, "Path=%s\n", PND_DEFAULT_WORKDIR );
  }
#endif

  // exec line
  char args [ 1001 ];
  char *pargs = args;
  char *viewerargs = pnd_conf_get_as_char ( desktoph, "info.viewer_args" );
  if ( viewerargs && viewerargs [ 0 ] ) {
    snprintf ( pargs, 1001, "%s %s",
	       pnd_conf_get_as_char ( desktoph, "info.viewer_args" ), p -> info_filename );
  } else {
    pargs = NULL;
    // WARNING: This might not be quite right; if no viewer-args, shouldn't we still append the info-filename? likewise,
    //          even if we do have view-args, shouldn't we check if filename is present?
  }

  char pndfile [ 1024 ];
  if ( p -> object_type == pnd_object_type_directory ) {
    // for PXML-app-dir, pnd_run.sh doesn't want the PXML.xml.. it just wants the dir-name
    strncpy ( pndfile, p -> object_path, 1000 );
  } else if ( p -> object_type == pnd_object_type_pnd ) {
    // pnd_run.sh wants the full path and filename for the .pnd file
    snprintf ( pndfile, 1020, "%s/%s", p -> object_path, p -> object_filename );
  }

  pnd_apps_exec_info_t info;
  info.viewer = viewer;
  info.args = pargs;

  if ( ! pnd_apps_exec_disco ( pndrun, p, PND_EXEC_OPTION_NORUN | PND_EXEC_OPTION_INFO, &info ) ) {
    return ( 0 );
  }

  fprintf ( f, "Exec=%s\n", pnd_apps_exec_runline() );

  if ( pnd_conf_get_as_char ( desktoph, "info.category" ) ) {
    fprintf ( f, "Categories=%s\n", pnd_conf_get_as_char ( desktoph, "info.category" ) );
  } else {
    fprintf ( f, "Categories=Documentation\n" );
  }

  fprintf ( f, "%s\n", PND_DOTDESKTOP_SOURCE ); // should we need to know 'who' created the file during trimming

  fclose ( f );

  return ( 1 );
}

unsigned char pnd_emit_icon ( char *targetpath, pnd_disco_t *p ) {
  //#define BITLEN (8*1024)
#define BITLEN (64*1024)
  char buffer [ FILENAME_MAX ]; // target filename
  char from [ FILENAME_MAX ];   // source filename
  unsigned char bits [ BITLEN ];
  unsigned int bitlen;
  FILE *pnd, *target;

  // prelim .. if a pnd file, and no offset found, discovery code didn't locate icon.. so bail.
  if ( ( p -> object_type == pnd_object_type_pnd ) &&
       ( ! p -> pnd_icon_pos ) )
  {
    return ( 0 ); // discover code didn't find it, so FAIL
  }

  // determine filename for target
  sprintf ( buffer, "%s/%s.png", targetpath, p -> unique_id /*, p -> subapp_number*/ ); // target

  /* first.. open the source file, by type of application:
   * are we looking through a pnd file or a dir?
   */
  if ( p -> object_type == pnd_object_type_directory ) {
    sprintf ( from, "%s/%s", p -> object_path, p -> icon );
  } else if ( p -> object_type == pnd_object_type_pnd ) {
    sprintf ( from, "%s/%s", p -> object_path, p -> object_filename );
  }

  pnd = fopen ( from, "rb" );

  if ( ! pnd ) {
    pnd_log ( PND_LOG_DEFAULT, "    Emit icon, couldn't open source\n" );
    return ( 0 );
  }

  unsigned int len;

  target = fopen ( buffer, "wb" );

  if ( ! target ) {
    fclose ( pnd );
    pnd_log ( PND_LOG_DEFAULT, "    Emit icon, couldn't open target\n" );
    return ( 0 );
  }

  fseek ( pnd, 0, SEEK_END );
  len = ftell ( pnd );
  //fseek ( pnd, 0, SEEK_SET );

  fseek ( pnd, p -> pnd_icon_pos, SEEK_SET );

  len -= p -> pnd_icon_pos;

  pnd_log ( PND_LOG_DEFAULT, "    Emit icon, length: %u\n", len );

  while ( len ) {

    if ( len > (BITLEN) ) {
      bitlen = (BITLEN);
    } else {
      bitlen = len;
    }

    if ( fread ( bits, bitlen, 1, pnd ) != 1 ) {
      fclose ( pnd );
      fclose ( target );
      unlink ( buffer );
      pnd_log ( PND_LOG_DEFAULT, "    Emit icon, bad read\n" );
      return ( 0 );
    }

#if 0
    {
      unsigned int i = 0;
      char bigbuffer [ 200 * 1024 ] = "\0";
      char b [ 10 ];
      pnd_log ( PND_LOG_DEFAULT, "    Read hexdump\n" );
      while ( i < bitlen ) {
	sprintf ( b, "%x,", bits [ i ] );
	strcat ( bigbuffer, b );
	i++;
      }
      pnd_log ( PND_LOG_DEFAULT, bigbuffer );
    }
#endif

    if ( fwrite ( bits, bitlen, 1, target ) != 1 ) {
      fclose ( pnd );
      fclose ( target );
      unlink ( buffer );
      pnd_log ( PND_LOG_DEFAULT, "    Emit icon, bad write\n" );
      return ( 0 );
    }

    len -= bitlen;
    //pnd_log ( PND_LOG_DEFAULT, "    Emit icon, next block, length: %u\n", len );
  } // while

  fclose ( pnd );
  fclose ( target );

  //pnd_log ( PND_LOG_DEFAULT, "    Emit icon, done.\n" );

  return ( 1 );
}

#if 1 // we switched direction to freedesktop standard categories
// if no categories herein, return 0; otherwise, if some category-like-text, return 1
int pnd_map_dotdesktop_categories ( pnd_conf_handle c, char *target_buffer, unsigned short int len, pnd_disco_t *d ) {
  char *t;
  char *match;

  // clear target so we can easily append
  memset ( target_buffer, '\0', len );

  // for each main-cat and sub-cat, including alternates, just append them all together
  // we'll try mapping them, since the categories file is there, but we'll default to
  // copying over; this lets the conf file do merging or renaming of cagtegories, which
  // could still be useful, but we can leave the conf file empty to effect a pure
  // trusted-PXML-copying

  // it would be sort of cumbersome to copy all the freedesktop.org defined categories (as
  // there are hundreds), and would also mean new ones and peoples custom ones would
  // flop

  /* attempt primary category chain
   */
  #define MAPCAT(field)				            \
    if ( ( t = d -> field ) ) {                             \
      match = pnd_map_dotdesktop_category ( c, t );         \
      if ( ( !match ) || ( strcasecmp ( match, "NoCategory" ) ) ) {	\
	strncat ( target_buffer, match ? match : t, len );  \
	strncat ( target_buffer, ";", len );		    \
      }                                                     \
    }

  MAPCAT(main_category);
  MAPCAT(main_category1);
  MAPCAT(main_category2);
  MAPCAT(alt_category);
  MAPCAT(alt_category1);
  MAPCAT(alt_category2);

  if ( target_buffer [ 0 ] ) {
    return ( 1 ); // I guess its 'good'?
  }

#if 0
  if ( ( t = d -> main_category ) ) {
    match = pnd_map_dotdesktop_category ( c, t );
    strncat ( target_buffer, match ? match : t, len );
    strncat ( target_buffer, ";", len );
  }
#endif

  return ( 0 );
}
#endif

#if 0 // we switched direction
//int pnd_map_dotdesktop_categories ( pnd_conf_handle c, char *target_buffer, unsigned short int len, pnd_pxml_handle h ) {
int pnd_map_dotdesktop_categories ( pnd_conf_handle c, char *target_buffer, unsigned short int len, pnd_disco_t *d ) {
  unsigned short int n = 0; // no. matches
  char *t;
  char *match;

  // clear target so we can easily append
  memset ( target_buffer, '\0', len );

  /* attempt primary category chain
   */
  match = NULL;

  if ( ( t = d -> main_category ) ) {
    match = pnd_map_dotdesktop_category ( c, t );
  }

  if ( ( ! match ) &&
       ( t = d -> main_category1 ) )
  {
    match = pnd_map_dotdesktop_category ( c, t );
  }

  if ( ( ! match ) &&
       ( t = d -> main_category2 ) )
  {
    match = pnd_map_dotdesktop_category ( c, t );
  }

  if ( match ) {
    strncat ( target_buffer, match, len );
    len -= strlen ( target_buffer );
    n += 1;
  }

  /* attempt secondary category chain
   */
  match = NULL;

  if ( ( t = d -> alt_category ) ) {
    match = pnd_map_dotdesktop_category ( c, t );
  }

  if ( ( ! match ) &&
       ( t = d -> alt_category1 ) )
  {
    match = pnd_map_dotdesktop_category ( c, t );
  }

  if ( ( ! match ) &&
       ( t = d -> alt_category2 ) )
  {
    match = pnd_map_dotdesktop_category ( c, t );
  }

  if ( match ) {
    if ( target_buffer [ 0 ] != '\0' && len > 0 ) {
      strcat ( target_buffer, ";" );
      len -= 1;
    }
    strncat ( target_buffer, match, len );
    len -= strlen ( target_buffer );
    n += 1;
  }

#if 0 // doh, originally I was using pxml-t till I realized I pre-boned myself on that one
  match = NULL;

  if ( ( t = pnd_pxml_get_main_category ( h ) ) ) {
    match = pnd_map_dotdesktop_category ( c, t );
  }

  if ( ( ! match ) &&
       ( t = pnd_pxml_get_subcategory1 ( h ) ) )
  {
    match = pnd_map_dotdesktop_category ( c, t );
  }

  if ( ( ! match ) &&
       ( t = pnd_pxml_get_subcategory2 ( h ) ) )
  {
    match = pnd_map_dotdesktop_category ( c, t );
  }

  if ( match ) {
    strncat ( target_buffer, match, len );
    len -= strlen ( target_buffer );
    n += 1;
  }

  /* attempt secondary category chain
   */
  match = NULL;

  if ( ( t = pnd_pxml_get_altcategory ( h ) ) ) {
    match = pnd_map_dotdesktop_category ( c, t );
  }

  if ( ( ! match ) &&
       ( t = pnd_pxml_get_altsubcategory1 ( h ) ) )
  {
    match = pnd_map_dotdesktop_category ( c, t );
  }

  if ( ( ! match ) &&
       ( t = pnd_pxml_get_altsubcategory2 ( h ) ) )
  {
    match = pnd_map_dotdesktop_category ( c, t );
  }

  if ( match ) {
    if ( target_buffer [ 0 ] != '\0' && len > 0 ) {
      strcat ( target_buffer, ";" );
      len -= 1;
    }
    strncat ( target_buffer, match, len );
    len -= strlen ( target_buffer );
    n += 1;
  }
#endif

  if ( n && len ) {
    strcat ( target_buffer, ";" );
  }

  return ( n );
}
#endif

// given category 'foo', look it up in the provided config map. return the char* reference, or NULL
char *pnd_map_dotdesktop_category ( pnd_conf_handle c, char *single_category ) {
  char *key;
  char *ret;

  key = malloc ( strlen ( single_category ) + 4 + 1 );

  sprintf ( key, "map.%s", single_category );

  ret = pnd_conf_get_as_char ( c, key );

  free ( key );

  return ( ret );
}

unsigned char *pnd_emit_icon_to_buffer ( pnd_disco_t *p, unsigned int *r_buflen ) {
  // this is shamefully mostly a copy of emit_icon() above; really, need to refactor that to use this routine
  // with a fwrite at the end...
  char from [ FILENAME_MAX ];   // source filename
  char bits [ 8 * 1024 ];
  unsigned int bitlen;
  FILE *pnd = NULL;
  unsigned char *target = NULL, *targiter = NULL;

  // prelim .. if a pnd file, and no offset found, discovery code didn't locate icon.. so bail.
  if ( ( p -> object_type == pnd_object_type_pnd ) &&
       ( ! p -> pnd_icon_pos ) )
  {
    return ( NULL ); // discover code didn't find it, so FAIL
  }

  /* first.. open the source file, by type of application:
   * are we looking through a pnd file or a dir?
   */
  if ( p -> object_type == pnd_object_type_directory ) {
    sprintf ( from, "%s/%s", p -> object_path, p -> icon );
  } else if ( p -> object_type == pnd_object_type_pnd ) {
    sprintf ( from, "%s/%s", p -> object_path, p -> object_filename );
  }

  pnd = fopen ( from, "r" );

  if ( ! pnd ) {
    return ( NULL );
  }

  // determine length of file, then adjust by icon position to find begin of icon
  unsigned int len;

  fseek ( pnd, 0, SEEK_END );
  len = ftell ( pnd );
  //fseek ( pnd, 0, SEEK_SET );

  fseek ( pnd, p -> pnd_icon_pos, SEEK_SET );

  len -= p -> pnd_icon_pos;

  // create target buffer
  target = malloc ( len );

  if ( ! target ) {
    fclose ( pnd );
    return ( 0 );
  }

  targiter = target;

  if ( r_buflen ) {
    *r_buflen = len;
  }

  // copy over icon to target
  while ( len ) {

    if ( len > (8*1024) ) {
      bitlen = (8*1024);
    } else {
      bitlen = len;
    }

    if ( fread ( bits, bitlen, 1, pnd ) != 1 ) {
      fclose ( pnd );
      free ( target );
      return ( NULL );
    }

    memmove ( targiter, bits, bitlen );
    targiter += bitlen;

    len -= bitlen;
  } // while

  fclose ( pnd );

  return ( target );
}

// parse_dotdesktop() can be used to read a libpnd generated .desktop and return a limited
// but useful disco-t structure back; possibly useful for scanning .desktops rather than
// scanning pnd-files?
pnd_disco_t *pnd_parse_dotdesktop ( char *ddpath, unsigned int flags ) {

  // will verify the .desktop has the libpnd-marking on it (X-Pandora-Source): PND_DOTDESKTOP_SOURCE

  // attempt to extract..
  // - unique-id (from filename or field)
  // - subapp number (from filename)
  // - exec required info
  // - icon path
  // - name (title-en)
  // - comment (desc-en)
  // - option_no_x11
  // - object path
  // - appdata name (or unique-id if not present)
  // - start dir
  // - args
  // - clockspeed
  // - categories

  char pndpath [ 1024 ];
  bzero ( pndpath, 1024 );

  // filter on filename?
  if ( flags & PND_DOTDESKTOP_LIBPND_ONLY ) {
    // too bad we didn't put some libpnd token at the front of the filename or something
    // hell, we should cleanse unique-id to ensure its not full of special chars like '*' and '..'.. eep!
    if ( strrchr ( ddpath, '#' ) == NULL ) { // but if requiring libpnd, we can at least check for #subapp-number
      return ( NULL );
    }
  }

  if ( strstr ( ddpath, ".desktop" ) == NULL ) {
    return ( NULL ); // no .desktop in filename, must be something else... skip!
  }

  if ( strstr ( ddpath, "info.desktop" ) != NULL ) {
    // ".....info.desktop" is the 'document help' (README) emitted from a pnd, not an actual app; minimenu rather
    // expects the doc-info as part of the main app, not a separate app.. so lets drop it here, to avoid doubling up
    // the number of applications, needlessly..
    return ( NULL );
  }

  // determine file length
  struct stat statbuf;

  if ( stat ( ddpath, &statbuf) < 0 ) {
    return ( NULL ); // couldn't open
  }

  // buffers..
  char dd [ 1024 ];
  unsigned char libpnd_origin = 0;

  // disco
  pnd_disco_t *p = malloc ( sizeof(pnd_disco_t) );
  if ( ! p ) {
    return ( NULL );
  }
  bzero ( p, sizeof(pnd_disco_t) );

  // inhale file
  FILE *f = fopen ( ddpath, "r" );

  if ( ! f ) {
    return ( NULL ); // not up or shut up!
  }

  while ( fgets ( dd, 1024, f ) ) {
    char *nl = strchr ( dd, '\n' );
    if ( nl ) {
      *nl = '\0';
    }

    // grep
    //

    if ( strncmp ( dd, "Name=", 5 ) == 0 ) {
      p -> title_en = strdup ( dd + 5 );
    } else if ( strncmp ( dd, "Name[en]=", 9 ) == 0 ) {
      p -> title_en = strdup ( dd + 9 );
    } else if ( strncmp ( dd, "Icon=", 5 ) == 0 ) {
      p -> icon = strdup ( dd + 5 );
    } else if ( strcmp ( dd, PND_DOTDESKTOP_SOURCE ) == 0 ) {
      libpnd_origin = 1;
    } else if ( strncmp ( dd, "X-Pandora-UID=", 14 ) == 0 ) {
      p -> unique_id = strdup ( dd + 14 );
    } else if ( strncmp ( dd, "X-Pandora-Preview-Pic-1=", 24 ) == 0 ) {
      p -> preview_pic1 = strdup ( dd + 24 );
    } else if ( strncmp ( dd, "X-Pandora-Clockspeed=", 21 ) == 0 ) {
      p -> clockspeed = strdup ( dd + 21 );
    } else if ( strncmp ( dd, "X-Pandora-Startdir=", 19 ) == 0 ) {
      p -> startdir = strdup ( dd + 19 );
    } else if ( strncmp ( dd, "X-Pandora-Appdata-Dirname=", 26 ) == 0 ) {
      p -> appdata_dirname = strdup ( dd + 26 );
    } else if ( strncmp ( dd, "X-Pandora-ExecArgs=", 19 ) == 0 ) {
      p -> execargs = strdup ( dd + 19 );
    } else if ( strncmp ( dd, "X-Pandora-Exec=", 15 ) == 0 ) {
      p -> exec = strdup ( dd + 15 );
    } else if ( strncmp ( dd, "X-Pandora-Object-Path=", 22 ) == 0 ) {
      p -> object_path = strdup ( dd + 22 );
    } else if ( strncmp ( dd, "X-Pandora-Object-Filename=", 26 ) == 0 ) {
      p -> object_filename = strdup ( dd + 26 );
    } else if ( strncmp ( dd, "X-Pandora-Object-Flag-OVR=", 26 ) == 0 ) {
      p -> object_flags |= PND_DISCO_FLAG_OVR;

    } else if ( strncmp ( dd, "X-Pandora-MainCategory=", 23 ) == 0 ) {
      p -> main_category = strdup ( dd + 23 );
    } else if ( strncmp ( dd, "X-Pandora-MainCategory1=", 24 ) == 0 ) {
      p -> main_category1 = strdup ( dd + 24 );
    } else if ( strncmp ( dd, "X-Pandora-MainCategory2=", 24 ) == 0 ) {
      p -> main_category2 = strdup ( dd + 24 );

    } else if ( strncmp ( dd, "X-Pandora-AltCategory=", 22 ) == 0 ) {
      p -> alt_category = strdup ( dd + 22 );
    } else if ( strncmp ( dd, "X-Pandora-AltCategory1=", 23 ) == 0 ) {
      p -> alt_category1 = strdup ( dd + 23 );
    } else if ( strncmp ( dd, "X-Pandora-AltCategory2=", 23 ) == 0 ) {
      p -> alt_category2 = strdup ( dd + 23 );

    } else if ( strncmp ( dd, "X-Pandora-Info-Filename=", 24 ) == 0 ) {
      p -> info_filename = strdup ( dd + 24 );
    } else if ( strncmp ( dd, "X-Pandora-Info-Name=", 20 ) == 0 ) {
      p -> info_name = strdup ( dd + 20 );

    } else if ( strncmp ( dd, "Comment=", 8 ) == 0 ) {
      p -> desc_en = strdup ( dd + 8 );
    } else if ( strncmp ( dd, "Comment[en]=", 12 ) == 0 ) {
      p -> desc_en = strdup ( dd + 12 );
    } else if ( strncmp ( dd, "Exec=", 5 ) == 0 ) {

      char *e = strstr ( dd, " -e " );

      if ( e ) {
	// probably libpnd app
#if 0 // no needed due to above X-Pandora attributes

	if ( e ) {
	  e += 5;

	  char *space = strchr ( e, ' ' );
	  p -> exec = strndup ( e, space - e - 1 );
	}

	char *b = strstr ( dd, " -b " );
	if ( b ) {
	  b += 5;
	  char *space = strchr ( b, '\0' );
	  p -> appdata_dirname = strndup ( b, space - b - 1 );
	}

	char *p = strstr ( dd, " -p " );
	if ( p ) {
	  p += 5;
	  char *space = strchr ( p, ' ' );
	  strncpy ( pndpath, p, space - p - 1 );
	}
#endif

      } else {
	// probably not libpnd app
	p -> exec = strdup ( dd + 5 );
      }

#if 0 // ignore; using X- categories now
    } else if ( strncmp ( dd, "Categories=", 11 ) == 0 ) {
      // HACK; only honours first category
      char *semi = strchr ( dd, ';' );
      if ( semi ) {
	p -> main_category = strndup ( dd + 11, semi - dd + 11 );
      } else {
	p -> main_category = strdup ( dd + 11 );
      }
      semi = strchr ( p -> main_category, ';' );
      if ( semi ) {
	*semi = '\0';
      }
#endif

    }

    //
    // /grep

  } // while

  fclose ( f );

  // filter
  if ( ! libpnd_origin ) {

    // convenience flag
    if ( flags & PND_DOTDESKTOP_LIBPND_ONLY ) {
      pnd_disco_destroy ( p );
      free ( p );
      return ( NULL );
    }

  } else {
    p -> object_flags |= PND_DISCO_LIBPND_DD; // so caller can do something if it wishes
  }

  // filter on content
  if ( ( ! p -> title_en ) ||
       ( ! p -> exec )
     )
  {
    pnd_disco_destroy ( p );
    free ( p );
    return ( NULL );
  }

  if ( ! p -> unique_id ) {
    if ( flags & PND_DOTDESKTOP_LIBPND_ONLY ) {
      pnd_disco_destroy ( p );
      free ( p );
      return ( NULL );
    } else {
      char hack [ 100 ];
      snprintf ( hack, 100, "inode-%lu", statbuf.st_ino );
      p -> unique_id = strdup ( hack );
    }
  }

  // additional
  p -> object_type = pnd_object_type_pnd;

#if 0 // nolonger needed due to above X-Pandora attributes
  char *source;
  if ( pndpath [ 0 ] ) {
    source = pndpath;
  } else {
    source = ddpath;
  }

  char *slash = strrchr ( source, '/' );
  if ( slash ) {
    p -> object_path = strndup ( source, slash - source );
    p -> object_filename = strdup ( slash + 1 );
  } else {
    p -> object_path = "./";
    p -> object_filename = strdup ( source );
  }
#endif

  // lame guards, in case of lazy consumers and broken .desktop files
  if ( p -> object_path == NULL ) {
    p -> object_path = strdup ( "/tmp" );
  }
  if ( p -> object_filename == NULL ) {
    p -> object_filename = strdup ( "" ); // force bad filename
  }

  // return disco-t
  return ( p );
}
