#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include "datafifo.h"
#include <QMap>
#include "mylog.h"
#include "rtkprocess.h"
#include <QDir>
#include <QDateTime>

#define SQRT(x)    ((x)<0.0?0.0:sqrt(x))

/* baseline length -----------------------------------------------------------*/
static double baseline_len(const rtk_t *rtk)
{
    double dr[3];
    int i;

    if (norm(rtk->sol.rr,3)<=0.0||norm(rtk->rb,3)<=0.0) return 0.0;

    for (i=0;i<3;i++) {
        dr[i]=rtk->sol.rr[i]-rtk->rb[i];
    }
    return norm(dr,3)*0.001; /* (km) */
}

static void outrpos(FILE *fp, const double *r, const solopt_t *opt)
{
    double pos[3],dms1[3],dms2[3];
    const char *sep=opt->sep;

    trace(3,"outrpos :\n");

    if (opt->posf==SOLF_LLH||opt->posf==SOLF_ENU) {
        ecef2pos(r,pos);
        if (opt->degf) {
            deg2dms(pos[0]*R2D,dms1,5);
            deg2dms(pos[1]*R2D,dms2,5);
            fprintf(fp,"%3.0f%s%02.0f%s%08.5f%s%4.0f%s%02.0f%s%08.5f%s%10.4f",
                    dms1[0],sep,dms1[1],sep,dms1[2],sep,dms2[0],sep,dms2[1],
                    sep,dms2[2],sep,pos[2]);
        }
        else {
            fprintf(fp,"%13.9f%s%14.9f%s%10.4f",pos[0]*R2D,sep,pos[1]*R2D,
                    sep,pos[2]);
        }
    }
    else if (opt->posf==SOLF_XYZ) {
        fprintf(fp,"%14.4f%s%14.4f%s%14.4f",r[0],sep,r[1],sep,r[2]);
    }
}

/* output header -------------------------------------------------------------*/
static void outMyheader(FILE *fp, const prcopt_t *popt,
                      const solopt_t *sopt)
{
    if (sopt->posf==SOLF_NMEA||sopt->posf==SOLF_STAT) {
        return;
    }

    outprcopt(fp,popt);

    if (PMODE_DGPS<=popt->mode&&popt->mode<=PMODE_FIXED&&popt->mode!=PMODE_MOVEB) {
        fprintf(fp,"%s ref pos   :",COMMENTH);
        outrpos(fp,popt->rb,sopt);
        fprintf(fp,"\n");
    }
    if (sopt->outhead||sopt->outopt) fprintf(fp,"%s\n",COMMENTH);

    outsolhead(fp,sopt);
}

static void writesolhead(stream_t *stream, const solopt_t *solopt)
{
    unsigned char buff[1024];
    int n;

    n=outsolheads(buff,solopt);
    strwrite(stream,buff,n);
}

static void saveoutbuf(rtksvr_t *svr, unsigned char *buff, int n, int index)
{
    rtksvrlock(svr);

    n=n<svr->buffsize-svr->nsb[index]?n:svr->buffsize-svr->nsb[index];
    memcpy(svr->sbuf[index]+svr->nsb[index],buff,n);
    svr->nsb[index]+=n;

    rtksvrunlock(svr);
}

/* periodic command ----------------------------------------------------------*/
//static void periodic_cmd(int cycle, const char *cmd, stream_t *stream)
//{
//    const char *p=cmd,*q;
//    char msg[1024],*r;
//    int n,period;

//    for (p=cmd;;p=q+1) {
//        for (q=p;;q++) if (*q=='\r'||*q=='\n'||*q=='\0') break;
//        n=(int)(q-p); strncpy(msg,p,n); msg[n]='\0';

//        period=0;
//        if ((r=strrchr(msg,'#'))) {
//            sscanf(r,"# %d",&period);
//            *r='\0';
//            while (*--r==' ') *r='\0'; /* delete tail spaces */
//        }
//        if (period<=0) period=1000;
//        if (*msg&&cycle%period==0) {
//            strsendcmd(stream,msg);
//        }
//        if (!*q) break;
//    }
//}

static void writesol(rtksvr_t *svr, int index)
{
    solopt_t solopt=solopt_default;
    unsigned char buff[MAXSOLMSG+1];
    int i,n;

    tracet(4,"writesol: index=%d\n",index);

    for (i=0;i<2;i++) {

        if (svr->solopt[i].posf==SOLF_STAT) {

            /* output solution status */
            rtksvrlock(svr);
            n=rtkoutstat(&svr->rtk,(char *)buff);
            rtksvrunlock(svr);
        }
        else {
            /* output solution */
            n=outsols(buff,&svr->rtk.sol,svr->rtk.rb,svr->solopt+i);
        }
        strwrite(svr->stream+i+3,buff,n);

        /* save output buffer */
        saveoutbuf(svr,buff,n,i);

        /* output extended solution */
        n=outsolexs(buff,&svr->rtk.sol,svr->rtk.ssat,svr->solopt+i);
        strwrite(svr->stream+i+3,buff,n);

        /* save output buffer */
        saveoutbuf(svr,buff,n,i);
    }
    /* output solution to monitor port */
    if (svr->moni) {
        n=outsols(buff,&svr->rtk.sol,svr->rtk.rb,&solopt);
        strwrite(svr->moni,buff,n);
    }
    /* save solution buffer */
    if (svr->nsol<MAXSOLBUF) {
        rtksvrlock(svr);
        svr->solbuf[svr->nsol++]=svr->rtk.sol;
        rtksvrunlock(svr);
    }
}

static void corr_phase_bias(obsd_t *obs, int n, const nav_t *nav)
{
    double lam;
    int i,j,code;

    for (i=0;i<n;i++) for (j=0;j<NFREQ;j++) {

        if (!(code=obs[i].code[j])) continue;
        if ((lam=nav->lam[obs[i].sat-1][j])==0.0) continue;

        /* correct phase bias (cyc) */
        obs[i].L[j]-=nav->ssr[obs[i].sat-1].pbias[code-1]/lam;
    }
}

static void updatenav(nav_t *nav)
{
    int i, j;
    for (i = 0; i < MAXSAT; i++)
        for (j = 0; j < NFREQ; j++)
        {
            nav->lam[i][j] = satwavelen(i + 1, j, nav);
        }
}

static void updatefcn(rtksvr_t *svr)
{
    int i,j,sat,frq;

    for (i=0;i<MAXPRNGLO;i++) {
        sat=satno(SYS_GLO,i+1);

        for (j=0,frq=-999;j<3;j++) {
            if (svr->raw[j].nav.geph[i].sat!=sat) continue;
            frq=svr->raw[j].nav.geph[i].frq;
        }
        if (frq<-7||frq>6) continue;

        for (j=0;j<3;j++) {
            if (svr->raw[j].nav.geph[i].sat==sat) continue;
            svr->raw[j].nav.geph[i].sat=sat;
            svr->raw[j].nav.geph[i].frq=frq;
        }
    }
}

static void updatesvr(rtksvr_t *svr, int ret, obs_t *obs, nav_t *nav, int sat,
                      sbsmsg_t *sbsmsg, int index, int iobs)
{
    eph_t *eph1,*eph2,*eph3;
    geph_t *geph1,*geph2,*geph3;
    gtime_t tof;
    double pos[3],del[3]={0},dr[3];
    int i,n=0,prn,sbssat=svr->rtk.opt.sbassatsel,sys,iode;

    tracet(4,"updatesvr: ret=%d sat=%2d index=%d\n",ret,sat,index);

    if (ret==1) { /* observation data */
        if (iobs<MAXOBSBUF) {
            for (i=0;i<obs->n;i++) {
                if (svr->rtk.opt.exsats[obs->data[i].sat-1]==1||
                    !(satsys(obs->data[i].sat,NULL)&svr->rtk.opt.navsys)) continue;
                svr->obs[index][iobs].data[n]=obs->data[i];
                svr->obs[index][iobs].data[n++].rcv=index+1;
            }
            svr->obs[index][iobs].n=n;
            sortobs(&svr->obs[index][iobs]);
        }
        svr->nmsg[index][0]++;
    }
    else if (ret==2) { /* ephemeris */
        if (satsys(sat,&prn)!=SYS_GLO) {
            if (!svr->navsel||svr->navsel==index+1) {
                eph1=nav->eph+sat-1;
                eph2=svr->nav.eph+sat-1;
                eph3=svr->nav.eph+sat-1+MAXSAT;
                if (eph2->ttr.time==0||
                    (eph1->iode!=eph3->iode&&eph1->iode!=eph2->iode)||
                    (timediff(eph1->toe,eph3->toe)!=0.0&&
                     timediff(eph1->toe,eph2->toe)!=0.0)) {
                    *eph3=*eph2;
                    *eph2=*eph1;
                    updatenav(&svr->nav);
                }
            }
            svr->nmsg[index][1]++;
        }
        else {
           if (!svr->navsel||svr->navsel==index+1) {
               geph1=nav->geph+prn-1;
               geph2=svr->nav.geph+prn-1;
               geph3=svr->nav.geph+prn-1+MAXPRNGLO;
               if (geph2->tof.time==0||
                   (geph1->iode!=geph3->iode&&geph1->iode!=geph2->iode)) {
                   *geph3=*geph2;
                   *geph2=*geph1;
                   updatenav(&svr->nav);
                   updatefcn(svr);
               }
           }
           svr->nmsg[index][6]++;
        }
    }
    else if (ret==3) { /* sbas message */
        if (sbsmsg&&(sbssat==sbsmsg->prn||sbssat==0)) {
            if (svr->nsbs<MAXSBSMSG) {
                svr->sbsmsg[svr->nsbs++]=*sbsmsg;
            }
            else {
                for (i=0;i<MAXSBSMSG-1;i++) svr->sbsmsg[i]=svr->sbsmsg[i+1];
                svr->sbsmsg[i]=*sbsmsg;
            }
            sbsupdatecorr(sbsmsg,&svr->nav);
        }
        svr->nmsg[index][3]++;
    }
    else if (ret==9) { /* ion/utc parameters */
        if (svr->navsel==0||svr->navsel==index+1) {
            for (i=0;i<8;i++) svr->nav.ion_gps[i]=nav->ion_gps[i];
            for (i=0;i<4;i++) svr->nav.utc_gps[i]=nav->utc_gps[i];
            for (i=0;i<4;i++) svr->nav.ion_gal[i]=nav->ion_gal[i];
            for (i=0;i<4;i++) svr->nav.utc_gal[i]=nav->utc_gal[i];
            for (i=0;i<8;i++) svr->nav.ion_qzs[i]=nav->ion_qzs[i];
            for (i=0;i<4;i++) svr->nav.utc_qzs[i]=nav->utc_qzs[i];
            svr->nav.leaps=nav->leaps;
        }
        svr->nmsg[index][2]++;
    }
    else if (ret==5) { /* antenna postion parameters */
        if (svr->rtk.opt.refpos==POSOPT_RTCM&&index==1) {
            for (i=0;i<3;i++) {
                svr->rtk.rb[i]=svr->rtcm[1].sta.pos[i];
            }
            /* antenna delta */
            ecef2pos(svr->rtk.rb,pos);
            if (svr->rtcm[1].sta.deltype) { /* xyz */
                del[2]=svr->rtcm[1].sta.hgt;
                enu2ecef(pos,del,dr);
                for (i=0;i<3;i++) {
                    svr->rtk.rb[i]+=svr->rtcm[1].sta.del[i]+dr[i];
                }
            }
            else { /* enu */
                enu2ecef(pos,svr->rtcm[1].sta.del,dr);
                for (i=0;i<3;i++) {
                    svr->rtk.rb[i]+=dr[i];
                }
            }
        }
        else if (svr->rtk.opt.refpos==POSOPT_RAW&&index==1) {
            for (i=0;i<3;i++) {
                svr->rtk.rb[i]=svr->raw[1].sta.pos[i];
            }
            /* antenna delta */
            ecef2pos(svr->rtk.rb,pos);
            if (svr->raw[1].sta.deltype) { /* xyz */
                del[2]=svr->raw[1].sta.hgt;
                enu2ecef(pos,del,dr);
                for (i=0;i<3;i++) {
                    svr->rtk.rb[i]+=svr->raw[1].sta.del[i]+dr[i];
                }
            }
            else { /* enu */
                enu2ecef(pos,svr->raw[1].sta.del,dr);
                for (i=0;i<3;i++) {
                    svr->rtk.rb[i]+=dr[i];
                }
            }
        }
        svr->nmsg[index][4]++;
    }
    else if (ret==7) { /* dgps correction */
        svr->nmsg[index][5]++;
    }
    else if (ret==10) { /* ssr message */
        for (i=0;i<MAXSAT;i++) {
            if (!svr->rtcm[index].ssr[i].update) continue;

            /* check consistency between iods of orbit and clock */
            if (svr->rtcm[index].ssr[i].iod[0]!=
                svr->rtcm[index].ssr[i].iod[1]) continue;

            svr->rtcm[index].ssr[i].update=0;

            iode=svr->rtcm[index].ssr[i].iode;
            sys=satsys(i+1,&prn);

            /* check corresponding ephemeris exists */
            if (sys==SYS_GPS||sys==SYS_GAL||sys==SYS_QZS) {
                if (svr->nav.eph[i       ].iode!=iode&&
                    svr->nav.eph[i+MAXSAT].iode!=iode) {
                    continue;
                }
            }
            else if (sys==SYS_GLO) {
                if (svr->nav.geph[prn-1          ].iode!=iode&&
                    svr->nav.geph[prn-1+MAXPRNGLO].iode!=iode) {
                    continue;
                }
            }
            svr->nav.ssr[i]=svr->rtcm[index].ssr[i];
        }
        svr->nmsg[index][7]++;
    }
    else if (ret==31) { /* lex message */
        lexupdatecorr(&svr->raw[index].lexmsg,&svr->nav,&tof);
        svr->nmsg[index][8]++;
    }
    else if (ret==-1) { /* error */
        svr->nmsg[index][9]++;
    }
}

static int decoderaw(rtksvr_t *svr, int index)
{
    obs_t *obs;
    nav_t *nav;
    sbsmsg_t *sbsmsg=NULL;
    int i,ret,sat,fobs=0;

    tracet(4,"decoderaw: index=%d\n",index);

    rtksvrlock(svr);

    for (i=0;i<svr->nb[index];i++) {

        /* input rtcm/receiver raw data from stream */
        if (svr->format[index]==STRFMT_RTCM2) {
            ret=input_rtcm2(svr->rtcm+index,svr->buff[index][i]);
            obs=&svr->rtcm[index].obs;
            nav=&svr->rtcm[index].nav;
            sat=svr->rtcm[index].ephsat;
        }
        else if (svr->format[index]==STRFMT_RTCM3) {
            ret=input_rtcm3(svr->rtcm+index,svr->buff[index][i]);
            obs=&svr->rtcm[index].obs;
            nav=&svr->rtcm[index].nav;
            sat=svr->rtcm[index].ephsat;
        }
        else {
            ret=input_raw(svr->raw+index,svr->format[index],svr->buff[index][i]);
            obs=&svr->raw[index].obs;
            nav=&svr->raw[index].nav;
            sat=svr->raw[index].ephsat;
            sbsmsg=&svr->raw[index].sbsmsg;
        }
#if 0 /* record for receiving tick */
        if (ret==1) {
            trace(0,"%d %10d T=%s NS=%2d\n",index,tickget(),
                  time_str(obs->data[0].time,0),obs->n);
        }
#endif
        /* update cmr rover observations cache */
        if (svr->format[1]==STRFMT_CMR&&index==0&&ret==1) {
            update_cmr(&svr->raw[1],svr,obs);
        }
        /* update rtk server */
        if (ret>0) updatesvr(svr,ret,obs,nav,sat,sbsmsg,index,fobs);

        /* observation data received */
        if (ret==1) {
            if (fobs<MAXOBSBUF) fobs++; else svr->prcout++;
        }
    }
    svr->nb[index]=0;

    rtksvrunlock(svr);

    return fobs;
}
/* decode download file ------------------------------------------------------*/
static void decodefile(rtksvr_t *svr, int index)
{
    nav_t nav={0};
    char file[1024];
    int nb;

    tracet(4,"decodefile: index=%d\n",index);

    rtksvrlock(svr);

    /* check file path completed */
    if ((nb=svr->nb[index])<=2||
        svr->buff[index][nb-2]!='\r'||svr->buff[index][nb-1]!='\n') {
        rtksvrunlock(svr);
        return;
    }
    strncpy(file,(char *)svr->buff[index],nb-2); file[nb-2]='\0';
    svr->nb[index]=0;

    rtksvrunlock(svr);

    if (svr->format[index]==STRFMT_SP3) { /* precise ephemeris */

        /* read sp3 precise ephemeris */
        readsp3(file,&nav,0);
        if (nav.ne<=0) {
            tracet(1,"sp3 file read error: %s\n",file);
            return;
        }
        /* update precise ephemeris */
        rtksvrlock(svr);

        if (svr->nav.peph) free(svr->nav.peph);
        svr->nav.ne=svr->nav.nemax=nav.ne;
        svr->nav.peph=nav.peph;
        svr->ftime[index]=utc2gpst(timeget());
        strcpy(svr->files[index],file);

        rtksvrunlock(svr);
    }
    else if (svr->format[index]==STRFMT_RNXCLK) { /* precise clock */

        /* read rinex clock */
        if (readrnxc(file,&nav)<=0) {
            tracet(1,"rinex clock file read error: %s\n",file);
            return;
        }
        /* update precise clock */
        rtksvrlock(svr);

        if (svr->nav.pclk) free(svr->nav.pclk);
        svr->nav.nc=svr->nav.ncmax=nav.nc;
        svr->nav.pclk=nav.pclk;
        svr->ftime[index]=utc2gpst(timeget());
        strcpy(svr->files[index],file);

        rtksvrunlock(svr);
    }
}

static int rtksvrMyStart(rtksvr_t *svr, int cycle, int buffsize, int *strs, char **paths, int *formats, int navsel, char **cmds,
                  char **cmds_periodic, char **rcvopts, int nmeacycle, int nmeareq,
        const double *nmeapos, prcopt_t *prcopt, solopt_t *solopt, stream_t *moni, char *errmsg)
{
    gtime_t time,time0={0};
    int i,j,rw;

    tracet(3,"rtksvrstart: cycle=%d buffsize=%d navsel=%d nmeacycle=%d nmeareq=%d\n",
           cycle,buffsize,navsel,nmeacycle,nmeareq);

    if (svr->state) {
        sprintf(errmsg,"server already started");
        return 0;
    }
    strinitcom();
    svr->cycle=cycle>1?cycle:1;
    svr->nmeacycle=nmeacycle>1000?nmeacycle:1000;
    svr->nmeareq=nmeareq;
    for (i=0;i<3;i++) svr->nmeapos[i]=nmeapos[i];
    svr->buffsize=buffsize>4096?buffsize:4096;
    for (i=0;i<3;i++) svr->format[i]=formats[i];
    svr->navsel=navsel;
    svr->nsbs=0;
    svr->nsol=0;
    svr->prcout=0;
    rtkfree(&svr->rtk);
    rtkinit(&svr->rtk, prcopt);

    if (prcopt->initrst) { /* init averaging pos by restart */
        svr->nave=0;
        for (i=0;i<3;i++) svr->rb_ave[i]=0.0;
    }
    for (i=0;i<3;i++) { /* input/log streams */
        svr->nb[i]=svr->npb[i]=0;
        if (!(svr->buff[i]=(unsigned char *)malloc(buffsize))||
            !(svr->pbuf[i]=(unsigned char *)malloc(buffsize))) {
            tracet(1,"rtksvrstart: malloc error\n");
            sprintf(errmsg,"rtk server malloc error");
            return 0;
        }
        for (j=0;j<10;j++) svr->nmsg[i][j]=0;
        for (j=0;j<MAXOBSBUF;j++) svr->obs[i][j].n=0;
        strcpy(svr->cmds_periodic[i],!cmds_periodic[i]?"":cmds_periodic[i]);

        /* initialize receiver raw and rtcm control */
        init_raw(svr->raw+i,formats[i]);
        init_rtcm(svr->rtcm+i);

        /* set receiver and rtcm option */
        if(rcvopts[i] != NULL)
        {
            strcpy(svr->raw [i].opt,rcvopts[i]);
            strcpy(svr->rtcm[i].opt,rcvopts[i]);
        }


        /* connect dgps corrections */
        svr->rtcm[i].dgps=svr->nav.dgps;
    }
    for (i=0;i<2;i++) { /* output peek buffer */
        if (!(svr->sbuf[i]=(unsigned char *)malloc(buffsize))) {
            tracet(1,"rtksvrstart: malloc error\n");
            sprintf(errmsg,"rtk server malloc error");
            return 0;
        }
    }
    /* set solution options */
    for (i=0;i<2;i++) {
        svr->solopt[i]=solopt[i];
    }
    /* set base station position */
    if (prcopt->refpos!=POSOPT_SINGLE) {
        for (i=0;i<6;i++) {
            svr->rtk.rb[i]=i<3?prcopt->rb[i]:0.0;
        }
    }
    /* update navigation data */
    for (i=0;i<MAXSAT *2;i++) svr->nav.eph [i].ttr=time0;
    for (i=0;i<NSATGLO*2;i++) svr->nav.geph[i].tof=time0;
    for (i=0;i<NSATSBS*2;i++) svr->nav.seph[i].tof=time0;
    updatenav(&svr->nav);

    /* set monitor stream */
    svr->moni=moni;

    /* open input streams */
//    for (i=0;i<8;i++) {
//        rw=i<3?STR_MODE_R:STR_MODE_W;
//        if (strs[i]!=STR_FILE) rw|=STR_MODE_W;
//        if (!stropen(svr->stream+i,strs[i],rw,paths[i])) {
//            sprintf(errmsg,"str%d open error path=%s",i+1,paths[i]);
//            for (i--;i>=0;i--) strclose(svr->stream+i);
//            return 0;
//        }
//        /* set initial time for rtcm and raw */
//        if (i<3) {
//            time=utc2gpst(timeget());
//            svr->raw [i].time=strs[i]==STR_FILE?strgettime(svr->stream+i):time;
//            svr->rtcm[i].time=strs[i]==STR_FILE?strgettime(svr->stream+i):time;
//        }
//    }
    /* sync input streams */
    strsync(svr->stream,svr->stream+1);
    strsync(svr->stream,svr->stream+2);

    /* write start commands to input streams */
    for (i=0;i<3;i++) {
        if (cmds[i]) strsendcmd(svr->stream+i,cmds[i]);
    }
    /* write solution header to solution streams */
    for (i=3;i<5;i++) {
        writesolhead(svr->stream+i,svr->solopt+i-3);
    }

    return 0;
}

typedef struct tagRtkPocHandle
{
    rtksvr_t rtkServer;
    QMap<QString, DataFifo*> mapData;
    pairInfo pInfo;
    RtkConfig rtkconfig;
    sol_t  sol_best;
    QDateTime last_porcessTime;
}RtkPocHandle;

void *rtkprocess_create(pairInfo *pInfo, prcopt_t *prcopt, solopt_t *solopt, RtkConfig *pRtkcfg)
{
    RtkPocHandle *handle = new RtkPocHandle;
    QString strlog;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "rtkprocess_create new failed!!!!!!!");
        mylog->error(strlog);
        return handle;
    }

    double pos[3] = { 0, 0, 0 }, nmeapos[3] = { 0, 0, 0 };
    int format[MAXSTRRTK] = { STRFMT_UBX, STRFMT_UBX, STRFMT_UBX };
    int strs[MAXSTRRTK]={0};
    char errmsg[1024];
    char *paths[8]={NULL},*cmds[3]={0},*cmds_periodic[3]={NULL},*rcvopts[3]={NULL};

    memset(&handle->rtkServer, 0, sizeof(handle->rtkServer));
    rtksvrinit(&handle->rtkServer);

    pos[0] = pos[0] * D2R;
    pos[1] = pos[1] * D2R;
    pos[2] = pos[2];
    pos2ecef(pos, nmeapos);

    rtksvrMyStart(&handle->rtkServer, 10, 32768, strs, paths, format, 0, cmds, cmds_periodic, rcvopts,
                  5000, 0, nmeapos, prcopt, solopt, NULL, errmsg);

    handle->mapData.clear();
    handle->mapData[pInfo->device[0].id] = new DataFifo;
    handle->mapData[pInfo->device[1].id] = new DataFifo;
    handle->pInfo = *pInfo;
    handle->rtkconfig = *pRtkcfg;
    memset(&handle->sol_best, 0, sizeof(handle->sol_best));
    handle->last_porcessTime = QDateTime::currentDateTime();



    return handle;
}

int rtkprocess_destory(void *hRtk)
{
    RtkPocHandle *handle = (RtkPocHandle *)hRtk;
    QString strlog;
    int ret = -1;
    int i;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "rtkprocess_destory NULL handle!!!!!!!");
        mylog->error(strlog);
        return ret;
    }


    rtksvr_t *svr = &handle->rtkServer;

    for (i = 0; i < MAXSTRRTK; i++)
        strclose(svr->stream + i);
    for (i = 0; i < 3; i++)
    {
        svr->nb[i] = svr->npb[i] = 0;
        free(svr->buff[i]);
        svr->buff[i] = NULL;
        free(svr->pbuf[i]);
        svr->pbuf[i] = NULL;
        free_raw(svr->raw + i);
        free_rtcm(svr->rtcm + i);
    }
    for (i = 0; i < 2; i++)
    {
        svr->nsb[i] = 0;
        free(svr->sbuf[i]);
        svr->sbuf[i] = NULL;
    }

    rtkfree(&svr->rtk);
    rtksvrfree(svr);


    QMapIterator<QString, DataFifo*> iter(handle->mapData);
    while (iter.hasNext()) {
        iter.next();
        delete iter.value();
    }

    delete handle;

    return 0;
}

int rtkprocess_pushData(void *hRtk, QString id, char *data, int size)
{
    RtkPocHandle *handle = (RtkPocHandle *)hRtk;
    QString strlog;
    int ret = -1;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "rtkprocess_pushData NULL handle!!!!!!!");
        mylog->error(strlog);
        return ret;
    }

    QMap<QString, DataFifo*>::const_iterator iter = handle->mapData.find(id);
    if(iter != handle->mapData.end())
    {
        QString str;

        if(handle->rtkconfig.savePath != "")
        {
            QDir dir;

            str = handle->rtkconfig.savePath + "/";
            str += id + "/";

            if(dir.exists(str) == false)
            {
                dir.mkpath(str);
            }

            if(dir.exists(handle->rtkconfig.savePath) == true)
            {
                str += QDateTime::currentDateTime().toString("yyyyMMdd") + "-" + id + ".txt";
                FILE *pf = fopen(str.toStdString().c_str(), "ab+");
                if(pf)
                {
                    fwrite(data, 1, size, pf);
                    fclose(pf);
                }
            }
        }

        ret = iter.value()->pushData(data, size);
    }

    return ret;
}

int rtkprocess_process(void *hRtk)
{
    RtkPocHandle *handle = (RtkPocHandle *)hRtk;
    QString strlog;
    int ret = -1;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "rtkprocess_process NULL handle!!!!!!!");
        mylog->error(strlog);
        return ret;
    }

    obs_t obs;
    obsd_t data[MAXOBS*2];
    sol_t sol={{0}},sol_nmea={{0}};
    double tt;
    unsigned int tick,ticknmea,tick1hz;
    unsigned char *p,*q;
    char msg[128];
    int i,j,n,fobs[3]={0},/*cycle,*/cputime,loops;
    rtksvr_t *svr = &handle->rtkServer;
    int pri[]={MAXSOLQ+1,1,2,3,4,5,1,6};

    tracet(3, "rtksvrthread:\n");

    svr->state = 1;
    obs.data = data;
    svr->tick = tickget();
    ticknmea = tick1hz = svr->tick - 1000;

    tick = tickget();

    loops = handle->mapData.size();
    if(loops > 3)
        loops = 3;

    QMap<QString, DataFifo*>::const_iterator iter = handle->mapData.begin();
    for (i = 0; i < loops; i++,iter++)
    {
        p = svr->buff[i] + svr->nb[i];
        q = svr->buff[i] + svr->buffsize;

        /* read receiver raw/rtcm data from input stream */
        if((n = iter.value()->popData((char *)p, q - p)) <= 0)
        {
            continue;
        }

        svr->nb[i] += n;

        /* save peek buffer */
        rtksvrlock(svr);
        n = n < svr->buffsize - svr->npb[i] ? n : svr->buffsize - svr->npb[i];
        memcpy(svr->pbuf[i] + svr->npb[i], p, n);
        svr->npb[i] += n;
        rtksvrunlock(svr);
    }

    for (i = 0; i < 3; i++)
    {
        if (svr->format[i] == STRFMT_SP3 || svr->format[i] == STRFMT_RNXCLK)
        {
            /* decode download file */
            decodefile(svr, i);
        }
        else
        {
            /* decode receiver raw/rtcm data */
            fobs[i] = decoderaw(svr, i);
        }
    }

    if (fobs[1] > 0 && svr->rtk.opt.refpos == POSOPT_SINGLE)
    {
        if ((svr->rtk.opt.maxaveep <= 0 || svr->nave < svr->rtk.opt.maxaveep) && pntpos(svr->obs[1][0].data, svr->obs[1][0].n, &svr->nav, &svr->rtk.opt, &sol, NULL, NULL, msg))
        {
            svr->nave++;
            for (i = 0; i < 3; i++)
            {
                svr->rb_ave[i] += (sol.rr[i] - svr->rb_ave[i]) / svr->nave;
            }
        }
        for (i = 0; i < 3; i++)
            svr->rtk.opt.rb[i] = svr->rb_ave[i];
    }
    for (i = 0; i < fobs[0]; i++)
    { /* for each rover observation data */
        obs.n = 0;
        for (j = 0; j < svr->obs[0][i].n && obs.n < MAXOBS * 2; j++)
        {
            obs.data[obs.n++] = svr->obs[0][i].data[j];
        }
        for (j = 0; j < svr->obs[1][0].n && obs.n < MAXOBS * 2; j++)
        {
            obs.data[obs.n++] = svr->obs[1][0].data[j];
        }
        /* carrier phase bias correction */
        if (!strstr(svr->rtk.opt.pppopt, "-DIS_FCB"))
        {
            corr_phase_bias(obs.data, obs.n, &svr->nav);
        }
        /* rtk positioning */

        tracelevel(0);
        rtksvrlock(svr);
        rtkpos(&svr->rtk, obs.data, obs.n, &svr->nav);
        rtksvrunlock(svr);
        tracelevel(0);

        if(handle->rtkconfig.resultPath != "")
        {
            QString str;
            str = handle->rtkconfig.resultPath + "/" +
                    QDateTime::currentDateTime().toString("yyyyMMdd") + "-" +
                    handle->pInfo.device[0].id + "_result.pos";
            FILE *pf = fopen(str.toStdString().c_str(), "a+");
            if(pf != NULL)
            {
                fseek(pf,0,SEEK_END);
                if(ftell(pf)==0)
                {
                    outMyheader(pf, &svr->rtk.opt, &solopt_default);
                }
                outsol(pf, &svr->rtk.sol, svr->rtk.rb, &solopt_default);
                fclose(pf);
            }
        }

        rtksvrlock(svr);
        if (svr->rtk.sol.stat != SOLQ_NONE)
        {
            if(pri[svr->rtk.sol.stat] <= pri[handle->sol_best.stat])
            {
                handle->sol_best = svr->rtk.sol;
            }
        }
        rtksvrunlock(svr);


        if (svr->rtk.sol.stat != SOLQ_NONE)
        {

            /* adjust current time */
            tt = (int) (tickget() - tick) / 1000.0 + DTTOL;
            timeset(gpst2utc(timeadd(svr->rtk.sol.time, tt)));

            /* write solution */
            writesol(svr, i);
            strlog.sprintf("[%s][%s][%d]rtkprocess_process %s get solution!!!!!!!", __FILE__, __func__, __LINE__, handle->pInfo.device[0].id.toStdString().c_str());
            mylog_console->info(strlog);
        }
        /* if cpu overload, inclement obs outage counter and break */
        if ((int) (tickget() - tick) >= svr->cycle)
        {
            svr->prcout += fobs[0] - i - 1;
#if 0 /* omitted v.2.4.1 */
            break;
#endif
        }
    }

    /* send null solution if no solution (1hz) */
    if (svr->rtk.sol.stat == SOLQ_NONE && (int) (tick - tick1hz) >= 1000)
    {
        writesol(svr, 0);
        tick1hz = tick;
    }
//    /* write periodic command to input stream */
//    for (i = 0; i < 3; i++)
//    {
//        periodic_cmd(cycle * svr->cycle, svr->cmds_periodic[i], svr->stream + i);
//    }
    /* send nmea request to base/nrtk input stream */
    if (svr->nmeacycle > 0 && (int) (tick - ticknmea) >= svr->nmeacycle)
    {
        if (svr->stream[1].state == 1)
        {
            if (svr->nmeareq == 1)
            {
                sol_nmea.stat = SOLQ_SINGLE;
                sol_nmea.time = utc2gpst(timeget());
                matcpy(sol_nmea.rr, svr->nmeapos, 3, 1);
                strsendnmea(svr->stream + 1, &sol_nmea);
            }
            else if (svr->nmeareq == 2 && norm(svr->rtk.sol.rr, 3) > 0.0)
            {
                strsendnmea(svr->stream + 1, &svr->rtk.sol);
            }
        }
        ticknmea = tick;
    }
    if ((cputime = (int) (tickget() - tick)) > 0)
        svr->cputime = cputime;


    return 0;
}

/* solution option to field separator ----------------------------------------*/
static const char *opt2sep(const solopt_t *opt)
{
    if (!*opt->sep) return " ";
    else if (!strcmp(opt->sep,"\\t")) return "\t";
    return opt->sep;
}

/* solution to covariance ----------------------------------------------------*/
static void soltocov(const sol_t *sol, double *P)
{
    P[0]     =sol->qr[0]; /* xx or ee */
    P[4]     =sol->qr[1]; /* yy or nn */
    P[8]     =sol->qr[2]; /* zz or uu */
    P[1]=P[3]=sol->qr[3]; /* xy or en */
    P[5]=P[7]=sol->qr[4]; /* yz or nu */
    P[2]=P[6]=sol->qr[5]; /* zx or ue */
}

/* sqrt of covariance --------------------------------------------------------*/
static double sqvar(double covar)
{
    return covar<0.0?-sqrt(-covar):sqrt(covar);
}

static int sol_convert(void *hRtk, const sol_t *sol, const solopt_t *opt, double *rb, RtkOutSolution *out)
{
    RtkPocHandle *handle = (RtkPocHandle *)hRtk;
    double pos[3],P[9],Q[9];
    double dr[3];
    double baseline_len;
    QDateTime date;

    QString strlog;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "sol_convert NULL handle!!!!!!!");
        mylog->error(strlog);
        return -1;
    }

    ecef2pos(sol->rr,pos);
    soltocov(sol,P);
    covenu(pos,P,Q);
    if (opt->height==1)
    { /* geodetic height */
        pos[2]-=geoidh(pos);
    }

    out->id = handle->pInfo.device[0].id;
    out->Q = QString("%1").arg(sol->stat);
    out->status = sol->stat;
    if(out->status == 0)
        return -1;

    if(norm(sol->rr,3)<=0.0||norm(rb,3)<=0.0)
        baseline_len = 0;
    else
    {
        for (int i=0;i<3;i++)
        {
            dr[i]=sol->rr[i]-rb[i];
        }
        baseline_len = norm(dr,3); /* (m) */
    }

    date.setMSecsSinceEpoch((sol->time.sec+sol->time.time)*1000);
    out->GPST = date;
    out->latitude = QString("%1").arg(pos[0]*R2D, 0, 'f', 9);
    out->longitude = QString("%1").arg(pos[1]*R2D, 0, 'f', 9);
    out->height = QString("%1").arg(pos[2], 0, 'f', 4);
    out->ns = QString("%1").arg(sol->ns);
    out->sdn = QString("%1").arg(SQRT(Q[4]), 0, 'f', 4);
    out->sde = QString("%1").arg(SQRT(Q[0]), 0, 'f', 4);
    out->sdu = QString("%1").arg(SQRT(Q[8]), 0, 'f', 4);
    out->sdne = QString("%1").arg(sqvar(Q[1]), 0, 'f', 4);
    out->sdeu = QString("%1").arg(sqvar(Q[2]), 0, 'f', 4);
    out->sdun = QString("%1").arg(sqvar(Q[5]), 0, 'f', 4);
    out->age = QString("%1").arg(sol->age, 0, 'f', 2);
    out->ratio = QString("%1").arg(sol->ratio, 0, 'f', 1);
    out->baseline = QString("%1").arg(baseline_len, 0, 'f', 4);

    return 0;
}

int rtkprocess_getSolAll(void *hRtk, RtkOutSolution *now, RtkOutSolution *best)
{
    RtkPocHandle *handle = (RtkPocHandle *)hRtk;
    rtksvr_t *svr = &handle->rtkServer;
    QString strlog;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "rtkprocess_getSolAll NULL handle!!!!!!!");
        mylog->error(strlog);
        return -1;
    }

    rtksvrlock(svr);
    sol_convert(hRtk, &svr->rtk.sol, svr->solopt, svr->rtk.rb, now);
    sol_convert(hRtk, &handle->sol_best, svr->solopt, svr->rtk.rb, best);
    rtksvrunlock(svr);

    return 0;
}

int rtkprocess_getSolBest(void *hRtk, RtkOutSolution *best)
{
    RtkPocHandle *handle = (RtkPocHandle *)hRtk;
    rtksvr_t *svr = &handle->rtkServer;
    QString strlog;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "rtkprocess_getSolBest NULL handle!!!!!!!");
        mylog->error(strlog);
        return -1;
    }

    rtksvrlock(svr);
    sol_convert(hRtk, &handle->sol_best, svr->solopt, svr->rtk.rb, best);
    rtksvrunlock(svr);

    return 0;
}


int rtkprocess_resetBestSol(void *hRtk)
{
    RtkPocHandle *handle = (RtkPocHandle *)hRtk;
    rtksvr_t *svr = &handle->rtkServer;
    QString strlog;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "rtkprocess_resetBestSol NULL handle!!!!!!!");
        mylog->error(strlog);
        return -1;
    }

    rtksvrlock(svr);
    memset(&handle->sol_best, 0, sizeof(handle->sol_best));
    rtksvrunlock(svr);

    return 0;
}

int rtkprocess_getLastProcTime(void *hRtk, QDateTime *lastPtime)
{
    RtkPocHandle *handle = (RtkPocHandle *)hRtk;
    rtksvr_t *svr = &handle->rtkServer;
    QString strlog;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "rtkprocess_getLastProcTime NULL handle!!!!!!!");
        mylog->error(strlog);
        return -1;
    }

    rtksvrlock(svr);
    *lastPtime = handle->last_porcessTime;
    rtksvrunlock(svr);

    return 0;
}


int rtkprocess_setLastProcTime(void *hRtk, QDateTime Ptime)
{
    RtkPocHandle *handle = (RtkPocHandle *)hRtk;
    rtksvr_t *svr = &handle->rtkServer;
    QString strlog;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "rtkprocess_setLastProcTime NULL handle!!!!!!!");
        mylog->error(strlog);
        return -1;
    }

    rtksvrlock(svr);
    handle->last_porcessTime = Ptime;
    rtksvrunlock(svr);

    return 0;
}




