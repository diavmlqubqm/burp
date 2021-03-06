#ifndef DPTH_BURP1_H
#define DPTH_BURP1_H

struct dpthl
{
	int prim;
	int seco;
	int tert;
	char path[32];
};

extern int init_dpthl(struct dpthl *dpthl,
	struct sdirs *sdirs, struct conf *cconf);
extern int incr_dpthl(struct dpthl *dpthl, struct conf *cconf);
extern int set_dpthl_from_string(struct dpthl *dpthl,
	const char *datapath, struct conf *conf);
extern void mk_dpthl(struct dpthl *dpthl, struct conf *cconf, enum cmd cmd);

#endif
