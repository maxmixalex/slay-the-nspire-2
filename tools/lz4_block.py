"""Small deterministic LZ4-block encoder/decoder, without external dependencies.

Independent implementation of the public LZ4 block format. Retains the required
five final literals and twelve-byte last-match margin. No frame/dictionary data.
"""
def encode(data: bytes) -> bytes:
    n=len(data);out=bytearray();table={};anchor=i=0
    def length(value: int):
        while value>=255:out.append(255);value-=255
        out.append(value)
    while i<=n-12:
        key=data[i:i+4];previous=table.get(key,-1);table[key]=i
        if previous<0 or i-previous>65535:
            i+=1;continue
        end=i+4
        while end<n-5 and data[end]==data[previous+end-i]:end+=1
        literals=i-anchor;match=end-i-4
        out.append((min(literals,15)<<4)|min(match,15))
        if literals>=15:length(literals-15)
        out.extend(data[anchor:i]);offset=i-previous;out.extend((offset&255,offset>>8))
        if match>=15:length(match-15)
        for j in range(i+1,end):table[data[j:j+4]]=j
        i=anchor=end
    literals=n-anchor;out.append(min(literals,15)<<4)
    if literals>=15:length(literals-15)
    out.extend(data[anchor:]);return bytes(out)

def decode(data: bytes, size: int) -> bytes:
    out=bytearray();i=0
    def extended(value):
        nonlocal i
        if value==15:
            while True:
                if i==len(data):raise ValueError('Truncated length')
                b=data[i];i+=1;value+=b
                if value>size:raise ValueError('Excessive length')
                if b!=255:break
        return value
    while i<len(data):
        token=data[i];i+=1;n=extended(token>>4)
        if n>len(data)-i or n>size-len(out):raise ValueError('Literal bounds')
        out.extend(data[i:i+n]);i+=n
        if i==len(data):
            if len(out)!=size:raise ValueError('Incorrect output size')
            return bytes(out)
        if len(data)-i<2:raise ValueError('Truncated offset')
        offset=data[i]|data[i+1]<<8;i+=2
        if not offset or offset>len(out):raise ValueError('Invalid offset')
        n=extended(token&15)+4
        if n>size-len(out):raise ValueError('Match bounds')
        for _ in range(n):out.append(out[-offset])
    raise ValueError('Missing final literals')
