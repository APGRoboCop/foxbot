/* 
   Unix SMB/CIFS implementation.
   Utility functions for Samba
   Copyright (C) Andrew Tridgell 1992-1999
   Copyright (C) Jelmer Vernooij 2005
    
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SAMBA_UTIL_H_
#define _SAMBA_UTIL_H_

#ifndef SAMBA_UTIL_CORE_ONLY
#include <charset.h>
#else
#include "charset_compat.h"
#endif

#include <util/attr.h>

/* for TALLOC_CTX */
#include <talloc.h>

/* for struct stat */
#include <sys/stat.h>

/**
 * @file
 * @brief Helpful macros
 */

struct smbsrv_tcon;

extern const char *panic_action;

#include <util/time.h>
#include <util/data_blob.h>
#include <util/xfile.h>
#include <util/byteorder.h>
#include <util/talloc_stack.h>

#ifndef ABS
#define ABS(a) ((a)>0?(a):(-(a)))
#endif

#include <util/memory.h>

#include <core/ntstatus.h>
#include <util/string_wrappers.h>

#include <util/fault.h>

/**
 * Write backtrace to debug log
 */
_PUBLIC_ void call_backtrace(void);

_PUBLIC_ void dump_core_setup(const char *progname, const char *logfile);

/**
  register a fault handler. 
  Should only be called once in the execution of smbd.
*/
_PUBLIC_ bool register_fault_handler(const char *name, void (*fault_handler)(int sig));

#include <util/signal.h>

struct sockaddr;

_PUBLIC_ int sys_getnameinfo(const struct sockaddr *psa,
			     int salen,
			     char *host,
			     size_t hostlen,
			     char *service,
			     size_t servlen,
			     int flags);

/* The following definitions come from lib/util/genrand.c  */

#include <util/genrand.h>

/**
  generate a single random uint32_t
**/
_PUBLIC_ uint32_t generate_random(void);

/**
  very basic password quality checker
**/
_PUBLIC_ bool check_password_quality(const char *s);

/**
 * Generate a random text password.
 */
_PUBLIC_ char *generate_random_password(TALLOC_CTX *mem_ctx, size_t min, size_t max);

/**
 Use the random number generator to generate a random string.
**/
_PUBLIC_ char *generate_random_str_list(TALLOC_CTX *mem_ctx, size_t len, const char *list);

/**
 * Generate a random text string consisting of the specified length.
 * The returned string will be allocated.
 *
 * Characters used are: ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+_-#.,
 */
_PUBLIC_ char *generate_random_str(TALLOC_CTX *mem_ctx, size_t len);

/**
 * Generate an array of unique text strings all of the same length.
 * The returned strings will be allocated.
 * Returns NULL if the number of unique combinations cannot be created.
 *
 * Characters used are: ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+_-#.,
 */
_PUBLIC_ char** generate_unique_strs(TALLOC_CTX *mem_ctx, size_t len,
                                         uint32_t num);

/* The following definitions come from lib/util/dprintf.c  */

_PUBLIC_ int d_fprintf(FILE *f, const char *format, ...) PRINTF_ATTRIBUTE(2,3);
_PUBLIC_ int d_printf(const char *format, ...) PRINTF_ATTRIBUTE(1,2);
_PUBLIC_ void display_set_stderr(void);

/* The following definitions come from lib/util/util_str.c  */

bool next_token_talloc(TALLOC_CTX *ctx,
			const char **ptr,
			char **pp_buff,
			const char *sep);

/**
 * Get the next token from a string, return false if none found.  Handles
 * double-quotes.  This version does not trim leading separator characters
 * before looking for a token.
 */
bool next_token_no_ltrim_talloc(TALLOC_CTX *ctx,
			const char **ptr,
			char **pp_buff,
			const char *sep);


/**
 Trim the specified elements off the front and back of a string.
**/
_PUBLIC_ bool trim_string(char *s, const char *front, const char *back);

/**
 Find the number of 'c' chars in a string
**/
_PUBLIC_ _PURE_ size_t count_chars(const char *s, char c);

/**
 Routine to get hex characters and turn them into a 16 byte array.
 the array can be variable length, and any non-hex-numeric
 characters are skipped.  "0xnn" or "0Xnn" is specially catered
 for.

 valid examples: "0A5D15"; "0x15, 0x49, 0xa2"; "59\ta9\te3\n"


**/
_PUBLIC_ size_t strhex_to_str(char *p, size_t p_len, const char *strhex, size_t strhex_len);

/** 
 * Parse a hex string and return a data blob. 
 */
_PUBLIC_ _PURE_ DATA_BLOB strhex_to_data_blob(TALLOC_CTX *mem_ctx, const char *strhex) ;

/**
 * Print a buf in hex. Assumes dst is at least (srclen*2)+1 large.
 */
_PUBLIC_ void hex_encode_buf(char *dst, const uint8_t *src, size_t srclen);

/**
 * talloc version of hex_encode_buf()
 */
_PUBLIC_ char *hex_encode_talloc(TALLOC_CTX *mem_ctx, const unsigned char *buff_in, size_t len);

#include <util/substitute.h>

/**
 Unescape a URL encoded string, in place.
**/
_PUBLIC_ void rfc1738_unescape(char *buf);


/**
 * rfc1738_escape
 * Returns a static buffer that contains the RFC
 * 1738 compliant, escaped version of the given url. (escapes unsafe and % characters)
 **/
_PUBLIC_ char *rfc1738_escape(TALLOC_CTX *mem_ctx, const char *url);

/**
 * rfc1738_escape_unescaped
 *
 * Returns a static buffer that contains
 * the RFC 1738 compliant, escaped version of the given url (escapes unsafe chars only)
 **/
_PUBLIC_ char *rfc1738_escape_unescaped(TALLOC_CTX *mem_ctx, const char *url);

/**
 * rfc1738_escape_part 
 * Returns a static buffer that contains the RFC
 * 1738 compliant, escaped version of the given url segment. (escapes
 * unsafe, reserved and % chars) It would mangle the :// in http://,
 * and mangle paths (because of /).
 **/
_PUBLIC_ char *rfc1738_escape_part(TALLOC_CTX *mem_ctx, const char *url);

/**
 * Add a string to an array of strings.
 *
 * num should be a pointer to an integer that holds the current 
 * number of elements in strings. It will be updated by this function.
 */
_PUBLIC_ bool add_string_to_array(TALLOC_CTX *mem_ctx,
			 const char *str, const char ***strings, size_t *num);

/**
  varient of strcmp() that handles NULL ptrs
**/
_PUBLIC_ int strcmp_safe(const char *s1, const char *s2);

/**
return the number of bytes occupied by a buffer in ASCII format
the result includes the null termination
limited by 'n' bytes
**/
_PUBLIC_ size_t ascii_len_n(const char *src, size_t n);

/**
 Set a boolean variable from the text value stored in the passed string.
 Returns true in success, false if the passed string does not correctly 
 represent a boolean.
**/
_PUBLIC_ bool set_boolean(const char *boolean_string, bool *boolean);

/**
 * Parse a string containing a boolean value.
 *
 * val will be set to the read value.
 *
 * @retval true if a boolean value was parsed, false otherwise.
 */
_PUBLIC_ bool conv_str_bool(const char * str, bool * val);

/**
 * Convert a size specification like 16K into an integral number of bytes. 
 **/
_PUBLIC_ bool conv_str_size_error(const char * str, uint64_t * val);

/**
 * Parse a uint64_t value from a string
 *
 * val will be set to the value read.
 *
 * @retval true if parsing was successful, false otherwise
 */
_PUBLIC_ bool conv_str_u64(const char * str, uint64_t * val);

/**
return the number of bytes occupied by a buffer in CH_UTF16 format
the result includes the null termination
**/
_PUBLIC_ size_t utf16_len(const void *buf);

/**
return the number of bytes occupied by a buffer in CH_UTF16 format
the result includes the null termination
limited by 'n' bytes
**/
_PUBLIC_ size_t utf16_len_n(const void *src, size_t n);
_PUBLIC_ size_t ucs2_align(const void *base_ptr, const void *p, int flags);

/**
Do a case-insensitive, whitespace-ignoring string compare.
**/
_PUBLIC_ int strwicmp(const char *psz1, const char *psz2);

/**
 String replace.
**/
_PUBLIC_ void string_replace(char *s, char oldc, char newc);

/**
 Base64 decode a string, place into a data blob.  Caller to data_blob_free() the result.
**/
_PUBLIC_ DATA_BLOB base64_decode_data_blob_talloc(TALLOC_CTX *mem_ctx, const char *s);

/**
 Base64 decode a string, place into a data blob on NULL context.
 Caller to data_blob_free() the result.
**/
_PUBLIC_ DATA_BLOB base64_decode_data_blob(const char *s);


/**
 Base64 decode a string, inplace
**/
_PUBLIC_ void base64_decode_inplace(char *s);
/**
 Base64 encode a binary data blob into a string
**/
_PUBLIC_ char *base64_encode_data_blob(TALLOC_CTX *mem_ctx, DATA_BLOB data);

/**
 * Compare 2 strings.
 *
 * @note The comparison is case-insensitive.
 **/
_PUBLIC_ bool strequal(const char *s1, const char *s2);

/* The following definitions come from lib/util/util_strlist.c  */

/* separators for lists */
#ifndef LIST_SEP
#define LIST_SEP " \t,\n\r"
#endif

/**
  build an empty (only NULL terminated) list of strings (for expansion with str_list_add() etc)
*/
_PUBLIC_ char **str_list_make_empty(TALLOC_CTX *mem_ctx);

/**
  place the only element 'entry' into a new, NULL terminated string list
*/
_PUBLIC_ char **str_list_make_single(TALLOC_CTX *mem_ctx,
	const char *entry);

/**
  build a null terminated list of strings from a input string and a
  separator list. The separator list must contain characters less than
  or equal to 0x2f for this to work correctly on multi-byte strings
*/
_PUBLIC_ char **str_list_make(TALLOC_CTX *mem_ctx, const char *string,
	const char *sep);

/**
 * build a null terminated list of strings from an argv-like input string 
 * Entries are separated by spaces and can be enclosed by quotes.
 * Does NOT support escaping
 */
_PUBLIC_ char **str_list_make_shell(TALLOC_CTX *mem_ctx, const char *string, const char *sep);

/**
 * join a list back to one string 
 */
_PUBLIC_ char *str_list_join(TALLOC_CTX *mem_ctx, const char **list, char separator);

/** join a list back to one (shell-like) string; entries 
 * separated by spaces, using quotes where necessary */
_PUBLIC_ char *str_list_join_shell(TALLOC_CTX *mem_ctx, const char **list, char sep);

/**
  return the number of elements in a string list
*/
_PUBLIC_ size_t str_list_length(const char * const *list);

/**
  copy a string list
*/
_PUBLIC_ char **str_list_copy(TALLOC_CTX *mem_ctx, const char **list);

/**
   Return true if all the elements of the list match exactly.
 */
_PUBLIC_ bool str_list_equal(const char * const *list1, const char * const *list2);

/**
  add an entry to a string list
*/
_PUBLIC_ const char **str_list_add(const char **list, const char *s);

/**
  remove an entry from a string list
*/
_PUBLIC_ void str_list_remove(const char **list, const char *s);

/**
  return true if a string is in a list
*/
_PUBLIC_ bool str_list_check(const char **list, const char *s);

/**
  return true if a string is in a list, case insensitively
*/
_PUBLIC_ bool str_list_check_ci(const char **list, const char *s);
/**
  append one list to another - expanding list1
*/
_PUBLIC_ const char **str_list_append(const char **list1,
	const char * const *list2);

/**
 remove duplicate elements from a list 
*/
_PUBLIC_ const char **str_list_unique(const char **list);

/*
  very useful when debugging complex list related code
 */
_PUBLIC_ void str_list_show(const char **list);


/**
  append one list to another - expanding list1
  this assumes the elements of list2 are const pointers, so we can re-use them
*/
_PUBLIC_ const char **str_list_append_const(const char **list1,
					    const char **list2);

/**
  add an entry to a string list
  this assumes s will not change
*/
_PUBLIC_ const char **str_list_add_const(const char **list, const char *s);

/**
  copy a string list
  this assumes list will not change
*/
_PUBLIC_ const char **str_list_copy_const(TALLOC_CTX *mem_ctx,
					  const char **list);

/**
 * Needed for making an "unconst" list "const"
 */
_PUBLIC_ const char **const_str_list(char **list);

/**
 * str_list_make, v3 version. The v4 version does not
 * look at quoted strings with embedded blanks, so
 * do NOT merge this function please!
 */
char **str_list_make_v3(TALLOC_CTX *mem_ctx, const char *string,
	const char *sep);


const char **str_list_make_v3_const(TALLOC_CTX *mem_ctx,
				    const char *string,
				    const char *sep);

/* The following definitions come from lib/util/util_file.c  */


/**
read a line from a file with possible \ continuation chars. 
Blanks at the start or end of a line are stripped.
The string will be allocated if s2 is NULL
**/
_PUBLIC_ char *fgets_slash(char *s2,int maxlen,XFILE *f);

/**
 * Read one line (data until next newline or eof) and allocate it 
 */
_PUBLIC_ char *afdgets(int fd, TALLOC_CTX *mem_ctx, size_t hint);

/**
load a file into memory from a fd.
**/
_PUBLIC_ char *fd_load(int fd, size_t *size, size_t maxsize, TALLOC_CTX *mem_ctx);


char **file_lines_parse(char *p, size_t size, int *numlines, TALLOC_CTX *mem_ctx);

/**
load a file into memory
**/
_PUBLIC_ char *file_load(const char *fname, size_t *size, size_t maxsize, TALLOC_CTX *mem_ctx);

/**
load a file into memory and return an array of pointers to lines in the file
must be freed with talloc_free(). 
**/
_PUBLIC_ char **file_lines_load(const char *fname, int *numlines, size_t maxsize, TALLOC_CTX *mem_ctx);

/**
load a fd into memory and return an array of pointers to lines in the file
must be freed with talloc_free(). If convert is true calls unix_to_dos on
the list.
**/
_PUBLIC_ char **fd_lines_load(int fd, int *numlines, size_t maxsize, TALLOC_CTX *mem_ctx);

_PUBLIC_ bool file_save_mode(const char *fname, const void *packet,
			     size_t length, mode_t mode);
/**
  save a lump of data into a file. Mostly used for debugging 
*/
_PUBLIC_ bool file_save(const char *fname, const void *packet, size_t length);
_PUBLIC_ int vfdprintf(int fd, const char *format, va_list ap) PRINTF_ATTRIBUTE(2,0);
_PUBLIC_ int fdprintf(int fd, const char *format, ...) PRINTF_ATTRIBUTE(2,3);

/*
  compare two files, return true if the two files have the same content
 */
bool file_compare(const char *path1, const char *path2);

/* The following definitions come from lib/util/util.c  */


/**
 Find a suitable temporary directory. The result should be copied immediately
 as it may be overwritten by a subsequent call.
**/
_PUBLIC_ const char *tmpdir(void);

/**
 * Creates and immediately unlinks a file. Returns open file descriptor.
 **/
_PUBLIC_ int create_unlink_tmp(const char *dir);

/**
 Check if a file exists - call vfs_file_exist for samba files.
**/
_PUBLIC_ bool file_exist(const char *fname);

/**
 Check a files mod time.
**/
_PUBLIC_ time_t file_modtime(const char *fname);

/**
 Check if a directory exists.
**/
_PUBLIC_ bool directory_exist(const char *dname);

/**
 Check file permissions.
**/
_PUBLIC_ bool file_check_permissions(const char *fname,
				     uid_t uid,
				     mode_t file_perms,
				     struct stat *pst);

/**
 * Try to create the specified directory if it didn't exist.
 *
 * @retval true if the directory already existed and has the right permissions 
 * or was successfully created.
 */
_PUBLIC_ bool directory_create_or_exist(const char *dname, mode_t dir_perms);

_PUBLIC_ bool directory_create_or_exist_strict(const char *dname,
					       uid_t uid,
					       mode_t dir_perms);

#include <util/blocking.h>

/**
 Sleep for a specified number of milliseconds.
**/
_PUBLIC_ void smb_msleep(unsigned int t);

/**
 Get my own name, return in talloc'ed storage.
**/
_PUBLIC_ char* get_myname(TALLOC_CTX *mem_ctx);

/**
 Check if a process exists. Does this work on all unixes?
**/
_PUBLIC_ bool process_exists_by_pid(pid_t pid);

/**
 Simple routine to do POSIX file locking. Cruft in NFS and 64->32 bit mapping
 is dealt with in posix.c
**/
_PUBLIC_ bool fcntl_lock(int fd, int op, off_t offset, off_t count, int type);

/**
 * Write dump of binary data to a callback
 */
void dump_data_cb(const uint8_t *buf, int len,
		  bool omit_zero_bytes,
		  void (*cb)(const char *buf, void *private_data),
		  void *private_data);

/**
 * Write dump of binary data to a FILE
 */
void dump_data_file(const uint8_t *buf, int len, bool omit_zero_bytes,
		    FILE *f);

/**
 * Write dump of binary data to the log file.
 *
 * The data is only written if the log level is at least level.
 */
_PUBLIC_ void dump_data(int level, const uint8_t *buf,int len);

/**
 * Write dump of binary data to the log file.
 *
 * The data is only written if the log level is at least level for
 * debug class dbgc_class.
 */
_PUBLIC_ void dump_data_dbgc(int dbgc_class, int level, const uint8_t *buf, int len);

/**
 * Write dump of binary data to the log file.
 *
 * The data is only written if the log level is at least level.
 * 16 zero bytes in a row are omitted
 */
_PUBLIC_ void dump_data_skip_zeros(int level, const uint8_t *buf, int len);

/**
 malloc that aborts with smb_panic on fail or zero size.
**/
_PUBLIC_ void *smb_xmalloc(size_t size);

/**
 Memdup with smb_panic on fail.
**/
_PUBLIC_ void *smb_xmemdup(const void *p, size_t size);

/**
 strdup that aborts on malloc fail.
**/
_PUBLIC_ char *smb_xstrdup(const char *s);

char *smb_xstrndup(const char *s, size_t n);

/**
 Like strdup but for memory.
**/
_PUBLIC_ void *smb_memdup(const void *p, size_t size);

/**
 * Write a password to the log file.
 *
 * @note Only actually does something if DEBUG_PASSWORD was defined during 
 * compile-time.
 */
_PUBLIC_ void dump_data_pw(const char *msg, const uint8_t * data, size_t len);

/**
 * see if a range of memory is all zero. A NULL pointer is considered
 * to be all zero 
 */
_PUBLIC_ bool all_zero(const uint8_t *ptr, size_t size);

/**
  realloc an array, checking for integer overflow in the array size
*/
_PUBLIC_ void *realloc_array(void *ptr, size_t el_size, unsigned count, bool free_on_fail);

void *malloc_array(size_t el_size, unsigned int count);

void *memalign_array(size_t el_size, size_t align, unsigned int count);

void *calloc_array(size_t size, size_t nmemb);

/* The following definitions come from lib/util/fsusage.c  */


/**
 * Retrieve amount of free disk space.
 * this does all of the system specific guff to get the free disk space.
 * It is derived from code in the GNU fileutils package, but has been
 * considerably mangled for use here 
 *
 * results are returned in *dfree and *dsize, in 512 byte units
*/
_PUBLIC_ int sys_fsusage(const char *path, uint64_t *dfree, uint64_t *dsize);

/* The following definitions come from lib/util/ms_fnmatch.c  */


/**
 * @file
 * @brief MS-style Filename matching
 */

int ms_fnmatch_protocol(const char *pattern, const char *string, int protocol);

/** a generic fnmatch function - uses for non-CIFS pattern matching */
int gen_fnmatch(const char *pattern, const char *string);

#include <util/idtree.h>
#include <util/idtree_random.h>

/**
 Close the low 3 fd's and open dev/null in their place
**/
_PUBLIC_ void close_low_fds(bool stdin_too, bool stdout_too, bool stderr_too);

/**
 Become a daemon, discarding the controlling terminal.
**/
_PUBLIC_ void become_daemon(bool do_fork, bool no_process_group, bool log_stdout);

/**
 Exit daemon and print error message to the log at level 0
 Optionally report failure to systemd if systemd integration is enabled
**/
_PUBLIC_ void exit_daemon(const char *msg, int error);

/**
 Report that the daemon is ready to serve connections to the log at level 0
 Optionally report status to systemd if systemd integration is enabled
**/
_PUBLIC_ void daemon_ready(const char *daemon);

/*
 * Report the daemon status. For example if it is not ready to serve connections
 * and is waiting for some event to happen.
 */
_PUBLIC_ void daemon_status(const char *name, const char *msg);

/**
 * @brief Get a password from the console.
 *
 * You should make sure that the buffer is an empty string!
 *
 * You can also use this function to ask for a username. Then you can fill the
 * buffer with the username and it is shows to the users. If the users just
 * presses enter the buffer will be untouched.
 *
 * @code
 *   char username[128];
 *
 *   snprintf(username, sizeof(username), "john");
 *
 *   smb_getpass("Username:", username, sizeof(username), 1, 0);
 * @endcode
 *
 * The prompt will look like this:
 *
 *   Username: [john]
 *
 * If you press enter then john is used as the username, or you can type it in
 * to change it.
 *
 * @param[in]  prompt   The prompt to show to ask for the password.
 *
 * @param[out] buf    The buffer the password should be stored. It NEEDS to be
 *                      empty or filled out.
 *
 * @param[in]  len      The length of the buffer.
 *
 * @param[in]  echo     Should we echo what you type.
 *
 * @param[in]  verify   Should we ask for the password twice.
 *
 * @return              0 on success, -1 on error.
 */
_PUBLIC_ int samba_getpass(const char *prompt, char *buf, size_t len,
			   bool echo, bool verify);

/**
 * Load a ini-style file.
 */
bool pm_process( const char *fileName,
                 bool (*sfunc)(const char *, void *),
                 bool (*pfunc)(const char *, const char *, void *),
				 void *userdata);

void print_asc(int level, const uint8_t *buf,int len);
void print_asc_cb(const uint8_t *buf, int len,
		  void (*cb)(const char *buf, void *private_data),
		  void *private_data);

/**
 * Add an id to an array of ids.
 *
 * num should be a pointer to an integer that holds the current
 * number of elements in ids. It will be updated by this function.
 */

bool add_uid_to_array_unique(TALLOC_CTX *mem_ctx, uid_t uid,
			     uid_t **uids, uint32_t *num_uids);
bool add_gid_to_array_unique(TALLOC_CTX *mem_ctx, gid_t gid,
			     gid_t **gids, uint32_t *num_gids);

/**
 * Allocate anonymous shared memory of the given size
 */
void *anonymous_shared_allocate(size_t bufsz);
void *anonymous_shared_resize(void *ptr, size_t new_size, bool maymove);
void anonymous_shared_free(void *ptr);

/*
  run a command as a child process, with a timeout.

  any stdout/stderr from the child will appear in the Samba logs with
  the specified log levels

  If callback is set then the callback is called on completion
  with the return code from the command
 */
struct tevent_context;
struct tevent_req;
struct tevent_req *samba_runcmd_send(TALLOC_CTX *mem_ctx,
				     struct tevent_context *ev,
				     struct timeval endtime,
				     int stdout_log_level,
				     int stderr_log_level,
				     const char * const *argv0, ...);
int samba_runcmd_recv(struct tevent_req *req, int *perrno);

#ifdef DEVELOPER
void samba_start_debugger(void);
#endif

/**
 * @brief Returns an absolute path to a file in the Samba modules directory.
 *
 * @param name File to find, relative to MODULESDIR.
 *
 * @retval Pointer to a string containing the full path.
 **/
char *modules_path(TALLOC_CTX *mem_ctx, const char *name);

/**
 * @brief Returns an absolute path to a file in the Samba data directory.
 *
 * @param name File to find, relative to CODEPAGEDIR.
 *
 * @retval Pointer to a talloc'ed string containing the full path.
 **/
char *data_path(TALLOC_CTX *mem_ctx, const char *name);

/**
 * @brief Returns the platform specific shared library extension.
 *
 * @retval Pointer to a const char * containing the extension.
 **/
const char *shlib_ext(void);

struct server_id;

struct server_id_buf { char buf[48]; }; /* probably a bit too large ... */
char *server_id_str_buf(struct server_id id, struct server_id_buf *dst);

bool server_id_same_process(const struct server_id *p1,
			    const struct server_id *p2);
bool server_id_equal(const struct server_id *p1, const struct server_id *p2);
struct server_id server_id_from_string(uint32_t local_vnn,
				       const char *pid_string);

/**
 * Set the serverid to the special value that represents a disconnected
 * client for (e.g.) durable handles.
 */
void server_id_set_disconnected(struct server_id *id);

/**
 * check whether a serverid is the special placeholder for
 * a disconnected client
 */
bool server_id_is_disconnected(const struct server_id *id);

void server_id_put(uint8_t buf[24], const struct server_id id);
void server_id_get(struct server_id *id, const uint8_t buf[24]);

/*
 * Samba code should use samba_tevent_context_init() instead of
 * tevent_context_init() in order to get the debug output.
 */
struct tevent_context *samba_tevent_context_init(TALLOC_CTX *mem_ctx);

/*
 * if same samba code needs to use a specific tevent backend
 * it can use something like this:
 *
 * samba_tevent_set_debug(ev, "pysmb_tevent");
 */
void samba_tevent_set_debug(struct tevent_context *ev, const char *name);

#endif /* _SAMBA_UTIL_H_ */
