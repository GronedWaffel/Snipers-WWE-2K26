import test from 'node:test';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {mkdirSync,readFileSync} from 'node:fs';
import path from 'node:path';
const root=path.resolve(import.meta.dirname,'..'),out=path.join(root,'build/tests');mkdirSync(out,{recursive:true});
test('installer failure and recovery paths',()=>{
 const exe=path.join(out,process.platform==='win32'?'installer.exe':'installer');
 const compiled=spawnSync(process.env.ZIG||'zig',['c++','-O1','-std=c++17','tests/installer.cpp','-o',exe],{cwd:root,encoding:'utf8'});
 assert.equal(compiled.status,0,compiled.stdout+compiled.stderr);
 const run=spawnSync(exe,[],{encoding:'utf8'});assert.equal(run.status,0,run.stdout+run.stderr);assert.match(run.stdout,/scenarios passed/);
});
test('release manifest has bounded non-overlapping patches and valid targets',()=>{
 const m=JSON.parse(readFileSync(path.join(root,'build/install-manifest.json'))),b=readFileSync(path.join(root,'build/resident.bin'));
 assert.equal(b.length,0x4000);assert.equal(b[0x100],0);assert.deepEqual([...b.subarray(0x1900,0x1909)],Array(9).fill(0));
 assert.equal(b.subarray(0x3f00,0x3f08).toString(),'WWEELF01');
 const occupied=new Set();for(const p of m.hooks){const a=BigInt(p.address),orig=Buffer.from(p.original,'hex'),patch=Buffer.from(p.patch,'hex');assert.equal(orig.length,patch.length);assert.ok(patch.length<=16);for(let i=0;i<patch.length;i++){const k=(a+BigInt(i)).toString();assert.ok(!occupied.has(k));occupied.add(k);}
 let target;if(patch[0]===0xe8||patch[0]===0xe9)target=a+5n+BigInt(patch.readInt32LE(1));else target=patch.readBigUInt64LE();assert.ok(target>=0x4000n&&target<0x7800n);}
 assert.ok(m.hooks.some(p=>p.address==='0x1130764'),'held-grapple fix must be packaged');
});
