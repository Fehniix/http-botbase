from socketserver import ThreadingTCPServer,StreamRequestHandler

class echohandler(StreamRequestHandler):
    def handle(self):
        print(f'Connected: {self.client_address[0]}:{self.client_address[1]}')
        while True:
            # get message
            msg = self.rfile.readline()
            if not msg:
                print(f'Disconnected: {self.client_address[0]}:{self.client_address[1]}')
                break # exits handler, framework closes socket
            print(f'Received: {msg}')

server = ThreadingTCPServer(('',9002),echohandler)
server.serve_forever()