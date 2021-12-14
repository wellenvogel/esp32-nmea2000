#! /usr/bin/env python3

import shutil
import sys
import http
import http.server
import urllib.request
import urllib.parse
import posixpath
import os
import traceback

class RequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_proxy(self):
        p=self.path
        print("path=%s"%p)
        if p.startswith("/api/"):
            apiurl=self.server.proxyUrl
            url=apiurl+p.replace("/api","")
            hostname=urllib.parse.urlparse(url).netloc
            print("proxy to %s"%url)
            try:
                body = None
                if self.headers.get('content-length') is not None:
                    content_len = int(self.headers.get('content-length'))
                    print("reading %d bytes body"%content_len)
                    body = self.rfile.read(content_len)

                # set new headers
                new_headers = {}
                for item in self.headers.items():
                    new_headers[item[0]] = item[1]
                new_headers['host'] = hostname
                try:
                    del new_headers['accept-encoding']
                except KeyError:
                    pass
                req=urllib.request.Request(url,headers=new_headers,data=body)
                with urllib.request.urlopen(req,timeout=50) as response:
                    self.send_response(http.HTTPStatus.OK)
                    for h in response.getheaders():
                        self.send_header(h[0],h[1])               
                    self.end_headers()
                    shutil.copyfileobj(response,self.wfile)
                    return True
                return None
                self.send_error(http.HTTPStatus.NOT_FOUND, "api not found")
                return None
            except Exception as e:
                print("Exception: %s"%traceback.format_exc())
                self.send_error(http.HTTPStatus.INTERNAL_SERVER_ERROR, "api error %s"%str(e))
                return None
    def do_GET(self):
        if not self.do_proxy():            
            super().do_GET()
    def do_POST(self):
        if not self.do_proxy():            
            super().do_POST()        
    def translate_path(self, path):
        """Translate a /-separated PATH to the local filename syntax.

        Components that mean special things to the local file system
        (e.g. drive or directory names) are ignored.  (XXX They should
        probably be diagnosed.)

        """
        # abandon query parameters
        path = path.split('?',1)[0]
        path = path.split('#',1)[0]
        # Don't forget explicit trailing slash when normalizing. Issue17324
        trailing_slash = path.rstrip().endswith('/')
        try:
            path = urllib.parse.unquote(path, errors='surrogatepass')
        except UnicodeDecodeError:
            path = urllib.parse.unquote(path)
        path = posixpath.normpath(path)
        words = path.split('/')
        words = list(filter(None, words))
        isSecond=False
        for baseDir in [
            os.path.join(self.server.baseDir,'lib','generated'),
            os.path.join(self.server.baseDir,'web')]:
            rpath = baseDir
            for word in words:
                if os.path.dirname(word) or word in (os.curdir, os.pardir):
                    # Ignore components that are not a simple file/directory name
                    continue
                rpath = os.path.join(rpath, word)
            if trailing_slash:
                rpath += '/'
            if os.path.exists(rpath):
                return rpath
            if isSecond:
                return rpath
            isSecond=True
                
def run(port,apiUrl,server_class=http.server.HTTPServer, handler_class=RequestHandler):
    basedir=os.path.join(os.path.dirname(__file__),'..')
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    httpd.proxyUrl=apiUrl
    httpd.baseDir=basedir
    httpd.serve_forever()

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("usage: %s port apiurl"%sys.argv[0])
        sys.exit(1)
    run(int(sys.argv[1]),sys.argv[2])    