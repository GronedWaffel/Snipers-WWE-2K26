// SPDX-License-Identifier: GPL-3.0-or-later
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {spawnSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
import path from 'node:path';
import {buildFinisherHook,hookAddress,original} from '../src/finisher-hook.mjs';
import {buildSignatureHook,signatureAddress,signatureOriginal} from '../src/signature-hook.mjs';
import {inspectElf} from './verify-elf.mjs';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const out=path.join(root,'build');mkdirSync(out,{recursive:true});
const zig=process.env.ZIG||'zig', sdk=process.env.PS5_PAYLOAD_SDK;
function run(args){const r=spawnSync(zig,args,{cwd:root,encoding:'utf8',env:{...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(out,'zig-cache')}});if(r.status!==0)throw Error(r.stdout+r.stderr+(r.error||''));}
run(['cc','-target','x86_64-linux-none','-O2','-ffreestanding','-fno-stack-protector','-fno-pic','-fno-pie','-mno-red-zone','-nostdlib','-static','-Wl,-T,src/menu.ld','-Wl,--build-id=none','src/pause-menu.c','-o','build/menu.elf']);
run(['objcopy','-O','binary','build/menu.elf','build/menu.bin']);
const menu=readFileSync(path.join(out,'menu.bin')), image=Buffer.alloc(0x4000);
if(menu.length>0x1800)throw Error('Resident menu exceeds reserved space');
const fin=buildFinisherHook(0x4000n),sig=buildSignatureHook(0x5700n,0x5900n);
fin.code.copy(image);image[0x100]=0; // All release toggles initially OFF.
sig.code.copy(image,0x1700);menu.copy(image,0x2000);
const profile=JSON.parse(readFileSync(path.join(root,'src/profile.json')));
const hooks=[{address:hookAddress,original,patch:fin.patch},{address:signatureAddress,original:signatureOriginal,patch:sig.patch}];
for(const h of profile.hooks){
 const address=BigInt(h.address),original=Buffer.from(h.original,'hex'),dest=menu.readBigUInt64LE(h.exportOffset),patch=Buffer.alloc(original.length,0x90);
 if(h.kind==='pointer')patch.writeBigUInt64LE(dest);
 else {patch[0]=h.kind==='call'?original[0]:0xe9;patch.writeInt32LE(Number(dest-address-5n),1);}
 if(h.trampoline){const at=BigInt(h.trampoline),tramp=Buffer.concat([original,Buffer.alloc(5)]);tramp[original.length]=0xe9;tramp.writeInt32LE(Number(address+BigInt(original.length)-at-BigInt(tramp.length)),original.length+1);tramp.copy(image,Number(at-0x4000n));}
 hooks.push({address,original,patch});
}
// Immutable install identity lives outside mutable resident state and flags.
const hash=createHash('sha256').update(image).digest();Buffer.from('WWEELF01').copy(image,0x3f00);hash.copy(image,0x3f08);
writeFileSync(path.join(out,'resident.bin'),image);
writeFileSync(path.join(out,'install-manifest.json'),JSON.stringify({titleId:profile.titleId,version:profile.version,imageHash:hash.toString('hex'),hooks:hooks.map(h=>({address:'0x'+h.address.toString(16),original:h.original.toString('hex'),patch:h.patch.toString('hex')}))},null,2));
const arr=b=>'{'+[...b].join(',')+'}';
writeFileSync(path.join(out,'generated.h'),`// Generated from original mod source and executable fingerprints.\n#pragma once\nstruct Patch {uint64_t address;unsigned length;unsigned char original[16],replacement[16];};\nstruct Fingerprint {uint64_t address;unsigned length;unsigned char hash[32];};\nstatic const Patch patches[]={\n${hooks.map(h=>`{${h.address}ULL,${h.original.length},${arr(h.original)},${arr(h.patch)}}`).join(',\n')}\n};\nstatic const Fingerprint fingerprints[]={\n${profile.fingerprints.map(f=>`{${f.address}ULL,${f.length},${arr(Buffer.from(f.sha256,'hex'))}}`).join(',\n')}\n};\nstatic const unsigned char install_identity[40]=${arr(image.subarray(0x3f00,0x3f28))};\nextern "C" const unsigned char resident_image[];\n`);
writeFileSync(path.join(out,'image.S'),`.section .rodata\n.balign 16\n.global resident_image\nresident_image:\n.incbin "${path.join(out,'resident.bin').replaceAll('\\','/')}"\n`);
if(process.argv.includes('--menu-only')){console.log('Built resident image and installer profile');process.exit(0);}
if(!sdk)throw Error('Set PS5_PAYLOAD_SDK to SDK v0.43');
const common=['-target','x86_64-linux-none','-U__linux__','-D__FreeBSD__=11','-D__PS5__','-isystem',path.join(sdk,'target/include'),'-march=znver2','-fPIC','-fno-stack-protector','-fno-plt','-ffunction-sections','-fdata-sections','-O2','-I',out];
const objects=[];
for(const file of ['payload/main.cpp','payload/syscall-shim.S','build/image.S']){const obj=path.join(out,path.basename(file)+'.o');objects.push(obj);run(['cc',...common,...(file.endsWith('.cpp')?['-std=c++17','-fno-exceptions','-fno-rtti']:[]),'-c',file,'-o',obj]);}
const elf=path.join(out,'Snipers-WWE-2K26.elf');
run(['ld.lld','--eh-frame-hdr','--gc-sections','--no-dependent-libraries','--wrap=syscall','--wrap=__syscall','--wrap=ptrace','-pie','--hash-style=gnu','-z','max-page-size=0x4000','-T',path.join(sdk,'ldscripts/elf_x86_64.x'),...objects,path.join(sdk,'target/lib/crt1.o'),'-L',path.join(sdk,'target/lib'),'--start-group',path.join(sdk,'target/lib/libc.a'),'-ldl','--end-group','-lkernel_web','-lSceLibcInternal','-o',elf]);
const report=inspectElf(readFileSync(elf));writeFileSync(path.join(out,'elf-manifest.json'),JSON.stringify(report,null,2));console.log(report);
