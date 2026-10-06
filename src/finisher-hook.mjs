export const hookAddress=0x107b3f0n;
export const original=Buffer.from('554889e541574156','hex');
export function buildFinisherHook(base) {
  base=BigInt(base); const code=Buffer.alloc(0x1000);let p=0;const jumps=[];
  const emit=h=>{const b=Buffer.from(h,'hex');b.copy(code,p);p+=b.length;};
  const relative=(at,next)=>{const n=Number(BigInt(at)-BigInt(next));if(!Number.isSafeInteger(n)||n< -2147483648||n>2147483647)throw Error('Hook allocation is outside jump range');return n;};
  const rip=(h,target)=>{emit(h);code.writeInt32LE(relative(base+BigInt(target),base+BigInt(p+4)),p);p+=4;};
  const jump=h=>{emit(h);jumps.push(p);emit('00');};
  emit('509c'); // preserve scratch register and flags
  rip('ff05',0x108); // invocation counter
  rip('803d',0x100); // cmp enabled, 0 (displacement includes immediate)
  code.writeInt32LE(code.readInt32LE(p-4)-1,p-4);emit('00');jump('74');
  emit('4885ff');jump('74');
  emit('488b074885c0');jump('74'); // meter owner
  emit('488b80f82500004885c0');jump('74'); // controller
  emit('80b8ca00000000');jump('75'); // AI => normal game behavior
  rip('ff05',0x10c); // human refill counter
  rip('f30f1005',0x104); // requested stock = 3.0; game's setter does HUD dispatch
  const skip=p;emit('9d58');
  for(const o of jumps)code.writeInt8(skip-o-1,o);
  original.copy(code,p);p+=original.length;
  emit('e9');code.writeInt32LE(relative(hookAddress+8n,base+BigInt(p+4)),p);p+=4;
  code[0x100]=1;code.writeFloatLE(3,0x104);Buffer.from('SNIPWWE1').copy(code,0x110);
  const patch=Buffer.alloc(8,0x90);patch[0]=0xe9;patch.writeInt32LE(relative(base,hookAddress+5n),1);
  return {code,patch,codeLength:p};
}
