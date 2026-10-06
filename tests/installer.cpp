// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
struct Patch {uint64_t address;unsigned length;unsigned char original[16],replacement[16];};
struct Fingerprint {uint64_t address;unsigned length;unsigned char hash[32];};
static Patch patches[]={{0x100,5,{1,2,3,4,5},{6,7,8,9,10}},{0x200,8,{1},{2}},{0x300,6,{3},{4}}};
static Fingerprint fingerprints[]={{0x400,32,{0}}};
static unsigned char resident_image[0x4000],install_identity[40];
#define WWE_INSTALLER_TEST
#include "../payload/installer.h"
struct Fake:Backend {
 unsigned char game[4096]={},mapped[0x4000]={};
 int map=0,writes=0,failAt=0;bool partial=false,stopped=false,stopLost=false,resumeLost=false,recoveryBad=false,changeOnStop=false;
 int stops=0,resumes=0;uint64_t base=0x4000;
 Fake(){for(const auto& p:patches)memcpy(game+p.address,p.original,p.length);for(unsigned i=0;i<32;i++)game[0x400+i]=i;sha256(game+0x400,32,fingerprints[0].hash);}
 unsigned char* ptr(uint64_t a,size_t n){if(a<sizeof(game)&&a+n<=sizeof(game))return game+a;if(map==1&&a>=0x4000&&a+n<=0x8000)return mapped+a-0x4000;return nullptr;}
 bool read(uint64_t a,void* out,size_t n)override{auto* p=ptr(a,n);if(!p)return false;memcpy(out,p,n);return true;}
 bool write(uint64_t a,const void* in,size_t n)override{writes++;auto* p=ptr(a,n);assert(p);if(a<0x4000)assert(stopped);if(writes==failAt){if(partial)memcpy(p,in,n/2+1);return false;}memcpy(p,in,n);return true;}
 int mapping()override{return map;}
 uint64_t allocate()override{map=1;return base;}
 bool stop(bool pause)override{if(pause){stops++;stopped=true;if(changeOnStop)game[0x400]^=1;return !stopLost;}resumes++;stopped=false;if(resumeLost){resumeLost=false;return false;}return true;}
 bool recover()override{return !recoveryBad;}
};
int main(){
 memcpy(install_identity,"WWEELF01",8);memcpy(resident_image+0x3f00,install_identity,40);
 {Fake f;assert(install(f)==Loaded);assert(patch_state(f,true));assert(!f.stopped);int n=f.writes;f.mapped[0x100]=1;assert(install(f)==AlreadyLoaded);assert(f.writes==n&&f.mapped[0x100]==1);}
 {Fake f;f.game[0x400]^=1;assert(install(f)==Unsupported);assert(f.writes==0);}
 {Fake f;f.game[0x100]^=1;assert(install(f)==Conflict);assert(f.writes==0);}
 {Fake f;f.map=-1;assert(install(f)==Conflict);assert(f.writes==0);}
 {Fake f;f.map=1;assert(install(f)==Conflict);assert(f.writes==0);}
 {Fake f;f.base=0x10000;assert(install(f)==AllocationFailed);assert(f.writes==0);}
 for(int n=1;n<=4;n++){Fake f;f.failAt=n;assert(install(f)==UploadFailed);assert(patch_state(f,false));assert(f.stops==0);}
 for(int n=5;n<=7;n++)for(int partial=0;partial<2;partial++){Fake f;f.failAt=n;f.partial=partial;assert(install(f)==PatchFailed);assert(patch_state(f,false));assert(!f.stopped&&f.resumes==1);}
 {Fake f;f.changeOnStop=true;assert(install(f)==Conflict);assert(patch_state(f,false));assert(!f.stopped);}
 {Fake f;f.stopLost=true;assert(install(f)==StopFailed);assert(patch_state(f,false));assert(f.resumes==1&&!f.stopped);}
 {Fake f;f.resumeLost=true;assert(install(f)==Loaded);assert(f.resumes==2&&!f.stopped);}
 {Fake f;f.failAt=6;f.recoveryBad=true;assert(install(f)==RecoveryFailed);assert(!f.stopped);}
 puts("20 installer scenarios passed: refusal, duplicate, upload failure, partial patch rollback, lost acknowledgements, resume.");
}
