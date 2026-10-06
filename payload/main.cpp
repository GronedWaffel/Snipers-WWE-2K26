// SPDX-License-Identifier: GPL-3.0-or-later
// Console-side client of PS5Debug-NG. No PC, kernel offsets or embedded debugger.
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "installer.h"
extern "C" int sceKernelSendNotificationRequest(int,void*,size_t,int);
static FILE* log_file;
template<class T>static T get(const void* p){T n;memcpy(&n,p,sizeof(n));return n;}
template<class T>static void put(void* p,T n){memcpy(p,&n,sizeof(n));}
class Debug final:public Backend {
 int fd=-1;
 bool fail(){if(fd>=0)close(fd);fd=-1;return false;}
 bool transfer(void* data,size_t n,bool sending){
  auto* p=(unsigned char*)data;
  while(n){ssize_t r=sending?send(fd,p,n,0):recv(fd,p,n,0);if(r<0&&errno==EINTR)continue;if(r<=0)return fail();p+=r;n-=r;}return true;
 }
 bool command(uint32_t command,const void* data=nullptr,uint32_t n=0){
  uint32_t h[3]={0xffaabbcc,command,n};return fd>=0&&transfer(h,sizeof(h),true)&&(!n||transfer((void*)data,n,true));
 }
 bool status(){uint32_t s=0;return transfer(&s,4,false)&&(s==0x80000000||fail());}
 bool request(uint32_t cmd,const void* body,uint32_t n){return command(cmd,body,n)&&status();}
 bool process_identity(){
  unsigned char b[188];return request(0xbdaa000a,&pid,4)&&transfer(b,sizeof(b),false)&&get<uint32_t>(b)==pid&&
   !memcmp(b+108,"PPSA34299",sizeof("PPSA34299"))&&
   !memcmp(b+124,"UP1001-PPSA34299_00-WWE2K26000000000",sizeof("UP1001-PPSA34299_00-WWE2K26000000000"));
 }
 public:
 uint32_t pid=0;
 ~Debug(){if(fd>=0)close(fd);}
 bool connect_local(){
  if(fd>=0)close(fd);fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return false;
  timeval timeout={3,0};int one=1;
  setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
  setsockopt(fd,SOL_SOCKET,SO_NOSIGPIPE,&one,sizeof(one));
  sockaddr_in address={};address.sin_len=sizeof(address);address.sin_family=AF_INET;address.sin_port=htons(744);address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  if(::connect(fd,(sockaddr*)&address,sizeof(address)))return fail();
  uint16_t platform=0;uint32_t n=0;char brand[513]={};
  if(!command(0xbd000502)||!transfer(&platform,2,false)||platform!=5||!command(0xbd000501)||!transfer(&n,4,false)||!n||n>512||!transfer(brand,n,false))return fail();
  if(strncasecmp(brand,"ps5debug-ng",11))return fail();
  if(!strstr(brand,"1.3.2"))return fail();
  if(log_file){fprintf(log_file,"debugger=%s\n",brand);fflush(log_file);}return true;
 }
 bool find_game(){
  uint32_t n=0;if(!request(0xbdaa0001,nullptr,0)||!transfer(&n,4,false)||n>4096)return fail();
  unsigned char* data=(unsigned char*)malloc((size_t)n*36+1);if(!data)return false;
  if(!transfer(data,(size_t)n*36,false)){free(data);return false;}
  unsigned found=0;uint32_t selected=0;
  for(uint32_t i=0;i<n;i++)if(!memcmp(data+i*36,"eboot.bin\0",10)){
   pid=get<uint32_t>(data+i*36+32);if(process_identity()){selected=pid;found++;}
   if(fd<0)break;
  }
  free(data);pid=selected;return found==1&&fd>=0;
 }
 bool read(uint64_t a,void* out,size_t n)override{
  if(!n||n>0x100000)return false;unsigned char b[16];put(b,pid);put(b+4,a);put(b+12,(uint32_t)n);
  return request(0xbdaa0002,b,sizeof(b))&&transfer(out,n,false);
 }
 bool write(uint64_t a,const void* bytes,size_t n)override{
  if(!n||n>4096)return false;unsigned char b[16];put(b,pid);put(b+4,a);put(b+12,(uint32_t)n);
  return request(0xbdaa0003,b,sizeof(b))&&transfer((void*)bytes,n,true)&&status();
 }
 int mapping()override{
  uint32_t n=0;if(!request(0xbdaa0004,&pid,4)||!transfer(&n,4,false)||n>65536){fail();return -1;}
  int result=0;for(uint32_t i=0;i<n;i++){
   unsigned char m[58];if(!transfer(m,sizeof(m),false))return -1;
   uint64_t start=get<uint64_t>(m+32),end=get<uint64_t>(m+40);unsigned prot=get<uint16_t>(m+56);
   if(start<0x8000&&end>0x4000){if(start==0x4000&&end>=0x8000&&(prot&7)==7&&result==0)result=1;else result=-1;}
  }return result;
 }
 uint64_t allocate()override{
  uint32_t body[2]={pid,0x4000};uint64_t at=0;
  if(!request(0xbdaa000b,body,sizeof(body))||!transfer(&at,8,false))return 0;
  if(log_file){fprintf(log_file,"allocation=%#lx\n",at);fflush(log_file);}return at;
 }
 bool stop(bool pause)override{
  unsigned char b[5];put(b,pid);b[4]=pause?1:0;
  bool ok=request(0xbdbb0500,b,sizeof(b));
  if(log_file){fprintf(log_file,"%s=%s\n",pause?"stop":"resume",ok?"ok":"failed");fflush(log_file);}
  if(ok&&pause)usleep(150000);return ok;
 }
 bool recover()override{return (fd>=0||connect_local())&&process_identity();}
};
static int finish(int result,const char* message){
 if(log_file){fprintf(log_file,"result=%d %s\n",result,message);fclose(log_file);log_file=nullptr;}
 unsigned char notification[0xc30]={};put<int>(notification+0x10,-1);
 snprintf((char*)notification+0x2d,1024,"Snipers WWE 2K26: %s",message);
 sceKernelSendNotificationRequest(0,notification,sizeof(notification),0);return result;
}
int main(){
 int lock=open("/system_tmp/snipers-wwe-menu.lock",O_CREAT|O_RDWR,0600);
 if(lock<0||flock(lock,LOCK_EX|LOCK_NB)){if(lock>=0)close(lock);return finish(1,"Another installer is active. Try again shortly.");}
 log_file=fopen("/data/snipers-wwe-menu.log","w");
 Debug debugger;int rc;
 if(!debugger.connect_local())rc=finish(1,"Start PS5Debug-NG 1.3.2 on port 744, then load this ELF again.");
 else if(!debugger.find_game())rc=finish(1,"Open WWE 2K26 PPSA34299 v01.000.000, then load this ELF again.");
 else {
  if(log_file){fprintf(log_file,"build=1.0.0 pid=%u title=PPSA34299 version=01.000.000\n",debugger.pid);fflush(log_file);}
  Result result=install(debugger);
  const char* messages[]={"Loaded! In a match: Options > Snipers Mods. All mods start OFF.",
   "Already loaded. In a match: Options > Snipers Mods. Your toggles are unchanged.",
   "Unsupported game executable. No patches applied.",
   "Another menu or incompatible memory layout is active. Close WWE completely and retry.",
   "Could not reserve the required menu memory. Close WWE completely and retry.",
   "Menu upload failed; no hooks activated. Close WWE and retry.",
   "Could not pause WWE safely. No hooks activated.",
   "Installation failed; original hooks restored. Close WWE and retry.",
   "Installation recovery was incomplete. Close WWE completely before retrying.",
   "WWE resume was not confirmed. Close WWE completely before retrying."};
  rc=finish(result<=AlreadyLoaded?0:1,messages[result]);
 }
 close(lock);return rc;
}
