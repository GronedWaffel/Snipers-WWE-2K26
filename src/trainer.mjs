// WWE 2K26 PPSA34299 01.000.000. Addresses traced from this executable.
// Finishers are a float (whole stocks plus fractional progress), not an integer.
export const profile = Object.freeze({
  titleId: 'PPSA34299', version: '01.000.000', table: 0x5352548n,
  signatures: [
    [0x107bdb0n, '4885ff7414488b87c82500004885c07408488b80f0020000c3'],
    [0x107bfa0n, 'c5fa104708c3'],
    [0xd1d3d0n, '0fb687ca000000c3'],
  ],
});
export class WWETrainer {
  constructor(debug) { this.debug=debug; this.pid=null; }
  async attach() {
    this.pid=null;
    for (const p of await this.debug.processes()) {
      if(p.name!=='eboot.bin')continue;
      const info=await this.debug.info(p.pid);
      if(info.titleId!==profile.titleId)continue;
      for(const [at,hex] of profile.signatures) {
        const b=await this.debug.read(p.pid,at,hex.length/2);
        if(b.toString('hex')!==hex)throw Error('Unsupported WWE executable. No memory changed.');
      }
      this.pid=p.pid;return {pid:p.pid,titleId:info.titleId,version:profile.version};
    }
    throw Error('Start WWE 2K26 PPSA34299 first.');
  }
  async read(at,length) {
    if(this.pid===null)throw Error('Connect to WWE first.');
    return this.debug.read(this.pid,at,length);
  }
  async pointer(at) {
    const p=(await this.read(at,8)).readBigUInt64LE();
    if(p && (p<0x10000n||p>0x7fffffffffn||(p&7n)))throw Error('Invalid WWE pointer.');
    return p;
  }
  async wrestler(slot) {
    if(!Number.isInteger(slot)||slot<0||slot>7)throw Error('Invalid wrestler slot.');
    const character=await this.pointer(profile.table+BigInt(slot*8));if(!character)return null;
    if((await this.read(character+0xa0n,4)).readUInt32LE()!==slot)throw Error('Wrestler identity changed.');
    const components=await this.pointer(character+0x25c8n);if(!components)return null;
    const meter=await this.pointer(components+0x2f0n);if(!meter)return null;
    const data=await this.read(meter,12);
    if(data.readBigUInt64LE()!==character)throw Error('Finisher owner mismatch.');
    const value=data.readFloatLE(8);
    if(!Number.isFinite(value)||value<0||value>3.001)throw Error('Unexpected finisher value.');
    const controller=await this.pointer(character+0x25f8n);
    const ai=controller?(await this.read(controller+0xcan,1))[0]:1;
    if(ai>1)throw Error('Unexpected controller state.');
    return {slot,character,components,meter,controller,human:ai===0,value};
  }
  async wrestlers() {
    const result=[];for(let slot=0;slot<8;slot++){const w=await this.wrestler(slot);if(w)result.push(w);}return result;
  }
  async refill(slot) {
    const before=await this.wrestler(slot);
    if(!before||!before.human)throw Error('The selected wrestler is not human-controlled.');
    // Resolve again immediately before each write. Never retain a heap address
    // across match changes, character swaps, or trainer ticks.
    const current=await this.wrestler(slot);
    if(!current||!current.human||current.character!==before.character||current.meter!==before.meter||current.controller!==before.controller)throw Error('Match changed; refill stopped.');
    if(current.value>=3)return {before:current.value,after:current.value,written:false};
    const value=Buffer.alloc(4);value.writeFloatLE(3);
    await this.debug.writeMemory(this.pid,current.meter+8n,value);
    const after=await this.wrestler(slot);
    if(!after||after.character!==current.character||after.meter!==current.meter)throw Error('Match changed during refill.');
    if(after.value<2.999)throw Error('The game did not retain the finisher refill.');
    return {before:current.value,after:after.value,written:true};
  }
}
