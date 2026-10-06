// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "sha256.h"
#ifndef WWE_INSTALLER_TEST
#include "generated.h"
#endif
struct Backend {
 virtual bool read(uint64_t,void*,size_t)=0;
 virtual bool write(uint64_t,const void*,size_t)=0;
 // 0 = free range, 1 = full RWX allocation, -1 = conflicting/unreadable map.
 virtual int mapping()=0;
 virtual uint64_t allocate()=0;
 virtual bool stop(bool)=0;
 virtual bool recover()=0;
 virtual ~Backend()=default;
};
enum Result {Loaded,AlreadyLoaded,Unsupported,Conflict,AllocationFailed,UploadFailed,
             StopFailed,PatchFailed,RecoveryFailed,ResumeFailed};
static bool verify(Backend& b,uint64_t at,const void* bytes,size_t n){
 unsigned char v[4096];return n<=sizeof(v)&&b.read(at,v,n)&&!memcmp(v,bytes,n);
}
static bool identity(Backend& b){
 unsigned char data[256],hash[32];
 for(const auto& f:fingerprints){if(f.length>sizeof(data)||!b.read(f.address,data,f.length))return false;sha256(data,f.length,hash);if(memcmp(hash,f.hash,32))return false;}return true;
}
static bool patch_state(Backend& b,bool installed){for(const auto& p:patches)if(!verify(b,p.address,installed?p.replacement:p.original,p.length))return false;return true;}
static Result install(Backend& b){
 if(!identity(b))return Unsupported;
 int mapped=b.mapping();
 if(mapped==1&&verify(b,0x7f00,install_identity,40)&&patch_state(b,true))return AlreadyLoaded;
 if(mapped!=0||!patch_state(b,false))return Conflict;
 uint64_t base=b.allocate();
 // Never MAP_FIXED over an existing allocation. This validated resident image
 // requires this base; a different result is left unused until the game exits.
 if(base!=0x4000||b.mapping()!=1)return AllocationFailed;
 for(size_t off=0;off<0x4000;off+=4096)
  if(!b.write(base+off,resident_image+off,4096)||!verify(b,base+off,resident_image+off,4096))return UploadFailed;
 // Any ambiguous stop request gets a resume attempt, even when its ACK was lost.
 if(!b.stop(true)){b.recover();return b.stop(false)?StopFailed:ResumeFailed;}
 Result result=Loaded;size_t attempted=0;
 if(!identity(b)||!patch_state(b,false))result=Conflict;
 else for(const auto& p:patches){
  attempted++;
  if(!b.write(p.address,p.replacement,p.length)||!verify(b,p.address,p.replacement,p.length)){result=PatchFailed;break;}
 }
 if(result==PatchFailed){
  bool recovered=b.recover();
  // Restore every attempted patch, including a write whose response was lost.
  // Keep the image mapped: a thread may already have referenced it.
  if(recovered){for(size_t i=attempted;i>0;i--){const auto& p=patches[i-1];if(!b.write(p.address,p.original,p.length)||!verify(b,p.address,p.original,p.length))recovered=false;}}
  if(!recovered)result=RecoveryFailed;
 }
 if(!b.stop(false)){b.recover();if(!b.stop(false))return ResumeFailed;}
 return result;
}
