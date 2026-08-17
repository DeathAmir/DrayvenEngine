#!/usr/bin/env python3
import json, os, pathlib, sys, time, urllib.error, urllib.parse, urllib.request
TOKEN=os.environ.get('GH_TOKEN') or os.environ.get('GITHUB_TOKEN')
REPO=os.environ.get('GITHUB_REPOSITORY','DeathAmir/DrayvenEngine')
API=f'https://api.github.com/repos/{REPO}'
HEADERS={'Authorization':f'Bearer {TOKEN}','Accept':'application/vnd.github+json','X-GitHub-Api-Version':'2022-11-28','User-Agent':'DrayvenEngine-DirectRelease'}
RETRY={429,500,502,503,504}
def open_retry(req, attempts=15):
    for i in range(attempts):
        try: return urllib.request.urlopen(req,timeout=120)
        except urllib.error.HTTPError as e:
            if e.code not in RETRY or i==attempts-1: raise
            delay=int(e.headers.get('Retry-After','0') or 0) or min(3*(i+1),30); print('GitHub API',e.code,'retry in',delay,'s',flush=True); time.sleep(delay)
        except urllib.error.URLError:
            if i==attempts-1: raise
            time.sleep(min(3*(i+1),30))
def get_json(url):
    with open_retry(urllib.request.Request(url,headers=HEADERS)) as r: return json.load(r)
def json_request(url,payload,method='POST'):
    h=dict(HEADERS); h['Content-Type']='application/json'; req=urllib.request.Request(url,data=json.dumps(payload).encode(),headers=h,method=method)
    with open_retry(req) as r: return json.load(r) if r.length!=0 else {}
def get_release(tag):
    try: return get_json(f'{API}/releases/tags/{urllib.parse.quote(tag)}')
    except urllib.error.HTTPError as e:
        if e.code!=404: raise
        return None
def ensure(tag,sha):
    r=get_release(tag)
    if not r:
        try:r=json_request(f'{API}/releases',{'tag_name':tag,'target_commitish':sha,'name':f'Drayven Engine {tag}','body':'Validated Windows x64, Linux x64 and Android ARM64 SDK release for Drayven Engine 0.2.','draft':False,'prerelease':False,'generate_release_notes':True})
        except urllib.error.HTTPError as e:
            if e.code!=422: raise
            r=get_release(tag)
    print(r['html_url'])
def upload(tag,path):
    f=pathlib.Path(path); r=get_release(tag)
    if not f.is_file() or not r: raise SystemExit('missing file or release')
    existing={a['name']:a for a in r.get('assets',[])}
    if f.name in existing: print('already uploaded',f.name,existing[f.name]['browser_download_url']); return
    url=r['upload_url'].split('{',1)[0]+'?'+urllib.parse.urlencode({'name':f.name}); h=dict(HEADERS); h['Content-Type']='application/zip'; req=urllib.request.Request(url,data=f.read_bytes(),headers=h,method='POST')
    try:
        with open_retry(req) as res:a=json.load(res)
    except urllib.error.HTTPError as e:
        if e.code!=422: raise
        r=get_release(tag)
        for a in r.get('assets',[]):
            if a['name']==f.name: print('already uploaded',f.name,a['browser_download_url']); return
        raise
    print('uploaded',a['name'],a['browser_download_url'])
def verify(tag,names):
    r=get_release(tag)
    if not r: raise SystemExit('release missing')
    a={x['name']:x for x in r.get('assets',[])}; missing=[n for n in names if n not in a]
    if missing: raise SystemExit('missing: '+', '.join(missing))
    print('release',r['html_url'])
    for n in names: print(n,a[n]['browser_download_url'],a[n].get('digest',''))
if __name__=='__main__':
    if not TOKEN: raise SystemExit('GH_TOKEN required')
    cmd=sys.argv[1]
    if cmd=='ensure': ensure(sys.argv[2],sys.argv[3])
    elif cmd=='upload': upload(sys.argv[2],sys.argv[3])
    elif cmd=='verify': verify(sys.argv[2],sys.argv[3:])
    else: raise SystemExit('invalid command')
