#include "include.h"
#include "cmd.h"

static const char *endreqstrf;
static const char *endrepstrf;

static int add_to_incexc(char **incexc, const char *src, size_t len, const char *sep)
{
	char *tmp;
	if(!(tmp=prepend(*incexc?:"", src, len, *incexc?"\n":""))) return -1;
	if(*incexc) free(*incexc);
	*incexc=tmp;
	return 0;
}

static enum asl_ret incexc_recv_func(struct asfd *asfd,
        struct conf *conf, void *param)
{
	char **incexc=(char **)param;
	if(!strcmp(asfd->rbuf->buf, endreqstrf))
	{
		if(asfd->write_str(asfd, CMD_GEN, endrepstrf))
			return ASL_END_ERROR;
		return ASL_END_OK;
	}
	if(add_to_incexc(incexc,
		asfd->rbuf->buf, asfd->rbuf->len, *incexc?"\n":""))
			return ASL_END_ERROR;
	return ASL_CONTINUE;
}


static int incexc_recv(struct asfd *asfd, char **incexc,
	const char *reqstr, const char *repstr,
	const char *endreqstr, const char *endrepstr, struct conf *conf)
{
	free_w(incexc);
	if(asfd->write_str(asfd, CMD_GEN, repstr)) return -1;

	endreqstrf=endreqstr;
	endrepstrf=endrepstr;
	if(asfd->simple_loop(asfd, conf, incexc, __func__, incexc_recv_func))
		return -1;

	// Need to put another new line at the end.
	return add_to_incexc(incexc, "\n", 1, "");
}

int incexc_recv_client(struct asfd *asfd,
	char **incexc, struct conf *conf)
{
	return incexc_recv(asfd, incexc,
		"sincexc", "sincexc ok",
		"sincexc end", "sincexc end ok",
		conf);
}

int incexc_recv_client_restore(struct asfd *asfd,
	char **incexc, struct conf *conf)
{
	return incexc_recv(asfd, incexc,
		"srestore", "srestore ok",
		"srestore end", "srestore end ok",
		conf);
}

int incexc_recv_server(struct asfd *asfd,
	char **incexc, struct conf *conf)
{
	return incexc_recv(asfd, incexc,
		"incexc", "incexc ok",
		"incexc end", "incexc end ok",
		conf);
}
