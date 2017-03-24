#include <stdio.h>
#include <stdio.h>
#include "rtklib/rtklib.h"
#include <unistd.h>
#include "datafifo.h"
#include <QMap>

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

static void periodic_cmd(int cycle, const char *cmd, stream_t *stream)
{
    const char *p=cmd,*q;
    char msg[1024],*r;
    int n,period;

    for (p=cmd;;p=q+1) {
        for (q=p;;q++) if (*q=='\r'||*q=='\n'||*q=='\0') break;
        n=(int)(q-p); strncpy(msg,p,n); msg[n]='\0';

        period=0;
        if ((r=strrchr(msg,'#'))) {
            sscanf(r,"# %d",&period);
            *r='\0';
        }
        if (period<=0) period=1000;
        if (*msg&&cycle%period==0) {
            strsendcmd(stream,msg);
        }
        if (!*q) break;
    }
}

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
    rtkinit(&svr->rtk,prcopt);

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
    QMap<QString, DataFifo *> mapData;

}RtkPocHandle;

void *rtkprocess_create(prcopt_t *prcopt, solopt_t *solopt)
{
    RtkPocHandle *handle = new RtkPocHandle;

    if(handle == NULL)
        return handle;


    double pos[3] = { 0, 0, 0 }, nmeapos[3] = { 0, 0, 0 };
    int format[MAXSTRRTK] = { STRFMT_UBX, STRFMT_UBX, STRFMT_UBX };
    int strs[MAXSTRRTK]={0};


}



















