import datetime
import math
import requests
import time
from typing import List, Tuple

org = "my-org"
bucket = "my-bucket"
token = '***'
url = f"http://localhost:8084/api/v2/write?org={org}&bucket={bucket}&precision=ms"

def to_ts(dt: datetime.datetime) -> str:
    r = dt.timestamp() * 1000
    return str(int(r))

def build_1second_data(start: datetime.datetime) -> List[Tuple[datetime.datetime, int]]:
    r = []
    for i in range(1000):
        dt = start + datetime.timedelta(milliseconds=i)
        v = math.sin(i*60/1000)
        r.append((dt, v))
    return r


def send_data(s: requests.Session, token: str, data: List[Tuple[datetime.datetime, int]]) -> requests.Response:
    reqs = '\n'.join(f'test2 variable1={v} {to_ts(ts)}' for ts, v in data)
    res = s.post(url, headers={'Authorization': f'Token {token}','Content-Type': 'text/plain; charset=utf-8', 'Accept': 'application/json'}, data=reqs)
    return res

def main():
    start = datetime.datetime.now() # timezone do timestamp na hora do envio deve ser no fuso horario do servidor. ainda nao descobri a fonte do bug
    s = requests.session()
    for _ in range(600):
        now = datetime.datetime.now()
        data = build_1second_data(now)
        res = send_data(s, token, data)
        print(res.status_code)
        time.sleep(1)
    
    end = datetime.datetime.now()
    elapsed = end - start
    print('elapsed:::', elapsed.total_seconds())

    return
    q = '''from(bucket: "my-bucket")
    |> range(start: -1021h, stop: 1h)
    |> filter(fn: (r) => r["_field"] == "variable1")
    '''

    r2 = s.post(f"http://localhost:8086/api/v2/query?org={org}&bucket={bucket}",
        headers = {
            'Authorization': f'Token {token}',
            'Content-type': 'application/vnd.flux',
        },
        data=q
    )
    print(r2.status_code)
    print(r2.text)

  
if __name__ == '__main__':
    main()
