# Drops plugin references the target engine doesn't have.  usage: fix_uproject.py <uproject> <engine dir>
import json,os,sys
p,eng=sys.argv[1],sys.argv[2]
have=set()
for dp,dn,fn in os.walk(os.path.join(eng,'Engine','Plugins')):
    have.update(f[:-8] for f in fn if f.endswith('.uplugin'))
u=json.load(open(p))
kept=[x for x in u.get('Plugins',[]) if x['Name'] in have]
dropped=[x['Name'] for x in u.get('Plugins',[]) if x['Name'] not in have]
u['Plugins']=kept
json.dump(u,open(p,'w'),indent='\t')
if dropped: print('uproject: dropped plugins not in this engine:',', '.join(dropped))
