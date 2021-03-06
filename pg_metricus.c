#include <postgres.h>
#include <fmgr.h>
#include <utils/guc.h>
#include <arpa/inet.h>
#include <unistd.h>

#if PG_VERSION_NUM >= 90600
/* GetConfigOptionByName has a new signature from 9.6 on */
#define GetConfigOptionByName(name, varname) GetConfigOptionByName(name, varname, false)
#endif  /* PG_VERSION_NUM */

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

char *dup_pgtext(text *what);
extern Datum send_metric(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(send_metric);

Datum
send_metric(PG_FUNCTION_ARGS)
{
    int sockfd, port;
    char *message, *host;
    struct sockaddr_in serv_addr;

    if (PG_ARGISNULL(0))
        ereport(ERROR, (errmsg("null message not accepted")));

    message = dup_pgtext(PG_GETARG_TEXT_P(0));

    if (strlen(message) == 0)
        ereport(ERROR, (errcode(ERRCODE_SYNTAX_ERROR), errmsg("invalid input message")));

    host = GetConfigOptionByName("pg_metricus.host", NULL);
    port = atoi(GetConfigOptionByName("pg_metricus.port", NULL));

    if (port < 0 || strlen(host) == 0)
        ereport(ERROR, (errcode(ERRCODE_SYNTAX_ERROR), errmsg("invalid host or port")));

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        ereport(ERROR, (errmsg("create socket")));

    bzero(&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port); 

    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0)
        ereport(ERROR, (errmsg("inet_pton, invalid host format")));

    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
         ereport(ERROR, (errmsg("send to socket")));

    close(sockfd);

    PG_RETURN_VOID();
}

char *
dup_pgtext(text *what)
{
    size_t len = VARSIZE(what)-VARHDRSZ;
    char *dup = palloc(len+1);
    memcpy(dup, VARDATA(what), len);
    dup[len] = 0;
    return dup;
}
