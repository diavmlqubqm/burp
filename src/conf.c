#include "include.h"
#include "client/glob_windows.h"

struct conf *conf_alloc(void)
{
	return (struct conf *)calloc_w(1, sizeof(struct conf), __func__);
}

/* Init only stuff related to includes/excludes.
   This is so that the server can override them all on the client. */
// FIX THIS: Maybe have this as a substructure of a struct conf.
// Could then just memset them all to zero here.
static void init_incexcs(struct conf *c)
{
	c->startdir=NULL;
	c->incexcdir=NULL;
	c->fschgdir=NULL;
	c->nobackup=NULL;
	c->incext=NULL; // include extensions
	c->excext=NULL; // exclude extensions
	c->increg=NULL; // include (regular expression)
	c->excreg=NULL; // include (regular expression)
	c->excfs=NULL; // exclude filesystems
	c->excom=NULL; // exclude from compression
	c->incglob=NULL; // exclude from compression
	c->fifos=NULL;
	c->blockdevs=NULL;
	c->split_vss=0;
	c->strip_vss=0;
	c->vss_drives=NULL;
	c->atime=0;
	/* stuff to do with restore */
	c->overwrite=0;
	c->strip=0;
	c->backup=NULL;
	c->restoreprefix=NULL;
	c->regex=NULL;
}

/* Free only stuff related to includes/excludes.
   This is so that the server can override them all on the client. */
static void free_incexcs(struct conf *c)
{
	strlists_free(&c->startdir);
	strlists_free(&c->incexcdir);
	strlists_free(&c->fschgdir);
	strlists_free(&c->nobackup);
	strlists_free(&c->incext); // include extensions
	strlists_free(&c->excext); // exclude extensions
	strlists_free(&c->increg); // include (regular expression)
	strlists_free(&c->excreg); // exclude (regular expression)
	strlists_free(&c->excfs); // exclude filesystems
	strlists_free(&c->excom); // exclude from compression
	strlists_free(&c->incglob); // include (glob)
	strlists_free(&c->fifos);
	strlists_free(&c->blockdevs);
	free_w(&c->backup);
	free_w(&c->restoreprefix);
	free_w(&c->regex);
	free_w(&c->vss_drives);
	init_incexcs(c);
}

void conf_init(struct conf *c)
{
	// Set everything to 0.
	memset(c, 0, sizeof(struct conf));

	// Turn on defaults that are non-zero.
	c->forking=1;
	c->daemon=1;
	c->directory_tree=1;
	c->password_check=1;
	c->log_to_stdout=1;
	c->network_timeout=60*60*2; // two hours
	// ext3 maximum number of subdirs is 32000, so leave a little room.
	c->max_storage_subdirs=30000;
	c->librsync=1;
	c->compression=9;
	c->ssl_compression=5;
	c->version_warn=1;
	c->path_length_warn=1;
	c->umask=0022;
	c->max_hardlinks=10000;

	c->client_can|=CLIENT_CAN_DELETE;
	c->client_can|=CLIENT_CAN_DIFF;
	c->client_can|=CLIENT_CAN_FORCE_BACKUP;
	c->client_can|=CLIENT_CAN_LIST;
	c->client_can|=CLIENT_CAN_RESTORE;
	c->client_can|=CLIENT_CAN_VERIFY;

	c->server_can|=SERVER_CAN_RESTORE;

	rconf_init(&c->rconf);
}

void conf_free_content(struct conf *c)
{
	if(!c) return;
	free_w(&c->address);
	free_w(&c->port);
	free_w(&c->conffile);
	free_w(&c->clientconfdir);
	free_w(&c->cname);
	free_w(&c->peer_version);
	free_w(&c->directory);
	free_w(&c->timestamp_format);
	free_w(&c->ca_conf);
	free_w(&c->ca_name);
	free_w(&c->ca_server_name);
	free_w(&c->ca_burp_ca);
	free_w(&c->ca_csr_dir);
	free_w(&c->lockfile);
	free_w(&c->password);
	free_w(&c->passwd);
	free_w(&c->server);
 	free_w(&c->recovery_method);
 	free_w(&c->ssl_cert_ca);
	free_w(&c->ssl_cert);
	free_w(&c->ssl_key);
	free_w(&c->ssl_key_password);
	free_w(&c->ssl_ciphers);
	free_w(&c->ssl_dhfile);
	free_w(&c->ssl_peer_cn);
	free_w(&c->user);
	free_w(&c->group);
	free_w(&c->encryption_password);
	free_w(&c->client_lockdir);
	free_w(&c->autoupgrade_dir);
	free_w(&c->autoupgrade_os);
	free_w(&c->manual_delete);
	free_w(&c->monitor_logfile);

	free_w(&c->timer_script);
	strlists_free(&c->timer_arg);

	free_w(&c->n_success_script);
	strlists_free(&c->n_success_arg);

	free_w(&c->n_failure_script);
	strlists_free(&c->n_failure_arg);

	strlists_free(&c->rclients);

	free_w(&c->b_script_pre);
	strlists_free(&c->b_script_pre_arg);
	free_w(&c->b_script_post);
	strlists_free(&c->b_script_post_arg);
	free_w(&c->r_script_pre);
	strlists_free(&c->r_script_pre_arg);
	free_w(&c->r_script_post);
	strlists_free(&c->r_script_post_arg);

	free_w(&c->s_script_pre);
	strlists_free(&c->s_script_pre_arg);
	free_w(&c->s_script_post);
	strlists_free(&c->s_script_post_arg);

	free_w(&c->b_script);
	free_w(&c->r_script);
	strlists_free(&c->b_script_arg);
	strlists_free(&c->r_script_arg);

	free_w(&c->s_script);
	strlists_free(&c->s_script_arg);

	strlists_free(&c->keep);

	free_w(&c->dedup_group);
	free_w(&c->browsefile);
	free_w(&c->browsedir);
	free_w(&c->restore_spool);
	free_w(&c->restore_client);
	free_w(&c->restore_path);
	free_w(&c->orig_client);

	free_incexcs(c);
}

void conf_free(struct conf *c)
{
	if(!c) return;
	conf_free_content(c);
	free(c);
}

// Get configuration value.
static int gcv(const char *f, const char *v, const char *want, char **dest)
{
	if(strcmp(f, want)) return 0;
	free_w(dest);
	if(!(*dest=strdup_w(v, __func__))) return -1;
	return 0;
}

// Get configuration value integer.
static void gcv_int(const char *f, const char *v, const char *want, int *dest)
{
	if(!strcmp(f, want)) *dest=atoi(v);
}

// Get configuration value 8 bit integer.
static void gcv_uint8(const char *f, const char *v,
	const char *want, uint8_t *dest)
{
	if(!strcmp(f, want)) *dest=(uint8_t)atoi(v);
}

static void gcv_bit(const char *f, const char *v,
	const char *want, uint8_t *dest, uint8_t bit)
{
	if(strcmp(f, want)) return;
	// Clear the bit.
	(*dest)&=~bit;
	if(!(uint8_t)atoi(v)) return;
	// Set the bit.
	(*dest)|=bit;
}

// This will strip off everything after the last quote. So, configs like this
// should work:
// exclude_regex = "[A-Z]:/pagefile.sys" # swap file (Windows XP, 7, 8)
// Return 1 for quotes removed, -1 for error, 0 for OK.
static int remove_quotes(const char *f, char **v, char quote)
{
	char *dp=NULL;
	char *sp=NULL;
	char *copy=NULL;

	// If it does not start with a quote, leave it alone.
	if(**v!=quote) return 0;

	if(!(copy=strdup_w(*v, __func__)))
		return -1;

	for(dp=*v, sp=copy+1; *sp; sp++)
	{
		if(*sp==quote)
		{
			// Found a matching quote. Stop here.
			*dp='\0';
			for(sp++; *sp && isspace(*sp); sp++) { }
			// Do not complain about trailing comments.
			if(*sp && *sp!='#')
				logp("ignoring trailing characters after quote in config '%s = %s'\n", f, copy);
			return 1;
		}
		else if(*sp=='\\')
		{
			sp++;
			*dp=*sp;
			dp++;
			if(*sp!=quote
			  && *sp!='\\')
				logp("unknown escape sequence '\\%c' in config '%s = %s' - treating it as '%c'\n", *sp, f, copy, *sp);
		}
		else
		{
			*dp=*sp;
			dp++;
		}
	}
	logp("Did not find closing quote in config '%s = %s'\n", f, copy);
	*dp='\0';
	return 1;
}

// Get field and value pair.
int conf_get_pair(char buf[], char **f, char **v)
{
	char *cp=NULL;
	char *eq=NULL;

	// strip leading space
	for(cp=buf; *cp && isspace(*cp); cp++) { }
	if(!*cp || *cp=='#')
	{
		*f=NULL;
		*v=NULL;
		return 0;
	}
	*f=cp;
	if(!(eq=strchr(*f, '='))) return -1;
	*eq='\0';

	// Strip white space from before the equals sign.
	for(cp=eq-1; *cp && isspace(*cp); cp--) *cp='\0';
	// Skip white space after the equals sign.
	for(cp=eq+1; *cp && isspace(*cp); cp++) { }
	*v=cp;
	// Strip white space at the end of the line.
	for(cp+=strlen(cp)-1; *cp && isspace(*cp); cp--) { *cp='\0'; }

	// FIX THIS: Make this more sophisticated - it should understand
	// escapes, for example.

	switch(remove_quotes(*f, v, '\''))
	{
		case -1: return -1;
		case 1: break;
		default:
			// If single quotes were not removed, try to remove
			// double quotes.
			if(remove_quotes(*f, v, '\"')<0) return -1;
			break;
	}

	if(!*f || !**f || !*v || !**v) return -1;

	return 0;
}

// Get configuration value args.
static int do_gcv_a(const char *f, const char *v,
	const char *opt, struct strlist **list, int include, int sorted)
{
	char *tmp=NULL;
	if(gcv(f, v, opt, &tmp)) return -1;
	if(!tmp) return 0;
	if(sorted)
	{
		if(strlist_add_sorted(list, tmp, include)) return -1;
	}
	else
	{
		if(strlist_add(list, tmp, include)) return -1;
	}
	free(tmp);
	return 0;
}

// Get configuration value args (unsorted).
static int gcv_a(const char *f, const char *v,
	const char *opt, struct strlist **list, int include)
{
	return do_gcv_a(f, v, opt, list, include, 0);
}

// Get configuration value args (sorted).
static int gcv_a_sort(const char *f, const char *v,
	const char *opt, struct strlist **list, int include)
{
	return do_gcv_a(f, v, opt, list, include, 1);
}

/* Windows users have a nasty habit of putting in backslashes. Convert them. */
#ifdef HAVE_WIN32
void convert_backslashes(char **path)
{
	char *p=NULL;
	for(p=*path; *p; p++) if(*p=='\\') *p='/';
}
#endif

static int path_checks(const char *path, const char *err_msg)
{
	const char *p=NULL;
	for(p=path; *p; p++)
	{
		if(*p!='.' || *(p+1)!='.') continue;
		if((p==path || *(p-1)=='/') && (*(p+2)=='/' || !*(p+2)))
		{
			logp(err_msg);
			return -1;
		}
	}
// This is being run on the server too, where you can enter paths for the
// clients, so need to allow windows style paths for windows and unix.
	if((!isalpha(*path) || *(path+1)!=':')
#ifndef HAVE_WIN32
	  // Windows does not need to check for unix style paths.
	  && *path!='/'
#endif
	)
	{
		logp(err_msg);
		return -1;
	}
	return 0;
}

static int conf_error(const char *conf_path, int line)
{
	logp("%s: parse error on line %d\n", conf_path, line);
	return -1;
}

static int get_file_size(const char *v, ssize_t *dest, const char *conf_path, int line)
{
	// Store in bytes, allow k/m/g.
	const char *cp=NULL;
	*dest=strtoul(v, NULL, 10);
	for(cp=v; *cp && (isspace(*cp) || isdigit(*cp)); cp++) { }
	if(tolower(*cp)=='k') *dest*=1024;
	else if(tolower(*cp)=='m') *dest*=1024*1024;
	else if(tolower(*cp)=='g') *dest*=1024*1024*1024;
	else if(!*cp || *cp=='b')
	{ }
	else
	{
		logp("Unknown file size type '%s' - please use b/kb/mb/gb\n",
			cp);
		return conf_error(conf_path, line);
	}
	return 0;
}

int conf_val_reset(const char *src, char **dest)
{
	if(!src) return 0;
	free_w(dest);
	if(!(*dest=strdup_w(src, __func__))) return -1;
	return 0;
}

static int pre_post_override(char **override, char **pre, char **post)
{
	if(!override || !*override) return 0;
	if(conf_val_reset(*override, pre)
	  || conf_val_reset(*override, post))
		return -1;
	free_w(override);
	return 0;
}

#ifdef HAVE_LINUX_OS
struct fstype
{
	const char *str;
	uint64_t flag;
};

static struct fstype fstypes[]={
	{ "debugfs",		0x64626720 },
	{ "devfs",		0x00001373 },
	{ "devpts",		0x00001CD1 },
	{ "devtmpfs",		0x00009FA0 },
	{ "ext2",		0x0000EF53 },
	{ "ext3",		0x0000EF53 },
	{ "ext4",		0x0000EF53 },
	{ "iso9660",		0x00009660 },
	{ "jfs",		0x3153464A },
	{ "nfs",		0x00006969 },
	{ "ntfs",		0x5346544E },
	{ "proc",		0x00009fa0 },
	{ "reiserfs",		0x52654973 },
	{ "securityfs",		0x73636673 },
	{ "sysfs",		0x62656572 },
	{ "smbfs",		0x0000517B },
	{ "usbdevfs",		0x00009fa2 },
	{ "xfs",		0x58465342 },
	{ "ramfs",		0x858458f6 },
	{ "romfs",		0x00007275 },
	{ "tmpfs",		0x01021994 },
	{ NULL,			0 },
};
/* Use this C code to figure out what f_type gets set to.
#include <stdio.h>
#include <sys/vfs.h>

int main(int argc, char *argv[])
{
	int i=0;
	struct statfs buf;
	if(argc<1)
	{
		printf("not enough args\n");
		return -1;
	}
	if(statfs(argv[1], &buf))
	{
		printf("error\n");
		return -1;
	}
	printf("0x%08X\n", buf.f_type);
	return 0;
}
*/

#endif

static int fstype_to_flag(const char *fstype, long *flag)
{
#ifdef HAVE_LINUX_OS
	int i=0;
	for(i=0; fstypes[i].str; i++)
	{
		if(!strcmp(fstypes[i].str, fstype))
		{
			*flag=fstypes[i].flag;
			return 0;
		}
	}
#else
	return 0;
#endif
	return -1;
}

static int load_conf_ints(struct conf *c,
	const char *f, // field
	const char *v) //value
{
	gcv_uint8(f, v, "syslog", &(c->log_to_syslog));
	gcv_uint8(f, v, "stdout", &(c->log_to_stdout));
	gcv_uint8(f, v, "progress_counter", &(c->progress_counter));
	gcv_uint8(f, v, "hardlinked_archive", &(c->hardlinked_archive));
	gcv_int(f, v, "max_hardlinks", &(c->max_hardlinks));
	gcv_uint8(f, v, "librsync", &(c->librsync));
	gcv_uint8(f, v, "version_warn", &(c->version_warn));
	gcv_uint8(f, v, "path_length_warn", &(c->path_length_warn));
	gcv_uint8(f, v, "cross_all_filesystems", &(c->cross_all_filesystems));
	gcv_uint8(f, v, "read_all_fifos", &(c->read_all_fifos));
	gcv_uint8(f, v, "read_all_blockdevs", &(c->read_all_blockdevs));
	gcv_uint8(f, v, "backup_script_post_run_on_fail",
					&(c->b_script_post_run_on_fail));
	gcv_uint8(f, v, "server_script_post_run_on_fail",
					&(c->s_script_post_run_on_fail));
	gcv_uint8(f, v, "server_script_pre_notify",
					&(c->s_script_pre_notify));
	gcv_uint8(f, v, "server_script_post_notify",
					&(c->s_script_post_notify));
	gcv_uint8(f, v, "server_script_notify", &(c->s_script_notify));
	gcv_uint8(f, v, "notify_success_warnings_only",
					&(c->n_success_warnings_only));
	gcv_uint8(f, v, "notify_success_changes_only",
					&(c->n_success_changes_only));
	gcv_int(f, v, "network_timeout", &(c->network_timeout));
	gcv_int(f, v, "max_children", &(c->max_children));
	gcv_int(f, v, "max_status_children", &(c->max_status_children));
	gcv_int(f, v, "max_storage_subdirs", &(c->max_storage_subdirs));
	gcv_uint8(f, v, "overwrite", &(c->overwrite));
	gcv_uint8(f, v, "split_vss", &(c->split_vss));
	gcv_uint8(f, v, "strip_vss", &(c->strip_vss));
	gcv_uint8(f, v, "atime", &(c->atime));
	gcv_int(f, v, "strip", &(c->strip));
	gcv_int(f, v, "randomise", &(c->randomise));
	gcv_uint8(f, v, "fork", &(c->forking));
	gcv_uint8(f, v, "daemon", &(c->daemon));
	gcv_uint8(f, v, "directory_tree", &(c->directory_tree));
	gcv_uint8(f, v, "password_check", &(c->password_check));
	gcv_uint8(f, v, "monitor_browse_cache", &(c->monitor_browse_cache));

	gcv_bit(f, v, "client_can_delete",
		&(c->client_can), CLIENT_CAN_DELETE);
	gcv_bit(f, v, "client_can_diff",
		&(c->client_can), CLIENT_CAN_DIFF);
	gcv_bit(f, v, "client_can_force_backup",
		&(c->client_can), CLIENT_CAN_FORCE_BACKUP);
	gcv_bit(f, v, "client_can_list",
		&(c->client_can), CLIENT_CAN_LIST);
	gcv_bit(f, v, "client_can_restore",
		&(c->client_can), CLIENT_CAN_RESTORE);
	gcv_bit(f, v, "client_can_verify",
		&(c->client_can), CLIENT_CAN_VERIFY);
	gcv_bit(f, v, "server_can_restore",
		&(c->server_can), SERVER_CAN_RESTORE);

	return 0;
}

static int load_conf_strings(struct conf *c,
	const char *f, // field
	const char *v  // value
	)
{
	if(  gcv(f, v, "address", &(c->address))
	  || gcv(f, v, "port", &(c->port))
	  || gcv(f, v, "status_address", &(c->status_address))
	  || gcv(f, v, "status_port", &(c->status_port))
	  || gcv(f, v, "ssl_cert_ca", &(c->ssl_cert_ca))
	  || gcv(f, v, "ssl_cert", &(c->ssl_cert))
	  || gcv(f, v, "ssl_key", &(c->ssl_key))
	// ssl_cert_password is a synonym for ssl_key_password
	  || gcv(f, v, "ssl_cert_password", &(c->ssl_key_password))
	  || gcv(f, v, "ssl_key_password", &(c->ssl_key_password))
	  || gcv(f, v, "ssl_dhfile", &(c->ssl_dhfile))
	  || gcv(f, v, "ssl_peer_cn", &(c->ssl_peer_cn))
	  || gcv(f, v, "ssl_ciphers", &(c->ssl_ciphers))
	  || gcv(f, v, "clientconfdir", &(c->clientconfdir))
	  || gcv(f, v, "cname", &(c->cname))
	  || gcv(f, v, "directory", &(c->directory))
	  || gcv(f, v, "timestamp_format", &(c->timestamp_format))
	  || gcv(f, v, "ca_conf", &(c->ca_conf))
	  || gcv(f, v, "ca_name", &(c->ca_name))
	  || gcv(f, v, "ca_server_name", &(c->ca_server_name))
	  || gcv(f, v, "ca_burp_ca", &(c->ca_burp_ca))
	  || gcv(f, v, "ca_csr_dir", &(c->ca_csr_dir))
	  || gcv(f, v, "backup", &(c->backup))
	  || gcv(f, v, "restoreprefix", &(c->restoreprefix))
	  || gcv(f, v, "regex", &(c->regex))
	  || gcv(f, v, "vss_drives", &(c->vss_drives))
	  || gcv(f, v, "browsedir", &(c->browsedir))
	  || gcv(f, v, "browsefile", &(c->browsefile))
	  || gcv(f, v, "manual_delete", &(c->manual_delete))
	  || gcv(f, v, "restore_spool", &(c->restore_spool))
	  || gcv(f, v, "working_dir_recovery_method", &(c->recovery_method))
	  || gcv(f, v, "autoupgrade_dir", &(c->autoupgrade_dir))
	  || gcv(f, v, "autoupgrade_os", &(c->autoupgrade_os))
	  || gcv(f, v, "lockfile", &(c->lockfile))
	// "pidfile" is a synonym for "lockfile".
	  || gcv(f, v, "pidfile", &(c->lockfile))
	  || gcv(f, v, "password", &(c->password))
	  || gcv(f, v, "passwd", &(c->passwd))
	  || gcv(f, v, "server", &(c->server))
	  || gcv(f, v, "user", &(c->user))
	  || gcv(f, v, "group", &(c->group))
	  || gcv(f, v, "client_lockdir", &(c->client_lockdir))
	  || gcv(f, v, "encryption_password", &(c->encryption_password))
	  || gcv_a(f, v, "keep", &c->keep, 1)
	  || gcv_a_sort(f, v, "include", &c->incexcdir, 1)
	  || gcv_a_sort(f, v, "exclude", &c->incexcdir, 0)
	  || gcv_a_sort(f, v, "cross_filesystem", &c->fschgdir, 0)
	  || gcv_a_sort(f, v, "nobackup", &c->nobackup, 0)
	  || gcv_a_sort(f, v, "read_fifo", &c->fifos, 0)
	  || gcv_a_sort(f, v, "read_blockdev", &c->blockdevs, 0)
	  || gcv_a_sort(f, v, "include_ext", &c->incext, 0)
	  || gcv_a_sort(f, v, "exclude_ext", &c->excext, 0)
	  || gcv_a_sort(f, v, "include_regex", &c->increg, 0)
	  || gcv_a_sort(f, v, "exclude_regex", &c->excreg, 0)
	  || gcv_a_sort(f, v, "include_glob", &c->incglob, 0)
	  || gcv_a_sort(f, v, "exclude_fs", &c->excfs, 0)
	  || gcv_a_sort(f, v, "exclude_comp", &c->excom, 0)
	  || gcv(f, v, "timer_script", &(c->timer_script))
	  || gcv_a(f, v, "timer_arg", &(c->timer_arg), 0)
	  || gcv(f, v, "notify_success_script", &(c->n_success_script))
	  || gcv_a(f, v, "notify_success_arg", &(c->n_success_arg), 0)
	  || gcv(f, v, "notify_failure_script", &(c->n_failure_script))
	  || gcv_a(f, v, "notify_failure_arg", &(c->n_failure_arg), 0)
	  || gcv(f, v, "backup_script_pre", &(c->b_script_pre))
	  || gcv_a(f, v, "backup_script_pre_arg", &(c->b_script_pre_arg), 0)
	  || gcv(f, v, "backup_script_post", &(c->b_script_post))
	  || gcv_a(f, v, "backup_script_post_arg", &(c->b_script_post_arg), 0)
	  || gcv(f, v, "restore_script_pre", &(c->r_script_pre))
	  || gcv_a(f, v, "restore_script_pre_arg", &(c->r_script_pre_arg), 0)
	  || gcv(f, v, "restore_script_post", &(c->r_script_post))
	  || gcv_a(f, v, "restore_script_post_arg", &(c->r_script_post_arg), 0)
	  || gcv(f, v, "server_script_pre", &(c->s_script_pre))
	  || gcv_a(f, v, "server_script_pre_arg", &(c->s_script_pre_arg), 0)
	  || gcv(f, v, "server_script_post", &(c->s_script_post))
	  || gcv_a(f, v, "server_script_post_arg", &(c->s_script_post_arg), 0)
	  || gcv(f, v, "backup_script", &(c->b_script))
	  || gcv_a(f, v, "backup_script_arg", &(c->b_script_arg), 0)
	  || gcv(f, v, "restore_script", &(c->r_script))
	  || gcv_a(f, v, "restore_script_arg", &(c->r_script_arg), 0)
	  || gcv(f, v, "server_script", &(c->s_script))
	  || gcv_a(f, v, "server_script_arg", &(c->s_script_arg), 0)
	  || gcv_a_sort(f, v, "restore_client", &(c->rclients), 0)
	  || gcv(f, v, "dedup_group", &(c->dedup_group))
	  || gcv(f, v, "orig_client", &(c->orig_client)))
		return -1;

	return 0;
}

static int get_compression(const char *v)
{
	const char *cp=v;
	if(!strncmp(v, "gzip", strlen("gzip"))
	  || !(strncmp(v, "zlib", strlen("zlib"))))
		cp=v+strlen("gzip"); // Or "zlib".
	if(strlen(cp)==1 && isdigit(*cp))
		return atoi(cp);
	return -1;
}

static int load_conf_field_and_value(struct conf *c,
	const char *f, // field
	const char *v, // value
	const char *conf_path,
	int line)
{
	if(!strcmp(f, "mode"))
	{
		if(!strcmp(v, "server"))
		{
			c->mode=MODE_SERVER;
			c->progress_counter=0; // default to off for server
		}
		else if(!strcmp(v, "client"))
		{
			c->mode=MODE_CLIENT;
			c->progress_counter=1; // default to on for client
		}
		else return -1;
	}
	else if(!strcmp(f, "protocol"))
	{
		if(!strcmp(v, "0")) c->protocol=PROTO_AUTO;
		else if(!strcmp(v, "1")) c->protocol=PROTO_BURP1;
		else if(!strcmp(v, "2")) c->protocol=PROTO_BURP2;
		else return -1;
	}
	else if(!strcmp(f, "compression"))
	{
		if((c->compression=get_compression(v))<0)
			return -1;
	}
	else if(!strcmp(f, "ssl_compression"))
	{
		if((c->ssl_compression=get_compression(v))<0)
			return -1;
	}
	else if(!strcmp(f, "umask"))
	{
		c->umask=strtol(v, NULL, 8);
	}
	else if(!strcmp(f, "ratelimit"))
	{
		float f=0;
		f=atof(v);
		// User is specifying Mega bits per second.
		// Need to convert to bytes per second.
		f=(f*1024*1024)/8;
		if(!f)
		{
			logp("ratelimit should be greater than zero\n");
			return -1;
		}
		c->ratelimit=f;
	}
	else if(!strcmp(f, "min_file_size"))
	{
		if(get_file_size(v, &(c->min_file_size), conf_path, line))
			return -1;
	}
	else if(!strcmp(f, "max_file_size"))
	{
		if(get_file_size(v, &(c->max_file_size),
			conf_path, line)) return -1;
	}
	else if(!strcmp(f, "hard_quota"))
	{
		if(get_file_size(v, &(c->hard_quota),
			conf_path, line)) return -1;
	}
	else if(!strcmp(f, "soft_quota"))
	{
		if(get_file_size(v, &(c->soft_quota),
			conf_path, line)) return -1;
	}
	else
	{
		if(load_conf_ints(c, f, v)
		  || load_conf_strings(c, f, v))
			return -1;
	}
	return 0;
}

// Recursing, so need to define load_conf_lines ahead of parse_conf_line.
static int load_conf_lines(const char *conf_path, struct conf *c);

static int parse_conf_line(struct conf *c, const char *conf_path,
	char buf[], int line)
{
	char *f=NULL; // field
	char *v=NULL; // value

	if(!strncmp(buf, ". ", 2))
	{
		// The conf file specifies another file to include.
		char *np=NULL;
		char *extrafile=NULL;

		if(!(extrafile=strdup_w(buf+2, __func__))) return -1;

		if((np=strrchr(extrafile, '\n'))) *np='\0';
		if(!*extrafile)
		{
			free_w(&extrafile);
			return -1;
		}

#ifdef HAVE_WIN32
		if(strlen(extrafile)>2
		  && extrafile[1]!=':')
#else
		if(*extrafile!='/')
#endif
		{
			// It is relative to the directory that the
			// current conf file is in.
			char *cp=NULL;
			char *copy=NULL;
			char *tmp=NULL;
			if(!(copy=strdup_w(conf_path, __func__)))
			{
				free_w(&extrafile);
				return -1;
			}
			if((cp=strrchr(copy, '/'))) *cp='\0';
			if(!(tmp=prepend_s(copy, extrafile)))
			{
				log_out_of_memory(__func__);
				free_w(&extrafile);
				free_w(&copy);
			}
			free_w(&extrafile);
			free_w(&copy);
			extrafile=tmp;
		}

		if(load_conf_lines(extrafile, c))
		{
			free_w(&extrafile);
			return -1;
		}
		free_w(&extrafile);
		return 0;
	}

	if(conf_get_pair(buf, &f, &v)) return -1;
	if(!f || !v) return 0;

	if(load_conf_field_and_value(c, f, v, conf_path, line))
		return -1;
	return 0;
}

static void conf_problem(const char *conf_path, const char *msg, int *r)
{
	logp("%s: %s\n", conf_path, msg);
	(*r)--;
}

#ifdef HAVE_IPV6
// These should work for IPv4 connections too.
#define DEFAULT_ADDRESS_MAIN	"::"
#define DEFAULT_ADDRESS_STATUS	"::1"
#else
// Fall back to IPv4 address if IPv6 is not compiled in.
#define DEFAULT_ADDRESS_MAIN	"0.0.0.0"
#define DEFAULT_ADDRESS_STATUS	"127.0.0.1"
#endif

static int server_conf_checks(struct conf *c, const char *path, int *r)
{
	if(!c->address
	  && !(c->address=strdup_w(DEFAULT_ADDRESS_MAIN, __func__)))
			return -1;
	if(!c->directory)
		conf_problem(path, "directory unset", r);
	if(!c->dedup_group)
		conf_problem(path, "dedup_group unset", r);
	if(!c->clientconfdir)
		conf_problem(path, "clientconfdir unset", r);
	if(!c->recovery_method
	  || (strcmp(c->recovery_method, "delete")
	   && strcmp(c->recovery_method, "resume")
	   && strcmp(c->recovery_method, "use")))
		conf_problem(path, "unknown working_dir_recovery_method", r);
	if(!c->ssl_cert)
		conf_problem(path, "ssl_cert unset", r);
	if(!c->ssl_cert_ca)
		conf_problem(path, "ssl_cert_ca unset", r);
	if(!c->ssl_dhfile)
		conf_problem(path, "ssl_dhfile unset", r);
	if(c->encryption_password)
		conf_problem(path,
		  "encryption_password should not be set on the server!", r);
	if(!c->status_address
	  && !(c->status_address=strdup_w(DEFAULT_ADDRESS_STATUS, __func__)))
			return -1;
	if(!c->status_port) // carry on if not set.
		logp("%s: status_port unset", path);
	if(!c->max_children)
	{
		logp("%s: max_children unset - using 5\n", path);
		c->max_children=5;
	}
	if(!c->max_status_children)
	{
		logp("%s: max_status_children unset - using 5\n", path);
		c->max_status_children=5;
	}
	if(!c->keep)
		conf_problem(path, "keep unset", r);
	if(c->max_hardlinks<2)
		conf_problem(path, "max_hardlinks too low", r);
	if(c->max_children<=0)
		conf_problem(path, "max_children too low", r);
	if(c->max_status_children<=0)
		conf_problem(path, "max_status_children too low", r);
	if(c->max_storage_subdirs<=1000)
		conf_problem(path, "max_storage_subdirs too low", r);
	if(c->ca_conf)
	{
		int ca_err=0;
		if(!c->ca_name)
		{
			logp("ca_conf set, but ca_name not set\n");
			ca_err++;
		}
		if(!c->ca_server_name)
		{
			logp("ca_conf set, but ca_server_name not set\n");
			ca_err++;
		}
		if(!c->ca_burp_ca)
		{
			logp("ca_conf set, but ca_burp_ca not set\n");
			ca_err++;
		}
		if(!c->ssl_dhfile)
		{
			logp("ca_conf set, but ssl_dhfile not set\n");
			ca_err++;
		}
		if(!c->ssl_cert_ca)
		{
			logp("ca_conf set, but ssl_cert_ca not set\n");
			ca_err++;
		}
		if(!c->ssl_cert)
		{
			logp("ca_conf set, but ssl_cert not set\n");
			ca_err++;
		}
		if(!c->ssl_key)
		{
			logp("ca_conf set, but ssl_key not set\n");
			ca_err++;
		}
		if(ca_err) return -1;
	}
	if(c->manual_delete)
	{
		if(path_checks(c->manual_delete,
			"ERROR: Please use an absolute manual_delete path.\n"))
				return -1;
	}

	return 0;
}

#ifdef HAVE_WIN32
#undef X509_NAME
#include <openssl/x509.h>
#endif

static char *extract_cn(X509_NAME *subj)
{
	int nid;
	int index;
	ASN1_STRING *d;
	X509_NAME_ENTRY *e;
	unsigned char *str;

	nid=OBJ_txt2nid("CN");
	if((index=X509_NAME_get_index_by_NID(subj, nid, -1))<0
	  || !(e=X509_NAME_get_entry(subj, index))
	  || !(d=X509_NAME_ENTRY_get_data(e))
	  || !(str=ASN1_STRING_data(d)))
		return NULL;
	return strdup_w((char *)str, __func__);
}

static int get_cname_from_ssl_cert(struct conf *c)
{
	int ret=-1;
	FILE *fp=NULL;
	X509 *cert=NULL;
	X509_NAME *subj=NULL;
	char *path=c->ssl_cert;

	if(!path || !(fp=open_file(path, "rb"))) return 0;

	if(!(cert=PEM_read_X509(fp, NULL, NULL, NULL)))
	{
		logp("unable to parse %s in: %s\n", path, __func__);
		goto end;
	}
	if(!(subj=X509_get_subject_name(cert)))
	{
		logp("unable to get subject from %s in: %s\n", path, __func__);
		goto end;
	}

	if(!(c->cname=extract_cn(subj)))
	{
		logp("could not get CN from %s\n", path);
		goto end;
	}
	logp("cname from cert: %s\n", c->cname);

	ret=0;
end:
	if(cert) X509_free(cert);
	if(fp) fclose(fp);
	return ret;
}

#ifdef HAVE_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

static int get_fqdn(struct conf *c)
{
	int ret=-1;
	int gai_result;
	struct addrinfo hints;
	struct addrinfo *info;
	char hostname[1024]="";
	hostname[1023] = '\0';
	if(gethostname(hostname, 1023))
	{
		logp("gethostname() failed: %s\n", strerror(errno));
		goto end;
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_flags=AI_CANONNAME;

	if((gai_result=getaddrinfo(hostname, "http", &hints, &info)))
	{
		logp("getaddrinfo in %s: %s\n", __func__,
			gai_strerror(gai_result));
		goto end;
	}

	//for(p=info; p; p=p->ai_next)
	// Just use the first one.
	if(!info)
	{
		logp("Got no hostname in %s\n", __func__);
		goto end;
	}

	if(!(c->cname=strdup_w(info->ai_canonname, __func__)))
		goto end;
	logp("cname from hostname: %s\n", c->cname);

	ret=0;
end:
	freeaddrinfo(info);
	return ret;
}

static int client_conf_checks(struct conf *c, const char *path, int *r)
{
	if(!c->cname)
	{
		if(get_cname_from_ssl_cert(c)) return -1;
		// There was no error. This is probably a new install.
		// Try getting the fqdn and using that.
		if(!c->cname)
		{
			if(get_fqdn(c)) return -1;
			if(!c->cname)
				conf_problem(path, "client name unset", r);
		}
	}
	if(!c->password)
	{
		logp("password not set, falling back to \"password\"\n");
		if(!(c->password=strdup_w("password", __func__)))
			return -1;
	}
	if(!c->server)
		conf_problem(path, "server unset", r);
	if(!c->status_port) // carry on if not set.
		logp("%s: status_port unset\n", path);
	if(!c->ssl_cert)
		conf_problem(path, "ssl_cert unset", r);
	if(!c->ssl_cert_ca)
		conf_problem(path, "ssl_cert_ca unset", r);
	if(!c->ssl_peer_cn)
	{
		logp("ssl_peer_cn unset\n");
		if(c->server)
		{
			logp("falling back to '%s'\n", c->server);
			if(!(c->ssl_peer_cn=strdup_w(c->server, __func__)))
				return -1;
		}
	}
	if(!c->lockfile)
		conf_problem(path, "lockfile unset", r);
	if(c->autoupgrade_os
	  && strstr(c->autoupgrade_os, ".."))
		conf_problem(path,
			"autoupgrade_os must not contain a '..' component", r);
	if(c->ca_burp_ca)
	{
		if(!c->ca_csr_dir)
			conf_problem(path,
				"ca_burp_ca set, but ca_csr_dir not set\n", r);
		if(!c->ssl_cert_ca)
			conf_problem(path,
				"ca_burp_ca set, but ssl_cert_ca not set\n", r);
		if(!c->ssl_cert)
			conf_problem(path,
				"ca_burp_ca set, but ssl_cert not set\n", r);
		if(!c->ssl_key)
			conf_problem(path,
				"ca_burp_ca set, but ssl_key not set\n", r);
	}

	if(!r)
	{
		struct strlist *l;
		logp("Listing configured paths:\n");
		for(l=c->incexcdir; l; l=l->next)
			logp("%s: %s\n", l->flag?"include":"exclude", l->path);
		logp("Listing starting paths:\n");
		for(l=c->startdir; l; l=l->next)
			if(l->flag) logp("%s\n", l->path);
	}
	return 0;
}

static int finalise_keep_args(struct conf *c)
{
	struct strlist *k;
	struct strlist *last=NULL;
	unsigned long long mult=1;
	for(k=c->keep; k; k=k->next)
	{
		if(!(k->flag=atoi(k->path)))
		{
			logp("'keep' value cannot be set to '%s'\n", k->path);
			return -1;
		}
		mult*=k->flag;

		// An error if you try to keep backups every second
		// for 100 years.
		if(mult>52560000)
		{
			logp("Your 'keep' values are far too high. High enough to keep a backup every second for 10 years. Please lower them to something sensible.\n");
			return -1;
		}
		last=k;
	}
	// If more than one keep value is set, add one to the last one.
	// This is so that, for example, having set 7, 4, 6, then
	// a backup of age 7*4*6=168 or more is guaranteed to be kept.
	// Otherwise, only 7*4*5=140 would be guaranteed to be kept.
	if(c->keep && c->keep->next) last->flag++;
	return 0;
}

// This decides which directories to start backing up, and which
// are subdirectories which don't need to be started separately.
static int finalise_start_dirs(struct conf *c)
{
	struct strlist *s=NULL;
	struct strlist *last_ie=NULL;
	struct strlist *last_sd=NULL;

	for(s=c->incexcdir; s; s=s->next)
	{
#ifdef HAVE_WIN32
		convert_backslashes(&s->path);
#endif
		if(path_checks(s->path,
			"ERROR: Please use absolute include/exclude paths.\n"))
				return -1;
		
		if(!s->flag) continue; // an exclude

		// Ensure that we do not backup the same directory twice.
		if(last_ie && !strcmp(s->path, last_ie->path))
		{
			logp("Directory appears twice in conf: %s\n",
				s->path);
			return -1;
		}
		// If it is not a subdirectory of the most recent start point,
		// we have found another start point.
		if(!c->startdir
		  || !is_subdir(last_sd->path, s->path))
		{
			// Do not use strlist_add_sorted, because last_sd is
			// relying on incexcdir already being sorted.
			if(strlist_add(&c->startdir,s->path, s->flag))
				return -1;
			last_sd=s;
		}
		last_ie=s;
	}
	return 0;
}

// The glob stuff should only run on the client side.
static int finalise_glob(struct conf *c)
{
#ifdef HAVE_WIN32
	if(glob_windows(c)) return -1;
#else
	int i;
	glob_t globbuf;
	struct strlist *l;
	struct strlist *last=NULL;
	memset(&globbuf, 0, sizeof(globbuf));
	for(l=c->incglob; l; l=l->next)
	{
		glob(l->path, last?GLOB_APPEND:0, NULL, &globbuf);
		last=l;
	}

	for(i=0; (unsigned int)i<globbuf.gl_pathc; i++)
		strlist_add_sorted(&c->incexcdir, globbuf.gl_pathv[i], 1);

	globfree(&globbuf);
#endif
	return 0;
}

// Set the flag of the first item in a list that looks at extensions to the
// maximum number of characters that need to be checked, plus one. This is for
// a bit of added efficiency.
static void set_max_ext(struct strlist *list)
{
	int max=0;
	struct strlist *l=NULL;
	struct strlist *last=NULL;
	for(l=list; l; l=l->next)
	{
		int s=strlen(l->path);
		if(s>max) max=s;
		last=l;
	}
	if(last) last->flag=max+1;
}

static int finalise_fstypes(struct conf *c)
{
	struct strlist *l;
	// Set the strlist flag for the excluded fstypes
	for(l=c->excfs; l; l=l->next)
	{
		l->flag=0;
		if(!strncasecmp(l->path, "0x", 2))
		{
			l->flag=strtol((l->path)+2, NULL, 16);
			logp("Excluding file system type 0x%08X\n", l->flag);
		}
		else
		{
			if(fstype_to_flag(l->path, &(l->flag)))
			{
				logp("Unknown exclude fs type: %s\n", l->path);
				l->flag=0;
			}
		}
	}
	return 0;
}

/*
static int setup_script_arg_override(struct strlist **list, int count, struct strlist ***prelist, struct strlist ***postlist, int *precount, int *postcount)
{
	int i=0;
	if(!list) return 0;
	strlists_free(*prelist, *precount);
	strlists_free(*postlist, *postcount);
	*precount=0;
	*postcount=0;
	for(i=0; i<count; i++)
	{
		if(strlist_add(prelist, precount,
			list[i]->path, 0)) return -1;
		if(strlist_add(postlist, postcount,
			list[i]->path, 0)) return -1;
	}
	return 0;
}
*/

static int conf_finalise(const char *conf_path, struct conf *c)
{
	if(finalise_fstypes(c)) return -1;

	strlist_compile_regexes(c->increg);
	strlist_compile_regexes(c->excreg);

	set_max_ext(c->incext);
	set_max_ext(c->excext);
	set_max_ext(c->excom);

	if(c->mode==MODE_CLIENT && finalise_glob(c)) return -1;

	if(finalise_start_dirs(c)) return -1;

	if(finalise_keep_args(c)) return -1;

	pre_post_override(&c->b_script, &c->b_script_pre, &c->b_script_post);
	pre_post_override(&c->r_script, &c->r_script_pre, &c->r_script_post);
	pre_post_override(&c->s_script, &c->s_script_pre, &c->s_script_post);
	if(c->s_script_notify)
	{
		c->s_script_pre_notify=c->s_script_notify;
		c->s_script_post_notify=c->s_script_notify;
	}

/* FIX THIS: Need to figure out what this was supposed to do, and make sure
   burp-2 does it too.
	setup_script_arg_override(l->bslist, conf->bscount,
		&(l->bprelist), &(l->bpostlist),
		&(conf->bprecount), &(conf->bpostcount));
	setup_script_arg_override(l->rslist, conf->rscount,
		&(l->rprelist), &(l->rpostlist),
		&(conf->rprecount), &(conf->rpostcount));
	setup_script_arg_override(conf->server_script_arg, conf->sscount,
		&(l->sprelist), &(l->spostlist),
		&(conf->sprecount), &(conf->spostcount));
*/
	return 0;
}

static int conf_finalise_global_only(const char *conf_path, struct conf *c)
{
	int r=0;

	if(!c->port) conf_problem(conf_path, "port unset", &r);

	if(rconf_check(&c->rconf)) r--;

	// Let the caller check the 'keep' value.

	if(!c->ssl_key_password
	  && !(c->ssl_key_password=strdup_w("", __func__)))
		r--;

	switch(c->mode)
	{
		case MODE_SERVER:
			if(server_conf_checks(c, conf_path, &r)) r--;
			break;
		case MODE_CLIENT:
			if(client_conf_checks(c, conf_path, &r)) r--;
			break;
		case MODE_UNSET:
		default:
			logp("%s: mode unset - need 'server' or 'client'\n",
				conf_path);
			r--;
			break;
	}

	return r;
}

static int load_conf_lines(const char *conf_path, struct conf *c)
{
	int ret=0;
	int line=0;
	FILE *fp=NULL;
	char buf[4096]="";

	if(!(fp=fopen(conf_path, "r")))
	{
		logp("could not open '%s' for reading.\n", conf_path);
		return -1;
	}
	while(fgets(buf, sizeof(buf), fp))
	{
		line++;
		if(parse_conf_line(c, conf_path, buf, line))
		{
			conf_error(conf_path, line);
			ret=-1;
		}
	}
	if(fp) fclose(fp);
	return ret;
}

/* The client runs this when the server overrides the incexcs. */
int conf_parse_incexcs_buf(struct conf *c, const char *incexc)
{
	int ret=0;
	int line=0;
	char *tok=NULL;
	char *copy=NULL;

	if(!incexc) return 0;
	
	if(!(copy=strdup_w(incexc, __func__))) return -1;
	free_incexcs(c);
	if(!(tok=strtok(copy, "\n")))
	{
		logp("unable to parse server incexc\n");
		free_w(&copy);
		return -1;
	}
	do
	{
		line++;
		if(parse_conf_line(c, "", tok, line))
		{
			ret=-1;
			break;
		}
	} while((tok=strtok(NULL, "\n")));
	free_w(&copy);

	if(ret) return ret;
	return conf_finalise("server override", c);
}

int log_incexcs_buf(const char *incexc)
{
	char *tok=NULL;
	char *copy=NULL;
	if(!incexc || !*incexc) return 0;
	if(!(copy=strdup_w(incexc, __func__)))
		return -1;
	if(!(tok=strtok(copy, "\n")))
	{
		logp("unable to parse server incexc\n");
		free_w(&copy);
		return -1;
	}
	do
	{
		logp("%s\n", tok);
	} while((tok=strtok(NULL, "\n")));
	free_w(&copy);
	return 0;
}

/* The server runs this when parsing a restore file on the server. */
int conf_parse_incexcs_path(struct conf *c, const char *path)
{
	free_incexcs(c);
	if(load_conf_lines(path, c)
	  || conf_finalise(path, c))
		return -1;
	return 0;
}

static int set_global_str(char **dst, const char *src)
{
	if(src && !(*dst=strdup_w(src, __func__)))
		return -1;
	return 0;
}

static int set_global_arglist(struct strlist **dst, struct strlist *src)
{
	struct strlist *s=NULL;
	// Not using strlist_add_sorted, as they should be set in the order
	// that they were first found.
	for(s=src; s; s=s->next)
		if(strlist_add(dst, s->path, s->flag))
			return -1;
	return 0;
}

// Remember to update the list in the man page when you change these.
static int conf_set_from_global(struct conf *globalc, struct conf *cc)
{
	cc->forking=globalc->forking;
	cc->protocol=globalc->protocol;
	cc->log_to_syslog=globalc->log_to_syslog;
	cc->log_to_stdout=globalc->log_to_stdout;
	cc->progress_counter=globalc->progress_counter;
	cc->password_check=globalc->password_check;
	cc->manual_delete=globalc->manual_delete;
	cc->client_can=globalc->client_can;
	cc->server_can=globalc->server_can;
	cc->hardlinked_archive=globalc->hardlinked_archive;
	cc->librsync=globalc->librsync;
	cc->compression=globalc->compression;
	cc->version_warn=globalc->version_warn;
	cc->hard_quota=globalc->hard_quota;
	cc->soft_quota=globalc->soft_quota;
	cc->path_length_warn=globalc->path_length_warn;
	cc->n_success_warnings_only=globalc->n_success_warnings_only;
	cc->n_success_changes_only=globalc->n_success_changes_only;
	cc->s_script_post_run_on_fail=globalc->s_script_post_run_on_fail;
	cc->s_script_pre_notify=globalc->s_script_pre_notify;
	cc->s_script_post_notify=globalc->s_script_post_notify;
	cc->s_script_notify=globalc->s_script_notify;
	cc->directory_tree=globalc->directory_tree;
	cc->monitor_browse_cache=globalc->monitor_browse_cache;
	// clientconfdir needed to make the status monitor stuff work.
	if(set_global_str(&(cc->conffile), globalc->conffile))
		return -1;
	if(set_global_str(&(cc->clientconfdir), globalc->clientconfdir))
		return -1;
	if(set_global_str(&(cc->directory), globalc->directory))
		return -1;
	if(set_global_str(&(cc->timestamp_format), globalc->timestamp_format))
		return -1;
	if(set_global_str(&(cc->recovery_method),
		globalc->recovery_method)) return -1;
	if(set_global_str(&(cc->timer_script), globalc->timer_script))
		return -1;
	if(set_global_str(&(cc->user), globalc->user))
		return -1;
	if(set_global_str(&(cc->group), globalc->group))
		return -1;
	if(set_global_str(&(cc->n_success_script),
		globalc->n_success_script)) return -1;
	if(set_global_str(&(cc->n_failure_script),
		globalc->n_failure_script)) return -1;
	if(set_global_str(&(cc->dedup_group), globalc->dedup_group))
		return -1;
	if(set_global_str(&(cc->s_script_pre),
		globalc->s_script_pre)) return -1;
	if(set_global_str(&(cc->s_script_post),
		globalc->s_script_post)) return -1;
	if(set_global_str(&(cc->s_script),
		globalc->s_script)) return -1;

	if(set_global_arglist(&(cc->rclients),
		globalc->rclients)) return -1;

	// If ssl_peer_cn is not set, default it to the client name.
	if(!globalc->ssl_peer_cn
	  && set_global_str(&(cc->ssl_peer_cn), cc->cname))
		return -1;

	return 0;
}

static int set_global_arglist_override(struct strlist **dst,
	struct strlist *src)
{
	if(*dst) return 0; // Was overriden by the client.
	// Otherwise, use the global list.
	return set_global_arglist(dst, src);
}

// Remember to update the list in the man page when you change these.
// Instead of adding onto the end of the list, these replace the list.
static int conf_set_from_global_arg_list_overrides(struct conf *globalc,
	struct conf *cc)
{
	if(set_global_arglist_override(&(cc->timer_arg),
		globalc->timer_arg)) return -1;
	if(set_global_arglist_override(&(cc->n_success_arg),
		globalc->n_success_arg)) return -1;
	if(set_global_arglist_override(&(cc->n_failure_arg),
		globalc->n_failure_arg)) return -1;
	if(set_global_arglist_override(&(cc->keep),
		globalc->keep)) return -1;
	if(set_global_arglist_override(&(cc->s_script_pre_arg),
		globalc->s_script_pre_arg)) return -1;
	if(set_global_arglist_override(&(cc->s_script_post_arg),
		globalc->s_script_post_arg)) return -1;
	if(set_global_arglist_override(&(cc->s_script_arg),
		globalc->s_script_arg)) return -1;
	return 0;
}

static void conf_init_save_cname_and_version(struct conf *cc)
{
	char *cname=cc->cname;
	char *cversion=cc->peer_version;

	cc->cname=NULL;
	cc->peer_version=NULL;
	conf_init(cc);
	cc->cname=cname;
	cc->peer_version=cversion;
}

int conf_load_clientconfdir(struct conf *globalc, struct conf *cc)
{
	int ret=-1;
	char *path=NULL;

	conf_init_save_cname_and_version(cc);
	if(looks_like_tmp_or_hidden_file(cc->cname))
	{
		logp("client name '%s' is invalid\n", cc->cname);
		goto end;
	}

	if(!(path=prepend_s(globalc->clientconfdir, cc->cname)))
		goto end;

	// Some client settings can be globally set in the server conf and
	// overridden in the client specific conf.
	if(conf_set_from_global(globalc, cc)
	  || load_conf_lines(path, cc)
	  || conf_set_from_global_arg_list_overrides(globalc, cc)
	  || conf_finalise(path, cc))
		goto end;

	ret=0;
end:
	free_w(&path);
	return ret;
}

int conf_load_global_only(const char *path, struct conf *globalc)
{
	free_w(&globalc->conffile);
	if(!(globalc->conffile=strdup_w(path, __func__))
	  || load_conf_lines(path, globalc)
	  || conf_finalise(path, globalc)
	  || conf_finalise_global_only(path, globalc))
		return -1;
	return 0;
}
