export const signatureAddress=0x1082115n;
export const signatureOriginal=Buffer.from('c5f82ed8c5fa114330','hex');
export function buildSignatureHook(base,enabled) {
 base=BigInt(base);enabled=BigInt(enabled);const code=Buffer.alloc(256,0x90);let p=0;const exits=[];
 const emit=h=>{const b=Buffer.from(h,'hex');b.copy(code,p);p+=b.length;};
 const rel=(to,next)=>{const d=Number(to-next);if(!Number.isSafeInteger(d)||d< -2147483648||d>2147483647)throw Error('Jump range');return d;};
 const exit=op=>{emit(op);exits.push(p);emit('00');};
 emit('509c5152'); // RAX, flags, RCX, RDX
 emit('803d');code.writeInt32LE(rel(enabled,base+BigInt(p+5)),p);p+=4;emit('00');exit('74');
 emit('488b43284885c0');exit('74');
 emit('488b80f82500004885c0');exit('74');emit('80b8ca00000000');exit('75');
 emit('48b8503f350500000000488b004885c0');exit('74');
 emit('488b80a00200004885c0');exit('74');
 emit('4863084889c24829ca66833a05');exit('72');
 emit('0fb74a0485c9');exit('74');
 emit('f30f100408'); // game-configured full signature meter
 const restore=p;emit('5a599d58');for(const j of exits){if(restore-j-1>127)throw Error('Branch range');code.writeInt8(restore-j-1,j);}
 signatureOriginal.copy(code,p);p+=signatureOriginal.length;emit('e9');code.writeInt32LE(rel(signatureAddress+9n,base+BigInt(p+4)),p);p+=4;
 const patch=Buffer.alloc(9,0x90);patch[0]=0xe9;patch.writeInt32LE(rel(base,signatureAddress+5n),1);
 return{code:code.subarray(0,p),patch};
}
