// Usage: node run-emulator.cjs firmware.bin symbols.txt [avr8js module path]
const fs=require('fs');
const avr=require(process.argv[4] || 'avr8js');
const bytes=fs.readFileSync(process.argv[2]);
const program=new Uint16Array(16384);
for(let i=0;i<bytes.length;i+=2) program[i>>1]=bytes[i]|((bytes[i+1]||0)<<8);
const symbols=fs.readFileSync(process.argv[3],'utf8');
function address(name) {
  const match=symbols.match(new RegExp('^([0-9a-fA-F]+)\\s+\\w\\s+'+name+'$','m'));
  if(!match) throw new Error('Symbol missing: '+name);
  return parseInt(match[1],16)-0x800000;
}
const done=address('testDone'), count=address('testCount'), results=address('testResults');
const cpu=new avr.CPU(program);
const limit=500000000;
while(!cpu.data[done] && cpu.cycles<limit) { avr.avrInstruction(cpu); cpu.tick(); }
if(!cpu.data[done]) throw new Error('Emulation timeout at PC '+cpu.pc);
const total=cpu.data[count], failed=[];
for(let i=0;i<total;i++) if(cpu.data[results+i]!==1) failed.push(i+1);
console.log(JSON.stringify({tests:total,passed:total-failed.length,failed,cycles:cpu.cycles}));
if(failed.length) process.exit(1);
