import {test} from 'node:test';
import assert from 'node:assert/strict';
import {WWETrainer,profile} from '../src/trainer.mjs';
import {buildFinisherHook,hookAddress,original} from '../src/finisher-hook.mjs';
function fixture() {
 const blocks=new Map();const writes=[];
 const put=(a,b)=>blocks.set(BigInt(a),b);
 const ptr=(a,v)=>{const b=Buffer.alloc(8);b.writeBigUInt64LE(BigInt(v));put(a,b);};
 ptr(profile.table,0x100000);put(0x1000a0,Buffer.alloc(4));
 ptr(0x1025c8,0x200000);ptr(0x2002f0,0x300000);ptr(0x1025f8,0x400000);
 const meter=Buffer.alloc(12);meter.writeBigUInt64LE(0x100000n);meter.writeFloatLE(1.25,8);put(0x300000,meter);put(0x4000ca,Buffer.from([0]));
 const d={async read(pid,a,len){a=BigInt(a);for(const [base,b]of blocks)if(a>=base&&a+BigInt(len)<=base+BigInt(b.length))return Buffer.from(b.subarray(Number(a-base),Number(a-base)+len));throw Error('Unmapped');},async writeMemory(pid,a,b){writes.push([a,Buffer.from(b)]);for(const [base,v]of blocks)if(a>=base&&a+BigInt(b.length)<=base+BigInt(v.length)){b.copy(v,Number(a-base));return;}throw Error('Unmapped');}};
 const t=new WWETrainer(d);t.pid=115;return{t,d,put,ptr,meter,writes};
}
test('writes only the selected human meter; repeated refill is a no-op',async()=>{
 const f=fixture();assert.equal((await f.t.refill(0)).after,3);await f.t.refill(0);assert.equal(f.writes.length,1);assert.equal(f.writes[0][0],0x300008n);
});
test('AI, mismatched owner, and corrupt meter are rejected without writes',async()=>{
 for(const change of [f=>f.put(0x4000ca,Buffer.from([1])),f=>f.meter.writeBigUInt64LE(0x999999n),f=>f.meter.writeFloatLE(NaN,8)]){const f=fixture();change(f);await assert.rejects(f.t.refill(0));assert.equal(f.writes.length,0);}
});
test('character disappearing between checks cannot receive a stale write',async()=>{
 const f=fixture();const read=f.d.read.bind(f.d);let n=0;f.d.read=async(pid,a,len)=>{if(BigInt(a)===profile.table&&++n===2)f.ptr(profile.table,0);return read(pid,a,len);};
 await assert.rejects(f.t.refill(0),/Match changed/);assert.equal(f.writes.length,0);
});
test('hook preserves displaced instructions and returns past the patch',()=>{
 const base=0x4000n;const {code,patch,codeLength}=buildFinisherHook(base);
 assert.equal(hookAddress+5n+BigInt(patch.readInt32LE(1)),base);
 assert.ok(code.subarray(codeLength-13,codeLength-5).equals(original));
 assert.equal(base+BigInt(codeLength)+BigInt(code.readInt32LE(codeLength-4)),hookAddress+8n);
 assert.equal(code.readFloatLE(0x104),3);assert.equal(code[0x100],1);
 assert.throws(()=>buildFinisherHook(0x1000000000n),/jump range/);
});
