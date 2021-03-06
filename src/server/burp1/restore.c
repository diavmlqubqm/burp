#include "include.h"
#include "../../cmd.h"
#include "../../server/burp2/restore.h"

#include <librsync.h>

static int inflate_or_link_oldfile(struct asfd *asfd, const char *oldpath,
	const char *infpath, struct conf *cconf, int compression)
{
	int ret=0;
	struct stat statp;

	if(lstat(oldpath, &statp))
	{
		logp("could not lstat %s\n", oldpath);
		return -1;
	}

	if(dpthl_is_compressed(compression, oldpath))
	{
		//logp("inflating...\n");

		if(!statp.st_size)
		{
			FILE *dest;
			// Empty file - cannot inflate.
			// Just open and close the destination and we have
			// duplicated a zero length file.
			logp("asked to inflate zero length file: %s\n", oldpath);
			if(!(dest=open_file(infpath, "wb")))
			{
				close_fp(&dest);
				return -1;
			}
			close_fp(&dest);
			return 0;
		}

		if((ret=zlib_inflate(asfd, oldpath, infpath, cconf)))
			logp("zlib_inflate returned: %d\n", ret);
	}
	else
	{
		// Not compressed - just hard link it.
		if(do_link(oldpath, infpath, &statp, cconf,
			1 /* allow overwrite of infpath */))
				return -1;
	}
	return ret;
}

static int send_file(struct asfd *asfd, struct sbuf *sb,
	int patches, const char *best,
	unsigned long long *bytes, struct conf *cconf)
{
	int ret=0;
	static BFILE *bfd=NULL;

	if(!bfd && !(bfd=bfile_alloc())) return -1;

	bfile_init(bfd, 0, cconf);
	if(bfd->open_for_send(bfd, asfd, best, sb->winattr,
		1 /* no O_NOATIME */, cconf)) return -1;
	//logp("sending: %s\n", best);
	if(asfd->write(asfd, &sb->path))
		ret=-1;
	else if(patches)
	{
		// If we did some patches, the resulting file
		// is not gzipped. Gzip it during the send. 
		ret=send_whole_file_gzl(asfd, best, sb->burp1->datapth.buf,
			1, bytes, NULL, cconf, 9, bfd, NULL, 0);
	}
	else
	{
		// If it was encrypted, it may or may not have been compressed
		// before encryption. Send it as it as, and let the client
		// sort it out.
		if(sb->path.cmd==CMD_ENC_FILE
		  || sb->path.cmd==CMD_ENC_METADATA
		  || sb->path.cmd==CMD_ENC_VSS
		  || sb->path.cmd==CMD_ENC_VSS_T
		  || sb->path.cmd==CMD_EFS_FILE)
		{
			ret=send_whole_filel(asfd, sb->path.cmd, best,
				sb->burp1->datapth.buf, 1, bytes,
				cconf, bfd, NULL, 0);
		}
		// It might have been stored uncompressed. Gzip it during
		// the send. If the client knew what kind of file it would be
		// receiving, this step could disappear.
		else if(!dpthl_is_compressed(sb->compression,
			sb->burp1->datapth.buf))
		{
			ret=send_whole_file_gzl(asfd,
				best, sb->burp1->datapth.buf, 1, bytes,
				NULL, cconf, 9, bfd, NULL, 0);
		}
		else
		{
			// If we did not do some patches, the resulting
			// file might already be gzipped. Send it as it is.
			ret=send_whole_filel(asfd, sb->path.cmd, best,
				sb->burp1->datapth.buf, 1, bytes,
				cconf, bfd, NULL, 0);
		}
	}
	bfd->close(bfd, asfd);
	return ret;
}

static int verify_file(struct asfd *asfd, struct sbuf *sb,
	int patches, const char *best,
	unsigned long long *bytes, struct conf *cconf)
{
	MD5_CTX md5;
	size_t b=0;
	const char *cp=NULL;
	const char *newsum=NULL;
	uint8_t in[ZCHUNK];
	uint8_t checksum[MD5_DIGEST_LENGTH];
	unsigned long long cbytes=0;
	if(!(cp=strrchr(sb->burp1->endfile.buf, ':')))
	{
		logw(asfd, cconf, "%s has no md5sum!\n", sb->burp1->datapth.buf);
		return 0;
	}
	cp++;
	if(!MD5_Init(&md5))
	{
		logp("MD5_Init() failed\n");
		return -1;
	}
	if(patches
	  || sb->path.cmd==CMD_ENC_FILE
	  || sb->path.cmd==CMD_ENC_METADATA
	  || sb->path.cmd==CMD_EFS_FILE
	  || sb->path.cmd==CMD_ENC_VSS
	  || (!patches && !dpthl_is_compressed(sb->compression, best)))
	{
		// If we did some patches or encryption, or the compression
		// was turned off, the resulting file is not gzipped.
		FILE *fp=NULL;
		if(!(fp=open_file(best, "rb")))
		{
			logw(asfd, cconf, "could not open %s\n", best);
			return 0;
		}
		while((b=fread(in, 1, ZCHUNK, fp))>0)
		{
			cbytes+=b;
			if(!MD5_Update(&md5, in, b))
			{
				logp("MD5_Update() failed\n");
				close_fp(&fp);
				return -1;
			}
		}
		if(!feof(fp))
		{
			logw(asfd, cconf, "error while reading %s\n", best);
			close_fp(&fp);
			return 0;
		}
		close_fp(&fp);
	}
	else
	{
		gzFile zp=NULL;
		if(!(zp=gzopen_file(best, "rb")))
		{
			logw(asfd, cconf, "could not gzopen %s\n", best);
			return 0;
		}
		while((b=gzread(zp, in, ZCHUNK))>0)
		{
			cbytes+=b;
			if(!MD5_Update(&md5, in, b))
			{
				logp("MD5_Update() failed\n");
				gzclose_fp(&zp);
				return -1;
			}
		}
		if(!gzeof(zp))
		{
			logw(asfd, cconf, "error while gzreading %s\n", best);
			gzclose_fp(&zp);
			return 0;
		}
		gzclose_fp(&zp);
	}
	if(!MD5_Final(checksum, &md5))
	{
		logp("MD5_Final() failed\n");
		return -1;
	}
	newsum=bytes_to_md5str(checksum);

	if(strcmp(newsum, cp))
	{
		logp("%s %s\n", newsum, cp);
		logw(asfd, cconf, "md5sum for '%s (%s)' did not match!\n",
			sb->path.buf, sb->burp1->datapth.buf);
		logp("md5sum for '%s (%s)' did not match!\n",
			sb->path.buf, sb->burp1->datapth.buf);
		return 0;
	}
	*bytes+=cbytes;

	// Just send the file name to the client, so that it can show cntr.
	if(asfd->write(asfd, &sb->path)) return -1;
	return 0;
}

// a = length of struct bu array
// i = position to restore from
static int restore_file(struct asfd *asfd, struct bu *bu,
	struct sbuf *sb, int act, struct sdirs *sdirs, struct conf *cconf)
{
	struct bu *b;
	struct bu *hlwarn=NULL;
	static char *tmppath1=NULL;
	static char *tmppath2=NULL;

	if((!tmppath1 && !(tmppath1=prepend_s(sdirs->client, "tmp1")))
	  || (!tmppath2 && !(tmppath2=prepend_s(sdirs->client, "tmp2"))))
		return -1;

	// Go up the array until we find the file in the data directory.
	for(b=bu; b; b=b->next)
	{
		char *path=NULL;
		struct stat statp;
		if(!(path=prepend_s(b->data, sb->burp1->datapth.buf)))
		{
			log_and_send_oom(asfd, __func__);
			return -1;
		}

		//printf("server file: %s\n", path);

		if(lstat(path, &statp) || !S_ISREG(statp.st_mode))
		{
			free(path);
			continue;
		}
		else
		{
			int patches=0;
			struct stat dstatp;
			const char *tmp=NULL;
			const char *best=NULL;
			unsigned long long bytes=0;

			if(b!=bu && (bu->flags & BU_HARDLINKED)) hlwarn=b;

			best=path;
			tmp=tmppath1;
			// Now go down the list, applying any deltas.
			for(b=b->prev; b && b->next!=bu; b=b->prev)
			{
				char *dpath=NULL;

				if(!(dpath=prepend_s(b->delta,
					sb->burp1->datapth.buf)))
				{
					log_and_send_oom(asfd, __func__);
					free(path);
					return -1;
				}

				if(lstat(dpath, &dstatp)
				  || !S_ISREG(dstatp.st_mode))
				{
					free(dpath);
					continue;
				}

				if(!patches)
				{
					// Need to gunzip the first one.
					if(inflate_or_link_oldfile(asfd,
						best, tmp,
						cconf, sb->compression))
					{
						logp("error when inflating %s\n", best);
						free(path);
						free(dpath);
						return -1;
					}
					best=tmp;
					if(tmp==tmppath1) tmp=tmppath2;
					else tmp=tmppath1;
				}

				if(do_patch(asfd, best, dpath, tmp,
				  0 /* do not gzip the result */,
				  sb->compression /* from the manifest */,
				  cconf))
				{
					char msg[256]="";
					snprintf(msg, sizeof(msg),
						"error when patching %s\n",
							path);
					log_and_send(asfd, msg);
					free(path);
					free(dpath);
					return -1;
				}

				best=tmp;
				if(tmp==tmppath1) tmp=tmppath2;
				else tmp=tmppath1;
				unlink(tmp);
				patches++;
			}

			if(act==ACTION_RESTORE)
			{
				if(send_file(asfd, sb,
					patches, best, &bytes, cconf))
				{
					free(path);
					return -1;
				}
				else
				{
					cntr_add(cconf->cntr,
						sb->path.cmd, 0);
					cntr_add_bytes(cconf->cntr,
                 			  strtoull(sb->burp1->endfile.buf,
						NULL, 10));
				}
			}
			else if(act==ACTION_VERIFY)
			{
				if(verify_file(asfd, sb, patches,
					best, &bytes, cconf))
				{
					free(path);
					return -1;
				}
				else
				{
					cntr_add(cconf->cntr,
						sb->path.cmd, 0);
					cntr_add_bytes(cconf->cntr,
                 			  strtoull(sb->burp1->endfile.buf,
						NULL, 10));
				}
			}
			cntr_add_sentbytes(cconf->cntr, bytes);

			// This warning must be done after everything else,
			// Because the client does not expect another cmd after
			// the warning.
			if(hlwarn) logw(asfd, cconf,
				"restore found %s in %s\n",
					sb->path.buf, hlwarn->basename);
			free(path);
			return 0;
		}
	}

	logw(asfd, cconf, "restore could not find %s (%s)\n",
		sb->path.buf, sb->burp1->datapth.buf);
	//return -1;
	return 0;
}

static int restore_sbufl(struct asfd *asfd, struct sbuf *sb, struct bu *bu,
	enum action act, struct sdirs *sdirs,
	enum cntr_status cntr_status, struct conf *cconf)
{
	//printf("%s: %s\n", act==ACTION_RESTORE?"restore":"verify", sb->path.buf);
	if(write_status(cntr_status, sb->path.buf, cconf)) return -1;
	
	if((sb->burp1->datapth.buf && asfd->write(asfd, &(sb->burp1->datapth)))
	  || asfd->write(asfd, &sb->attr))
		return -1;
	else if(sbuf_is_filedata(sb))
	{
		if(!sb->burp1->datapth.buf)
		{
			logw(asfd, cconf,
				"Got filedata entry with no datapth: %c:%s\n",
					sb->path.cmd, sb->path.buf);
			return 0;
		}
		return restore_file(asfd, bu, sb, act, sdirs, cconf);
	}
	else
	{
		if(asfd->write(asfd, &sb->path))
			return -1;
		// If it is a link, send what
		// it points to.
		else if(sbuf_is_link(sb)
		  && asfd->write(asfd, &sb->link)) return -1;
		cntr_add(cconf->cntr, sb->path.cmd, 0);
	}
	return 0;
}

static int restore_ent(struct asfd *asfd, struct sbuf **sb,
	struct sbuf ***sblist, int *scount, struct bu *bu,
	enum action act, struct sdirs *sdirs,
	enum cntr_status cntr_status, struct conf *cconf)
{
	int s=0;
	int ret=-1;

	// Check if we have any directories waiting to be restored.
	for(s=(*scount)-1; s>=0; s--)
	{
		if(is_subdir((*sblist)[s]->path.buf, (*sb)->path.buf))
		{
			// We are still in a subdir.
			//printf(" subdir (%s %s)\n",
			// (*sblist)[s]->path, (*sb)->path);
			break;
		}
		else
		{
			// Can now restore sblist[s] because nothing else is
			// fiddling in a subdirectory.
			if(restore_sbufl(asfd, (*sblist)[s], bu,
				act, sdirs, cntr_status, cconf))
					goto end;
			else if(del_from_sbufl_arr(sblist, scount))
				goto end;
		}
	}

	/* If it is a directory, need to remember it and restore it later, so
	   that the permissions come out right. */
	/* Meta data of directories will also have the stat stuff set to be a
	   directory, so will also come out at the end. */
	if(S_ISDIR((*sb)->statp.st_mode))
	{
		if(add_to_sbufl_arr(sblist, *sb, scount))
			goto end;
		// Allocate a new sb to carry on with.
		if(!(*sb=sbuf_alloc(cconf)))
			goto end;
	}
	else if(restore_sbufl(asfd, *sb, bu, act, sdirs, cntr_status, cconf))
		goto end;
	ret=0;
end:
	return ret;
}

int restore_burp1(struct asfd *asfd, struct bu *bu,
	const char *manifest, regex_t *regex, int srestore,
	enum action act, struct sdirs *sdirs, enum cntr_status cntr_status,
	struct conf *cconf)
{
	int s=0;
	int ret=-1;
	struct sbuf *sb=NULL;
	// For out-of-sequence directory restoring so that the
	// timestamps come out right:
	int scount=0;
	struct sbuf **sblist=NULL;
	struct iobuf *rbuf=asfd->rbuf;
	gzFile zp;

	if(!(sb=sbuf_alloc(cconf))) goto end;
	if(!(zp=gzopen_file(manifest, "rb")))
	{
		log_and_send(asfd, "could not open manifest");
		goto end;
	}

	while(1)
	{
		int ars=0;
		iobuf_free_content(rbuf);
		if(asfd->as->read_quick(asfd->as))
		{
			logp("read quick error\n");
			goto end;
		}
		if(rbuf->buf)
		{
			//logp("got read quick\n");
			if(rbuf->cmd==CMD_WARNING)
			{
				logp("WARNING: %s\n", rbuf->buf);
				cntr_add(cconf->cntr, rbuf->cmd, 0);
				continue;
			}
			else if(rbuf->cmd==CMD_INTERRUPT)
			{
				// Client wanted to interrupt the
				// sending of a file. But if we are
				// here, we have already moved on.
				// Ignore.
				continue;
			}
			else
			{
				iobuf_log_unexpected(rbuf, __func__);
				goto end;
			}
		}

		if((ars=sbufl_fill(sb, asfd, NULL, zp, cconf->cntr)))
		{
			if(ars<0) goto end;
			break;
		}
		else
		{
			if((!srestore
			    || check_srestore(cconf, sb->path.buf))
			  && check_regex(regex, sb->path.buf)
			  && restore_ent(asfd, &sb, &sblist, &scount,
				bu, act, sdirs, cntr_status, cconf))
					goto end;
		}
		sbuf_free_content(sb);
	}
	// Restore any directories that are left in the list.
	for(s=scount-1; s>=0; s--)
	{
		if(restore_sbufl(asfd, sblist[s], bu,
			act, sdirs, cntr_status, cconf))
				goto end;
	}

	ret=restore_end(asfd, cconf);

	cntr_print(cconf->cntr, act);

	cntr_stats_to_file(cconf->cntr, bu->path, act, cconf);

end:
	iobuf_free_content(rbuf);
	gzclose_fp(&zp);
	sbuf_free(&sb);
	free_sbufls(sblist, scount);
	return ret;
}
