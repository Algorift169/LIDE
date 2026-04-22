#ifndef FILE_ROLLER_H
#define FILE_ROLLER_H

#include <gtk/gtk.h>
#include <glib.h>

/**
 * file-roller.h
 *
 * Universal file viewer for LIDE desktop environment.
 * Supports images, text, PDFs, videos, archives and more.
 *
 * This module is part of the LIDE desktop environment system.
 */

/* File type enumeration */
typedef enum {
    FILE_TYPE_IMAGE,      /* PNG, JPG, GIF, BMP, etc */
    FILE_TYPE_TEXT,       /* TXT, C, H, PY, etc */
    FILE_TYPE_PDF,        /* PDF documents */
    FILE_TYPE_VIDEO,      /* MP4, WebM, MKV, etc */
    FILE_TYPE_AUDIO,      /* MP3, WAV, FLAC, etc */
    FILE_TYPE_ARCHIVE,    /* ZIP, TAR, 7Z, etc */
    FILE_TYPE_UNKNOWN
} FileType;

/* Detect file type based on MIME type or extension */
FileType get_file_type(const char *filename);

/* Get human readable file type name */
const char *get_file_type_name(FileType type);

/* Check if file type is supported */
gboolean is_file_type_supported(FileType type);

#endif /* FILE_ROLLER_H */
