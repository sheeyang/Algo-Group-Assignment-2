/**
 * @file consolidate-images.js
 * @brief Copy repository image assets into an algorithm-labelled collection.
 * @details Originals and report links stay untouched. Includes local outputs
 * from ignored build directories, with source paths and SHA-256 provenance.
 * Run from any directory: node tools/consolidate-images.js
 */
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
const root=path.resolve(__dirname,'..'),collection=path.join(root,'image-collection');
const ignore=new Set(['.git','.vs','.vscode','.agents','.claude','node_modules','third_party','_deps','CMakeFiles','c3-diagram-browser','image-collection','doxygen']);
const extensions=new Set(['.png','.jpg','.jpeg','.bmp','.gif','.webp','.tif','.tiff','.svg']);
const files=[];
/** @brief Inventory real image files without following symlinks or vendor trees. */
function scan(dir){for(const e of fs.readdirSync(dir,{withFileTypes:true})){if(e.isSymbolicLink())continue;const p=path.join(dir,e.name);if(e.isDirectory()){if(!ignore.has(e.name))scan(p);}else if(extensions.has(path.extname(e.name).toLowerCase()))files.push(path.relative(root,p).replaceAll('\\','/'));}}
scan(root);
files.sort((a,b)=>{const rank=x=>x.startsWith('testdata/')?0:x.startsWith('docs/')?1:2;return rank(a)-rank(b)||a.localeCompare(b);});
const alg={dp:'dynamic-programming',greedy:'greedy',ek:'edmonds-karp',dinic:'dinic'};
const records=[],inputHashes=new Map();
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
/** @brief Read dimensions directly from common file headers, without resampling. */
function dimensions(b,ext){
 if(ext==='.png'&&b.length>=24)return [b.readUInt32BE(16),b.readUInt32BE(20)];
 if(ext==='.bmp'&&b.length>=26)return [Math.abs(b.readInt32LE(18)),Math.abs(b.readInt32LE(22))];
 if(ext==='.gif'&&b.length>=10)return [b.readUInt16LE(6),b.readUInt16LE(8)];
 if((ext==='.jpg'||ext==='.jpeg')&&b[0]===255){let i=2;while(i+8<b.length){if(b[i++]!==255)continue;while(b[i]===255)i++;const marker=b[i++];if(marker===217||marker===218)break;const n=b.readUInt16BE(i);if([192,193,194,195,197,198,199,201,202,203,205,206,207].includes(marker))return [b.readUInt16BE(i+5),b.readUInt16BE(i+3)];if(n<2)break;i+=n;}}
 if(ext==='.svg'){const s=b.toString('utf8'),w=s.match(/\bwidth="([\d.]+)"/),h=s.match(/\bheight="([\d.]+)"/);if(w&&h)return [Number(w[1]),Number(h[1])];}
 return ['',''];
}
/** @brief Classify only supported filename/documentation evidence. */
function classify(f){
 const b=path.basename(f),scope=f.includes('/c4/')?'report-c4':'report-c3';let m;
 if(f.startsWith('testdata/'))return ['inputs/'+(f.includes('/c3/')?'photo-and-small-fixtures':'original-fixtures')+'/'+b,'input','','Original fixture; provenance is documented in testdata/README.md and testdata/c3/README.md.'];
 if(/docs\/report\/figures\/c[34]\/.*-input\.png$/.test(f))return ['inputs/report-inputs/'+b,'input','','Input panel from documented C3/C4 evidence.'];
 if((m=f.match(/docs\/report\/figures\/c[34]\/(.*)-(dp|ek|dinic)-(result|first|overview)\.png$/))){const kind=m[3]==='result'?'results':'seam-overlays';return [`algorithms/${alg[m[2]]}/${kind}/${scope}/${b}`,kind,m[2],'Documented C3/C4 fixture comparison; 64x43 input, 54x43 result after ten vertical seams.'];}
 if((m=b.match(/^dp-vs-(ek|dinic)-/)))return [`comparisons/dp-vs-${alg[m[1]]}/${b}`,'comparison',`dp+${m[1]}`,'Mixed-algorithm comparison; do not treat as a standalone resized result.'];
 if(b.startsWith('greedy-vs-dp-'))return ['comparisons/dp-vs-greedy/'+b,'comparison','dp+greedy','Documented C2 panels at 90%, 70% and 50% width; originals and both algorithms share each figure.'];
 if(/^(graphcut-network|network-example)\./.test(b))return ['reference/network-diagrams/'+b,'diagram','ek+dinic','Shared graph-cut formulation, not an image-carving output.'];
 if(b.startsWith('gui-'))return ['reference/gui-screenshots/'+b,'screenshot','','GUI screenshot; may show previous results or several algorithms. See report figure README.'];
 if(b.startsWith('ci_'))return ['reference/ci-screenshots/'+b,'screenshot','','CI evidence, not a seam-carving image.'];
 if(b==='report-runtime.png')return ['reference/runtime-charts/'+b,'chart','dp+greedy+ek+dinic','Runtime comparison chart, not a resized output.'];
 if(b==='report-building-baselines.png')return ['comparisons/dp-vs-scaling-and-crop/'+b,'comparison','dp+bilinear+crop','Final report Figure 2: input, DP seam, DP result, bilinear scaling and centre crop.'];
 if(b==='report-greedy-comparison.png')return ['comparisons/dp-vs-greedy/'+b,'comparison','dp+greedy','Final report Figure 3: DP and greedy results at 560x533.'];
 if(b==='report-graphcut-portrait.png')return ['comparisons/dp-vs-graph-cut/'+b,'comparison','dp+ek+dinic','Final report Figure 4: first seams and ten-removal portrait results.'];
 if(b==='r1-dp-building.png')return ['algorithms/dynamic-programming/analysis-figures/'+b,'comparison','dp','R1 worked visual: original, energy, first DP seam and resized result.'];
 if(f==='docs/report/figures/original.png')return ['inputs/report-originals/'+b,'input','','Original scene used by the report step-visualisation example.'];
 if(f.startsWith('docs/report/figures/')&&/^(result|all_seams_overview|step_\d+|report-bonus-steps)\.png$/.test(b))return ['unverified/report-step-visualisation/'+b,'visualisation','','Report records mixed-direction removal to 640x430, but this file alone does not identify its finder. Preserved without guessing an algorithm.'];
 if(f.includes('/tests/cli-dimensions/'))return ['algorithms/dynamic-programming/regression-test-outputs/'+b,'regression-output','dp','tests/check_cli_dimensions.cmake: explicit DP or the CLI DP default.'];
 if((m=f.match(/^build[^/]*\/(?:release\/)?(portrait|building|castle)-(dp|ek|greedy|dinic)(-steps\/.*|\.png)$/))){const frames=m[3].startsWith('-steps/');return [`algorithms/${alg[m[2]]}/${frames?'step-frames':'results'}/local-runs/${f}`,frames?'step-frame':'result',m[2],'Local run identified by explicit algorithm filename; dimensions are recorded below.'];}
 if(f.startsWith('build/release/c3-smoke'))return ['algorithms/edmonds-karp/regression-test-outputs/'+f,'regression-output','ek','C3 mixed-direction EK CLI smoke test generated in this working session.'];
 return ['unverified/algorithm-unknown/'+f,'unverified','','No reliable algorithm label/provenance found. Do not assume the finder from appearance.'];
}
/** @brief Copy one asset and retain a traceable, byte-verified manifest row. */
function copy(f){
 const b=fs.readFileSync(path.join(root,f)),sha=hash(b),ext=path.extname(f).toLowerCase();
 let [dest,kind,algorithm,note]=classify(f);
 if(kind==='input'&&inputHashes.has(sha)){dest=inputHashes.get(sha);note+=' Byte-identical input alias; uses the canonical copy.';}
 else if(kind==='input')inputHashes.set(sha,dest);
 const target=path.join(collection,dest);
 fs.mkdirSync(path.dirname(target),{recursive:true});
 if(fs.existsSync(target)&&hash(fs.readFileSync(target))!==sha)throw Error('Destination has different contents: '+target);
 fs.copyFileSync(path.join(root,f),target);
 if(hash(fs.readFileSync(target))!==sha)throw Error('Copy verification failed: '+f);
 const [width,height]=dimensions(b,ext);
 records.push({source_path:f,collection_path:dest,algorithm,kind,width,height,sha256:sha,origin:'copied',notes:note,command:''});
}
files.forEach(copy);
/** @brief Fill the otherwise absent greedy-result folder using small real inputs. */
function greedyExamples(){
 const bins=['build/documentation-check/cli/seamcli.exe','build/release/cli/seamcli.exe'];
 const bin=bins.find(b=>fs.existsSync(path.join(root,b)));
 if(!bin)throw Error('No CLI executable found for the two small greedy examples.');
 for(const name of ['portrait','building']){
  const input=`testdata/c3/${name}-small.png`,prefix=`algorithms/greedy/generated-small-examples/${name}`,result=prefix+'/result.png',steps=prefix+'/steps';
  fs.mkdirSync(path.join(collection,prefix),{recursive:true});
  const args=[input,`image-collection/${result}`,'--algo','greedy','--width','54','--steps',`image-collection/${steps}`];
  const log=cp.execFileSync(path.join(root,bin),args,{cwd:root,encoding:'utf8'});
  console.log(log.trim());
  const generated=[result,...fs.readdirSync(path.join(collection,steps)).map(f=>steps+'/'+f)];
  for(const dest of generated){const b=fs.readFileSync(path.join(collection,dest)),[width,height]=dimensions(b,'.png');
   if(dest===result&&(width!==54||height!==43))throw Error('Wrong greedy result dimensions');
   records.push({source_path:input,collection_path:dest,algorithm:'greedy',kind:dest===result?'result':'step-frame',width,height,sha256:hash(b),origin:'generated',notes:'New small greedy example created for this collection; source PNG unchanged. Input 64x43, ten vertical seams, output 54x43. Not a historical benchmark result.',command:[bin,...args].join(' ')});
  }
 }
}
greedyExamples();
const cols=['source_path','collection_path','algorithm','kind','width','height','sha256','origin','notes','command'];
const csv=x=>'"'+String(x??'').replaceAll('"','""')+'"';
fs.writeFileSync(path.join(collection,'manifest.csv'),[cols.join(','),...records.map(r=>cols.map(k=>csv(r[k])).join(','))].join('\n')+'\n');
fs.writeFileSync(path.join(collection,'manifest.json'),JSON.stringify(records,null,2)+'\n');
for(const name of Object.values(alg))fs.mkdirSync(path.join(collection,'algorithms',name),{recursive:true});
const unique=[...new Map(records.map(r=>[r.collection_path,r])).values()];
const escape=s=>String(s).replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('>','&gt;').replaceAll('"','&quot;');
const url=p=>p.split('/').map(encodeURIComponent).join('/');
const groups=new Map();for(const r of unique){const key=r.collection_path.startsWith('algorithms/')?r.collection_path.split('/').slice(0,2).join('/'):r.collection_path.split('/')[0];if(!groups.has(key))groups.set(key,[]);groups.get(key).push(r);}
const labels={'inputs':'Input images','algorithms/dynamic-programming':'Dynamic programming (DP)','algorithms/greedy':'Greedy','algorithms/edmonds-karp':'Graph cut: Edmonds–Karp (EK)','algorithms/dinic':'Graph cut: Dinic','comparisons':'Comparison panels (multiple algorithms)','reference':'Diagrams, charts and screenshots','unverified':'Algorithm not verified — check provenance before reuse'};
const sections=[...groups].map(([key,rows])=>`<details open><summary>${escape(labels[key]||key)} <span>${rows.length} files</span></summary><div class="grid">${rows.map(r=>`<article><a href="${url(r.collection_path)}"><img loading="lazy" src="${url(r.collection_path)}" alt="${escape(path.basename(r.collection_path))}"></a><h3>${escape(path.basename(r.collection_path))}</h3><p>${r.width?`${r.width} × ${r.height} · `:''}${escape(r.kind)} · ${escape(r.origin)}</p><code>${escape(r.collection_path)}</code><p class="source">Source: ${escape(r.source_path)}</p><p>${escape(r.notes)}</p></article>`).join('')}</div></details>`).join('\n');
fs.writeFileSync(path.join(collection,'index.html'),`<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Seam-carving image collection</title><style>body{font:16px/1.45 system-ui,sans-serif;max-width:1480px;margin:32px auto;padding:0 24px;background:#f4f6f8;color:#162b3a}h1{margin-bottom:8px}details{margin:24px 0}summary{font-size:24px;font-weight:650;cursor:pointer}summary span{font-size:14px;color:#596b7a}.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(280px,1fr));gap:16px;margin-top:18px}article{padding:16px;background:white;border:1px solid #d5dfe7;border-radius:8px;overflow:hidden}img{width:100%;height:190px;object-fit:contain;background:#e7edf1}h3{font-size:16px;margin-bottom:4px}p{font-size:14px}code,.source{overflow-wrap:anywhere;font-size:12px}a{color:#075985}.note{padding:16px;border-left:4px solid #0d9488;background:white}</style><h1>Seam-carving image collection</h1><p>${files.length} existing image paths inventoried; ${unique.length} collection files, including two new small greedy runs.</p><p class="note">Original files and report/test paths are preserved. Mixed comparison panels have their own section. Uncertain files are explicitly unverified. New greedy results are labelled generated and do not replace recorded benchmark evidence. Exact source paths, dimensions, SHA-256 hashes and commands are in <a href="manifest.csv">manifest.csv</a>.</p>${sections}</html>`);
fs.writeFileSync(path.join(collection,'README.txt'),`SEAM-CARVING IMAGE COLLECTION\n\nOpen index.html for a visual catalogue.\n\ninputs/                         Original and scaled input fixtures\nalgorithms/dynamic-programming/ DP results, seam overlays and local frames\nalgorithms/greedy/               Two new 64x43 -> 54x43 greedy examples\nalgorithms/edmonds-karp/         EK results, seam overlays and local frames\nalgorithms/dinic/                Dinic results and seam overlays\ncomparisons/                    Labelled multi-algorithm report panels\nreference/                      Network diagrams, charts, GUI and CI screenshots\nunverified/                     Images whose finder cannot be reliably identified\n\nOriginal files were copied, never moved or deleted. Input aliases with identical\nbytes share one canonical copy; manifest.csv retains every original source path.\nOther algorithms remain separated even when their pixels match.\n\nGreedy standalone images were not present in the inventory. New examples use\ntestdata/c3/portrait-small.png and building-small.png with --algo greedy --width 54.\nThey are under generated-small-examples and explicitly marked generated.\n\nThe CSV/JSON manifests record source path, destination, algorithm, role, dimensions,\nSHA-256, provenance and generation command. Empty algorithm fields mean unknown\nor not applicable; dimensions may be blank for unsupported image formats.\n\nRe-run from the repository root: node tools/consolidate-images.js\nThe collection contains ignored local-build outputs as well as tracked report\nassets. Its contents describe this machine's inventory, not just files in Git.\nSVG sources are included with network diagrams. Vendor and browser-cache assets\nare excluded. Existing differing destination copies are not silently overwritten.\n`);
for(const r of records){const actual=hash(fs.readFileSync(path.join(collection,r.collection_path)));if(actual!==r.sha256)throw Error('Manifest verification failed: '+r.collection_path);}
console.log(`Inventoried ${files.length} sources; ${records.length} manifest entries; ${unique.length} collection files verified.`);
console.log([...groups].map(([g,r])=>g+': '+r.length).join('\n'));
