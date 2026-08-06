import { readdirSync, readFileSync, statSync } from 'node:fs';
const files=[];
function walk(dir){ for(const ent of readdirSync(dir)){ const p=`${dir}/${ent}`; if(p.startsWith('src/workers/')) continue; const st=statSync(p); if(st.isDirectory()) walk(p); else if(/\.(ts|tsx)$/.test(p) && !p.endsWith('.test.ts')) files.push(p); } }
walk('src');
const offenders = files.filter((f)=>readFileSync(f,'utf8').includes('@dimforge/rapier3d-compat'));
if (offenders.length) { console.error(`Rapier main-thread import found:\n${offenders.join('\n')}`); process.exit(1); }
