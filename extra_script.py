print("running extra...")
import gzip
import shutil
FILES=['web/index.html']

def compressFile(inFile):
    outfile=inFile+".gz"
    with open(inFile, 'rb') as f_in:
        with gzip.open(outfile, 'wb') as f_out:
            shutil.copyfileobj(f_in, f_out)

for f in FILES:
    print("compressing %s"%f)
    compressFile(f)