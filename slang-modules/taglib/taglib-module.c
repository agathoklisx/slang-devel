/*
 * This is a S-Lang binding to TagLib an Audio meta-data library
 * http://developer.kde.org/~wheeler/taglib.html
 * licensed with:
 *      GNU LESSER GENERAL PUBLIC LICENSE
 *           Version 2.1, February 1999
 *
 * Initial code written by Agathoklis D.E Chatzimanikas
 * 
 * Last checked against with libtag.so.1.17.0
 *
 * compiled with gcc:   
  gcc taglib-module.c -I/usr/local/include -g -O2 -Wl,-R/usr/local/lib \
    --shared -fPIC -ltag_c -o taglib-module.so

 * compiled with gcc and with the followings warning flags on
  gcc taglib-module.c -I/usr/local/include -g -O2 -Wl,-R/usr/local/lib \
    --shared -fPIC -ltag_c -Wall -Wformat=2 -W -Wunused -Wundef -pedantic \
    -Wno-long-long -Winline -Wmissing-prototypes -Wnested-externs \
    -Wpointer-arith -Wcast-align -Wshadow -Wstrict-prototypes -Wextra \
    -Wc++-compat -Wlogical-op -o taglib-module.so
  */

#include <taglib/tag_c.h>
#include <slang.h>

SLANG_MODULE(taglib);

typedef struct
  {
  char *title;
  char *artist;
  char *album;
  char *comment;
  char *genre;
  int track;
  int year;
  } TagLib;

static SLang_CStruct_Field_Type TagLib_Struct [] =
{
  MAKE_CSTRUCT_FIELD(TagLib, title, "title", SLANG_STRING_TYPE, 0),
  MAKE_CSTRUCT_FIELD(TagLib, artist, "artist", SLANG_STRING_TYPE, 0),
  MAKE_CSTRUCT_FIELD(TagLib, album, "album", SLANG_STRING_TYPE, 0),
  MAKE_CSTRUCT_FIELD(TagLib, comment, "comment", SLANG_STRING_TYPE, 0),
  MAKE_CSTRUCT_FIELD(TagLib, genre, "genre", SLANG_STRING_TYPE, 0),
  MAKE_CSTRUCT_INT_FIELD(TagLib, track, "track", 0),
  MAKE_CSTRUCT_INT_FIELD(TagLib, year, "year", 0),
  SLANG_END_CSTRUCT_TABLE
};

typedef struct
  {
  int bitrate;
  int length;
  int channels;
  int samplerate;
  } TagLib_Audio_Properties;

static SLang_CStruct_Field_Type TagLib_AudioProperties_Struct [] =
{
  MAKE_CSTRUCT_INT_FIELD(TagLib_Audio_Properties, bitrate, "bitrate", 0),
  MAKE_CSTRUCT_INT_FIELD(TagLib_Audio_Properties, length, "length", 0),
  MAKE_CSTRUCT_INT_FIELD(TagLib_Audio_Properties, channels, "channels", 0),
  MAKE_CSTRUCT_INT_FIELD(TagLib_Audio_Properties, samplerate, "samplerate", 0),
  SLANG_END_CSTRUCT_TABLE
};

static int tagwrite_intrinsic (void)
{
  TagLib tags;
  TagLib_File *file;
  TagLib_Tag *tag;
  char *fname;

  if (-1 == SLang_pop_cstruct ((VOID_STAR)&tags, TagLib_Struct))
    return -1;

  if (-1 == SLpop_string (&fname))
    {
    SLang_free_cstruct ((VOID_STAR)&tags, TagLib_Struct);
    return -1;
    }

  file = taglib_file_new (fname);
  SLfree (fname);

  if (file == NULL)
    {
    SLang_free_cstruct ((VOID_STAR)&tags, TagLib_Struct);
    return -1;
    }

  if (!taglib_file_is_valid (file))
    {
    SLang_free_cstruct ((VOID_STAR)&tags, TagLib_Struct);
    taglib_file_free (file);
    return -1;
    }

  taglib_set_strings_unicode (1);
  tag = taglib_file_tag (file);

  if (!tag)
    {
    SLang_free_cstruct ((VOID_STAR)&tags, TagLib_Struct);
    taglib_file_free (file);
    return -1;
    }

  taglib_tag_set_title (tag, tags.title);
  taglib_tag_set_artist (tag, tags.artist);
  taglib_tag_set_album (tag, tags.album);
  taglib_tag_set_comment (tag, tags.comment);
  taglib_tag_set_genre (tag, tags.genre);
  taglib_tag_set_year (tag, tags.year);
  taglib_tag_set_track (tag, tags.track);

  taglib_file_save (file);
  taglib_tag_free_strings ();
  taglib_file_free (file);

  SLang_free_cstruct ((VOID_STAR)&tags, TagLib_Struct);
  return 0;
}

static void tagread_intrinsic (char *fname)
{
  TagLib tags;
  TagLib_File *file;
  TagLib_Tag *tag;

  taglib_set_strings_unicode (1);

  file = taglib_file_new (fname);

  if (file == NULL)
    {
    (void) SLang_push_null ();
    return;
    }

  if (!taglib_file_is_valid (file))
    {
    taglib_file_free (file);
    (void) SLang_push_null ();
    return;
    }

  tag = taglib_file_tag (file);

  if (!tag)
    {
    taglib_file_free (file);
    (void) SLang_push_null ();
    return;
    }

  tags.title = taglib_tag_title (tag);
  tags.artist = taglib_tag_artist (tag);
  tags.album = taglib_tag_album (tag);
  tags.comment = taglib_tag_comment (tag);
  tags.genre = taglib_tag_genre (tag);
  tags.year = taglib_tag_year (tag);
  tags.track = taglib_tag_track (tag);

  SLang_push_cstruct ((VOID_STAR) &tags, TagLib_Struct);

  taglib_tag_free_strings ();
  taglib_file_free (file);
}

static void audio_properties_intrin (char *fname)
{
  TagLib_Audio_Properties p;
  const TagLib_AudioProperties *prop;
  TagLib_File *file;

  file = taglib_file_new (fname);

  if (file == NULL)
    {
    (void) SLang_push_null ();
    return;
    }

  if (!taglib_file_is_valid (file))
    {
    taglib_file_free (file);
    (void) SLang_push_null ();
    return;
    }

  prop = taglib_file_audioproperties (file);

  if (!prop)
    {
    taglib_file_free (file);
    (void) SLang_push_null ();
    return;
    }

  p.bitrate = taglib_audioproperties_bitrate (prop);
  p.length  = taglib_audioproperties_length (prop);
  p.channels = taglib_audioproperties_channels (prop);
  p.samplerate = taglib_audioproperties_samplerate (prop);

  SLang_push_cstruct ((VOID_STAR) &p, TagLib_AudioProperties_Struct);

  taglib_tag_free_strings ();
  taglib_file_free (file);
}

static SLang_Intrin_Fun_Type taglib_Intrinsics [] =
{
  MAKE_INTRINSIC_S("tagread", tagread_intrinsic, VOID_TYPE),
  MAKE_INTRINSIC_0("tagwrite", tagwrite_intrinsic, SLANG_INT_TYPE),
  MAKE_INTRINSIC_S("audioproperties", audio_properties_intrin, VOID_TYPE),
  SLANG_END_INTRIN_FUN_TABLE
};

int init_taglib_module_ns (char *ns_name)
{
  SLang_NameSpace_Type *ns;

  if (NULL == (ns = SLns_create_namespace (ns_name)))
    return -1;

  if (-1 == SLns_add_intrin_fun_table (ns, taglib_Intrinsics, NULL))
    return -1;

  return 0;
}
